#ifndef ADCS_MAIN_H_
#define ADCS_MAIN_H_

#include "sub-systemCode/COMM/GSC.h"
#include "sub-systemCode/Global/Global.h"
#include "AdcsTroubleShooting.h"


typedef enum
{
	CMD_UPDATE_STATE_MATRIX	// update chain matrix

}AdcsStateMachineCMD;

typedef enum __attribute__ ((__packed__)){
	DELAY_LOOP,
	DELAY_SYSTEM_OFF,
	QUEUE_WAIT_TIME
}AdcsFramParameters;

TroubleErrCode UpdateAdcsFramParameters(AdcsFramParameters param, unsigned char *data);

TroubleErrCode AdcsInit();

void AdcsTask();


#endif // !ADCS_MAIN_H_
