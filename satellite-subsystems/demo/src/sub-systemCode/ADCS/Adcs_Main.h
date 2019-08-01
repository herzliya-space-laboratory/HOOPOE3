#ifndef ADCS_MAIN_H_
#define ADCS_MAIN_H_

#define ADCS_ID 0
#define ADCS_I2C_ADRR 0x57

#include "AdcsTroubleShooting.h"


typedef enum
{
	CMD_UPDATE_STATE_MATRIX	// update chain matrix


}AdcsStateMachineCMD;

//ADCS main

int AddCommandToAdcsQueue();

void AdcsTask();

TroubleErrCode InitAdcs();

#endif // !ADCS_MAIN_H_
