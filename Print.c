#include "cmsis_os2.h"                                        // CMSIS RTOS header file
#include "Objects.h"
#include "Board_GLCD.h"

#define XOFFSETT 16*12
#define YOFFSETT 0
#define XOFFSETM 32
#define YOFFSETM 24*2
#define XOFFSETW 0
#define YOFFSETW 297
#define CHARWIDTH 16
#define CHARHEIGHT 24
#define LCDX 320
#define LCDY 240
#define __MAX_TIME 86400

const char nm[11] = {'N','O',' ','M','E','S','S','A','G','E','S'}; //('N','O',' ','M','E','S','S','A','G','E','S');

const char del[7] = {'D','e','l','e','t','e','?'};

/*----------------------------------------------------------------------------
 *      Thread 'Print': Print Messages
 *---------------------------------------------------------------------------*/
 
void thdPrint (void *argument);
osThreadId_t tid_thdPrint;                                      // thread id
void printTime(void);
void printMessage(void);
void printWatch(void);

const int line_width = 16;
const int line_height = 3;

int Init_thdPrint (void) {
  tid_thdPrint = osThreadNew (thdPrint, NULL, NULL);
  if (!tid_thdPrint) return(-1);
  return(0);
}

void thdPrint (void *argument) {
	while (1) {
		//print 30 times per second
		osDelay(osKernelGetTickFreq()/30);
		
		//OH BABY A TRIPLE ... OH YEA
		
		//print text message
		printMessage();
		
		//print time on top right
		printTime();
		
		//print stopwatch on bottom left
		printWatch();
		
		
		osThreadYield();
	}
}

//print message with timestamp and conditional delete prompt
//oh boy it's a big one
void printMessage(void){
	uint16_t x = XOFFSETM;
	uint16_t y = YOFFSETM;
	uint32_t t = NULL;
	osStatus_t status;
	
	//only print message section on update
	status = osSemaphoreAcquire(semMSG, 0);
	if(status != osOK) return;
	
	//time message was received is stored
	uint32_t hourMSD, hourLSD, minuteMSD, minuteLSD, secondMSD, secondLSD;
	if(current == NULL){
		int index;
		
		//print "NO MESSAGE'
		for(index = 0; index<11; index++){
			GLCD_DrawChar(x, y, nm[index]);
			x += CHARWIDTH;
		}
		
		//overwrite remaining message space
		for(int i = index/line_width; i<line_height; i++){
			for(int j = index%line_width; j<line_width; j++){
				GLCD_DrawChar(x, y, ' ');
				x += CHARWIDTH;
				index++;
				
			}
			x = XOFFSETM;
			y += CHARHEIGHT;
		}
		
		
		//overwrite timestamp
		x = XOFFSETM;
		y = YOFFSETM+4*CHARHEIGHT;		
		GLCD_DrawChar( x+(0*CHARWIDTH),y, ' ');
		GLCD_DrawChar( x+(1*CHARWIDTH),y, ' ');
		GLCD_DrawChar( x+(2*CHARWIDTH),y, ' ');
		GLCD_DrawChar( x+(3*CHARWIDTH),y, ' ');
		GLCD_DrawChar( x+(4*CHARWIDTH),y, ' ');
		GLCD_DrawChar( x+(5*CHARWIDTH),y, ' ');
		GLCD_DrawChar( x+(6*CHARWIDTH),y, ' ');
		GLCD_DrawChar( x+(7*CHARWIDTH),y, ' ');
		
		x = XOFFSETM+6*CHARWIDTH;
		y = YOFFSETM+5*CHARHEIGHT;
		for(int i = 0; i<7; i++){
			GLCD_DrawChar(x,y,' ');
			x+=CHARWIDTH;
		}
		
	}
	
	//if current != NULL
	else{		
		int index;
		
		//get scroll value
		osMutexAcquire(mutScroll, osWaitForever);
		int scr = scroll;
		osMutexRelease(mutScroll);
		
		//set starting index based on scroll value
		index = scr*line_width;
		
		
		//somehow, copying into a local array crashes the code
		/*
		char c[160];
		osMutexAcquire(mutDLL, osWaitForever);
		for(int i = 0; i<160; i++){
			c[i] = current->str[i];
		}
		osMutexRelease(mutDLL);
		*/
		
		x = XOFFSETM+6*CHARWIDTH;
		y = YOFFSETM+5*CHARHEIGHT;
		
		status = osSemaphoreAcquire(semDelete, NULL);
		//if about to delete message
		if(status == osOK) {
			//reset semaphore for next print
			osSemaphoreRelease(semDelete);
			//write characters to prompt user
			for(int i = 0; i<7; i++){
				GLCD_DrawChar(x,y,del[i]);
				x+=CHARWIDTH;
			}
		}
		
		//if not about to delete message
		else{
			//overwrite delete message space
			for(int i = 0; i<7; i++){
				GLCD_DrawChar(x,y,' ');
				x+=CHARWIDTH;
			}
		}
		
		//reset x and y to starting values
		x = XOFFSETM;
		y = YOFFSETM;
		
		//access Doubly Linked List to read string
		osMutexAcquire(mutDLL, osWaitForever);
		
		//write whole message
		for(int i = 0; i<line_height; i++){
			for(int j = 0; j<line_width; j++){
				//if a character, write character
				if((current->str[index]>0x1F) && (current->str[index]<0x7F))
					GLCD_DrawChar(x, y, current->str[index]);
				
				//if not a character, write empty space
				else GLCD_DrawChar(x, y, ' ');
				x += CHARWIDTH;
				index++;
			}
			x = XOFFSETM;
			y += CHARHEIGHT;
		}
		
		if(current!=NULL) t = current->time;
		else t = NULL;
		osMutexRelease(mutDLL);
		
		
		x = XOFFSETM;
		y += CHARHEIGHT;
		
		//if there is a time, write the time
		if(t != NULL){
			secondLSD = (t%10)+0x30;
			t/=10;
			secondMSD = (t%6)+0x30;
			t/=6;
			minuteLSD = (t%10)+0x30;
			t/=10;
			minuteMSD = (t%6)+0x30;
			t/=6;
			hourLSD = (t%10)+0x30;
			t/=10;
			if(t) hourMSD = t+0x30;
			else hourMSD = ' ';
			
			GLCD_DrawChar( x+(2*CHARWIDTH),y, ':');
			GLCD_DrawChar( x+(5*CHARWIDTH),y, ':');		
			GLCD_DrawChar( x+(0*CHARWIDTH),y, hourMSD);
			GLCD_DrawChar( x+(1*CHARWIDTH),y, hourLSD);
			GLCD_DrawChar( x+(3*CHARWIDTH),y, minuteMSD);
			GLCD_DrawChar( x+(4*CHARWIDTH),y, minuteLSD);
			GLCD_DrawChar( x+(6*CHARWIDTH),y, secondMSD);
			GLCD_DrawChar( x+(7*CHARWIDTH),y, secondLSD);
		}
		//if there is not a time, write empty space
		else{
			GLCD_DrawChar( x+(2*CHARWIDTH),y, ' ');
			GLCD_DrawChar( x+(5*CHARWIDTH),y, ' ');		
			GLCD_DrawChar( x+(0*CHARWIDTH),y, ' ');
			GLCD_DrawChar( x+(1*CHARWIDTH),y, ' ');
			GLCD_DrawChar( x+(3*CHARWIDTH),y, ' ');
			GLCD_DrawChar( x+(4*CHARWIDTH),y, ' ');
			GLCD_DrawChar( x+(6*CHARWIDTH),y, ' ');
			GLCD_DrawChar( x+(7*CHARWIDTH),y, ' ');
		}
	}
	
}

//print time at top right of screen
void printTime(void){
	uint32_t t;
	uint32_t hourMSD, hourLSD, minuteMSD, minuteLSD, secondMSD, secondLSD; // calculated digits for display
	
	//get clock time 
	osMutexAcquire(mutClkTime, osWaitForever);
	t = ClkTime;
	osMutexRelease(mutClkTime);
	
	//divide the clock into digits
	secondLSD = (t%10)+0x30;
	t/=10;
	secondMSD = (t%6)+0x30;
	t/=6;
	minuteLSD = (t%10)+0x30;
	t/=10;
	minuteMSD = (t%6)+0x30;
	t/=6;
	hourLSD = (t%10)+0x30;
	t/=10;
	if(t) hourMSD = t+0x30;
	else hourMSD = ' ';
	
	
	//draw digits
	GLCD_DrawChar( XOFFSETT+(2*CHARWIDTH),YOFFSETT*CHARHEIGHT, ':');
	GLCD_DrawChar( XOFFSETT+(5*CHARWIDTH),YOFFSETT*CHARHEIGHT, ':');		
	GLCD_DrawChar( XOFFSETT+(0*CHARWIDTH),YOFFSETT*CHARHEIGHT, hourMSD);
	GLCD_DrawChar( XOFFSETT+(1*CHARWIDTH),YOFFSETT*CHARHEIGHT, hourLSD);
	GLCD_DrawChar( XOFFSETT+(3*CHARWIDTH),YOFFSETT*CHARHEIGHT, minuteMSD);
	GLCD_DrawChar( XOFFSETT+(4*CHARWIDTH),YOFFSETT*CHARHEIGHT, minuteLSD);
	GLCD_DrawChar( XOFFSETT+(6*CHARWIDTH),YOFFSETT*CHARHEIGHT, secondMSD);
	GLCD_DrawChar( XOFFSETT+(7*CHARWIDTH),YOFFSETT*CHARHEIGHT, secondLSD);
}

//print stopwatch at bottom left of screen
void printWatch(void){
	uint32_t t;
	int32_t minuteMSD, minuteLSD, secondMSD, secondLSD, hsecondMSD, hsecondLSD; // calculated digits for display
	
	//get stopwatch time
	osMutexAcquire(mutWatchTime, osWaitForever);
	t = WatchTime;
	osMutexRelease(mutWatchTime);
	
	//divide the stopwatch into digits
	hsecondLSD = (t%10)+0x30;
	t/=10;
	hsecondMSD = (t%10)+0x30;
	t/=10;
	secondLSD = (t%10)+0x30;
	t/=10;
	secondMSD = (t%6)+0x30;
	t/=6;
	minuteLSD = (t%10)+0x30;
	t/=10;
	minuteMSD = (t%6)+0x30;
	t/=6;
	
	
	//draw digits
	GLCD_DrawChar( XOFFSETW+(2*CHARWIDTH),YOFFSETW*CHARHEIGHT, ':');
	GLCD_DrawChar( XOFFSETW+(5*CHARWIDTH),YOFFSETW*CHARHEIGHT, '.');		
	GLCD_DrawChar( XOFFSETW+(0*CHARWIDTH),YOFFSETW*CHARHEIGHT, minuteMSD);
	GLCD_DrawChar( XOFFSETW+(1*CHARWIDTH),YOFFSETW*CHARHEIGHT, minuteLSD);
	GLCD_DrawChar( XOFFSETW+(3*CHARWIDTH),YOFFSETW*CHARHEIGHT, secondMSD);
	GLCD_DrawChar( XOFFSETW+(4*CHARWIDTH),YOFFSETW*CHARHEIGHT, secondLSD);
	GLCD_DrawChar( XOFFSETW+(6*CHARWIDTH),YOFFSETW*CHARHEIGHT, hsecondMSD);
	GLCD_DrawChar( XOFFSETW+(7*CHARWIDTH),YOFFSETW*CHARHEIGHT, hsecondLSD);
}
