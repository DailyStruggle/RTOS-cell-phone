#include "cmsis_os2.h"                                        // CMSIS RTOS header file
#include "Objects.h"
#define __MAX_TIME 86400
/*----------------------------------------------------------------------------
 *      Thread 1 'Watch_control': manage stopwatch
 *---------------------------------------------------------------------------*/
 
void thdWatchControl (void *argument);
osThreadId_t tid_thdWatchControl;                                      // thread id
 
int Init_thdWatchControl (void) {
 
  tid_thdWatchControl = osThreadNew (thdWatchControl, NULL, NULL);
  if (!tid_thdWatchControl) return(-1);
  return(0);
}
 
void thdWatchControl (void *argument) {
	int32_t x;
	uint8_t state = 0;
	uint32_t i = osKernelGetTickFreq()*2;
	while (1) {
		//increment clock seconds at a rate of one second per second
		//checks time rather than using a delay
		uint32_t j = osKernelGetTickCount();
		if(j>i){
			osMutexAcquire(mutClkTime, osWaitForever);
			ClkTime = (ClkTime+1)%__MAX_TIME;
			osMutexRelease(mutClkTime);
			i = j + osKernelGetTickFreq();
		}
		
		//manage minute and hour increments
		//in no case does it wait long on the mutex
		osMutexAcquire(mutClkTime, osWaitForever);
		if(osSemaphoreAcquire(semMinute, 0)>=0)ClkTime = (ClkTime+60)%__MAX_TIME;
		if(osSemaphoreAcquire(semHour, 0) >=0)ClkTime = (ClkTime+3600)%__MAX_TIME;
		osMutexRelease(mutClkTime);
		
		//manage stopwatch
		x = osSemaphoreAcquire(semWatch, 0);
		if(x>=0) state++;
		if(state ==0){
				osMutexAcquire(mutWatchTime, osWaitForever);
				WatchTime = 0;
				osMutexRelease(mutWatchTime);
			}
		else if(state == 1){
				//no other OS delays in this script are allowed, to preserve the stopwatch speed
				//roughly modified to account for computation time
				osDelay((910000*osKernelGetTickFreq())/101111111);
				osMutexAcquire(mutWatchTime, osWaitForever);
				WatchTime = (WatchTime + 1)%360000;
				osMutexRelease(mutWatchTime);
			}
			//time is set
		else if(state == 2){
				osDelay(osKernelGetTickFreq()/10);
			}
			//reset time and return to state 0
		else if(state == 3){
				osMutexAcquire(mutWatchTime, osWaitForever);
				WatchTime = 0;
				osMutexRelease(mutWatchTime);
				state = 0;
				osDelay(osKernelGetTickFreq()/10);
			}
		osThreadYield ();                                         // suspend thread
  }
}
