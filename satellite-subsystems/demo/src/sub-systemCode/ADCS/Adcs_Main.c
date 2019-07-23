
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
	InitStateMachine();
	int ActiveStateMachine = 1;
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

		if(ActiveStateMachine == 1)
		{
		err[1] = UpdateAdcsStateMachine(CMD_UPDATE_STATE_MATRIX, NULL, 0);
		//todo: SM Log
		ActiveStateMachine = 0;
		}

		err[0] = FindErr();

		if(err != TRBL_NO_ERROR)
		{
			ActiveStateMachine = 0;
			AdcsTroubleShooting(err);
			continue;
		}

		err[2] = GatherTlmAndData();
		vTaskDelay(1000);

		if(uxQueueSpacesAvailable(AdcsCmdTour) == sizeof(TC_spl)*COMMAND_LIST_SIZE)//cheak if queue free size = size of queue
		{
			ActiveStateMachine = 1;
		}else{
			GetCmdFromQueue();
			ActiveStateMachine = 0;
			//todo System Log
		}
	}
}






TC_spl GetCmdFromQueue()
{

	return 0;
}
