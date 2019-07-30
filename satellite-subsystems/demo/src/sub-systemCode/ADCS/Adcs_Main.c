
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <hcc/api_fs_err.h>
#include "Adcs_Main.h"
#include "../Main/commands.h"
#include "Adcs_Config.h"

#include "../Main/CMD/ADCS_CMD.h"
#include "StateMachine.h"
#include "AdcsTroubleShooting.h"
#include "AdcsGetDataAndTlm.h"

#include <stdio.h>
#include <stdlib.h>

#define SIZE_OF_TELEMTRY 6
#define ADCS_RESPONSE_DELAY 1000 
#define ADCS_LOOP_DELAY 1000 //the loop runs in 1Hz


int ActiveStateMachine = TRUE;

TroubleErrCode InitAdcs()
{
	TroubleErrCode trbl = TRBL_NO_ERROR;
	//TODO: finish initialization of the ADCS. see the demos for further reference
	if(F_NO_ERROR !=  f_enterFS()){
		//TODO: handle file error
		return -1; //TODO: return appropriate TRBL
	}
	trbl = InitStateMachine();
	if(TRBL_NO_ERROR != trbl){
		return -1;//TODO: return appropriate TRBL
	}
	//todo: check xQueueCreate
	Boolean err = CreateTlmElementFiles();
	if(FALSE == err)
		return -1; //TODO: return appropriate TRBL
}

void AdcsTask()
{
	TroubleErrCode err = {0};
	TC_spl command = {0};
	while(TRUE)
	{
		if(SWITCH_OFF == get_system_state(ADCS_param))
		{
			vTaskDelay(ADCS_RESPONSE_DELAY);
			continue;
		}

		if(ActiveStateMachine == TRUE)
		{
			err[1] = UpdateAdcsStateMachine(&command, MANUAL_MODE);
			//todo: SM Log
			ActiveStateMachine = FALSE;
		}

		if(AdcsTroubleShooting(err) != TRBL_NO_ERROR)
		{
			ActiveStateMachine = TRUE;
			continue;
		}

		err[2] = GatherTlmAndData();
		vTaskDelay(ADCS_LOOP_DELAY);

		//todo: move into a separate function
		if(IsAdcsQueueEmpty())
		{
			ActiveStateMachine = FALSE;
		}
		else
		{
			GetCmdFromQueue(&command);
			ActiveStateMachine = TRUE;
			//todo: System Log
		}
	}
}

