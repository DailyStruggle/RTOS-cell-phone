/*------------------------------------------------------------------------------
 *      RL-ARM
 *------------------------------------------------------------------------------
 *      Name:    Serial.c
 *      Purpose: Serial Input Output for ST STM32F2xx
 *      Note(s): Possible defines to select the used communication interface:
 *               __DBG_ITM   - ITM SWO interface (USART is used by default)
 *                           - UART3 interface  (default)
 *------------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2012 KEIL - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include <stm32f2xx.h>
#include "Serial.h"

#ifdef __DBG_ITM
volatile int32_t ITM_RxBuffer;
#endif


/*------------------------------------------------------------------------------
 *       SER_Init:  Initialize Serial Interface
 *----------------------------------------------------------------------------*/

void SER_Init (void) {

#ifdef __DBG_ITM
  ITM_RxBuffer = ITM_RXBUFFER_EMPTY;
#else
  int i;

  /* Configure UART3 for 115200 baud                                          */
  RCC->AHB1ENR  |=  (   1 <<  2);       /* Enable GPIOC clock                 */
  GPIOC->MODER  &= ~(   3 << 20);
  GPIOC->MODER  |=  (   2 << 20);       /* PC10: Alternate function mode      */
  GPIOC->AFR[1] &= ~(0x0F <<  8);
  GPIOC->AFR[1] |=  (   7 <<  8);       /* PC10: Alternate function USART3_TX */
  GPIOC->MODER  &= ~(   3 << 22);
  GPIOC->MODER  |=  (   2 << 22);       /* PC11: Alternate function mode      */
  GPIOC->AFR[1] &= ~(0x0F << 12);
  GPIOC->AFR[1] |=  (   7 << 12);       /* PC11: Alternate function USART3_RX */

  RCC->APB1ENR  |=  (   1 << 18);       /* Enable USART3 clock                */
  ////////////////////////////////////////////////////////////////////////////////////////////////////
	USART3->BRR    =  SystemCoreClock/115200;             /* Configure 115200 baud, @ 30MHz     */
	////////////////////////////////////////////////////////////////////////////////////////////////////
  USART3->CR3    =  0x0000;             /*           8 bit, 1 stop bit,       */
  USART3->CR2    =  0x0000;             /*           no parity                */
  for (i = 0; i < 0x1000; i++) __NOP(); /* avoid unwanted output              */
  USART3->CR1    =  0x200C;
#endif
}


/*------------------------------------------------------------------------------
 *       SER_PutChar:  Write a character to the Serial Port
 *----------------------------------------------------------------------------*/

int32_t SER_PutChar (int32_t ch) {

#ifdef __DBG_ITM
  int i;
  ITM_SendChar (ch & 0xFF);
  for (i = 10000; i; i--);
#else
  while (!(USART3->SR & 0x0080));
  USART3->DR = (ch & 0xFF);
#endif
  return (ch);
}


/*------------------------------------------------------------------------------
 *       SER_GetChar:  Read a character from the Serial Port (non-blocking)
 *----------------------------------------------------------------------------*/

int32_t SER_GetChar (void) {

#ifdef __DBG_ITM
  if (ITM_CheckChar())
    return (ITM_ReceiveChar());
#else
  if (USART3->SR & 0x0020) 
    return (USART3->DR);
#endif
  return (-1);
}

/*------------------------------------------------------------------------------
 * End of file
 *----------------------------------------------------------------------------*/
