/*
 * General_CMD.c
 *
 *  Created on: Jun 22, 2019
 *      Author: Hoopoe3n
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <at91/utility/exithandler.h>

#include <hal/Drivers/I2C.h>
#include <hal/Timing/Time.h>

#include "General_CMD.h"
#include "../../Global/TLM_management.h"
#include "../../Global/OnlineTM.h"
#include "../../Main/HouseKeeping.h"
#include "../../TRXVU.h"
#include "../../Ants.h"
#include "../../payload/DataBase/DataBase.h"

#include "../../Global/logger.h"

#define create_task(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask) xTaskCreate( (pvTaskCode) , (pcName) , (usStackDepth) , (pvParameters), (uxPriority), (pxCreatedTask) ); vTaskDelay(10);

void cmd_generic_I2C(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	int error = I2C_write((unsigned int)cmd.data[0], &cmd.data[2] ,cmd.data[1]);

	if (error == 0)
	{
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}
	*type = ACK_I2C;
	if (error == 4)
		*err = ERR_WRITE_FAIL;
	else if (error == 6)
		*err = ERR_TIME_OUT;
	else if (error == -2)
		*err = ERR_PARAMETERS;
	else
		*err = ERR_FAIL;
}
void cmd_dump(TC_spl cmd)
{
	if (cmd.length != 2 * TIME_SIZE + NUM_FILES_IN_DUMP + 1 + 1)
	{
		return;
	}
	//1. build combine data with command_id
	unsigned char raw[2 * TIME_SIZE + NUM_FILES_IN_DUMP + 4 + 1 + 1] = {0};
	// 1.1. copying command id
	BigEnE_uInt_to_raw(cmd.id, &raw[0]);
	// 1.2. copying command data
	memcpy(raw + 4, cmd.data, 2 * TIME_SIZE + NUM_FILES_IN_DUMP + 1 + 1);
	create_task(Dump_task, (const signed char * const)"Dump_Task", (unsigned short)(STACK_DUMP_SIZE), (void*)raw, (unsigned portBASE_TYPE)(TASK_DEFAULT_PRIORITIES), xDumpHandle);
}
void cmd_delete_TM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	int i_error = 0;
	if (cmd.length != 13)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}

	time_unix start_time = BigEnE_raw_to_uInt(&cmd.data[5]);
	time_unix end_time = BigEnE_raw_to_uInt(&cmd.data[9]);
	HK_types files[5];
	for (int i = 0; i < 5; i++)
	{
		files[i] = (HK_types)cmd.data[i];
	}

	if (start_time > end_time)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
		return;
	}

	char file_name[MAX_F_FILE_NAME_SIZE];
	FileSystemResult result = FS_SUCCSESS;
	FileSystemResult errorRes = FS_SUCCSESS;
	for (int  i = 0; i < 5; i++)
	{
		if (files[i] != this_is_not_the_file_you_are_looking_for)
		{
			i_error = HK_find_fileName(files[i], file_name);
			if (i_error != 0)
				continue;
			result = c_fileDeleteElements(file_name, start_time, end_time);
			if (result != FS_SUCCSESS)
			{
				errorRes = result;
			}
		}
	}

	switch(errorRes)
	{
	case FS_SUCCSESS:
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
		break;
	case FS_NOT_EXIST:
		*err = ERR_NOT_EXIST;
		 break;
	case FS_FRAM_FAIL:
		*err = ERR_WRITE_FAIL;
		 break;
	default:
		WriteErrorLog(LOG_ERR_DELETE_TM, SYSTEM_OBC, errorRes);
		*err = ERR_FAIL;
		break;
	}
	*type = ACK_FILE_SYSTEM;
}
void cmd_reset_file(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;

	if (cmd.length != 5)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}

	char file_name[MAX_F_FILE_NAME_SIZE];
	FileSystemResult reslt = FS_SUCCSESS;
	for (int i = 0; i < 5; i++)
	{
		if ((HK_types)cmd.data[i] == this_is_not_the_file_you_are_looking_for)
			continue;

		if (HK_find_fileName((HK_types)cmd.data[i], file_name) == 0)
			reslt = c_fileReset(file_name);

		if (reslt != FS_SUCCSESS)
		{
			*type = ACK_FILE_SYSTEM;
			*err = ERR_NOT_EXIST;
		}
	}
}
void cmd_format_SD(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length == 0)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
	}
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
	sd_format(cmd.data[0]);
	FileSystemResult resFS = reset_FRAM_FS();
	if (resFS != FS_SUCCSESS)
	{
		*type = ACK_FRAM;
		*err = ERR_WRITE_FAIL;
	}
	clearImageDataBase();
	gracefulReset();
}
void cmd_dummy_func(Ack_type* type, ERR_type* err)
{
	//printf("Im sorry Hoopoe3.\nI can't let you do it...\n");
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
}
void cmd_reset_TLM_SD(Ack_type* type, ERR_type* err)
{
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
	char names[256];
	strcpy(names, "A:/");
	deleteDir(names, FALSE);
}

void cmd_stop_TM(Ack_type* type, ERR_type* err)
{
	Boolean8bit value = FALSE_8BIT;
	int i_error = FRAM_write_exte(&value, STOP_TELEMETRY_ADDR, 1);
	if (i_error == 0)
	{
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}
	else
	{
		*type = ACK_FRAM;
		*err = ERR_WRITE_FAIL;
	}
}
void cmd_resume_TM(Ack_type* type, ERR_type* err)
{
	Boolean8bit value = TRUE_8BIT;
	int i_error = FRAM_write_exte(&value, STOP_TELEMETRY_ADDR, 1);
	if (i_error == 0)
	{
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}
	else
	{
		*type = ACK_FRAM;
		*err = ERR_WRITE_FAIL;
	}
}

void cmd_soft_reset_cmponent(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 1)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	int error = soft_reset_subsystem((subSystem_indx)cmd.data[0]);
	switch (error)
	{
	case 0:
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
		break;
	case -444:
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
		break;
	default:
		*type = ACK_SYSTEM;
		*err = ERR_DRIVER;
		break;
	}
}
void cmd_hard_reset_cmponent(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 1)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	int error = hard_reset_subsystem((subSystem_indx)cmd.data[0]);
	switch (error)
	{
	case 0:
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
		break;
	case -444:
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
		break;
	default:
		*type = ACK_SYSTEM;
		*err = ERR_DRIVER;
		break;
	}
}
void cmd_reset_satellite(Ack_type* type, ERR_type* err)
{
	int error = hard_reset_subsystem(EPS);
	switch (error)
	{
	case 0:
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
		break;
	case -444:
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
		break;
	default:
		*type = ACK_SYSTEM;
		*err = ERR_DRIVER;
		break;
	}
}
void cmd_gracefull_reset_satellite(Ack_type* type, ERR_type* err)
{
	int error = soft_reset_subsystem(OBC);
	switch (error)
	{
	case 0:
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
		break;
	case -444:
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
		break;
	default:
		*type = ACK_SYSTEM;
		*err = ERR_DRIVER;
		break;
	}
}
void cmd_upload_time(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != TIME_SIZE)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	// 1. converting to time_unix
	time_unix new_time = BigEnE_raw_to_uInt(cmd.data);
	// 2. update time on satellite
	int error = Time_setUnixEpoch(new_time);
	if (error)
	{
		//printf("fuckeddddddd\n");
		//printf("I gues we need to stay on this time forever\n");
		WriteErrorLog(LOG_ERR_SET_TIME, SYSTEM_OBC, error);
		*type = ACK_SYSTEM;
		*err = ERR_FAIL;
	}
	else
	{
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}
}

void cmd_ARM_DIARM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 1)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	int error;

	switch (cmd.data[0])
	{
	case ARM_ANTS:
		error = ARM_ants();
		break;
	case DISARM_ANTS:
		error = DISARM_ants();
		break;
	default:
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
		return;
		break;
	}
	if (error == -2)
	{
		*type = ACK_ANTS;
		*err = ERR_NOT_INITIALIZED;
	}
	else if (error == -1)
	{
		*type = ACK_ANTS;
		*err = ERR_FAIL;
	}

	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;

}
void cmd_deploy_ants(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	(void)type;
	(void)err;
#ifndef ANTS_DO_NOT_DEPLOY
	int error = deploye_ants(cmd.data[0]);
	if (error == -2)
	{
		*type = ACK_ANTS;
		*err = ERR_NOT_INITIALIZED;
	}
	else if (error != 0)
	{
		if (error == 666)
		{
			printf("FUN FACT: deploy ants when you don't have permission can summon the DEVIL!!!\nerror: %d\n", error);
		}
		*type = ACK_ANTS;
		*err = ERR_FAIL;
	}
	else
	{
	 	*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}

#else
	printf("sho! sho!, get out before i kill you\n");
#endif
}

void cmd_get_onlineTM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 1)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}

	TM_spl packet;
	int error = get_online_packet((TM_struct_types)cmd.data[0], &packet);

	if (error == -1)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
		return;
	}
	else if (error == 0)
	{
		byte rawPacket[SIZE_TXFRAME];
		int size;
		error = encode_TMpacket(rawPacket, &size, packet);
		if (error)
		{
			*type = ACK_SYSTEM;
			*err = ERR_SPL;
			return;
		}
		error = TRX_sendFrame(rawPacket, (uint8_t)size);
		if (error == 0)
		{
			*type = ACK_NOTHING;
			*err = ERR_SUCCESS;
		}
		else if (error == 3)
		{
			*type = ACK_TRXVU;
			*err = ERR_MUTE;
		}
		else if (error == 4)
		{
			*type = ACK_TRXVU;
			*err = ERR_OFF;
		}
		else if (error == 5)
		{
			*type = ACK_TRXVU;
			*err = ERR_TRANSPONDER;
		}
		else if (error == -2)
		{
			*type = ACK_FREERTOS;
			*err = ERR_SEMAPHORE;
		}
		else
		{
			*type = ACK_TRXVU;
			*err = ERR_FAIL;
		}
	}
	*type = ACK_SYSTEM;
	*err = ERR_FAIL;
}
void cmd_reset_off_line(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 0)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}

	reset_offline_TM_list();

	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
}
void cmd_add_item_off_line(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 9)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	TM_struct_types index = (TM_struct_types)cmd.data[0];
	uint period;
	memcpy(&period, cmd.data + 1, 4);
	time_unix stopTime;
	memcpy(&stopTime, cmd.data + 5, 4);

	int error = add_onlineTM_param_to_save_list(index, period, stopTime);

	if (error == 0)
	{
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}
	else if (error == -1)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
	}
	else if (error == -3)
	{
		*type = ACK_LIST;
		*err = ERR_FULL;
	}
}
void cmd_delete_item_off_line(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 1)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}

	TM_struct_types index = (TM_struct_types)cmd.data[0];

	int error = delete_onlineTM_param_from_offline(index);

	if (error == 0)
	{
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}
	else
	{
		*type = ACK_LIST;
		*err = ERR_NOT_EXIST;
	}
}
void cmd_get_off_line_setting(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 0)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}

	TM_spl packet;
	int error = get_offlineSettingPacket(&packet);

	if (error == 0)
	{
		byte rawPacket[SIZE_TXFRAME];
		int size = 0;
		encode_TMpacket(rawPacket, &size, packet);
		error = TRX_sendFrame(rawPacket, (uint8_t)size);
		if (error == 0)
		{
			*type = ACK_NOTHING;
			*err = ERR_SUCCESS;
		}
		else if (error == 3)
		{
			*type = ACK_TRXVU;
			*err = ERR_MUTE;
		}
		else if (error == 4)
		{
			*type = ACK_TRXVU;
			*err = ERR_OFF;
		}
		else if (error == 5)
		{
			*type = ACK_TRXVU;
			*err = ERR_TRANSPONDER;
		}
		else if (error == -2)
		{
			*type = ACK_FREERTOS;
			*err = ERR_SEMAPHORE;
		}
		else
		{
			*type = ACK_TRXVU;
			*err = ERR_FAIL;
		}
	}
	else
	{
		*type = ACK_FRAM;
		*err = ERR_READ_FAIL;
	}
}
