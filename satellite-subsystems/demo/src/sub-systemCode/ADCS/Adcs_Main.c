
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "Adcs_Main.h"
#include "../Main/commands.h"

void AdcsTask()
{
	//todo: check xQueueCreate
	int ret = f_enterFS(); /* Register this task with filesystem */
	AdcsCmdTour = xQueueCreate(COMMAND_LIST_SIZE,sizeof(TC_spl));
	TC_spl commend;
	InitStateMachine();
	int ActiveStateMachine = TRUE;
	TroubleErrCode err[3];
	err[0] = TRBL_NO_ERROR;
	err[1] = TRBL_NO_ERROR;
	err[2] = TRBL_NO_ERROR;

	while(TRUE)
	{

		if(!get_system_state(ADCS_param))
		{
			vTaskDelay(1000);
			continue;
		}

		if(ActiveStateMachine == TRUE)
		{
		err[1] = UpdateAdcsStateMachine(CMD_UPDATE_STATE_MATRIX, NULL, 0);
		//todo: SM Log
		ActiveStateMachine = 0;
		}

		err[0] = FindErr();

		if(err != TRBL_NO_ERROR)
		{
			ActiveStateMachine = FALSE;
			AdcsTroubleShooting(err);
			continue;
		}

		err[2] = GatherTlmAndData();
		vTaskDelay(1000);

		if(uxQueueMessagesWaiting(AdcsCmdTour) == 0)//cheak if queue free size = size of queue
		{
			ActiveStateMachine = FALSE;
		}
		else
		{
			GetCmdFromQueue(&commend);
			ActiveStateMachine = TRUE;
			//todo System Log
		}
	}
}






Boolean8bit GetCmdFromQueue(TC_spl *request)
{
	Boolean8bit isWork = xQueueReceive(AdcsCmdTour, request, MAX_DELAY);
	return isWork;
}
