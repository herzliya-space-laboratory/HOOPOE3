#ifndef ADCS_MAIN_H_
#define ADCS_MAIN_H_

#include "sub-systemCode/COMM/GSC.h"

#define ADCS_ID 0
#define ADCS_I2C_ADRR 0x57

#define MAX_ADCS_QUEUE_LENGTH 42

#include "AdcsTroubleShooting.h"


typedef enum
{
	CMD_UPDATE_STATE_MATRIX	// update chain matrix


}AdcsStateMachineCMD;

typedef enum{
	DELAY_LOOP,
	QUEUE_WAIT_TIME
}AdcsFramParameters;

TroubleErrCode UpdateAdcsFramParameters(AdcsFramParameters param, unsigned char *data);

TroubleErrCode AdcsInit();

void AdcsTask();


#endif // !ADCS_MAIN_H_
