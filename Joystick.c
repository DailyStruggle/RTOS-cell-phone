#include "cmsis_os2.h"                                        // CMSIS RTOS header file
#include "Objects.h"

//re-defined joystick directions for a rotated axis
#define UP JOYSTICK_LEFT
#define DOWN JOYSTICK_RIGHT
#define LEFT JOYSTICK_DOWN
#define RIGHT JOYSTICK_UP

/*----------------------------------------------------------------------------
 *      Thread 'Joystick': sample joystick 10 times per second
 *---------------------------------------------------------------------------*/
 
void thdJoystick (void *argument);
osThreadId_t tid_thdJoystick;                                      // thread id

int Init_thdJoystick (void) {
 
  tid_thdJoystick = osThreadNew (thdJoystick, NULL, NULL);
  if (!tid_thdJoystick) return(-1);
  return(0);
}
 
void thdJoystick (void *argument) {
	int cd = 2;
	//additional 'del' variable for rising-edge detection
	uint8_t del = 1;
	while (1) {
		int x;
		//sample 10 times per second
		osDelay(osKernelGetTickFreq()/10);
		x = Joystick_GetState();
		
		//scroll left
		if(x&LEFT) {
			osMutexAcquire(mutDLL, osWaitForever);
			Previous();
			osMutexRelease(mutDLL);
			osSemaphoreRelease(semMSG);
			osSemaphoreAcquire(semDelete, 0);
		}
		
		//scroll right
		else if(x&RIGHT) {
			osMutexAcquire(mutDLL, osWaitForever);
			Next();
			osMutexRelease(mutDLL);
			osSemaphoreRelease(semMSG);
			osSemaphoreAcquire(semDelete, 0);
		}
		
		//scroll up
		if(x&UP){
			osMutexAcquire(mutScroll, osWaitForever);
			if(scroll>0) scroll--;
			osMutexRelease(mutScroll);
			osSemaphoreRelease(semMSG);
			osSemaphoreAcquire(semDelete, 0);
		}	
		
		//scroll down
		else if(x&DOWN){
			osMutexAcquire(mutScroll, osWaitForever);
			if(scroll<8 && current->str[(scroll+3)*16]>0) scroll++;
			osMutexRelease(mutScroll);
			osSemaphoreRelease(semMSG);
			osSemaphoreAcquire(semDelete, 0);
		}
		
		//delete message on center button, on the rising edge
		//if center button and 'can delete'
		if(x&JOYSTICK_CENTER && del){
			osStatus_t status;
			status = osSemaphoreAcquire(semDelete, 0);
			osMutexAcquire(mutDLL, osWaitForever);
			//if joystick pressed and semDelete, delete message
			if(status == osOK && current != NULL){
				DeleteCurrent();
			}
			//if joystick pressed and NOT semDelete, set semDelete to prompt user
			else if(current!=NULL) osSemaphoreRelease(semDelete);
			osMutexRelease(mutDLL);
			osSemaphoreRelease(semMSG);
			del--;
		}
		//if center button is unpressed, allow next press on the next rising edge
		else if(!(x&JOYSTICK_CENTER)){
			del = 1;
		}
		
		//joystick cooldown for ui
		else if(cd&&x){
			osDelay(osKernelGetTickFreq()/5);
			cd--;
		}
		else if(!x) cd = 2;
		
		
		osThreadYield ();                                         // suspend thread
  }
}
