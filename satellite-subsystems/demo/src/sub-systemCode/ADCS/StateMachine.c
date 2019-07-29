/*
 * StateMachine.c
 *
 *  Created on: Jul 21, 2019
 *      Author: Michael
 */
#include <stdio.h>
#include <stdlib.h>

#include <hcc/api_fat.h>

//#include "../ADCS.h"
#include "StateMachine.h"
#include "../Global/GlobalParam.h"
#include "../Global/FRAMadress.h"
#include "Stage_Table.h"
#include <hal/Storage/FRAM.h>

#define ADCS_ID 0
#define ADCS_I2C_ADRR 0x57

#define SGP4_INCLINATION_ID 46
#define SGP4_INCLINATION_LN 8

#define SGP4_ECCENTRICY_ID 47
#define SGP4_ECCENTRICY_LN 8

#define SGP4_INCLINATION_ID 53
#define SGP4_INCLINATION_LN 8

#define SGP4_RAAN_ID 48
#define SGP4_RAAN_LN 8

#define SGP4_ARGUMENT_ID 49
#define SGP4_ARGUMENT_LN 8

#define SGP4_B_STAR_ID 50
#define SGP4_B_STAR_LN 8

#define SGP4_MEAN_MOTION_ID 51
#define SGP4_MEAN_MOTION_LN 8

#define SGP4_MEAN_ANOMALY_ID 52
#define SGP4_MEAN_ANOMALY_LN 8

#define RESET_BOOTREGISTERS_ID 6
#define RESET_BOOTREGISTERS_LN 0

#define MAG_CONFIG_ID 21
#define MAG_CONFIG_LN 3

#define DETUMBLING_PARAMATER_ID 38
#define DETUMBLING_PARAMATER_LN 14

#define Y_WHEEL_PARAM_ID 39
#define Y_WHEEL_PARAM_LN 20

#define REACTION_WHEEL_PARAM_ID 40
#define REACTION_WHEEL_PARAM_LN 8

#define MOMENTS_INERTIA_ID 41
#define MOMENTS_INERTIA_LN 12

#define PRODUCTS_INERTIA_ID 42
#define PRODUCTS_INERTIA_LN 12

#define MODE_OF_MAG_OPERATION_ID 56
#define MODE_OF_MAG_OPERATION_LN 1

TroubleErrCode UpdateAdcsStateMachine(AdcsStateMachineCMD cmd, unsigned char *data, unsigned int length)
{

	return TRBL_NO_ERROR;
}

TroubleErrCode UpdateStateMatrix(StateMatrix New_mat)
{

	return TRBL_NO_ERROR;
}

int ADCS_command(unsigned char id, unsigned char* data, unsigned short dat_len)
{
	if(NULL == data)
	{
		return ERR_NULL_DATA;
	}


	unsigned char arr[dat_len + 1];
	unsigned int i;
	arr[0] = id;

	for (i = 1; i < dat_len + 1; i++)
	{
		arr[i] = data[i - 1];
	}

	I2C_write(ADCS_I2C_ADRR, &arr[0], dat_len + 1);
	return ReadACK(id);
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
//int Mag_Config(unsigned char * data)
//{
//	if(data == NULL)
//		{
//			return -1;
//		}
//	return ADCS_command(MAG_CONFIG_ID, data,MAG_CONFIG_LN);
//}
//int set_Detumbling_Param(unsigned char* data)
//{
//	if(data == NULL)
//		{
//			return -1;
//		}
//	return ADCS_command(DETUMBLING_PARAMATER_ID, data, DETUMBLING_PARAMATER_LN);
//}
//int Set_Y_Wheel_Param(unsigned char* data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(Y_WHEEL_PARAM_ID, data, Y_WHEEL_PARAM_LN);
//}
//int Set_REACTION_Wheel_Param(unsigned char* data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(REACTION_WHEEL_PARAM_ID, data, REACTION_WHEEL_PARAM_LN);
//}
//int Set_Moments_Inertia(unsigned char* data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(MOMENTS_INERTIA_ID, data, MOMENTS_INERTIA_LN);
//}
//int Set_Products_Inertia(unsigned char* data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(PRODUCTS_INERTIA_ID, data, PRODUCTS_INERTIA_LN);
//}
//int set_Mode_Of_Mag_OPeration(unsigned char * data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(MODE_OF_MAG_OPERATION_ID, data, MODE_OF_MAG_OPERATION_LN);
//}
int SGP4_Oribt_Param(unsigned char * data)
{
	if(data == NULL){return -1;}
	return cspaceADCS_setSGP4OrbitParameters(ADCS_ID,data);
}
//int SGP4_Oribt_Inclination(unsigned char * data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(SGP4_INCLINATION_ID, data, SGP4_INCLINATION_LN);
//}
//int SGP4_Oribt_Eccentricy(unsigned char * data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(SGP4_ECCENTRICY_ID, data, SGP4_ECCENTRICY_LN);
//}
//int SGP4_Oribt_RAAN(unsigned char * data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(SGP4_RAAN_ID, data, SGP4_RAAN_LN);
//}
//int SGP4_Oribt_Argument(unsigned char * data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(SGP4_ARGUMENT_ID, data, SGP4_ARGUMENT_LN);
//}
//int SGP4_Oribt_B_Star(unsigned char * data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(SGP4_B_STAR_ID, data, SGP4_B_STAR_LN);
//}
//int SGP4_Oribt_Mean_Motion(unsigned char * data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(SGP4_MEAN_MOTION_ID, data, SGP4_MEAN_MOTION_LN);
//}
//int SGP4_Oribt_Mean_Anomaly(unsigned char * data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(SGP4_MEAN_ANOMALY_ID, data, SGP4_MEAN_ANOMALY_LN);
//}
//int SGP4_Oribt_Epoc(unsigned char * data)
//{
//	if(data == NULL){return -1;}
//	return ADCS_command(SGP4_INCLINATION_ID, data, SGP4_INCLINATION_LN);
//}
//int Reset_Boot_Registers()
//{
//	char data = "Null";
//	return ADCS_command(RESET_BOOTREGISTERS_ID, &data, RESET_BOOTREGISTERS_LN);
//}


int ReadACK(int cmd)
{
	char data[3];
	unsigned char id = 240;
	I2C_write(ADCS_I2C_ADRR, &id, 1);
	vTaskDelay(5);
	I2C_read(ADCS_I2C_ADRR, data, 2);
	if (data[0] != cmd)
	{
		if(data == NULL)
		{
			return ERR_NULL_DATA;
		}
		return ERR_WRONG_TYPE;// cmd didn't go to the ADCS
	}
	int ret = data[2];
	return ret;
}
int Save_Config()
{
	return cspaceADCS_saveConfig(ADCS_ID);
}
int Save_Orbit_Param()
{

	return cspaceADCS_saveOrbitParam(ADCS_ID);
}
int Set_Mag_Mount(unsigned char * config_data)
{
	if(config_data == NULL){return -1;}
	return cspaceADCS_setMagMountConfig(ADCS_ID, config_data);
}
int Set_Mag_Offest_Scale(unsigned char* config_data)
{
	if(config_data == NULL){return -1;}
	return cspaceADCS_setMagOffsetScalingConfig(ADCS_ID, config_data);
}
int Set_Mag_Sense(unsigned char * config_data)
{if(config_data == NULL){return -1;}
	return cspaceADCS_setMagSensConfig(ADCS_ID, config_data);
}
int setUnixTime(unsigned char * unix_time)
{
	if(unix_time == NULL){return -1;}
	return cspaceADCS_setCurrentTime(ADCS_ID, unix_time);
}
int Cashe_State(unsigned char state)
{
	if(state == NULL){return -1;}
	return cspaceADCS_cacheStateADCS(ADCS_ID, state);
}
int ADCS_CONFIG_PART(char * Data,int offset,int leangth,char * Config)
{
	if(Data == NULL){return -1;}
	memccpy(Config[offset],Data,leangth);
	return FRAM_write(Data, ADCS_CONFIG_START + offset,leangth);
}
int ADCS_Config(unsigned char * data)
{
	if(data == NULL){return -1;}
	return ADCS_command(20, data, 272);
}
