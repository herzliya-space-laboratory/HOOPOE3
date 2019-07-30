
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <hcc/api_fs_err.h>
#include "Adcs_Main.h"
#include "../Main/commands.h"
#include "Adcs_Config.h"

#define SIZE_OF_TELEMTRY 6
#define ADCS_RESPONSE_DELAY 1000 
#define ADCS_LOOP_DELAY 1000 //the loop runs in 1Hz

Gather_TM_Data Data[24];
TC_spl command;
int ActiveStateMachine;
TroubleErrCode err[3]; //TODO: big no no. shouldn't be a global variable


void InitAdcs()
{
	InitData(Data);
	int ret = f_enterFS(); // Register this task with filesystem
	if(F_NO_ERROR != ret)
	{
		//TODO: handle file error
	}
	InitStateMachine();
	//todo: check xQueueCreate
	ActiveStateMachine = TRUE;
	err[0] = TRBL_NO_ERROR;
	err[1] = TRBL_NO_ERROR;
	err[2] = TRBL_NO_ERROR;
	CreateTlmElementFiles();

}

void AdcsTask()
{
	while(TRUE)
	{
		if(!get_system_state(ADCS_param))
		{
			vTaskDelay(ADCS_RESPONSE_DELAY);
			continue;
		}

		if(ActiveStateMachine == TRUE)
		{
			err[1] = UpdateAdcsStateMachine(&command);
			//todo: SM Log
			ActiveStateMachine = FALSE;
		}

		if(AdcsTroubleShooting(err) != TRBL_NO_ERROR)
		{
			ActiveStateMachine = TRUE;
			continue;
		}

		err[2] = GatherTlmAndData(Data);
		vTaskDelay(ADCS_LOOP_DELAY);

		//todo: move into a separate function
		if(IsAdcsQueueEmpty())//check if queue is empty
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

