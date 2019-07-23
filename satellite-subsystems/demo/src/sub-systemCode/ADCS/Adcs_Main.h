#ifndef ADCS_MAIN_H_
#define ADCS_MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include "../Main/CMD/ADCS_CMD.h"
#include "StateMachine.h"
#include "AdcsTroubleShooting.h"
#include "AdcsGetDataAndTlm.h"



typedef enum
{
	CMD_UPDATE_STATE_MATRIX	// update chain matrix


}AdcsStateMachineCMD;

xTaskHandle AdcsTaskHandle = NULL;	// a handle to the adcs task.

//ADCS main
void AdcsTask();

//Get Cmd from Queue
TC_spl GetCmdFromQueue();

#endif // !ADCS_MAIN_H_
