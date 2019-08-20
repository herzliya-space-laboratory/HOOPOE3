
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
#include "AdcsCommands.h"

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_ADCS_LOOP_DELAY 		1000 	//the loop runs in 1Hz
#define DEFAULT_ADCS_QUEUE_WAIT_TIME	 100	// amount of time to wait or cmd to arrive into Queue

time_unix delay_loop = DEFAULT_ADCS_LOOP_DELAY;
xSemaphoreHandle xAdcs_loop_param_mutex = NULL;	//TODO: need to check if need this


TroubleErrCode UpdateAdcsFramParameters(AdcsFramParameters param, unsigned char *data)
{
	if(NULL == data){
		return TRBL_NULL_DATA;
	}
	unsigned int addr = 0;
	unsigned int size = 0;
	unsigned char *ptr = NULL;

	switch(param)
	{
	case DELAY_LOOP:
		addr = ADCS_LOOP_DELAY_FRAM_ADDR;
		size = ADCS_LOOP_DELAY_FRAM_SIZE;
		ptr = (void*)&delay_loop;

		break;

	case QUEUE_WAIT_TIME:
		addr = ADCS_LOOP_DELAY_FRAM_ADDR;
		size = ADCS_LOOP_DELAY_FRAM_SIZE;
		ptr = (void*)getAdcsQueueWaitPointer();
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

//#define FIRST_ADCS_ACTIVATION

TroubleErrCode AdcsInit()
{
	if(SWITCH_OFF == get_system_state(ADCS_param))
	{
		while(SWITCH_OFF == get_system_state(ADCS_param)){
			vTaskDelay(CHANNEL_OFF_DELAY);
		}
	}

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
	
	trbl = AdcsCmdQueueInit();
	if (trbl != TRBL_SUCCESS){
		return trbl;
	}

	time_unix* adcsQueueWaitPointer = getAdcsQueueWaitPointer();
#ifdef FIRST_ADCS_ACTIVATION
	delay_loop = DEFAULT_ADCS_LOOP_DELAY;
	FRAM_write((byte*)&delay_loop,ADCS_LOOP_DELAY_FRAM_ADDR,ADCS_LOOP_DELAY_FRAM_SIZE);
	*adcsQueueWaitPointer = DEFAULT_ADCS_QUEUE_WAIT_TIME;
	FRAM_write((byte*)adcsQueueWaitPointer,ADCS_QUEUE_WAIT_TIME_FRAM_ADDR,ADCS_QUEUE_WAIT_TIME_FRAM_SIZE);
#endif
	if(0 != FRAM_read((byte*)&delay_loop,ADCS_LOOP_DELAY_FRAM_ADDR,ADCS_LOOP_DELAY_FRAM_SIZE)){
		delay_loop = DEFAULT_ADCS_LOOP_DELAY;
		//todo: log error
	}

	if(0 != FRAM_read((byte*)adcsQueueWaitPointer,ADCS_QUEUE_WAIT_TIME_FRAM_ADDR,ADCS_QUEUE_WAIT_TIME_FRAM_SIZE)){
		*adcsQueueWaitPointer = DEFAULT_ADCS_QUEUE_WAIT_TIME;
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
		if(SWITCH_OFF == get_system_state(ADCS_param)){
			vTaskDelay(DEFAULT_ADCS_LOOP_DELAY);
			continue;
		}
		UpdateAdcsStateMachine(); //TODO: finish, including return value errors
		if(SWITCH_OFF == get_system_state(ADCS_param)){
			vTaskDelay(CHANNEL_OFF_DELAY);
			continue;
		}

		if(!AdcsCmdQueueIsEmpty()){
			trbl = AdcsCmdQueueGet(&cmd);
			if(TRBL_SUCCESS != trbl){
				AdcsTroubleShooting(trbl);
			}
			trbl = AdcsExecuteCommand(&cmd);
			if(TRBL_SUCCESS != trbl){
				AdcsTroubleShooting(trbl);
			}
			//todo: log cmd received
		}

		trbl = GatherTlmAndData(); //TODO: check if enough time has passed
		if(TRBL_SUCCESS != trbl){
			AdcsTroubleShooting(trbl);
		}

		vTaskDelay(DEFAULT_ADCS_LOOP_DELAY);
	}
}

