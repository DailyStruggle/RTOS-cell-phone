#ifndef __Objects
	#define __Objects
	#include "stdint.h"
	#include "rtx_os.h"
	#include "Board_Joystick.h"
	#include "Board_Buttons.h"
	#include "Board_LED.h"


	//shared variables
	extern uint32_t ClkTime;
	extern uint8_t scroll;
	extern uint32_t WatchTime;

	//mutexes
	extern osMutexId_t mutClkTime;
	extern osMutexId_t mutWatchTime;
	extern osMutexId_t mutScroll;
	extern osMutexId_t mutDLL;
	
	// flag semaphores
	extern osSemaphoreId_t semHour;
	extern osSemaphoreId_t semMinute;
	extern osSemaphoreId_t semWatch;
	extern osSemaphoreId_t semDelete;
	extern osSemaphoreId_t semMSG;
	
	//text memory
	extern osMemoryPoolId_t 	mplTXT;
	extern osMemoryPoolId_t  mplCHAR;
	extern osMessageQueueId_t  msgqCHAR;

	//threads
	void thdPrint (void *argument);
	extern int Init_thdPrint(void);
	void thdJoystick (void *argument);
	extern int Init_thdJoystick(void);
	void thdWatchControl (void *argument);
	extern int Init_thdWatchControl(void);
	void thdDLL (void *argument);
	extern int Init_thdDLL(void);

	///////////////////////////////////////////////////////////////////////////////////
	//LINKED LIST STUFF
	struct node{
		char str[160];
		uint32_t time;
		struct node *next;
		struct node *prev;
	};
	
	extern struct node *head;
	extern struct node *current;
	
	//change current pointer to next node, if possible
	extern void Next(void);

	//change current pointer to previous node, if possible
	extern void Previous(void);
	
	//add node to front of list
	extern void Push(void);

	//free current node from memory
	extern void DeleteCurrent(void);
	
	//add character to string
	//manages a static index
	//pushes a new node on next character if given enter key or index exceeds 160
	extern void toString(char);
	///////////////////////////////////////////////////////////////////////////////////
#endif
