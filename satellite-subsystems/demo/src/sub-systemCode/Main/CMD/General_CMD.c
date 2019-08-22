/*
 * General_CMD.c
 *
 *  Created on: Jun 22, 2019
 *      Author: elain
 */
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <hal/Drivers/I2C.h>

#include "General_CMD.h"
#include "../../Global/TM_managment.h"
#include "../../Main/HouseKeeping.h"
#include "../../TRXVU.h"
#include "../../Ants.h"

#define create_task(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask) xTaskCreate( (pvTaskCode) , (pcName) , (usStackDepth) , (pvParameters), (uxPriority), (pxCreatedTask) ); vTaskDelay(10);


void cmd_generic_I2C(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_GENERIC_I2C_CMD;

	int error = I2C_write((unsigned int)cmd.data[0], &cmd.data[2] ,cmd.data[1]);

	if (error == 0)
		*err = ERR_SUCCESS;
	if (error < 0 || error == 4)
		*err = ERR_WRITE;
	if (error > 4)
		*err = ERR_FAIL;
}
void cmd_dump(TC_spl cmd)
{
	if (cmd.length != 2 * TIME_SIZE + 5)
	{
		return;
	}
	//1. build combine data with command_id
	unsigned char raw[2 * TIME_SIZE + 5 + 4] = {0};
	// 1.1. copying command id
	BigEnE_uInt_to_raw(cmd.id, &raw[0]);
	// 1.2. copying command data
	memcpy(raw + 4, cmd.data, 2 * TIME_SIZE + 5);
	create_task(Dump_task, (const signed char * const)"Dump_Task", (unsigned short)(STACK_DUMP_SIZE), (void*)raw, FIRST_PRIORITY, xDumpHandle);
}
void cmd_delete_TM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	//todo
	*type = ACK_MEMORY;
	if (cmd.length != 13)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	// 1. extracting time for deleting process
	time_unix start_time = BigEnE_raw_to_uInt(&cmd.data[0]);
	time_unix end_time = BigEnE_raw_to_uInt(&cmd.data[4]);
	HK_types files[5];
	for (int i = 0; i < 5; i++)
	{
		files[i] = (HK_types)cmd.data[8 + i];
	}
	// 2. check if start time comes before end time
	if (start_time > end_time)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	// 3. extracting the file name
	char file_name[MAX_F_FILE_NAME_SIZE];
	FileSystemResult result = FS_SUCCSESS;
	FileSystemResult errorRes = FS_SUCCSESS;
	for (int  i = 0; i < 5; i++)
	{
		if (files[i] != this_is_not_the_file_you_are_looking_for)
		{
			find_fileName(files[i], file_name);
			//todo: add real function
			//result = c_filedelete(file_name, start_time, end_time);
			if (result != FS_SUCCSESS)
			{
				errorRes = FS_FAIL;
			}
		}
	}
	// 6. check result and updating *err
	switch(errorRes)
	{
	case FS_SUCCSESS:
		*err = ERR_SUCCESS;
		break;
	case FS_TOO_LONG_NAME:
		*err = ERR_PARAMETERS;
		 break;
	case FS_NOT_EXIST:
		*err = ERR_PARAMETERS;
		 break;
	case FS_ALLOCATION_ERROR:
		*err = ERR_PARAMETERS;
		 break;
	case FS_FRAM_FAIL:
		*err = ERR_FRAM_WRITE_FAIL;
		 break;
	default:
		*err = ERR_FAIL;
		break;
	}
}
void cmd_dummy_func(Ack_type* type, ERR_type* err)
{
	printf("Im sorry Elai.\nI can't let you do it...\n");
	*type = ACK_THE_MIGHTY_DUMMY_FUNC;
	*err = ERR_SUCCESS;
}

void cmd_soft_reset_cmponent(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_SOFT_RESTART;
	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	int error = soft_reset_subsystem((subSystem_indx)cmd.data);
	switch (error)
	{
	case 0:
		*err = ERR_SUCCESS;
		break;
	case -444:
		*err = ERR_PARAMETERS;
		break;
	default:
		*err = ERR_FAIL;
		break;
	}
}
void cmd_hard_reset_cmponent(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_SOFT_RESTART;
	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	int error = soft_reset_subsystem((subSystem_indx)cmd.data);
	switch (error)
	{
	case 0:
		*err = ERR_SUCCESS;
		break;
	case -444:
		*err = ERR_PARAMETERS;
		break;
	default:
		*err = ERR_FAIL;
		break;
	}
}
void cmd_reset_satellite(Ack_type* type, ERR_type* err)
{
	*type = ACK_SOFT_RESTART;
	int error = hard_reset_subsystem(EPS);
	switch (error)
	{
	case 0:
		*err = ERR_SUCCESS;
		break;
	case -444:
		*err = ERR_PARAMETERS;
		break;
	default:
		*err = ERR_FAIL;
		break;
	}
}
void cmd_gracefull_reset_satellite(Ack_type* type, ERR_type* err)
{
	*type = ACK_SOFT_RESTART;
	int error = soft_reset_subsystem(OBC);
	switch (error)
	{
	case 0:
		*err = ERR_SUCCESS;
		break;
	case -444:
		*err = ERR_PARAMETERS;
		break;
	default:
		*err = ERR_FAIL;
		break;
	}
}
void cmd_upload_time(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_UPDATE_TIME;
	if (cmd.length != TIME_SIZE)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	// 1. converting to time_unix
	time_unix new_time = BigEnE_raw_to_uInt(&cmd.data[0]);
	// 2. update time on satellite
	if (Time_setUnixEpoch(new_time))
	{
		*err = ERR_FAIL;
	}
	else
	{
		*err = ERR_SUCCESS;
	}
}

void cmd_ARM_DIARM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ARM_DISARM;
	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	int error;//containing error from drivers
	//1. extracting data, ARM or DISARM
	switch (cmd.data[0])
	{
	case ARM_ANTS:
		//arm
		error = ARM_ants();
		break;
	case DISARM_ANTS:
		//disarm
		error = DISARM_ants();
		break;
	default:
		//command parameters are incorrect
		*err = ERR_PARAMETERS;
		return;
		break;
	}
	if (error == -2)
	{
		*err = ERR_NOT_INITIALIZED;
	}
	else if (error == -1)
	{
		*err = ERR_FAIL;
	}

	*err = ERR_SUCCESS;

}
void cmd_deploy_ants(Ack_type* type, ERR_type* err)
{
	/*type = ACK_REDEPLOY;
#ifndef ANTS_DO_NOT_DEPLOY
	int error = deploye_ants();
	if (error == -2)
	{
		*err = ERR_NOT_INITIALIZED;
	}
	else if (error != 0)
	{
		if (err == 666)
		{
			printf("FUN FACT: deploy ants when you don't have permission can summon the DEVIL!!!\nerror: %d\n", error);
		}
		*err = ERR_FAIL;
	}
	else
	{
		*err = ERR_SUCCESS;
	}

#else
	printf("sho! sho!, get out before i kill you\n");
	*err = ERR_TETST;
#endif*/
}
