
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "Adcs_Main.h"
#include "../Main/commands.h"

#define ADCS_RESPONSE_DELAY 1000 
#define ADCS_LOOP_DELAY 1000 //the loop runs in 1Hz

Gather_TM_Data Data[24]
int ret;
TC_spl commend;
int ActiveStateMachine;
TroubleErrCode err[3];

void InitAdcs()
{
	InitData(Data)
	ret = f_enterFS(); /* Register this task with filesystem */
	InitStateMachine();
	ActiveStateMachine = TRUE;
	err[0] = TRBL_NO_ERROR;
	err[1] = TRBL_NO_ERROR;
	err[2] = TRBL_NO_ERROR;
}
void AdcsTask()
{
	//todo: check xQueueCreate


	while(TRUE)
	{

		if(!get_system_state(ADCS_param))
		{
			vTaskDelay(ADCS_RESPONSE_DELAY);
			continue;
		}

		if(ActiveStateMachine == TRUE)
		{
		err[1] = UpdateAdcsStateMachine(CMD_UPDATE_STATE_MATRIX, NULL, 0);
		//todo: SM Log
		ActiveStateMachine = FALSE;
		}

		err[0] = FindErr();

		if(err != TRBL_NO_ERROR)
		{
			ActiveStateMachine = FALSE;
			AdcsTroubleShooting(err);
			continue;
		}

		err[2] = GatherTlmAndData(Data);
		vTaskDelay(ADCS_LOOP_DELAY);

		if(IsAdcsQueueEmpty())//cheak if queue is empty
		{
			ActiveStateMachine = FALSE;
		}
		else
		{
			GetCmdFromQueue(&commend);
			ActiveStateMachine = TRUE;
			//todo: System Log
		}
	}
}






Boolean8bit GetCmdFromQueue(TC_spl *request)
{
	Boolean8bit isWork = xQueueReceive(AdcsCmdTour, request, MAX_DELAY);
	return isWork;
}
