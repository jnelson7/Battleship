// Main.c
// Runs on LM4F120/TM4C123
// You may use, edit, run or distribute this file 
// You are free to change the syntax/organization of this file

// Jonathan W. Valvano 2/20/17, valvano@mail.utexas.edu
// Modified by Sile Shu 10/4/17, ss5de@virginia.edu
// Modified by Mustafa Hotaki 7/29/18, mkh3cf@virginia.edu

#include <stdio.h>
#include <stdint.h>
#include "OS.h"
#include "tm4c123gh6pm.h"
#include "LCD.h"
#include <string.h> 
#include "UART.h"
#include "FIFO.h"
#include "joystick.h"
#include "PORTE.h"

// Constants
#define BGCOLOR     					LCD_BLACK
#define CROSSSIZE            	5
#define PERIOD               	4000000   // DAS 20Hz sampling period in system time units
#define PSEUDOPERIOD         	8000000
#define LIFETIME             	1000
#define RUNLENGTH            	6000//600 // 30 seconds run length
#define SHIPNUM               5
#define MAXSHIPSIZE						5

extern Sema4Type LCDFree;
uint16_t origin[2]; 	// The original ADC value of x,y if the joystick is not touched, used as reference
int16_t x=63;  			// horizontal position of the crosshair, initially 63
int16_t y=63;  			// vertical position of the crosshair, initially 63
int16_t prevx, prevy;	// Previous x and y values of the crosshair
uint8_t select;  			// joystick push
uint8_t area[2];
uint32_t PseudoCount;
uint8_t screen=1,view=1;
uint8_t sound=0,players=1;
uint8_t ship_x[SHIPNUM]={0,2,0,2,0};
uint8_t ship_y[SHIPNUM]={2,2,3,3,4};
uint8_t ship5x[5],ship4x[4],ship3x[3],ship2x1[2],ship2x2[2];
uint8_t ship5y[5],ship4y[4],ship3y[3],ship2y1[2],ship2y2[2];
uint8_t ship_setup_x[SHIPNUM],ship_setup_y[SHIPNUM];
uint8_t ship_display[SHIPNUM]={0,0,0,0,0},ship_size[SHIPNUM]={5,4,3,2,2};
int8_t ship_blocks_pos_x[5]={0,0,0,0,0},ship_blocks_pos_y[5]={0,0,0,0,0};
uint8_t i=0,j=0; // for loop indexing
char ship_buf[1];
uint8_t ship_view[SHIPNUM];
int8_t ship_eraser_x[5]={0,0,0,0,0},ship_eraser_y[5]={0,0,0,0,0};

unsigned long NumCreated;   		// Number of foreground threads created
unsigned long NumSamples;   		// Incremented every ADC sample, in Producer
unsigned long UpdateWork;   		// Incremented every update on position values
unsigned long Calculation;  		// Incremented every cube number calculation

//---------------------User debugging-----------------------
unsigned long DataLost;     // data sent by Producer, but not received by Consumer
long MaxJitter;             // largest time jitter between interrupts in usec
#define JITTERSIZE 64 // -------------- CHANGE THIS STUFF -----------------
unsigned long const JitterSize=JITTERSIZE;
unsigned long JitterHistogram[JITTERSIZE]={0,};
unsigned long TotalWithI1;
unsigned short MaxWithI1;

void Device_Init(void){
	UART_Init();
	BSP_LCD_OutputInit();
	BSP_Joystick_Init();
}
//------------------Task 1--------------------------------
// background thread executed at 20 Hz
//******** Producer *************** 
int UpdatePosition(uint16_t rawx, uint16_t rawy, jsDataType* data){
	if (rawx > origin[0]){
		x = x + ((rawx - origin[0]) >> 9);
	}
	else{
		x = x - ((origin[0] - rawx) >> 9);
	}
	if (rawy < origin[1]){
		y = y + ((origin[1] - rawy) >> 9);
	}
	else{
		y = y - ((rawy - origin[1]) >> 9);
	}
	if (x > 127){
		x = 127;}
	if (x < 0){
		x = 0;}
	if (y > 120 - CROSSSIZE){
		y = 120 - CROSSSIZE;}
	if (y < 0){
		y = 0;}
	data->x = x; data->y = y;
	return 1;
}

void Producer(void){
	uint16_t rawX,rawY; // raw adc value
	uint8_t select;
	jsDataType data;
	unsigned static long LastTime;  // time at previous ADC sample
	unsigned long thisTime;         // time at current ADC sample
	long jitter;                    // time between measured and expected, in us
	if (NumSamples < RUNLENGTH){
		BSP_Joystick_Input(&rawX,&rawY,&select);
		thisTime = OS_Time();       // current time, 12.5 ns
		UpdateWork += UpdatePosition(rawX,rawY,&data); // calculation work
		NumSamples++;               // number of samples
		if(JsFifo_Put(data) == 0){ // send to consumer
			DataLost++;
		}
	//calculate jitter
		/*
		if(UpdateWork > 1){    // ignore timing of first interrupt
			unsigned long diff = OS_TimeDifference(LastTime,thisTime);
			if(diff > PERIOD){
				jitter = (diff-PERIOD+4)/8;  // in 0.1 usec
			}
			else{
				jitter = (PERIOD-diff+4)/8;  // in 0.1 usec
			}
			if(jitter > MaxJitter){
				MaxJitter = jitter; // in usec
			}       // jitter should be 0
			if(jitter >= JitterSize){
				jitter = JITTERSIZE-1;
			}
			JitterHistogram[jitter]++; 
		}
		*/
		LastTime = thisTime;
	}
}

//--------------end of Task 1-----------------------------

//------------------Task 2--------------------------------
// background thread executes with SW1 button
// one foreground task created with button push
// foreground treads run for 1 sec and die
// ***********ButtonWork*************
void ButtonWork(void){
	uint32_t StartTime,CurrentTime,ElapsedTime;
	StartTime = OS_MsTime();
	ElapsedTime = 0;
	OS_bWait(&LCDFree);
			
	for(i=0; i<SHIPNUM; i++){
		if(area[0]==ship_x[i] && area[1]==ship_y[i]&&ship_display[i]==0&& ship_view[i]==6){
			//BSP_LCD_FillRect(60, 60, 10, 6, LCD_WHITE); // just for debugging
			view = 6;
			if(i==0){ // size 5
				
				ship_setup_x[0]=0;
				ship_setup_x[1]=0;
				ship_setup_x[2]=0;
				ship_setup_x[3]=0;
				ship_setup_x[4]=0;
				
				ship_setup_y[0]=0;
				ship_setup_y[1]=1;
				ship_setup_y[2]=2;
				ship_setup_y[3]=3;
				ship_setup_y[4]=4;
				
				ship_display[i]=1;
				ship_view[i]=5;
				
			}else if(i==1){ // size 4
				
				ship_setup_x[0]=0;
				ship_setup_x[1]=0;
				ship_setup_x[2]=0;
				ship_setup_x[3]=0;
				ship_setup_x[4]=0;
				
				ship_setup_y[0]=0;
				ship_setup_y[1]=0;
				ship_setup_y[2]=1;
				ship_setup_y[3]=2;
				ship_setup_y[4]=3;
				
				ship_display[i]=1;
				ship_view[i]=5;
				
			}else if(i==2){ // size 3
				
				ship_setup_x[0]=0;
				ship_setup_x[1]=0;
				ship_setup_x[2]=0;
				ship_setup_x[3]=0;
				ship_setup_x[4]=0;
				
				ship_setup_y[0]=0;
				ship_setup_y[1]=0;
				ship_setup_y[2]=0;
				ship_setup_y[3]=1;
				ship_setup_y[4]=2;
				
				ship_display[i]=1;
				ship_view[i]=5;
				
			}else if(i==3){ // size 2
				
				ship_setup_x[0]=0;
				ship_setup_x[1]=0;
				ship_setup_x[2]=0;
				ship_setup_x[3]=0;
				ship_setup_x[4]=0;
				
				ship_setup_y[0]=0;
				ship_setup_y[1]=0;
				ship_setup_y[2]=0;
				ship_setup_y[3]=0;
				ship_setup_y[4]=1;
				
				ship_display[i]=1;
				ship_view[i]=5;
				
			}else if(i==4){ // size 2
				
				ship_setup_x[0]=0;
				ship_setup_x[1]=0;
				ship_setup_x[2]=0;
				ship_setup_x[3]=0;
				ship_setup_x[4]=0;
				
				ship_setup_y[0]=0;
				ship_setup_y[1]=0;
				ship_setup_y[2]=0;
				ship_setup_y[3]=0;
				ship_setup_y[4]=1;
				
				ship_display[i]=1;
				ship_view[i]=5;				
			}
		}
	}
	screen = view; 
//		uint8_t ship_x[5]={0,2,0,2,0};
//		uint8_t ship_y[5]={3,3,4,4,5};
//		uint8_t ship5x[5],ship4x[4],ship3x[3],ship2x1[2],ship2x2[2];
//		uint8_t ship5y[5],ship4y[4],ship3y[3],ship2y1[2],ship2y2[2];
		
	BSP_LCD_FillScreen(BGCOLOR);	
	/*	
	BSP_LCD_FillScreen(BGCOLOR);		
	while (ElapsedTime < LIFETIME){
		CurrentTime = OS_MsTime();
		ElapsedTime = CurrentTime - StartTime;
		BSP_LCD_Message(0,5,0,"Life Time:",screen);
		BSP_LCD_Message(1,0,0,"Horizontal Area:",area[0]);
		BSP_LCD_Message(1,1,0,"Vertical Area:",area[1]);
		BSP_LCD_Message(1,2,0,"Elapsed Time:",ElapsedTime);
		OS_Sleep(50);
	}	
	BSP_LCD_FillScreen(BGCOLOR);
	*/
	OS_bSignal(&LCDFree);
  OS_Kill();  // done, OS does not return from a Kill
} 

//************SW1Push*************
// remains untouched, this debounces
void SW1Push(void){
  if(OS_MsTime() > 20 ){ // debounce
			//view=1;
    if(OS_AddThread(&ButtonWork,128,4)){
      NumCreated++; 
    }
    OS_ClearMsTime();  // at least 20ms between touches
  }
}

//--------------end of Task 2-----------------------------

//------------------Task 3--------------------------------

//******** Consumer *************** 
// foreground thread, accepts data from producer
// Display crosshair and its positions
// inputs:  none
// outputs: none
void Consumer(void){
	while(NumSamples < RUNLENGTH){
		jsDataType data;
		JsFifo_Get(&data);
		OS_bWait(&LCDFree);

		BSP_LCD_DrawCrosshair(prevx, prevy, LCD_BLACK); // Draw a black crosshair
		BSP_LCD_DrawCrosshair(data.x, data.y, LCD_RED); // Draw a red crosshair

//		BSP_LCD_Message(1, 5, 3, "X: ", screen);//x);		
//		BSP_LCD_Message(1, 5, 12, "Y: ", y);
		OS_bSignal(&LCDFree);
		prevx = data.x; 
		prevy = data.y;
	}
  OS_Kill();  // done
}


//--------------end of Task 3-----------------------------

//------------------Task 4--------------------------------
// foreground thread that runs without waiting or sleeping
// it executes some calculation related to the position of crosshair 
//******** CubeNumCalc *************** 
// foreground thread, calculates the virtual cube number for the crosshair
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none

void CubeNumCal(void){ 
	uint16_t CurrentX,CurrentY;
  while(1) {
		if(NumSamples < RUNLENGTH){
			CurrentX = x; CurrentY = y;
			area[0] = CurrentX / 6;
			area[1] = CurrentY / 10;
			Calculation++;
		}
  }
}
//--------------end of Task 4-----------------------------

//------------------Task 5--------------------------------
// UART background ISR performs serial input/output
// Two software fifos are used to pass I/O data to foreground
// Interpreter is a foreground thread, accepts input from serial port, outputs to serial port
// inputs:  none
// outputs: none
// there are following commands
//    print performance measures 
//    time-jitter, number of data points lost, number of calculations performed
//    i.e., NumSamples, NumCreated, MaxJitter, DataLost, UpdateWork, Calculations
void Interpreter(void){
	char command[80];
  while(1){
		/*
    OutCRLF(); UART_OutString(">>");
		UART_InString(command,79);
		OutCRLF();
		if (!(strcmp(command,"NumSamples"))){
			UART_OutString("NumSamples: ");
			UART_OutUDec(NumSamples);
		}
		else if (!(strcmp(command,"NumCreated"))){
			UART_OutString("NumCreated: ");
			UART_OutUDec(NumCreated);
		}
		else if (!(strcmp(command,"MaxJitter"))){
			UART_OutString("MaxJitter: ");
			UART_OutUDec(MaxJitter);
		}
		else if (!(strcmp(command,"DataLost"))){
			UART_OutString("DataLost: ");
			UART_OutUDec(DataLost);
		}
		else if (!(strcmp(command,"UpdateWork"))){
			UART_OutString("UpdateWork: ");
			UART_OutUDec(UpdateWork);
		}
	  else if (!(strcmp(command,"Calculations"))){
			UART_OutString("Calculations: ");
			UART_OutUDec(Calculation);
		}
		else if (!(strcmp(command,"FifoSize"))){
			UART_OutString("JSFifoSize: ");
			UART_OutUDec(JSFIFOSIZE);
		}
		else{
			UART_OutString("Command incorrect!");
		}
		*/
  }
}
//--------------end of Task 5-----------------------------

//--------------start of Task 6-----------------------------
void SW2Push(void){
  if(OS_MsTime() > 20 ){ // debounce
			//view = 2;
    if(OS_AddThread(&ButtonWork,128,4)){
      NumCreated++; 
    }
    OS_ClearMsTime();  // at least 20ms between touches
  }
}
//--------------end of Task 6-----------------------------

//--------------start of Task 7-----------------------------

void Screen(void){ 
	uint8_t TitlePos=0,StartPos=5,InPos=8,SetPos=11;
	uint8_t SoundONy=5,SoundONx=4,SoundOFFy=5,SoundOFFx=8,P1y=9,P1x=4,P2y=9,P2x=8;
	int spacing_x=10, spacing_y=6, start_x=-2, start_y=1;
	uint16_t CurrentX,CurrentY;
	uint16_t CharPosX,CharPosY;
  while(1) {
		if(NumSamples < RUNLENGTH){
//			CurrentX = x; CurrentY = y;
			CharPosX = area[0]; //(CurrentX)/6;//
			CharPosY = area[1]; //(CurrentY)/10;// (128-2*12)/13....+2
			OS_bWait(&LCDFree);
			////// My addition below
			// screen has, 21 positions in a rows, 13 positions in single column
			// characters take up 8 spots and there are 2 pixel gaps between letters
			//BSP_LCD_Message(0,10, 0, "Y: ", screen);// for debugging
			if(screen == 1){
					if(TitlePos==CharPosY||TitlePos+1==CharPosY){
						BSP_LCD_DrawString(4,TitlePos,"BATTLESHIP:", LCD_BLUE);
						BSP_LCD_DrawString(4,TitlePos+1,"EMBEDDED", LCD_BLUE);
						view = 1;
					}else{
						BSP_LCD_DrawString(4,TitlePos,"BATTLESHIP:", LCD_WHITE);
						BSP_LCD_DrawString(4,TitlePos+1,"EMBEDDED", LCD_WHITE);
					}
					
					if(StartPos==CharPosY){
						BSP_LCD_DrawString(4,StartPos,"Start Battle", LCD_BLUE);
						view = 4;
					}else
						BSP_LCD_DrawString(4,StartPos,"Start Battle", LCD_WHITE);
					
					if(InPos==CharPosY){
						BSP_LCD_DrawString(4,InPos,"Instructions", LCD_BLUE);
						view = 3;
					}else
						BSP_LCD_DrawString(4,InPos,"Instructions", LCD_WHITE);
					
					if(SetPos==CharPosY){
						BSP_LCD_DrawString(4,SetPos,"Settings", LCD_BLUE);
						view = 2;
						
					}else
						BSP_LCD_DrawString(4,SetPos,"Settings", LCD_WHITE);
					
			}if(screen == 2){
				BSP_LCD_DrawString(4,0,"SETTINGS:", LCD_WHITE);
				BSP_LCD_DrawString(4,3,"Sounds:", LCD_WHITE);
				
				if(SoundONy==CharPosY&&(SoundONx==CharPosX||SoundONx+1==CharPosX)){
					BSP_LCD_DrawString(SoundONx,SoundONy,"ON", LCD_BLUE);
					sound = 1;
				}else
					BSP_LCD_DrawString(SoundONx,SoundONy,"ON", LCD_WHITE);
				
				if(SoundOFFy==CharPosY&&((SoundOFFx<=CharPosX)&&(SoundOFFx+2>=CharPosX))){
					BSP_LCD_DrawString(SoundOFFx,SoundOFFy,"OFF", LCD_BLUE);
					sound = 0;
				}else
					BSP_LCD_DrawString(SoundOFFx,SoundOFFy,"OFF", LCD_WHITE);
				
				BSP_LCD_DrawString(4,7,"Players:", LCD_WHITE);
				
				if(P1y==CharPosY&&((P1x<=CharPosX)&&(P1x+1>=CharPosX))){
					BSP_LCD_DrawString(P1x,P1y,"1P", LCD_BLUE);
					players = 1;
				}else
					BSP_LCD_DrawString(P1x,P1y,"1P", LCD_WHITE);
				
				if(P2y==CharPosY&&((P2x<=CharPosX)&&(P2x+1>=CharPosX))){
					BSP_LCD_DrawString(P2x,P2y,"2P", LCD_BLUE);
					players = 2; // need to make it so that a button click makes this value happen
				}else
					BSP_LCD_DrawString(P2x,P2y,"2P", LCD_WHITE);
				
				view = 1;
				
			}if(screen == 3){
				BSP_LCD_DrawString(4,0,"INSTRUCTIONS:", LCD_WHITE);
				BSP_LCD_DrawString(0,9,"will be determined...", LCD_WHITE);
				view = 1;
				
			}if(screen == 4){
				BSP_LCD_DrawString(4,0,"WAITING:", LCD_WHITE);
				view = 5;
				
			}if(screen == 5){// setting up screen
				// have numbers that can be clicked on 
				// number will make a ship pop up

				
				
				// top left screen
				BSP_LCD_DrawString(0,0,"T-30", LCD_WHITE);
				
				
				// I need a semaphore for ship select. 
				// number display
				for(i = 0; i<SHIPNUM; i++){
					if(ship_y[i]==CharPosY&&ship_x[i]==CharPosX&&ship_display[i]==0){
						sprintf(ship_buf,"%d",ship_size[i]);
						BSP_LCD_DrawString(ship_x[i],ship_y[i],ship_buf, LCD_BLUE);
						ship_view[i] = 6;			
					}else if(ship_display[i]==0){
						sprintf(ship_buf,"%d",ship_size[i]);
						BSP_LCD_DrawString(ship_x[i],ship_y[i],ship_buf, LCD_WHITE);
						ship_view[i] = 5;
					}else if(ship_display[i]==1){
						BSP_LCD_DrawString(ship_x[i],ship_y[i]," ", LCD_WHITE);
						ship_view[i] = 5;
					}
				}
				
				/*
				for(i = 0; i<SHIPNUM; i++){
					if(ship_display[i]==1){
						for(j=0;j<MAXSHIPSIZE;j++){
							ship_blocks_pos_x[j] = ship_setup_x[j]+CharPosX;
							ship_blocks_pos_y[j] = ship_setup_y[j]+CharPosY;
							BSP_LCD_FillRect(ship_blocks_pos_x[j]*10, ship_blocks_pos_y[j]*6, 10, 6, LCD_GREY);
						}
					}
				}
				*/
				
				
				// top right screen
				for(i = 0; i<=100; i=i+10){
					BSP_LCD_DrawFastVLine( 27+i, 0, 60, LCD_WHITE); // grid vertical lines
				}
				
				for(i = 0; i<60; i=i+6){
					BSP_LCD_DrawFastHLine( 27, i, 128, LCD_WHITE); // grid horizontal line
				}
				
				// dividing lines
				for(i = 0; i<=6; i++){ 
					BSP_LCD_DrawFastHLine( 0, 60+i, 128, LCD_WHITE); // dividing line
				}
				
				BSP_LCD_FillRect(2, 90, 10, 6, LCD_WHITE); // just for debugging
					
			}if(screen == 6){// placing ship //////////////////////////////////////////////
			
				BSP_LCD_FillRect(90, 90, 10, 6, LCD_WHITE); // just for debugging
				// top left screen
				BSP_LCD_DrawString(0,0,"T-29", LCD_WHITE);
				
				// number display
				for(i = 0; i<SHIPNUM; i++){
					if(ship_y[i]==CharPosY&&ship_x[i]==CharPosX&&ship_display[i]==0){
						sprintf(ship_buf,"%d",ship_size[i]);
						BSP_LCD_DrawString(ship_x[i],ship_y[i],ship_buf, LCD_BLUE);
						ship_view[i] = 6;			
					}else if(ship_display[i]==0){
						sprintf(ship_buf,"%d",ship_size[i]);
						BSP_LCD_DrawString(ship_x[i],ship_y[i],ship_buf, LCD_WHITE);
						ship_view[i] = 5;
					}else if(ship_display[i]==1){
						BSP_LCD_DrawString(ship_x[i],ship_y[i]," ", LCD_WHITE);
						ship_view[i] = 5;
					}
				}
				
				view = 5;
				
				// moving ship blocks
				//BSP_LCD_FillScreen(BGCOLOR);
				for(i = 0; i<SHIPNUM; i++){
					if(ship_display[i]==1){
						for(j=0;j<MAXSHIPSIZE;j++){
							ship_blocks_pos_x[j] = (ship_setup_x[j]*spacing_x+x+start_x)/spacing_x;
							ship_blocks_pos_y[j] = (ship_setup_y[j]*spacing_y+y+start_y)/spacing_y;
							
							if((ship_eraser_x[j]!=ship_blocks_pos_x[j]*spacing_x+(spacing_x+start_x))||(ship_eraser_y[j]!=ship_blocks_pos_y[j]*spacing_y+(start_y))){
								BSP_LCD_FillRect(ship_eraser_x[j], ship_eraser_y[j], spacing_x-1, spacing_y-1, LCD_BLACK);
							}
							BSP_LCD_FillRect(ship_blocks_pos_x[j]*spacing_x+(spacing_x+start_x), ship_blocks_pos_y[j]*spacing_y+(start_y), spacing_x-1, spacing_y-1, LCD_GREY);
							
							ship_eraser_x[j]=ship_blocks_pos_x[j]*spacing_x+(spacing_x+start_x);
							ship_eraser_y[j]=ship_blocks_pos_y[j]*spacing_y+(start_y);
						}
					}
				}
				
				// top right screen
				for(i = 0; i<=100; i=i+10){
					BSP_LCD_DrawFastVLine( 27+i, 0, 60, LCD_WHITE); // grid vertical lines
				}
				
				for(i = 0; i<60; i=i+6){
					BSP_LCD_DrawFastHLine( 27, i, 128, LCD_WHITE); // grid horizontal line
				}
				
				// dividing lines
				for(i = 0; i<=6; i++){ 
					BSP_LCD_DrawFastHLine( 0, 60+i, 128, LCD_WHITE); // dividing line
				}
				
			}if(screen == 7){// attacking screen
				
			}if(screen == 8){// waiting for attack
				
			}if(screen == 9){// win-lose screen
				
			}
			
			////// My addition above.
			OS_bSignal(&LCDFree);
			OS_Suspend(); 
		}
  }
}
//--------------end of Task 7-----------------------------

void CrossHair_Init(void){
	BSP_LCD_FillScreen(BGCOLOR);
	BSP_Joystick_Input(&origin[0],&origin[1],&select);
}

//******************* Main Function**********
int main(void){
  OS_Init();           // initialize, disable interrupts
	Device_Init();
  CrossHair_Init();
  DataLost = 0;        // lost data between producer and consumer
  NumSamples = 0;
  MaxJitter = 0;       // in 1us units

//********initialize communication channels
  JsFifo_Init();

//*******attach background tasks***********
  OS_AddSW1Task(&SW1Push,2);
	OS_AddSW2Task(&SW2Push,2);
	
  OS_AddPeriodicThread(&Producer,PERIOD,1); // 2 kHz real time sampling of PD3
	
  NumCreated = 0;
// create initial foreground threads
  NumCreated += OS_AddThread(&Interpreter, 128,2); 
  NumCreated += OS_AddThread(&Consumer, 128,1); 
	NumCreated += OS_AddThread(&CubeNumCal, 128,1); 
	NumCreated += OS_AddThread(&Screen, 128,1); 
 
	// change for Part 5) 
  OS_Launch(TIME_500US); // doesn't return, interrupts enabled in here
	return 0;            // this never executes
}
