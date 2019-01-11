#include "RTE_Components.h"
#include  CMSIS_device_header
#include "cmsis_os2.h"
#include "Board_GLCD.h"
#include "Board_LED.h"
#include "GLCD_Config.h"
#include "stm32f2xx_hal.h"
#include "rtx_os.h"
#include "Objects.h"
#include "Serial.h"



extern GLCD_FONT GLCD_Font_16x24;

//clock semaphores
osSemaphoreId_t semHour, semMinute;

//stopwatch manipulation
osSemaphoreId_t semWatch;

//for delete message
osSemaphoreId_t semDelete;

//print message if changed
osSemaphoreId_t semMSG;

uint32_t ClkTime = 0;
osMutexId_t mutClkTime;
uint32_t WatchTime = 0;
osMutexId_t mutWatchTime;
uint8_t scroll = 0;
osMutexId_t mutScroll;


osMessageQueueId_t  msgqCHAR;
osMemoryPoolId_t 	mplCHAR;
osMemoryPoolId_t 	mplTXT;

/********************************************/
// The RTOS and HAL need the SysTick for timing. The RTOS wins and gets control
// of SysTick, so we need to route the HAL's tick call to the RTOS's tick.
uint32_t HAL_GetTick(void) { 
  return osKernelGetTickCount();
}

void app_hw_init (void *argument) {
	Joystick_Initialize();
	GLCD_Initialize();
	GLCD_SetBackgroundColor(GLCD_COLOR_PURPLE);
	GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
	GLCD_ClearScreen(); 
	GLCD_SetFont(&GLCD_Font_16x24);
	
	//thread for joystick sampling and scrolling
	Init_thdJoystick ();
	
	//thread for printing to screen
	Init_thdPrint ();
	
	//thread for manipulating the clock and stopwatch
	Init_thdWatchControl();
	
	//thread to receive characters and append the list
	Init_thdDLL();
	
	
  osThreadExit(); // job is done, thread suicide. There better be other threads created...
}

int main(void){
	SystemCoreClockUpdate();	// always first, make sure the clock freq. is current
	osKernelInitialize();     // Initialize CMSIS-RTOS
	HAL_Init();
	SER_Init();
	LED_Initialize();
	Buttons_Initialize();
	
	//turns off those blasted lights
	LED_SetOut(0);
	
	
	mplTXT = osMemoryPoolNew(10, sizeof(struct node), NULL);
	
	//character transmission from interrupt to thread
	//space for pointer and character at 0th index
	mplCHAR = osMemoryPoolNew(4, sizeof(char*)+sizeof(char), NULL);
	msgqCHAR = osMessageQueueNew(4, sizeof(void*), NULL);
	
	
	// To configure the buttons as interrupts
	EXTI->IMR |= 1<<15;     	
	EXTI->FTSR |= 1<<15;  		
	EXTI->IMR |= 1<<13;     	
	EXTI->FTSR |= 1<<13;  		
	EXTI->IMR |= 1<<0;
	EXTI->FTSR |= 1<<0;
	RCC->APB2ENR |= 1<<14; 		// Turn on bus clock for the system configuration to turn on edge detector
													
	// Set the EXTI control register
	SYSCFG->EXTICR[0] =	(SYSCFG->EXTICR[0] & ~0x0000000F) | SYSCFG_EXTICR1_EXTI0_PA;
	SYSCFG->EXTICR[3] =	(SYSCFG->EXTICR[3] & ~0x0000F000) | SYSCFG_EXTICR4_EXTI15_PG;
	SYSCFG->EXTICR[3] =	(SYSCFG->EXTICR[3] & ~0x000000F0) | SYSCFG_EXTICR4_EXTI13_PC;

	USART3->CR1 |= USART_CR1_RXNEIE;

	NVIC->ISER[USART3_IRQn/32] |= 1UL<<(USART3_IRQn%32);
	NVIC->ISER[EXTI0_IRQn/32] |= 1<< (EXTI0_IRQn%32);
	NVIC->ISER[EXTI15_10_IRQn/32] |= 1<< (EXTI15_10_IRQn%32);
	
	NVIC_SetPriority(USART3_IRQn, 42);
	
	
 //THAT'S A LOT OF DAMAGE
	mutClkTime = osMutexNew(NULL);
			if (mutClkTime==NULL) while(1){} // failed, scream and die
	mutWatchTime = osMutexNew(NULL);
			if (mutWatchTime==NULL) while(1){} // failed, scream and die
	mutScroll = osMutexNew(NULL);
			if (mutScroll==NULL) while(1){} // failed, scream and die
	mutDLL = osMutexNew(NULL);
			if (mutDLL==NULL) while(1){} // failed, scream and die
	semHour = osSemaphoreNew(1, 0, NULL);
			if (semHour==NULL) while(1){} // failed, scream and die
	semMinute = osSemaphoreNew(1, 0, NULL);
			if (semMinute==NULL) while(1){} // failed, scream and die
	semWatch = osSemaphoreNew(1, 0, NULL);
			if (semWatch==NULL) while(1){} // failed, scream and die
	semDelete = osSemaphoreNew(1, 0, NULL);
			if (semDelete==NULL) while(1){} // failed, scream and die
	semMSG = osSemaphoreNew(1, 1, NULL);
			if (semMSG==NULL) while(1){} // failed, scream and die
	
	osThreadNew(app_hw_init, NULL, NULL); // Create application main thread
	
				
	osKernelStart();                      // Start thread execution
	for(;;);
}

//handle hour button
void EXTI0_IRQHandler(void){
	
	///////////////////////////////////////
	//proper interrupt cooldown code
	//osDelay wasn't good enough
	static uint32_t i = 0; 
	uint32_t j = osKernelGetTickCount();
	if(j<i){
		if (EXTI->PR & 1<<0)
			EXTI->PR = 1<<0;
		return;
	}
	i = j + osKernelGetTickFreq()/10;
	////////////////////////////////////////
	
	if (EXTI->PR & 1<<0){
		EXTI->PR = 1<<0;
		osSemaphoreRelease(semHour);
	}
	else while(1);
}


//handle minute and stopwatch buttons
void EXTI15_10_IRQHandler(void)
{
	
	///////////////////////////////////////
	//proper interrupt cooldown code
	//osDelay wasn't good enough
	static uint32_t i = 0; 
	uint32_t j = osKernelGetTickCount();
	if(j<i){
		if (EXTI->PR & 1<<15)
			EXTI->PR = 1<<15;
		
		if (EXTI->PR & 1<<13)  // flag bit for EXTI15 is set
			EXTI->PR = 1<<13;
		
		return;
	}
	i = j + osKernelGetTickFreq()/10;
	////////////////////////////////////////
	
		//USER button
		if (EXTI->PR & 1<<15)  // flag bit for EXTI15 is set
		{
			EXTI->PR = 1<<15;
			osSemaphoreRelease(semWatch);
			return;
		}
		//TAMPER button
		else if (EXTI->PR & 1<<13)  // flag bit for EXTI15 is set
		{
			EXTI->PR = 1<<13; 
			osSemaphoreRelease(semMinute);
			return;
		}
		else while(1);
}


//handle character stream over USART3
//transmits one character at a time
void USART3_IRQHandler (void){
	//store character
	char* ptr;
	ptr = osMemoryPoolAlloc(mplCHAR, 0);
	if (ptr == NULL) ; //do nothing if it can't allocate
	else{
		//put the character in the first spot available after the pointer
		ptr[0] = SER_GetChar();
		
		//if enter key
		if(ptr[0]==0x0D) {
			//send new line
			SER_PutChar(0x0A);
			SER_PutChar(0x0D);
			//add character to the queue
			osMessageQueuePut(msgqCHAR, &ptr, NULL, NULL);
		}
		//if normal character
		else if((ptr[0] > 0x1F) && (ptr[0] < 0x7F)){
			//send the character
			SER_PutChar(ptr[0]);
			//add character to the queue
			osMessageQueuePut(msgqCHAR, &ptr, NULL, NULL);
		}
		//if none of those, ignore it and free the memory
		else osMemoryPoolFree(mplCHAR, ptr);
	}
}
