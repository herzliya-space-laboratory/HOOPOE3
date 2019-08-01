
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <hcc/api_fs_err.h>

#include "../Main/commands.h"
#include "../Main/CMD/ADCS_CMD.h"
#include "sub-systemCode/Global/GlobalParam.h"

#include "Adcs_Main.h"
#include "Adcs_Config.h"
#include "AdcsTroubleShooting.h"
#include "AdcsGetDataAndTlm.h"
#include "StateMachine.h"

#include <stdio.h>
#include <stdlib.h>

#define SIZE_OF_TELEMTRY 6
#define ADCS_RESPONSE_DELAY 1000 
#define ADCS_LOOP_DELAY 1000 //the loop runs in 1Hz


int AddCommandToAdcsQueue()
{
	return 0;
}

int GetCmdFromQueue(TC_spl *command)
{
	(void)command;
	return TRBL_SUCCESS;
}

TroubleErrCode InitAdcs()
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
		//TODO: handle file error
		return -1; //TODO: return appropriate TRBL
	}

	return TRBL_SUCCESS;
}

void AdcsTask()
{

}

