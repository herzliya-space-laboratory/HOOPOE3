
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <hcc/api_fs_err.h>

#include "sub-systemCode/Main/commands.h"
#include "sub-systemCode/Main/CMD/ADCS_CMD.h"
#include "sub-systemCode/Global/GlobalParam.h"

#include "AdcsMain.h"
#include "AdcsTroubleShooting.h"
#include "AdcsGetDataAndTlm.h"
#include "StateMachine.h"
#include "AdcsCommands.h"

#include <stdio.h>
#include <stdlib.h>

#define ADCS_INIT_DELAY 				10000 	//10sec delay fot the ADCS to start
#define DEFAULT_ADCS_LOOP_DELAY 		1000 	//the loop runs in 1Hz
#define DEFAULT_ADCS_SYSTEM_OFF_DELAY	(10000)	// wait 10 seconds if the system is off
#define DEFAULT_ADCS_QUEUE_WAIT_TIME	 100	// amount of time to wait or cmd to arrive into Queue
#define DEFAULT_ADCS_SYSTEM_OFF_DELAY	(10000)	// wait 10 seconds if the system is off

time_unix delay_loop = DEFAULT_ADCS_LOOP_DELAY;
time_unix system_off_delay = DEFAULT_ADCS_SYSTEM_OFF_DELAY;

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

	case DELAY_SYSTEM_OFF:
		addr = ADCS_SYS_OFF_DELAY_ADDR;
		size = ADCS_SYS_OFF_DELAY_SIZE;
		ptr = (void*)&system_off_delay;
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

#define FIRST_ADCS_ACTIVATION

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

	
	trbl = AdcsCmdQueueInit();
	if (trbl != TRBL_SUCCESS){
		return trbl;
	}
	trbl = InitTlmElements();
	if (trbl != TRBL_SUCCESS){
		return trbl;
	}
	Boolean b = CreateTlmElementFiles();
	if (b != TRUE){
		//TODO: log err
	}
	time_unix* adcsQueueWaitPointer = getAdcsQueueWaitPointer();

	if(0 != FRAM_read((byte*)&delay_loop,ADCS_LOOP_DELAY_FRAM_ADDR,ADCS_LOOP_DELAY_FRAM_SIZE)){
		delay_loop = DEFAULT_ADCS_LOOP_DELAY;
		//todo: log error
	}
	if(0 != FRAM_read((unsigned char*)&system_off_delay,ADCS_SYS_OFF_DELAY_ADDR,ADCS_SYS_OFF_DELAY_SIZE)){
		system_off_delay = DEFAULT_ADCS_SYSTEM_OFF_DELAY;
	}
	if(0 != FRAM_read((byte*)adcsQueueWaitPointer,ADCS_QUEUE_WAIT_TIME_FRAM_ADDR,ADCS_QUEUE_WAIT_TIME_FRAM_SIZE)){
		*adcsQueueWaitPointer = DEFAULT_ADCS_QUEUE_WAIT_TIME;
		//todo: log error
	}
	if(0 != FRAM_read((byte*)&system_off_delay,ADCS_SYS_OFF_DELAY_ADDR,ADCS_SYS_OFF_DELAY_SIZE)){
		system_off_delay = DEFAULT_ADCS_SYSTEM_OFF_DELAY;
	}

	return TRBL_SUCCESS;
}

void AdcsTask()
{
	TC_spl cmd = {0};
	TroubleErrCode trbl = TRBL_SUCCESS;
	//TODO: log start task

	vTaskDelay(ADCS_INIT_DELAY);

	while(TRUE)
	{
		f_managed_enterFS();
		if(SWITCH_OFF == get_system_state(ADCS_param)){
			vTaskDelay(system_off_delay);
			f_managed_releaseFS();
			continue;
		}
		UpdateAdcsStateMachine();

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
		f_managed_releaseFS();
		vTaskDelay(delay_loop);
	}

	printf("that a shity thing");
}

