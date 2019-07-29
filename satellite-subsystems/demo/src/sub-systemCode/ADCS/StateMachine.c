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
#include "Stage_Table.h"
#include <hal/Storage/FRAM.h>

#define ADCS_ID 0
#define ADCS_I2C_ADRR 0x57





TroubleErrCode UpdateAdcsStateMachine(TC_spl *command)//AdcsStateMachineCMD cmd, unsigned char *data, unsigned int length)
{
	if(NULL == command){
		return ERR_NULL_DATA;	//TODO: use
	}

	AdcsStateMachineCMD cmd = command->subType; //TODO: maybe in a separate parse function
	return TRBL_NO_ERROR;
}


int ADCS_I3C_command(TC_spl *command)
{
	if(NULL == command)
	{
		return ERR_NULL_DATA;
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
	memcpy(Config[offset],Data,leangth);
	return FRAM_write(Data, ADCS_CONFIG_START + offset,leangth);
}
