
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

Gather_TM_Data Data[24]
TC_spl command;
int ActiveStateMachine;
TroubleErrCode err[3]; //TODO: big no no. shouldn't be a global variable


int CreateAdcsFiles();

void InitAdcs()
{
	InitData(Data)
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
	CreateAdcsFiles();

}

int CreateAdcsFiles()
{
	//TODO: check errors and return
	c_fileCreate(ESTIMATED_ANGLES_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(ESTIMATED_ANGULAR_RATE_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_ECI_POSITION_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_SATALLITE_VELOCITY_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_LLH_POSTION_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_MAGNETIC_FIELD_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(CSS_SUN_VECTOR_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(SENSOR_RATE_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(WHEEL_SPEED_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(MAGNETORQUER_CMD_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(WHEEL_SPEED_CMD_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(IGRF_MODEL_MAGNETIC_FIELD_VECTOR_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(GYRO_BIAS_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(INNOVATION_VECTOR_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(ERROR_VECTOR_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(QUATERNION_COVARIANCE_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(ANGULAR_RATE_COVARIANCE_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_CSS_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(RAW_MAGNETOMETER_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(ESTIMATED_QUATERNION_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_ECEF_POSITION_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
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
			ActiveStateMachine = FALSE;

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

