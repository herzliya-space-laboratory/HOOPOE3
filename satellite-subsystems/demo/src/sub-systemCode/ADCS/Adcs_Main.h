#ifndef ADCS_MAIN_H_
#define ADCS_MAIN_H_




typedef enum
{
	CMD_UPDATE_STATE_MATRIX	// update chain matrix


}AdcsStateMachineCMD;

xTaskHandle AdcsTaskHandle = NULL;	// a handle to the adcs task.

//ADCS main
void AdcsTask();
void InitAdcs();

#endif // !ADCS_MAIN_H_
