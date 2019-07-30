/*
 * StateMachine.c
 *
 *  Created on: Jul 21, 2019
 *      Author: Michael
 */
#include <stdio.h>
#include <stdlib.h>

#include <hcc/api_fat.h>

#include "Adcs_Config.h"
#include "StateMachine.h"
#include "../Global/GlobalParam.h"
#include "../Global/FRAMadress.h"


#include "../COMM/splTypes.h"

#include <hal/Storage/FRAM.h>

#define ADCS_ID 0
#define ADCS_I2C_ADRR 0x57

TroubleErrCode AdcsAutoControl(TC_spl *decode);
TroubleErrCode AdcsManualControl(TC_spl *decode);

TroubleErrCode UpdateAdcsStateMachine(TC_spl *decode, OperationMode Mode)
{
	if(NULL == command){
		return TRBL_NULL_DATA;	//TODO: use
	}

	if(Mode == MANUAL_MODE)
	{
		Manual_control(decode);
	}
	else
	{
		Auto_control(decode);
	}

	AdcsStateMachineCMD cmd = command->subType; //TODO: maybe in a separate parse function
	return TRBL_NO_ERROR;
}

TroubleErrCode AdcsManualControl(TC_spl *decode)
{
	if(NULL == decode)
	{
		return TRBL_NULL_DATA;
	}

	TroubleErrCode err;
	switch(decode->subType)
	{
	case(ADCS_GENRIC_ST):

		break;
	case(ADCS_SAVE_VICTOR_ST):

		break;
	case(ADCS_MODE_ST):

		break;

	case(SAVE_CONFIG_ST):

		break;
	case(SET_MAG_OFFEST_SCALE_ST):

		break;
	case(SET_MAG_SENSE_ST):

		break;
	case(SET_UNIX_TIME_ST):

		break;
	case(CACHE_STATE_ST):

		break;
	case(ADCS_CONFIG_PART_ST):

		break;
	case(INITIALIZE_ST):

	    break;
	case(COMPONENT_RESET_ST):

	    break;
	case(SET_RUN_MODE_ST):

	    break;
	case(SET_PWR_CTRL_DEVICE_ST):

	    break;
	case(ADCS_CLEAR_ERRORS_FLAG_ST):

	    break;
	case(SET_MAG_OUTPUT_ST):

	    break;
	case(SET_WHEEL_SPEED_ST):

	    break;
	case(DEPLOY_MAG_BOOM_ADCS_ST):

	    break;
	case(SET_ATT_CTRL_MODE_ST):

	    break;
	case(SET_ATT_EST_MODE_ST):

	    break;
	case(SET_CAM1_SENSOR_CONFIG_ST):

	    break;
	case(SET_CAM2_SENSOR_CONFIG_ST):

	    break;
	case(SET_MAG_MOUNT_CONFIG_ST):

	    break;
	case(SET_RATE_SENSOR_CONFIG_ST):

	    break;
	case(SET_ESTIMATION_PARAM1_ST):

	    break;
	case(SET_ESTIMATION_PARAM2_ST):

	    break;
	case(SET_MAGNETOMTER_MODE_ST):

	    break;
	case(SET_SGP4_ORBIT_PARM_ST):

		break;
	case(SAVE_IMAGE_ST):

	    break;
	case(BL_SETBOOT_INDEX_ST):

	    break;

	}

	return err;
}

TroubleErrCode AdcsAutoControl(TC_spl *decode)
{
	if(NULL == decode)
	{
		return TRBL_NULL_DATA;
	}

	TroubleErrCode err;
	switch(decode->data[0])
	{

	}
	//TODO: insert the correct subtype

	return err;

}

TroubleErrCode ChangeOperationMode(OperationMode Mode)
{
	OperationMode temp;
	int err;
	err = FRAM_read(&temp, CONTROL_MODE_ADDR, sizeof(temp));
	if(err != 0){
		return TRBL_FRAM_READ_ERR;
	}
	if(temp == Mode){
		return TRBL_NO_ERROR;
	}

	err = FRAM_write(&Mode, CONTROL_MODE_ADDR, sizeof(Mode));
	if(err != 0){
		return TRBL_FRAM_WRITE_ERR;
	}
	return TRBL_NO_ERROR;
}

TroubleErrCode GetOperationMode(OperationMode *Mode)
{
	//TODO: finish
	return TRBL_NO_ERROR;
}



int ADCS_I2C_command(TC_spl *command)
{
	if(NULL == command)
	{
		return TRBL_NULL_DATA;
	}

	I2C_write(ADCS_I2C_ADRR, command->data, command->length);
	return ReadACK(command->data[0]);
}

int Set_Boot_Index(unsigned char* data)
{
	if(data == NULL)
	{
		return -1;
	}
	return cspaceADCS_BLSetBootIndex(ADCS_ID,*data);
}
int Run_Selected_Boot_Program()
{
	return cspaceADCS_BLRunSelectedProgram(ADCS_ID);
}


int SGP4_Oribt_Param(unsigned char * data)
{
	if(data == NULL)
	{
		return -1;
	}
	return cspaceADCS_setSGP4OrbitParameters(ADCS_ID,data);
}


int ReadACK(int cmd)
{
	char data[3] = {0};
	unsigned char id = 240; // TODO: check what this means in the adcs command datasheet
	I2C_write(ADCS_I2C_ADRR, &id, 1);
	vTaskDelay(5);// todo: check if really need this delay. the delay can overwrite the requested data
	I2C_read(ADCS_I2C_ADRR, data, 2);
	if (data[0] != cmd)
	{
		return TRBL_WRONG_TYPE;// cmd didn't go to the ADCS
	}
	int ret = data[2];
	return ret;
}
//int Save_Config()
//{
//	return cspaceADCS_saveConfig(ADCS_ID);
//}
//
//int Save_Orbit_Param()
//{
//	return cspaceADCS_saveOrbitParam(ADCS_ID);
//}
//
//int Set_Mag_Mount(unsigned char * config_data)
//{
//	if(config_data == NULL){return -1;}
//	return cspaceADCS_setMagMountConfig(ADCS_ID, config_data);
//}
//
//int Set_Mag_Offest_Scale(unsigned char* config_data)
//{
//	if(config_data == NULL){return -1;}
//	return cspaceADCS_setMagOffsetScalingConfig(ADCS_ID, config_data);
//}
//
//int Set_Mag_Sense(unsigned char * config_data)
//{if(config_data == NULL){return -1;}
//	return cspaceADCS_setMagSensConfig(ADCS_ID, config_data);
//}
//
//int setUnixTime(unsigned char * unix_time)
//{
//	if(unix_time == NULL){return -1;}
//	return cspaceADCS_setCurrentTime(ADCS_ID, unix_time);
//}
//
//int Cashe_State(unsigned char state)
//{
//	if(state == NULL){return -1;}
//	return cspaceADCS_cacheStateADCS(ADCS_ID, state);
//}
//
//int AdcsConfigPart(char * Data, int offset,int leangth,char * Config)
//{
//	if(Data == NULL){return -1;}
//	memcpy(Config[offset],Data,leangth);
//	return FRAM_write(Data, ADCS_CONFIG_START + offset,leangth);
//}
