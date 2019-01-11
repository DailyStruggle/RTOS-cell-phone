#include <stdlib.h>
#include "Objects.h"

#include "cmsis_os2.h"                                        // CMSIS RTOS header file
#include "Objects.h"

#define __MAX_TIME 86400
#define __MAX_MESSAGES 10

/*----------------------------------------------------------------------------
 *      Thread 1 'DLL': manage doubly linked list
 *											mostly just receives characters
 *---------------------------------------------------------------------------*/
 
int message_count = 0;
 
void thdDLL (void *argument);
osThreadId_t tid_thdDLL;                                      // thread id
 
int Init_thdDLL (void) {
  tid_thdDLL = osThreadNew (thdDLL, NULL, NULL);
	if (!tid_thdDLL) return(-1);
  return(0);
}
 
void thdDLL (void *argument) {
	char* c;
	osStatus_t status;
	
	while (1) {
		//yield until there's a character on the queue
		//point c to character
		status = osMessageQueueGet(msgqCHAR, &c, NULL, osWaitForever);
		if (status != osOK) {while(1);}
		
		
		osMutexAcquire(mutDLL, osWaitForever);
		//add character to string on head
		toString(c[0]);
		osMutexRelease(mutDLL);
		
		//free this character from the queue
		osMemoryPoolFree(mplCHAR, c);
		osThreadYield ();                                         // suspend thread
  }
}

	struct node *head = NULL;
	struct node *current = NULL;

//mutex for accessing the linked list
osMutexId_t mutDLL;


//to the right
void Next() {
	if(current == NULL) return;
	if(current->next != NULL){
		current = current->next;
		osMutexAcquire(mutScroll, osWaitForever);
		scroll = 0;
		osMutexRelease(mutScroll);
		osSemaphoreRelease(semMSG);
	}
}

//to the left
void Previous() {
	if(current == NULL) return;
	if(current->prev != NULL){
		current = current->prev;
		osMutexAcquire(mutScroll, osWaitForever);
		scroll = 0;
		osMutexRelease(mutScroll);
	}
	osSemaphoreRelease(semMSG);
}


//insert a node at the beginning of the Doubly Linked List
void Push() {
  //create a new node
  if(message_count<__MAX_MESSAGES){
		message_count++;
	}
	else return;
	
	struct node *link = (struct node*) osMemoryPoolAlloc(mplTXT,0);
	if(link == NULL) return;
	if(head != NULL)	
		head->prev = link;
	
  //point it to old first node
  link->next = head;
	
  //point first to new first node
  head = link;
	if(current == NULL) current = head;
	osSemaphoreRelease(semMSG);
}

/*
// Function to insert a node in the middle of the Doubly Linked List
void Insert(){
	struct node *link = (struct node*) malloc(sizeof(struct node));
	if(current == NULL){
		head = link;
		current = link;
	}
	else if(current->next != NULL){
		current->next->prev = link;
		link->next = current->next;
	}
	else{
		tail = link;
	}
	current->next = link;
}
*/


//function to delete the current node
//links previous node to next node, and vice versa
void DeleteCurrent(void){
	if(current == NULL) return;
	if(current->next != NULL) current->next->prev = current->prev;
	if(current->prev != NULL) current->prev->next = current->next;
	for(int i = 0; i<160 && current->str[i]>0; i++)
		current->str[i] = 0;
	struct node* ptr = current;
	if(current->next != NULL) {
		if(current == head) head = head->next;
		current = current->next;
	}
	else if (current->prev != NULL) current = current->prev;
	else {
		current = NULL;
		head = NULL;
	}
	
	ptr->time = NULL;
	ptr->next = NULL;
	ptr->prev = NULL;
	osMemoryPoolFree(mplTXT, ptr);
	osSemaphoreRelease(semMSG);
	
	message_count--;
}


//manages character additions
//does not queue messages
void toString(char c){
	static uint16_t index = 0;
	if(message_count==__MAX_MESSAGES && current->str[index]>0) return;
	
	
	//if enter key
	if(c == 0x0D){
		//set index to 0, so the next message adds to the list
		index = 0;
		
		//set time
		osMutexAcquire(mutClkTime, osWaitForever);
		head->time = ClkTime;
		osMutexRelease(mutClkTime);
		
		//update screen
		osSemaphoreRelease(semMSG);
		return;
	}
	
	//add message to list
	if(index == 0) Push();
	
	//add message to list if more than 160 characters
	if((index >=160)){
		Push();
		index = 0;
	}
	
	//set character in node at index
	head->str[index] = c;
	
	//go to next index
	index++;
	
	//update screen
	osSemaphoreRelease(semMSG);
}
