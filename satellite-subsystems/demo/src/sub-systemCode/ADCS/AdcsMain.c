
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <hcc/api_fs_err.h>

#include "sub-systemCode/Main/commands.h"
#include "sub-systemCode/Main/CMD/ADCS_CMD.h"	//TODO: use this API
#include "sub-systemCode/Global/GlobalParam.h"

#include "AdcsMain.h"
#include "AdcsTroubleShooting.h"
#include "AdcsGetDataAndTlm.h"
#include "StateMachine.h"

#include <stdio.h>
#include <stdlib.h>



#define DEFAULT_ADCS_LOOP_DELAY 		1000 	//the loop runs in 1Hz
#define DEFAULT_ADCS_QUEUE_WAIT_TIME	 100	// amount of time to wait or cmd to arrive into Queue

time_unix delay_loop = 0;
time_unix queue_wait = 0;
xQueueHandle xAdcsCmdQueue = NULL;
xSemaphoreHandle xAdcs_loop_param_mutex = NULL;	//TODO: need to check if need this

TroubleErrCode AddCommandToAdcsQueue(TC_spl *cmd)
{
	if(NULL == cmd){
		return TRBL_NULL_DATA;
	}
	if(pdTRUE == xQueueSend(xAdcsCmdQueue,cmd,queue_wait)){
		return TRBL_SUCCESS;
	}
	return TRBL_FAIL;
}

TroubleErrCode GetQueueCmdCount(){
	return uxQueueMessagesWaiting(xAdcsCmdQueue);
}

TroubleErrCode GetCmdFromQueue(TC_spl *cmd)
{
	if(NULL == cmd){
		return TRBL_NULL_DATA;
	}
	if(0 == GetQueueCmdCount()){
		return TRBL_QUEUE_EMPTY;
	}
	if(pdTRUE == xQueueReceive(xAdcsCmdQueue, cmd, queue_wait)){
		return TRBL_SUCCESS;
	}
	return TRBL_FAIL;
}

TroubleErrCode UpdateAdcsFramParameters(AdcsFramParameters param, unsigned char *data)
{
	if(NULL == data){
		return TRBL_NULL_DATA;
	}
	unsigned int addr = 0;
	unsigned int size = 0;
	unsigned char *ptr = NULL;
	switch(param){
	case DELAY_LOOP:
		addr = ADCS_LOOP_DELAY_FRAM_ADDR;
		size = ADCS_LOOP_DELAY_FRAM_SIZE;
		ptr = &delay_loop;

		break;

	case QUEUE_WAIT_TIME:
		addr = ADCS_LOOP_DELAY_FRAM_ADDR;
		size = ADCS_LOOP_DELAY_FRAM_SIZE;
		ptr = &queue_wait;
		break;
	default:
		return TRBL_FAIL;
	}

	if(0 != FRAM_write(data,addr,size)){
		return TRBL_FRAM_WRITE_ERR;
	}
	if(NULL == memcpy(ptr,data,size)){
		return TRBL_FAIL;
	}
	return TRBL_SUCCESS;

}

TroubleErrCode AdcsInit()
{
	TroubleErrCode trbl = TRBL_SUCCESS;

	unsigned char adcs_i2c = ADCS_I2C_ADRR;
	int rv;

	rv = cspaceADCS_initialize(&adcs_i2c, 1);
	if(rv != E_NO_SS_ERR && rv != E_IS_INITIALIZED){
		//TODO: log error
		return TRBL_ADCS_INIT_ERR;
	}

	if(F_NO_ERROR !=  f_enterFS()){
		return TRBL_FS_INIT_ERR;
	}
	xAdcsCmdQueue = xQueueCreate(MAX_ADCS_QUEUE_LENGTH, sizeof(TC_spl));
	if (NULL == xAdcsCmdQueue){
		return TRBL_QUEUE_CREATE_ERR;
	}

	if(0 != FRAM_read(&delay_loop,ADCS_LOOP_DELAY_FRAM_ADDR,ADCS_LOOP_DELAY_FRAM_SIZE)){
		delay_loop = DEFAULT_ADCS_LOOP_DELAY;
		//todo: log error
	}

	if(0 != FRAM_read(&queue_wait,ADCS_QUEUE_WAIT_TIME_FRAM_ADDR,ADCS_QUEUE_WAIT_TIME_FRAM_SIZE)){
		queue_wait = DEFAULT_ADCS_QUEUE_WAIT_TIME;
		//todo: log error
	}
	return TRBL_SUCCESS;
}

void AdcsTask()
{
	TC_spl cmd = {0};
	TroubleErrCode trbl = TRBL_SUCCESS;
	//TODO: log start task

	while(TRUE)
	{
		UpdateAdcsStateMachine(); //TODO: finish, including return value errors

		if(TRBL_SUCCESS == GetCmdFromQueue(&cmd)){
			trbl = AdcsExecuteCommand(cmd);
			if(TRBL_SUCCESS != trbl){
				AdcsTroubleShooting(trbl);
			}
			//todo: log cmd received
		}else{

			trbl = GatherTlmAndData(); //TODO: check if enough time has passed
			if(TRBL_SUCCESS != trbl){
				AdcsTroubleShooting(trbl);
			}
			vTaskDelay(delay_loop);
		}
	}
}

