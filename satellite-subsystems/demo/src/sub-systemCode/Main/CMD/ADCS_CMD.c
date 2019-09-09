#include "Adcs_Cmd.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "../../ADCS/AdcsMain.h"

#define MAX_ADCS_QUEUE_LENGTH 42

static xQueueHandle AdcsCmdQueue = NULL;
time_unix queue_wait = 100;

TroubleErrCode AdcsCmdQueueInit(){
	AdcsCmdQueue = xQueueCreate(MAX_ADCS_QUEUE_LENGTH,sizeof(TC_spl));
	if (AdcsCmdQueue == NULL){
		return TRBL_QUEUE_CREATE_ERR;
	}else{
		return TRBL_SUCCESS;
	}
}

TroubleErrCode AdcsCmdQueueGet(TC_spl *cmd){
	if(cmd == NULL){
		return TRBL_NULL_DATA;
	}
	if(AdcsCmdQueueIsEmpty() == TRUE){
		return TRBL_QUEUE_EMPTY;
	}
	if (xQueueReceive(AdcsCmdQueue, cmd, queue_wait) == pdTRUE){
		return TRBL_SUCCESS;
	}else{
		return TRBL_FAIL;
	}
}

TroubleErrCode AdcsCmdQueueAdd(TC_spl *cmd){
	if(NULL == cmd){
		return TRBL_NULL_DATA;
	}
	if (xQueueSend(AdcsCmdQueue, cmd, queue_wait) == pdTRUE){
		return TRBL_SUCCESS;
	}else{
		return TRBL_FAIL;
	}
}

TroubleErrCode AdcsCmdQueueGetCount(){
	return uxQueueMessagesWaiting(AdcsCmdQueue);
}

unsigned int AdcsCmdQueueIsEmpty(){
	if(uxQueueMessagesWaiting(AdcsCmdQueue) == 0){//check if queue is empty
		return TRUE;
	}else{
		return FALSE;
	}
}

time_unix* getAdcsQueueWaitPointer(){
	return &queue_wait;
}


