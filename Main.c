// Main.c
// Runs on LM4F120/TM4C123
// You may use, edit, run or distribute this file 
// You are free to change the syntax/organization of this file

// Jonathan W. Valvano 2/20/17, valvano@mail.utexas.edu
// Modified by Sile Shu 10/4/17, ss5de@virginia.edu
// Modified by Mustafa Hotaki 7/29/18, mkh3cf@virginia.edu

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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

uint32_t PseudoCount;

uint16_t origin[2]; 	// The original ADC value of x,y if the joystick is not touched, used as reference
uint16_t water_color = LCD_BLUE;
uint16_t ship_color = LCD_BLACK;
uint16_t miss_color = LCD_WHITE;
uint16_t wait_color = LCD_GREY;
uint16_t grid_color = LCD_GREY;
uint16_t hit_color = LCD_RED;
uint16_t go_color = LCD_GREEN;

int val = 0;

char letter ='0';
char starter_data;
char Opp_finish;
char their_bomb_x;
char their_bomb_y;
char I_am_waiting;
char I_finish=0;

uint8_t select;  			// joystick push
uint8_t area[2];
uint8_t screen=1,view=1;
uint8_t sound=0,players=1,player=0;
uint8_t ship_x[SHIPNUM]={0,2,0,2,0};
uint8_t ship_y[SHIPNUM]={2,2,3,3,4};
uint8_t ship5x[5],ship4x[4],ship3x[3],ship2x1[2],ship2x2[2];
uint8_t ship5y[5],ship4y[4],ship3y[3],ship2y1[2],ship2y2[2];
uint8_t ship5count=0,ship4count=0,ship3count=0,ship2count=0,ship2count2=0;
uint8_t ship_setup_x[SHIPNUM],ship_setup_y[SHIPNUM];
uint8_t ship_display[SHIPNUM]={0,0,0,0,0},ship_size[SHIPNUM]={5,4,3,2,2};
uint8_t i=0,j=0; // for loop indexing
uint8_t ship_view[SHIPNUM];
uint8_t ship_orientation = 0;

//uint8_t Opp_finish=0;
uint8_t my_ship_hit=0;
uint8_t opp_ship_hit=0;

int16_t x=63;  			// horizontal position of the crosshair, initially 63
int16_t y=63;  			// vertical position of the crosshair, initially 63
int16_t prevx, prevy;	// Previous x and y values of the crosshair

int8_t ship_blocks_pos_x[5]={0,0,0,0,0},ship_blocks_pos_y[5]={0,0,0,0,0};
int8_t ship_eraser_x[5]={10,10,10,10,10},ship_eraser_y[5]={10,10,10,10,10};

char ship_buf[1];

uint8_t PGG[10][10] = { {0,0,0,0,0,0,0,0,0,0},  
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0}, }; // player game grid

uint8_t OGG[10][10] = { {0,0,0,0,0,0,0,0,0,0},  
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0}, }; // opponent game grid

uint8_t HOGG[10][10] = { {20,20,20,20,20,20,20,20,20,20},  
												{20,20,20,20,20,20,20,20,20,20},  
												{20,20,20,20,20,20,20,20,20,20},  
												{20,20,20,20,20,20,20,20,20,20},  
												{20,20,20,20,20,20,20,20,20,20},  
												{20,20,20,20,20,20,20,20,20,20},  
												{20,20,20,20,20,20,20,20,20,20},  
												{20,20,20,20,20,20,20,20,20,20},  
												{20,20,20,20,20,20,20,20,20,20},  
												{20,20,20,20,20,20,20,20,20,20},};// hidden opponent game grid


uint8_t Top[10][10] = { {0,0,0,0,0,0,0,0,0,0},  
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0}, }; // hidden opponent game grid


uint8_t Bott[10][10] = { {0,0,0,0,0,0,0,0,0,0},  
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0},
												{0,0,0,0,0,0,0,0,0,0}, }; // hidden opponent game grid
												
int8_t spacing_x=10, spacing_y=6, start_x=-2, start_y=1;

unsigned long NumCreated;   		// Number of foreground threads created
//unsigned long NumSamples;   		// Incremented every ADC sample, in Producer
unsigned long UpdateWork;   		// Incremented every update on position values

//---------------------User debugging-----------------------
unsigned long DataLost;     // data sent by Producer, but not received by Consumer
long MaxJitter;             // largest time jitter between interrupts in usec
#define JITTERSIZE 64 // -------------- CHANGE THIS STUFF -----------------
unsigned long const JitterSize=JITTERSIZE;
unsigned long JitterHistogram[JITTERSIZE]={0,};
unsigned long TotalWithI1;
unsigned short MaxWithI1;

// function prototypes
void grid_plotter(uint8_t xpos, uint8_t ypos, int16_t blkColor);
void grid_plotter_bottom(uint8_t xpos, uint8_t ypos, int16_t blkColor);
void rndmBtGn(void);
void bomb_spot(void);
void rand_bomb_cpu(void);
void grid_lines(void);
void color_plotter(uint8_t data[10][10]);
void color_plotter_bottom(uint8_t data[10][10]);
void refresh_vars(void);
void send_data(char some_data);
void player_num(void);
char receive_data(void);

void player_num(void){
	if(player==1){
			BSP_LCD_DrawString(0,0,"P-1", LCD_BLUE);
	}else if(player==2){
			BSP_LCD_DrawString(0,0,"P-2", LCD_WHITE);
	}
	return;
}

void send_data(char some_data){
	// clean up hardware fifo
	while((UART1_FR_R&UART_FR_TXFF) == 0){// transmit // && (Tx_UARTFifo_Size() > 0)){
		UART1_DR_R = some_data;
	}
	return;
}

char receive_data(void){
	// clean up hardware fifo
	char letter_received = 0x00; //0x73; // 0111 1111
	// 0xfd 						1111 1101
	while(((UART1_FR_R&UART_FR_RXFE) == 0)){// rest not necessary && (Rx_UARTFifo_Size() < (FIFOSIZE - 1))){
		letter_received = UART1_DR_R;
	}
	return letter_received;
}
// user functions
void refresh_vars(void){
	uint8_t index_x, index_y;
	
	ship_orientation = 0;
	
	I_finish=0;
	Opp_finish=0;
	my_ship_hit=0;
	opp_ship_hit=0;
	
	for(index_x=0;index_x<SHIPNUM;index_x++){
		ship_display[index_x]=0;
	}
	for(index_x=0;index_x<10;index_x++){
		for(index_y=0;index_y<10;index_y++){
			PGG[index_x][index_y]=0; // player game grid
			OGG[index_x][index_y]=0; // opponents game grid
			HOGG[index_x][index_y]=20; // hidden game grid
		}
	}
	return;
}

void rndmBtGn(void){
	// need to make it more random....
	uint8_t checker = 1, index_x, index_y, index=0,  index_2=0,  orientation=0, temp_ship_x[5]={0,0,0,0,0}, temp_ship_y[5]={0,0,0,0,0};	
	uint8_t count_2;
	while(checker==1){
		count_2 = 0;
		for(index_x=0;index_x<10;index_x++){
			for(index_y=0;index_y<10;index_y++){
				HOGG[index_x][index_y]=0;
			}
		}
		//index_x = rand();
		//index_y = rand();
		for(index=0;index<SHIPNUM;index++){
			// generates a random orientation
			orientation = rand()%2;
			// generates a random location within the grid that fits the ship
			if(orientation==0){  
				index_x=(rand()+x+y)%(10-ship_size[index]);
				index_y=(rand()%(x+y)+index_x)%10;
				for(index_2=0;index_2<SHIPNUM;index_2++){
					temp_ship_x[index_2]=index_x+index_2%ship_size[index];
					temp_ship_y[index_2]=index_y;
					if(index_2<ship_size[index]){
						HOGG[temp_ship_x[index_2]][temp_ship_y[index_2]]++;
					}
				}
			}else{
				index_x=(rand()+index+x%y)%10;
				index_y=(rand()+index_x)%(10-ship_size[index]);
				for(index_2=0;index_2<SHIPNUM;index_2++){
					temp_ship_x[index_2]=index_x;
					temp_ship_y[index_2]=index_y+index_2%ship_size[index];
					if(index_2<ship_size[index]){
						HOGG[temp_ship_x[index_2]][temp_ship_y[index_2]]++;
						// method for keeping track of ships and if they have been sunk. 
						if(ship_size[index]==2&&count_2<2){
							count_2++;
							ship2x1[index_2]=temp_ship_x[index_2];
							ship2y1[index_2]=temp_ship_y[index_2];
							ship2count=2;
							
						}else if(ship_size[index]==2&&count_2>=2){
							count_2++;
							ship2x2[index_2]=temp_ship_x[index_2];
							ship2y2[index_2]=temp_ship_y[index_2];
							ship2count2=2;
							if(count_2==4){
								count_2=0;
							}
							
						}else if(ship_size[index]==3){
							ship3x[index_2]=temp_ship_x[index_2];
							ship3y[index_2]=temp_ship_y[index_2];
							ship3count=3;
							
						}else if(ship_size[index]==4){
							ship4x[index_2]=temp_ship_x[index_2];
							ship4y[index_2]=temp_ship_y[index_2];
							ship4count=4;
							
						}else if(ship_size[index]==5){
							ship5x[index_2]=temp_ship_x[index_2];
							ship5y[index_2]=temp_ship_y[index_2];
							ship5count=5;
							
						}
					}
				}
			}
			// might have to be some type of while loop that checks to see if those positions are good. using both temp_ship_x and y
		}
		
		checker = 0;
		for(index_x=0;index_x<10;index_x++){
			for(index_y=0;index_y<10;index_y++){
				if(HOGG[index_x][index_y]>1){
					checker = 1;
				}
			}
		}
	}
	return;
}

void bomb_spot(void){
//	uint8_t indexer;
	uint8_t index_x = (x+start_x)/spacing_x-2,index_y=(y+start_y)/spacing_y;
	
	OGG[index_x][index_y]+=(2+HOGG[index_x][index_y]);
	
	if(OGG[index_x][index_y]==3){
		opp_ship_hit++;
	}
	
	/*
	// ship 2.1
	if(ship2count>0){
		for(indexer=0;indexer<2;indexer++){
			if(ship2x1[indexer]==index_x && ship2y1[indexer]==index_y){
				ship2count--;
			}
		}
		if(ship2count==0){
			//buzzer goes off
			OGG[index_x][index_y]++;
		}
	}
	
	// ship 2.2
	if(ship2count2>0){
		for(indexer=0;indexer<2;indexer++){
			if(ship2x2[indexer]==index_x && ship2y2[indexer]==index_y){
				ship2count2--;
			}
		}
		if(ship2count2==0){
			//buzzer goes off
			OGG[index_x][index_y]++;
		}
	}
	
	// ship 3
	if(ship3count>0){
		for(indexer=0;indexer<3;indexer++){
			if(ship3x[indexer]==index_x && ship3y[indexer]==index_y){
				ship3count--;
//				sprintf(ship_buf,"%d",ship3count);
//				BSP_LCD_DrawString(0,0,ship_buf, LCD_WHITE);
			}
		}
		if(ship3count==0){
			//buzzer goes off
			OGG[index_x][index_y]++;
		}
	}
	if(ship4count>0){
		for(indexer=0;indexer<4;indexer++){
			if(ship4x[indexer]==index_x && ship4y[indexer]==index_y){
				ship4count--;
			}
		}
		if(ship4count==0){
			//buzzer goes off
			OGG[index_x][index_y]++;
		}
	}
	if(ship5count>0){
		for(indexer=0;indexer<5;indexer++){
			if(ship5x[indexer]==index_x && ship5y[indexer]==index_y){
				ship5count--;
			}
		}
		if(ship5count==0){
			//buzzer goes off
			OGG[index_x][index_y]++;
		}
	}
	*/
	//check and see if the bomb is in the same location as one of the existing ships
	return;
}

void rand_bomb_cpu(void){
	uint8_t index_x, index_y;
	index_x=rand()%10;
	index_y=(rand()+index_x)%10;
	while(PGG[index_x][index_y]>1){
			index_x=(rand()+index_x)%10;
			index_y=(rand()+index_x)%10;
	}
	PGG[index_x][index_y]+=2;
	if(PGG[index_x][index_y]==3){
		my_ship_hit++;
	}
	return;
}



void Device_Init(void){
	UART_Init();
	BSP_LCD_OutputInit();
	BSP_Joystick_Init();
}

// 10 functions

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
	
	if(screen==5 && x > 27-CROSSSIZE){
		x = 27-CROSSSIZE;
	}else if(screen==6 && x>120 && ship_orientation==0){
		x = 120;
	}else if(screen==6 && x>120-CROSSSIZE-(ship_setup_x[4]+1)*6 && ship_orientation==1){
		x = 120 - CROSSSIZE-(ship_setup_x[4]+1)*6;
	}else if(screen==7 && x>120-CROSSSIZE){
		x = 120 - CROSSSIZE;
	}else if(x > 127){
		x = 127;
	}
	
	if( (screen==6||screen==7) && x < 27){
		x = 27;
	}else if (x < 0){
		x = 0;
	}
	
	if(screen==7&& y > 66 - CROSSSIZE-(1)*6){
		y = 66 - CROSSSIZE-(1)*6;
	}else if (screen==5 && y > 66 - CROSSSIZE-(ship_setup_y[4]+1)*6 && ship_orientation==0){
		y = 66 - CROSSSIZE-(ship_setup_y[4]+1)*6;
	}else if (screen==6 && y > 66 - CROSSSIZE-(ship_setup_y[4]+1)*6 && ship_orientation==0){
		y = 66 - CROSSSIZE-(ship_setup_y[4]+1)*6;
	}else if (screen==6 && y > 60 - CROSSSIZE && ship_orientation==1){
		y = 60 - CROSSSIZE;
	}else if (y > 127){
		y = 127;
	}
	
	if (y < 0){
		y = 0;
	}
	
	data->x = x; 
	data->y = y;
	
	return 1;
}
////////////////////////////////////// periodic thread //////////////////////////////////////
void Producer(void){ 
	uint16_t rawX,rawY; // raw adc value
	uint8_t select;
	jsDataType data;
	BSP_Joystick_Input(&rawX,&rawY,&select);
	UpdateWork += UpdatePosition(rawX,rawY,&data); // calculation work
	if(JsFifo_Put(data) == 0){ // send to consumer
		DataLost++;
	}
}
//--------------end of Task 1-----------------------------

//--------------Task 2-----------------------------
// making the game grid
void grid_lines(void){
	// top right screen //
	for(i = 0; i<=100; i=i+10){
		BSP_LCD_DrawFastVLine( 27+i, 0, 127, grid_color); 	// grid vertical lines
	}
	for(i = 0; i<60; i=i+6){
		BSP_LCD_DrawFastHLine( 27, i, 127, grid_color); 		// grid horizontal line
	}
	BSP_LCD_FillRect(0, 60, 128, 7, grid_color); // dividing lines
	
	// bottom right screen //
	for(i = 72; i<127; i=i+6){
		BSP_LCD_DrawFastHLine( 27, i, 128, grid_color); // grid horizontal line
	}
	return;
}
////////////////////////////////////// periodic thread //////////////////////////////////////
void Grid_divider(void){
	while(1){
		OS_bWait(&LCDFree);
		
		if(screen>=5&&screen<=8){
			grid_lines();
		}
		OS_bSignal(&LCDFree);

		if(screen<=8)
			OS_Suspend();
		else if(screen>8)
			OS_Kill();
	}
}
//--------------end of Task 2-----------------------------

//--------------Task 3-----------------------------
void color_plotter(uint8_t data[10][10]){
	uint8_t index_x, index_y;
	for(index_y=0; index_y<10; index_y++){
		for(index_x=0; index_x<10; index_x++){
			if(data[index_x][index_y]==0){
				grid_plotter(index_x+2,index_y,water_color);
			}else if(data[index_x][index_y]==1){
				grid_plotter(index_x+2,index_y,ship_color);
			}else if(data[index_x][index_y]==2){
				grid_plotter(index_x+2,index_y,miss_color);
			}else if(data[index_x][index_y]==3){
				grid_plotter(index_x+2,index_y,hit_color);
			}
		}
	}
	return;
}

void color_plotter_bottom(uint8_t data[10][10]){
	uint8_t index_x, index_y;
	for(index_y=0; index_y<10; index_y++){
		for(index_x=0; index_x<10; index_x++){
			if(data[index_x][index_y]==0){
				grid_plotter_bottom(index_x+2,index_y,water_color);
			}else if(data[index_x][index_y]==1){
				grid_plotter_bottom(index_x+2,index_y,ship_color);
			}else if(data[index_x][index_y]==2){
				grid_plotter_bottom(index_x+2,index_y,miss_color);
			}else if(data[index_x][index_y]==3){
				grid_plotter_bottom(index_x+2,index_y,hit_color);
			}else if(data[index_x][index_y]==4){
				grid_plotter_bottom(index_x+2,index_y,LCD_LIGHTGREEN);
			}
		}
	}
	return;
}

void grid_plotter(uint8_t xpos, uint8_t ypos, int16_t blkColor){
	BSP_LCD_FillRect(xpos*spacing_x+(spacing_x+start_x), ypos*spacing_y+(start_y), spacing_x-1, spacing_y-1, blkColor);
	return;
}

void grid_plotter_bottom(uint8_t xpos, uint8_t ypos, int16_t blkColor){
	BSP_LCD_FillRect(xpos*spacing_x+(spacing_x+start_x), ypos*spacing_y+(start_y)+66, spacing_x-1, spacing_y-1, blkColor);
	return;
}
////////////////////////////////////// periodic thread //////////////////////////////////////
void Game_grid(void){
	while(1){
		OS_bWait(&LCDFree);
		
		if(screen>=5&&screen<=8){
			if(screen <=6){
				color_plotter(PGG);
				color_plotter_bottom(OGG);
			}else{
				color_plotter(OGG);
				color_plotter_bottom(PGG);
			}
		}
		OS_bSignal(&LCDFree);

		if(screen<=8)
			OS_Suspend();
		else if(screen>8)
			OS_Kill();
	}
}
//--------------end of Task 3-----------------------------


//------------------Task 4--------------------------------
////////////////////////////////////// background thread //////////////////////////////////////
// background thread executes with SW1 button
// one foreground task created with button push
// foreground treads run for 1 sec and die
// ***********ButtonWork*************
void ButtonWork(void){
	ship_orientation = 0;
	OS_bWait(&LCDFree);
	// all ship movements and selecting
	if( screen == 5){
		for(i=0; i<SHIPNUM; i++){
			if(area[0]==ship_x[i] && area[1]==ship_y[i]&&ship_display[i]==0&& ship_view[i]==6){
				
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
	}else if(screen==6){
		view = 5;
		for(i=0; i<SHIPNUM; i++){
			PGG[ship_blocks_pos_x[i]-2][ship_blocks_pos_y[i]] = 1;
		}
		
		if(ship_display[0]+ship_display[1]+ship_display[2]+ship_display[3]+ship_display[4]==5){
			if(players==1){
				rndmBtGn();
				view = 7;
			}else{
				if(player==1){
					
					// receive_HOGG
					// send_PGG
					view = 7;
				}else if(player==2){
					// send_PGG
					// receive_HOGG
					view = 8;
				}
			}
		}
		
	}else if(screen == 7){
		
		if(players==1){
			bomb_spot();
			view=8;
		}else{
			//UART Commands
			// send location of bomb ------------------------------------------------------- figure out the specifics
			char they_r_waiting = receive_data();
			void send_data(char bomb_x);
			void send_data(char bomb_y);
			
			I_finish=1; // send this through UART
			send_data(I_finish); // turn ended can be a 1
			
			// reset I_finish 
			I_finish=0;
			
		}
	}
	
	screen = view; 
	BSP_LCD_FillScreen(BGCOLOR);	
	
	OS_bSignal(&LCDFree);
  OS_Kill();  // done, OS does not return from a Kill
} 

////////////////////////////////////// aperiodic thread //////////////////////////////////////
// remains untouched, this debounces
void SW1Push(void){
  if(OS_MsTime() > 20 ){ // debounce
    if(OS_AddThread(&ButtonWork,128,2)){
      NumCreated++; 
    }
    OS_ClearMsTime();  // at least 20ms between touches
  }
}

//--------------end of Task 4-----------------------------

//------------------Task 5--------------------------------

////////////////////////////////////// periodic thread //////////////////////////////////////
//******** Consumer *************** 
// foreground thread, accepts data from producer
// Display crosshair and its positions
// inputs:  none
// outputs: none
void Consumer(void){
	while(1){
		jsDataType data;
		JsFifo_Get(&data);
		
		OS_bWait(&LCDFree);

		if((screen<6&&players==1)||(screen!=6&&screen!=7&&players==2)){
			if(prevx!=data.x||prevy!=data.y){
				BSP_LCD_DrawCrosshair(prevx, prevy, LCD_BLACK); // Draw a black crosshair
			}
			BSP_LCD_DrawCrosshair(data.x, data.y, LCD_RED); // Draw a red crosshair
		}
		
		OS_bSignal(&LCDFree);
		prevx = data.x; 
		prevy = data.y;
	}
  OS_Kill();  // done
}
//--------------end of Task 5-----------------------------

//------------------Task 6--------------------------------
////////////////////////////////////// periodic thread //////////////////////////////////////
// foreground thread that runs without waiting or sleeping
// it executes some calculation related to the position of crosshair 
//******** CubeNumCalc *************** 
// foreground thread, calculates the virtual cube number for the crosshair
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none
void PositionCal(void){ 
  while(1) {
		area[0] = x / 6;
		area[1] = y / 10;
  }
}
//--------------end of Task 6-----------------------------

/*
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
  while(1){
		
  }
}
//--------------end of Task 5-----------------------------
*/
//--------------start of Task 7-----------------------------
////////////////////////////////////// background thread //////////////////////////////////////
void ButtonWork2(void){
	uint8_t temp_switch[5],index;
	
	if(screen==6){
		ship_orientation = (ship_orientation+1)%2;
		for(index=0;index<SHIPNUM;index++){
			temp_switch[index]=ship_setup_y[index];
			ship_setup_y[index]=ship_setup_x[index];
			ship_setup_x[index]=temp_switch[index];
		}
	}
	
	OS_Kill();  // done, OS does not return from a Kill
}

////////////////////////////////////// aperiodic thread //////////////////////////////////////
void SW2Push(void){
  if(OS_MsTime() > 20 ){ // debounce
    if(OS_AddThread(&ButtonWork2,128,2)){
      NumCreated++; 
    }
    OS_ClearMsTime();  // at least 20ms between touches
  }
}
//--------------end of Task 6-----------------------------

//--------------start of Task 7-----------------------------
void State(void){ 
//	uint8_t index_y, index_x;
	uint8_t grid_on, board_on;
	uint8_t TitlePos=0,StartPos=5,InPos=8,SetPos=11;
	uint8_t SoundONy=5,SoundONx=4,SoundOFFy=5,SoundOFFx=8,P1y=9,P1x=4,P2y=9,P2x=8;
//	uint16_t CurrentX,CurrentY;
	uint16_t CharPosX,CharPosY;
  while(1) {
		CharPosX = area[0]; 
		CharPosY = area[1]; 
		
		OS_bWait(&LCDFree);
		
		if(screen == 1){ // Main menu
			
				BSP_LCD_DrawString(4,TitlePos,"BATTLESHIP:", LCD_WHITE);
				BSP_LCD_DrawString(4,TitlePos+1,"EMBEDDED", LCD_WHITE);
				
				if(StartPos==CharPosY||(view==4||view==5)){
					BSP_LCD_DrawString(4,StartPos,"Start Battle", LCD_BLUE);
					if(players==2){
						view = 4;
					}else{
						view = 5;
					}
					grid_on = 0;
					board_on = 0;
				}else
					BSP_LCD_DrawString(4,StartPos,"Start Battle", LCD_WHITE);
				
				if(InPos==CharPosY||view==3){
					BSP_LCD_DrawString(4,InPos,"Instructions", LCD_BLUE);
					view = 3;
				}else
					BSP_LCD_DrawString(4,InPos,"Instructions", LCD_WHITE);
				
				if(SetPos==CharPosY||view==2){
					BSP_LCD_DrawString(4,SetPos,"Settings", LCD_BLUE);
					view = 2;
				}else
					BSP_LCD_DrawString(4,SetPos,"Settings", LCD_WHITE);
				
		}else if(screen == 2){// setting screen
			
			BSP_LCD_DrawString(4,0,"SETTINGS:", LCD_WHITE);
			
			/*
			BSP_LCD_DrawString(4,3,"Sounds:", LCD_WHITE);
			
			if((SoundONy==CharPosY&&(SoundONx==CharPosX||SoundONx+1==CharPosX))||sound==1){
				BSP_LCD_DrawString(SoundONx,SoundONy,"ON", LCD_BLUE);
				sound = 1;
			}else
				BSP_LCD_DrawString(SoundONx,SoundONy,"ON", LCD_WHITE);
			
			if((SoundOFFy==CharPosY&&((SoundOFFx<=CharPosX)&&(SoundOFFx+2>=CharPosX)))||sound==0){
				BSP_LCD_DrawString(SoundOFFx,SoundOFFy,"OFF", LCD_BLUE);
				sound = 0;
			}else
				BSP_LCD_DrawString(SoundOFFx,SoundOFFy,"OFF", LCD_WHITE);
			*/
			
			BSP_LCD_DrawString(4,7,"Players:", LCD_WHITE);
			
			if((P1y==CharPosY&&((P1x<=CharPosX)&&(P1x+1>=CharPosX)))||players==1){
				BSP_LCD_DrawString(P1x,P1y,"1P", LCD_BLUE);
				players = 1;
			}else
				BSP_LCD_DrawString(P1x,P1y,"1P", LCD_WHITE);
			
			if((P2y==CharPosY&&((P2x<=CharPosX)&&(P2x+1>=CharPosX)))||players==2){
				BSP_LCD_DrawString(P2x,P2y,"2P", LCD_BLUE);
				players = 2; // need to make it so that a button click makes this value happen
			}else
				BSP_LCD_DrawString(P2x,P2y,"2P", LCD_WHITE);
			
			view = 1;
			
		}else if(screen == 3){ // instructions screen
			
			BSP_LCD_DrawString(4,0,"INSTRUCTIONS:", LCD_WHITE);
			BSP_LCD_DrawString(0,9,"will be determined...", LCD_WHITE);
			view = 1;
			
		}else if(screen == 4){ // battle screen
			view = 5;
			starter_data = receive_data();
			// the one to receive data first will be the one to go
			if(starter_data == '0'){
				player = 2;
//				BSP_LCD_DrawString(0,5,"You are Player 2!", LCD_CYAN);
//				BSP_LCD_DrawString(0,7,"Press Btn 1 to start.", LCD_CYAN);
				players=1;
				BSP_LCD_DrawString(0,1,"You would have been", LCD_CYAN);
				BSP_LCD_DrawString(0,2,"player 2 of 2, but", LCD_CYAN);
				BSP_LCD_DrawString(0,3,"the game is only for", LCD_CYAN);
				BSP_LCD_DrawString(0,4,"1 player for now...", LCD_CYAN);
				
				BSP_LCD_DrawString(0,6,"Press Btn 1 to", LCD_CYAN);
				BSP_LCD_DrawString(0,7,"continue in", LCD_CYAN);
				BSP_LCD_DrawString(0,8,"1 player mode.", LCD_CYAN);
			}else{
				send_data('0');
				player = 1;
				BSP_LCD_DrawString(0,1,"You would have been", LCD_GREEN);
				BSP_LCD_DrawString(0,2,"player 1 of 2, but", LCD_GREEN);
				BSP_LCD_DrawString(0,3,"the game is only for", LCD_GREEN);
				BSP_LCD_DrawString(0,4,"1 player for now...", LCD_GREEN);
				
				BSP_LCD_DrawString(0,6,"Press Btn 1 to", LCD_GREEN);
				BSP_LCD_DrawString(0,7,"continue in", LCD_GREEN);
				BSP_LCD_DrawString(0,8,"1 player mode.", LCD_GREEN);
//				BSP_LCD_DrawString(0,5,"You are Player 1!", LCD_GREEN);
//				BSP_LCD_DrawString(0,7,"Press Btn 1 to start.", LCD_GREEN);
				players=1;
			}
			
			
			// use uart to transmit, and once we recieve we can move fortward. 
			// We need to determine who goes first here... that will determine who sends and who receives first @ different parts of the game
			
			
		}else if(screen == 5){// setting up screen
			// have numbers that can be clicked on 
			// number will make a ship pop up
			
			if(grid_on==0){
				if(OS_AddThread(&Grid_divider,128,2)){
					NumCreated++; 
					grid_on=1;
				}
			}
			
			if(board_on==0){
				if(OS_AddThread(&Game_grid,128,2)){
					NumCreated++; 
					board_on=1;
				}
			}
			
			// top left screen
			player_num();
							
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
				}
			}
			
			// top right screen /////////////////////////////////////////////////////////////////////////////////////
			//grid_lines();
//			color_plotter(PGG);
//			color_plotter_bottom(OGG);
//			Top = PGG;
//			Bott = OGG;
			
		}else if(screen == 6){// placing ship //////////////////////////////////////////////
			
			// top left screen
			player_num();
							
			// moving ship blocks
			for(i = 0; i<SHIPNUM; i++){
				if(ship_display[i]==1){
					for(j=0;j<MAXSHIPSIZE;j++){
						ship_blocks_pos_x[j] = (ship_setup_x[j]*spacing_x+x+start_x)/spacing_x;//box index
						ship_blocks_pos_y[j] = (ship_setup_y[j]*spacing_y+y+start_y)/spacing_y;
						
						if(ship_eraser_x[j]!=ship_blocks_pos_x[j]||ship_eraser_y[j]!=ship_blocks_pos_y[j]){
							grid_plotter(ship_eraser_x[j],ship_eraser_y[j],LCD_BLUE);
						}
						grid_plotter(ship_blocks_pos_x[j],ship_blocks_pos_y[j],LCD_BLACK);
						
						ship_eraser_x[j]=ship_blocks_pos_x[j];
						ship_eraser_y[j]=ship_blocks_pos_y[j];
						
						// finding a way to save boat. 
						// index for the y is accurate if we start indexing at 0
						// index for the x is off by 2 if we start indexing at 0
						// BSP_LCD_FillRect(0*spacing_x+(spacing_x+start_x), 0*spacing_y+(start_y), spacing_x-1, spacing_y-1, LCD_BLUE); 
						
					}
				}
			}
			
			// top right screen /////////////////////////////////////////////////////////////////////////////////////
			//grid_lines();
//			color_plotter(PGG);
//			color_plotter_bottom(OGG);
//			Top = PGG;
//			Bott = OGG;
			
		}else if(screen == 7){// attacking screen
			
			// top left screen
			player_num();
							
			if(my_ship_hit==16){
				view = 10;
				screen = 10;// lose screen
			}else{
				// top left screen
//				BSP_LCD_DrawString(0,0,"T-27", LCD_WHITE);
				// Game grid
				//grid_lines();
//				color_plotter(OGG);
//				color_plotter_bottom(PGG);
//				Top = OGG;
//				Bott = PGG;
				
				grid_plotter((x+start_x)/spacing_x,(y+start_y)/spacing_y,go_color); // Cursor
			}
		}else if(screen == 8){// waiting for attack
			
			// top left screen
			player_num();
							
			if(opp_ship_hit==16){
				view = 9;
				screen = 9;// win screen
			}else{
				if(players==1){
					rand_bomb_cpu();
					screen=7;// back to attack screen
					view=7;
				}else{
					// top left screen
//					BSP_LCD_DrawString(0,0,"T-26", LCD_WHITE);
					// Game grid
					//grid_lines();
//					color_plotter(OGG);
//					color_plotter_bottom(PGG);
//					Top = OGG;
//					Bott = PGG;
					grid_plotter((x+start_x)/spacing_x,(y+start_y)/spacing_y,wait_color); // Cursor
					
					// UART reading of what the opponent is doing, and if he finished. 
					// Get new PGG from opponent
					Opp_finish = receive_data();
					their_bomb_x = receive_data();
					their_bomb_y = receive_data();
					send_data(I_am_waiting);
		
					I_finish=1; // send this through UART
					send_data(I_finish); // turn ended can be a 1
					//
					//
					if(Opp_finish==1){
						view=7;
					}
					Opp_finish=0;
				}
			}
		}else if(screen == 9){// win screen
			
			BSP_LCD_DrawString(6,6,"YOU WIN!!!", LCD_WHITE);
			view = 1;
			refresh_vars();
			
		}else if(screen == 10){// lose screen
			
			BSP_LCD_DrawString(6,6,"YOU LOSE...",LCD_WHITE);
			view = 1;
			refresh_vars();
			
		}
		
		////// My addition above.
		OS_bSignal(&LCDFree);
		OS_Suspend(); 
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
//  NumSamples = 0;
  MaxJitter = 0;       // in 1us units

//********initialize communication channels
  JsFifo_Init();

//*******attach background tasks***********
  OS_AddSW1Task(&SW1Push,2);
	OS_AddSW2Task(&SW2Push,2);


	
  OS_AddPeriodicThread(&Producer,PERIOD,2); // 2 kHz real time sampling of PD3
	
  NumCreated = 0;
// create initial foreground threads
//  NumCreated += OS_AddThread(&Interpreter, 128,2); 
  NumCreated += OS_AddThread(&Consumer, 128,2); 
	NumCreated += OS_AddThread(&PositionCal, 128,2); 
	NumCreated += OS_AddThread(&State, 128,2); 
  
	// change for Part 5) 
  OS_Launch(TIME_250US); // doesn't return, interrupts enabled in here
	return 0;            // this never executes
}
