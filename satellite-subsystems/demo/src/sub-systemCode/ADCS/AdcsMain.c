
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <hcc/api_fs_err.h>

#include "sub-systemCode/Main/commands.h"
#include "sub-systemCode/Main/CMD/ADCS_CMD.h"
#include "sub-systemCode/Global/GlobalParam.h"
#include "sub-systemCode/Global/logger.h"

#include "AdcsMain.h"
#include "AdcsTroubleShooting.h"
#include "AdcsGetDataAndTlm.h"
#include "StateMachine.h"
#include "AdcsCommands.h"

#include <stdio.h>
#include <stdlib.h>

#define ADCS_INIT_DELAY 				10000 	//10sec delay fot the ADCS to start
#define DEFAULT_ADCS_LOOP_DELAY 		1000 	//the loop runs in 1Hz
#define DEFAULT_ADCS_SYSTEM_OFF_DELAY	10000	// wait 10 seconds if the system is off
#define DEFAULT_ADCS_QUEUE_WAIT_TIME	100		// amount of time to wait or cmd to arrive into Queue
#define DEFAULT_ADCS_SYSTEM_OFF_DELAY	10000	// wait 10 seconds if the system is off

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
		addr = ADCS_LOOP_DELAY_ADDR;
		size = ADCS_LOOP_DELAY_SIZE;
		ptr = (void*)&delay_loop;
		break;

	case QUEUE_WAIT_TIME:
		addr = ADCS_LOOP_DELAY_ADDR;
		size = ADCS_LOOP_DELAY_SIZE;
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

	if(0 != FRAM_write_exte(data,addr,size)){
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
	int init_err_flag = 0;
	FRAM_write_exte((byte*)&init_err_flag,ADCS_SUCCESSFUL_INIT_FLAG_ADDR,ADCS_SUCCESSFUL_INIT_FLAG_SIZE);

	rv = cspaceADCS_initialize(&adcs_i2c, 1);
	if(rv != E_NO_SS_ERR && rv != E_IS_INITIALIZED){
		init_err_flag = 1;
		FRAM_write_exte((byte*)&init_err_flag,ADCS_SUCCESSFUL_INIT_FLAG_ADDR,ADCS_SUCCESSFUL_INIT_FLAG_SIZE);
		return TRBL_ADCS_INIT_ERR;
	}

	
	trbl = AdcsCmdQueueInit();
	if (trbl != TRBL_SUCCESS){
		init_err_flag = 2;
		FRAM_write_exte((byte*)&init_err_flag,ADCS_SUCCESSFUL_INIT_FLAG_ADDR,ADCS_SUCCESSFUL_INIT_FLAG_SIZE);
		return trbl;
	}
	trbl = InitTlmElements();
	if (trbl != TRBL_SUCCESS){
		init_err_flag = 3;
		FRAM_write_exte((byte*)&init_err_flag,ADCS_SUCCESSFUL_INIT_FLAG_ADDR,ADCS_SUCCESSFUL_INIT_FLAG_SIZE);
		return trbl;
	}
	Boolean b = CreateTlmElementFiles();
	if (b != TRUE){
		init_err_flag = 4;
		FRAM_write_exte((byte*)&init_err_flag,ADCS_SUCCESSFUL_INIT_FLAG_ADDR,ADCS_SUCCESSFUL_INIT_FLAG_SIZE);
	}
	time_unix* adcsQueueWaitPointer = getAdcsQueueWaitPointer();

	if(0 != FRAM_read_exte((byte*)&delay_loop,ADCS_LOOP_DELAY_ADDR,ADCS_LOOP_DELAY_SIZE)){
		delay_loop = DEFAULT_ADCS_LOOP_DELAY;
	}
	if(0 != FRAM_read_exte((unsigned char*)&system_off_delay,ADCS_SYS_OFF_DELAY_ADDR,ADCS_SYS_OFF_DELAY_SIZE)){
		system_off_delay = DEFAULT_ADCS_SYSTEM_OFF_DELAY;
	}
	if(0 != FRAM_read_exte((byte*)adcsQueueWaitPointer,ADCS_QUEUE_WAIT_TIME_ADDR,ADCS_QUEUE_WAIT_TIME_SIZE)){
		*adcsQueueWaitPointer = DEFAULT_ADCS_QUEUE_WAIT_TIME;
	}

	init_err_flag = TRUE;
	FRAM_write_exte((byte*)&init_err_flag,ADCS_SUCCESSFUL_INIT_FLAG_ADDR,ADCS_SUCCESSFUL_INIT_FLAG_SIZE);
	return TRBL_SUCCESS;
}

void AdcsTask()
{
	TC_spl cmd = {0};
	TroubleErrCode trbl = TRBL_SUCCESS;
	int f_err = f_managed_enterFS();
	if(0 == f_err){
		WriteAdcsLog(LOG_ADCS_FS_INIT_ERR,0);// enter success
	}
	vTaskDelay(ADCS_INIT_DELAY);
	while(TRUE)
	{
		if(SWITCH_OFF == get_system_state(ADCS_param)){
			WriteAdcsLog(LOG_ADCS_CHANNEL_OFF,-1);
			vTaskDelay(system_off_delay);
			continue;
		}
		UpdateAdcsStateMachine();

		if(!AdcsCmdQueueIsEmpty()){
			trbl = AdcsCmdQueueGet(&cmd);
			if(TRBL_SUCCESS != trbl){
				WriteAdcsLog(LOG_ADCS_QUEUE_ERR,trbl);
				AdcsTroubleShooting(trbl);
			}
			trbl = AdcsExecuteCommand(&cmd);
			if(TRBL_SUCCESS != trbl){
				WriteAdcsLog(LOG_ADCS_CMD_ERR,trbl);
				AdcsTroubleShooting(trbl);
			}
			WriteAdcsLog(LOG_ADCS_CMD_RECEIVED,cmd.subType);
		}
		trbl = GatherTlmAndData();
		if(TRBL_SUCCESS != trbl){
			WriteAdcsLog(LOG_ADCS_TLM_ERR,trbl);
			AdcsTroubleShooting(trbl);
		}
		vTaskDelay(delay_loop);
	}
}

