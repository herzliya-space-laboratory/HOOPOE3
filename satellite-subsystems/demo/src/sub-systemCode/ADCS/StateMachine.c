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

#define TESTING
#define ADCS_ID 0
#define ADCS_I2C_ADRR 0x57

TroubleErrCode AdcsAutoControl(TC_spl *decode);
TroubleErrCode AdcsManualControl(TC_spl *decode);
int AdcsConfigPart(char * Data, int offset,int leangth,char * Config);

TroubleErrCode UpdateAdcsStateMachine(TC_spl *decode, OperationMode Mode)
{
	if(NULL == decode){
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

	AdcsStateMachineCMD cmd = decode->subType; //TODO: maybe in a separate parse function
	return TRBL_NO_ERROR;
}

TroubleErrCode AdcsManualControl(TC_spl *decode)
{
	if(NULL == decode)
	{
		return TRBL_NULL_DATA;
	}

	TroubleErrCode err = TRBL_NO_ERROR;
	OperationMode mode;
	cspace_adcs_bootprogram bootPro;
	cspace_adcs_savimag_t savimag;
	cspace_adcs_magmode_t magmode;
	switch(decode->subType)
	{
	case(ADCS_GENRIC_ST):
		err = ADCS_command(decode);
		break;
	case(ADCS_SAVE_VICTOR_ST):
		err = UpdateTlmSaveVector(decode->data);
		break;
	case(ADCS_MODE_ST):
		memcpy(&mode,decode->data,sizeof(mode));
		err = ChangeOperationMode(mode);
		break;
	case(SAVE_CONFIG_ST):
		err = cspaceADCS_saveConfig(ADCS_ID);
		break;
	case(SET_MAG_OFFEST_SCALE_ST):
		err = cspaceADCS_setMagOffsetScalingConfig(ADCS_ID, (cspace_adcs_offscal_cfg_t*)decode->data);
		break;
	case(SET_MAG_SENSE_ST):
		err = cspaceADCS_setMagSensConfig(ADCS_ID,(cspace_adcs_magsencfg_t*) decode->data);
		break;
	case(SET_UNIX_TIME_ST):
		err = cspaceADCS_setCurrentTime(ADCS_ID, (cspace_adcs_unixtm_t*)decode->data);
		break;
	case(CACHE_STATE_ST):
		err = cspaceADCS_cacheStateADCS(ADCS_ID, decode->data);
		break;
	case(ADCS_CONFIG_PART_ST):
		err = AdcsConfigPart(decode->data,decode->data,decode->length);
		break;
	case(INITIALIZE_ST):
		err = cspaceADCS_initialize(ADCS_ID, decode->data);
	    break;
	case(COMPONENT_RESET_ST):
		err = cspaceADCS_componentReset(ADCS_ID);
	    break;
	case(SET_RUN_MODE_ST):
		err = cspaceADCS_setRunMode(ADCS_ID);
	    break;
	case(SET_PWR_CTRL_DEVICE_ST):
		err = cspaceADCS_setPwrCtrlDevice(ADCS_ID, decode->data);
	    break;
	case(ADCS_CLEAR_ERRORS_FLAG_ST):
		err = cspaceADCS_clearErrors(ADCS_ID, decode->data);
	    break;
	case(SET_MAG_OUTPUT_ST):
		err = cspaceADCS_setMagOutput(ADCS_ID, decode->data);
	    break;
	case(SET_WHEEL_SPEED_ST):
		err = cspaceADCS_setWheelSpeed(ADCS_ID, decode->data);
	    break;
	case(DEPLOY_MAG_BOOM_ADCS_ST):
#ifndef TESTING
		//todo: err = cspaceADCS_deployMagBoomADCS(ADCS_ID, decode->data);
#else
		printf("MAG BOOM deployed!");
		err = TRBL_NO_ERROR;
#endif
	    break;
	case(SET_ATT_CTRL_MODE_ST):
		err = cspaceADCS_setAttCtrlMode(ADCS_ID, decode->data);
	    break;
	case(SET_ATT_EST_MODE_ST):
		err = cspaceADCS_setAttEstMode(ADCS_ID, decode->data);
	    break;
	case(SET_CAM1_SENSOR_CONFIG_ST):
		err = cspaceADCS_setCam1SensorConfig(ADCS_ID, decode->data);
	    break;
	case(SET_CAM2_SENSOR_CONFIG_ST):
		err = cspaceADCS_setCam2SensorConfig(ADCS_ID, decode->data);
	    break;
	case(SET_MAG_MOUNT_CONFIG_ST):
		err = cspaceADCS_setMagMountConfig(ADCS_ID, decode->data);
	    break;
	case(SET_RATE_SENSOR_CONFIG_ST):
		err = cspaceADCS_setMagSensConfig(ADCS_ID, decode->data);
	    break;
	case(SET_RATE_SENSOR_CONFIG_ST):
		err = cspaceADCS_setRateSensorConfig(ADCS_ID, decode->data);
		break;
	case(SET_ESTIMATION_PARAM1_ST):
		err = cspaceADCS_setEstimationParam1(ADCS_ID, decode->data);
	    break;
	case(SET_ESTIMATION_PARAM2_ST):
		err = cspaceADCS_setEstimationParam2(ADCS_ID, decode->data);
	    break;
	case(SET_MAGNETOMTER_MODE_ST):
		memcpy(magmode,decode->data, sizeof(magmode));
		err = cspaceADCS_setMagnetometerMode(ADCS_ID, magmode);
	    break;
	case(SET_SGP4_ORBIT_PARM_ST):
		err = cspaceADCS_setSGP4OrbitParameters(ADCS_ID, decode->data);
		break;
	case(SAVE_IMAGE_ST):
		memcpy(savimag,decode->data, sizeof(savimag));
		err = cspaceADCS_saveImage(ADCS_ID, savimag);
	    break;
	case(BL_SETBOOT_INDEX_ST):
			memcpy(bootPro,decode->data,sizeof(bootPro));
		err = cspaceADCS_BLSetBootIndex(ADCS_ID, decode->data);
	    break;
	case(BL_RUN_SELECTED_PROGRAM_ST):

		err = cspaceADCS_BLRunSelectedProgram(ADCS_ID);
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

int AdcsConfigPart(char * Data, int offset, int leangth, char * Config)
{
	if(Data == NULL){
		return -1; //TODO: return err according to TRBL...
	}
	memcpy(Config[offset],Data,leangth);
	 int err = FRAM_write(Data, ADCS_CONFIG_START + offset,leangth);
	 return err;
}

