/*
 * commands.c
 *
 *  Created on: Dec 5, 2018
 *      Author: elain
 */
#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <at91/utility/exithandler.h>
#include <at91/commons.h>
#include <at91/utility/trace.h>
#include <at91/peripherals/cp15/cp15.h>

#include <hal/Utility/util.h>
#include <hal/Timing/WatchDogTimer.h>
#include <hal/Timing/Time.h>
#include <hal/Drivers/I2C.h>
#include <hal/Drivers/LED.h>
#include <hal/boolean.h>
#include <hal/errors.h>

#include <hal/Storage/FRAM.h>

#include <string.h>

#include "commands.h"

#include "CMD/EPS_CMD.h"

#include "../COMM/splTypes.h"
#include "../COMM/DelayedCommand_list.h"
#include "../Global/Global.h"
#include "../TRXVU.h"
#include "../Ants.h"
#include "../ADCS.h"
#include "HouseKeeping.h"
#include "../EPS.h"
#include "../payload/CameraManeger.h"
#include "../payload/DataBase.h"

#define create_task(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask) xTaskCreate( (pvTaskCode) , (pcName) , (usStackDepth) , (pvParameters), (uxPriority), (pxCreatedTask) ); vTaskDelay(10);

/*TODO:
 * 1. finish all commands function
 * 2. remove comments from AUC to execute commands
 * 3. change the way commands paa throw tasks
 */

xSemaphoreHandle xCTE = NULL;

TC_spl command_to_execute[COMMAND_LIST_SIZE];
int place_in_list = 0;


void copy_command(TC_spl source, TC_spl* to)
{
	// copy command from source to to
	to->id = source.id;
	to->type = source.type;
	to->subType = source.subType;
	to->length = source.length;
	to->time = source.time;
	copyDATA(to->data, source.data, (int)source.length);
}
void reset_command(TC_spl *command)
{
	// set to 0 all parameters of command
	memset((void*)command, 0, SIZE_OF_COMMAND);
}

//todo: change name to more sugnifficent
int init_command()
{
	if (xCTE != NULL)
		return 1;
	// 1. create semaphore
	vSemaphoreCreateBinary(xCTE);
	// 2. set place in list to 0
	place_in_list = 0;
	// 3. allocate memory for data in command
	int i;
	for (i = 0; i < COMMAND_LIST_SIZE; i++)
	{
		reset_command(&command_to_execute[i]);
	}
	return 0;
}
int add_command(TC_spl command)
{
	portBASE_TYPE error;
	// 1. try to take semaphore
	if (xSemaphoreTake(xCTE, MAX_DELAY) == pdTRUE)
	{
		// 2. if queue full
		if (place_in_list == COMMAND_LIST_SIZE)
		{
			error = xSemaphoreGive(xCTE);
			check_portBASE_TYPE("could not return xCTE in add_command", error);
			return 1;
		}
		// 3. copy command to list
		copy_command(command, &command_to_execute[place_in_list]);
		// 4. moving forward current place in command queue
		place_in_list++;
		// 5. return semaphore
		error = xSemaphoreGive(xCTE);
		check_portBASE_TYPE("cold not return xCTE in add_command", error);
	}

	return 0;
}
int get_command(TC_spl* command)
{
	portBASE_TYPE error;
	// 1. try to take semaphore
	if (xSemaphoreTake(xCTE, MAX_DELAY) == pdTRUE)
	{
		// 2. check if there's commands in list
		if (place_in_list == 0)
		{
			error = xSemaphoreGive(xCTE);
			check_portBASE_TYPE("get_command, xSemaphoreGive(xCTE)", error);
			return 1;
		}
		// 3. copy first in command to TC_spl* command
		copy_command(command_to_execute[0], command);
		int i;
		// 4. move every command forward in queue
		for (i = 0; i < place_in_list - 1; i++)
		{
			reset_command(&command_to_execute[i]);
			copy_command(command_to_execute[i + 1], &command_to_execute[i]);
		}
		// 5. reset the last place for command
		reset_command(&command_to_execute[i]);
		place_in_list--;
		// 6. return semaphore
		error = xSemaphoreGive(xCTE);
		check_portBASE_TYPE("get_command, xSemaphoreGive(xCTE)", error);
	}
	return 0;
}
int check_number_commands()
{
	return place_in_list;
}

void act_upon_command(TC_spl decode)
{
	//later use in ACK
	switch (decode.type)
	{
	case (COMM_T):
		AUC_COMM(decode);
		break;
	case (GENERAL_T):
		AUC_general(decode);
		break;
	case (PAYLOAD_T):
		AUC_payload(decode);
		break;
	case (EPS_T):
		AUC_EPS(decode);
		break;
	case (TC_ADCS_T):
		AUC_ADCS(decode);
		break;
	case (GENERALLY_SPEAKING_T):
		AUC_GS(decode);
		break;
	case (SOFTWARE_T):
		AUC_SW(decode);
		break;
	case (SPECIAL_OPERATIONS_T):
		AUC_special_operation(decode);
		break;
	default:
		printf("wrong type: %d\n", decode.type);
		break;
	}
}


void AUC_COMM(TC_spl decode)
{
	Ack_type type;
	ERR_type err;
	switch (decode.subType)
	{
	case (MUTE_ST):
		cmd_mute(&type, &err, decode);
		break;
	case (UNMUTE_ST):
		cmd_unmute(&type, &err);
		break;
	case (ACTIVATE_TRANS_ST):
		cmd_active_trans(&type, &err, decode);
		return;
		break;
	case (SHUT_TRANS_ST):
		cmd_shut_trans(&type, &err);
		break;
	case (CHANGE_TRANS_RSSI_ST):
		cmd_change_trans_rssi(&type, &err, decode);
		break;
	case (APRS_DUMP_ST):
		cmd_aprs_dump(&type, &err);
		break;
	case (STOP_DUMP_ST):
	   cmd_stop_dump(&type, &err);
		break;
	case (TIME_FREQUENCY_ST):
		cmd_time_frequency(&type, &err, decode);
		break;
	default:
		cmd_error(&type, &err);
		break;
	}
	//Builds ACK
#ifndef NOT_USE_ACK_HK
	save_ACK(type, err, decode.id);
#endif
}

void AUC_general(TC_spl decode)
{
	Ack_type type;
	ERR_type err;

	switch (decode.subType)
	{
	case (SOFT_RESET_ST):
		cmd_soft_reset_cmponent(&type, &err, decode);
		break;
	case (HARD_RESET_ST):
		cmd_hard_reset_cmponent(&type, &err, decode);
		break;
	case (RESET_SAT_ST):
		cmd_reset_satellite(&type, &err);
		break;
	case (GRACEFUL_RESET_ST):
		cmd_gracefull_reset_satellite(&type, &err);
		break;
	case (UPLOAD_TIME_ST):
		cmd_upload_time(&type, &err, decode);
		break;
	default:
		cmd_error(&type, &err);
		break;
	}
	//Builds ACK
#ifndef NOT_USE_ACK_HK
	save_ACK(type, err, decode.id);
#endif
}

void AUC_payload(TC_spl decode)
{
	Ack_type type;
	ERR_type err;

	switch (decode.subType)
	{
	case (SEND_PIC_CHUNCK_ST):
		break;
	case (UPDATE_STN_PARAM_ST):
		//cmd_update_photography_values(NULL, NULL, decode);
		return;
		break;
	case GET_IMG_DATA_BASE_ST:
		break;
	case RESET_DATA_BASE_ST:
		break;
	case DELETE_PIC_ST:
		//cmd_delete_picture(NULL, NULL, decode);
		return;
		break;
	case UPD_DEF_DUR_ST:
		break;
	case OFF_CAM_ST:
		break;
	case ON_CAM_ST:
		break;
	case MOV_IMG_CAM_OBS_ST:
		//cmd_transfer_image_to_OBC(NULL, NULL, decode);
		return;
		break;
	case TAKE_IMG_DEF_VAL_ST:
		//cmd_take_picture_defult_values(NULL, NULL, decode);
		return;
		break;
	case TAKE_IMG_ST:
		break;
	default:
		cmd_error(&type, &err);
		break;
	}
	//Builds ACK
#ifndef NOT_USE_ACK_HK
	save_ACK(type, err, decode.id);
#endif
}

void AUC_EPS(TC_spl decode)
{
	Ack_type type;
	ERR_type err;

	switch (decode.subType)
	{
	case (UPD_LOGIC_VOLT_ST):
		cmd_upload_volt_logic(&type, &err, decode);
		break;
	case (CHANGE_HEATER_TMP_ST):
		cmd_heater_temp(&type, &err, decode);
		break;
	case (UPD_COMM_VOLTAGE):
		cmd_upload_volt_COMM(&type, &err, decode);
		break;
	case (SHUT_ADCS_ST):
		cmd_SHUT_ADCS(&type, &err, decode);
		break;
	case (SHUT_CAM_ST):
		cmd_SHUT_CAM(&type, &err, decode);
		break;
	default:
		cmd_error(&type, &err);
		break;
	}
	//Builds ACK
#ifndef NOT_USE_ACK_HK
	save_ACK(type, err, decode.id);
#endif
}

void AUC_ADCS(TC_spl decode)
{
	Ack_type type = ACK_ADCS_TEST;
		ERR_type err;
		int SubType = decode.subType - startIndex;

		if (decode.length != (ADCS_cmd[SubType]).Length)
		{
			*err = ERR_PARAMETERS;
			#ifndef NOT_USE_ACK_HK
			save_ACK(type, err, decode.id);
			#endif
			return;
		}

		long error = xQueueSend(AdcsCmdTour, &decode, MAX_DELAY);

		if (error != pdTRUE)
			save_ACK(type, ERR_FAIL, decode.id);

}

void AUC_GS(TC_spl decode)
{
	Ack_type type;
	ERR_type err;

	switch (decode.subType)
	{
	case (GENERIC_I2C_ST):
		cmd_generic_I2C(&type, &err, decode);
		break;
	case (DUMP_ST):
		cmd_dump(decode);
		return;
		break;
	case (DELETE_PACKETS_ST):
		cmd_delete_TM(&type, &err, decode);
		break;
	case (DELETE_FILE_ST):
		break;
	case (RESTSRT_FS_ST):
		break;
	case (DUMMY_FUNC_ST):
		cmd_dummy_func(&type, &err);
		break;
	case (REDEPLOY):

		break;
	case (ARM_DISARM):
		cmd_ARM_DIARM(&type, &err, decode);
		break;
	default:
		cmd_error(&type, &err);
		break;
	}
	//Builds ACK
#ifndef NOT_USE_ACK_HK
	save_ACK(type, err, decode.id);
#endif
}

void AUC_SW(TC_spl decode)
{
	Ack_type type;
	ERR_type err;

	switch (decode.subType)
	{
	case (RESET_APRS_LIST_ST):
		cmd_reset_APRS_list(&type, &err);
		break;
	case (RESET_DELAYED_CM_LIST_ST):
		cmd_reset_delayed_command_list(&type, &err);
		break;
	case (RESET_FRAM_ST):
		cmd_reset_FRAM(&type, &err, decode);
		break;
	default:
		cmd_error(&type, &err);
		break;
	}
	//Builds ACK
#ifndef NOT_USE_ACK_HK
	save_ACK(type, err, decode.id);
#endif
}

void AUC_special_operation(TC_spl decode)
{
	Ack_type type;
	ERR_type err;

	switch (decode.subType)
	{
	case DELETE_UNF_CUF_ST:
		break;
	case UPLOAD_CUF_ST:
		break;
	case CON_UNF_CUF_ST:
		break;
	case PAUSE_UP_CUF_ST:
		break;
	case EXECUTE_CUF:
		break;
	case REVERT_CUF:
		break;
	default:
		cmd_error(&type, &err);
		break;
	}
	//Builds ACK
#ifndef NOT_USE_ACK_HK
	save_ACK(type, err, decode.id);
#endif
}

void cmd_error(Ack_type* type, ERR_type* err)
{
	*type = ACK_NOTHING;
	*err = ERR_FAIL;
}
void cmd_mute(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	//1. send ACK before mutes satellite
	*type = ACK_MUTE;
	*err = ERR_ACTIVE;
	if (cmd.length != 2)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	//2. mute satellite
	unsigned short param = 	BigEnE_raw_to_uShort(cmd.data);

	int error = set_mute_time(param);
	if (error == 666)
	{
		*err = ERR_PARAMETERS;
	}
	else if (error != 0)
	{
		*err = ERR_FAIL;
	}
}
void cmd_unmute(Ack_type* type, ERR_type* err)
{
	*type = ACK_UNMUTE;
	//1. unmute satellite
	*err = ERR_FRAM_WRITE_FAIL;
	set_mute_time(0);
	mute_Tx(FALSE);
	*err = ERR_SUCCESS;
}
void cmd_active_trans(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_TRANSPONDER;
	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	//1. checks if the transponder is active
	byte raw[1 + 4];
	// 2.1. convert command id to raw
	BigEnE_uInt_to_raw(cmd.id, &raw[0]);
	// 2.2. copying data to raw
	raw[4] = cmd.data[0];
	// 3. activate transponder
	create_task(Transponder_task, (const signed char * const)"Transponder_Task", 1024, &raw, FIRST_PRIORITY, xTransponderHandle);
	//no ACK
}
void cmd_shut_trans(Ack_type* type, ERR_type* err)
{
	*type = ACK_TRANSPONDER;
	//1. shut down the transponder and returning the TRAX to regular transmitting
	stop_transponder();
	*err = ERR_TURNED_OFF;
}
void cmd_change_trans_rssi(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_UPDATE_TRANS_RSSI;
	if (cmd.length != 2)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	unsigned short param = cmd.data[1];
	param += cmd.data[0] << 8;
	if (MIN_TRANS_RSSI > param || param > MAX_TRANS_RSSI)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	*err = ERR_SUCCESS;
	change_trans_RSSI(cmd.data);
}
void cmd_aprs_dump(Ack_type* type, ERR_type* err)
{
	*type = ACK_DUMP;

	*err = ERR_NO_DATA;
	//1. send all APRS packets on satellite
	if (send_APRS_Dump())
	{
	//no ACK

	*err=ERR_SUCCESS;
	}
}
void cmd_stop_dump(Ack_type* type, ERR_type* err)
{
	*type = ACK_DUMP;
	//1. stop dump
	stop_dump();
	*err = ERR_TURNED_OFF;
}
void cmd_time_frequency(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_UPDATE_BEACON_TIME_DELAY;
	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	//1. check if parameter in range
	if (cmd.data[0] < MIN_TIME_DELAY_BEACON || cmd.data[0] > MAX_TIME_DELAY_BEACON)
	{
		*err = ERR_PARAMETERS;
	}
	//2. update time in FRAM
	else if (!FRAM_writeAndVerify(&cmd.data[0], BEACON_TIME_ADDR, 1))
	{
		*err = ERR_FRAM_WRITE_FAIL;
	}
	else
	{
		*err = ERR_SUCCESS;
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
	copyDATA(raw + 4, cmd.data, 2 * TIME_SIZE + 5);
	create_task(Dump_task, (const signed char * const)"Dump_Task", (unsigned short)(STACK_DUMP_SIZE), (void*)raw, FIRST_PRIORITY, xDumpHandle);
}
void cmd_image_dump(TC_spl cmd)
{
	if (cmd.length != 10)
	{
		return;
	}
	unsigned char raw[4 + 4 + 2] = {0};
	// 1.1. copying command id
	BigEnE_uInt_to_raw(cmd.id, &raw[0]);
	// 1.2. copying command data
	copyDATA(raw + 4, cmd.data, 4 + 2 + 1 + 1);
	create_task(Dump_image_task, (const signed char * const)"Dump_image_Task", (unsigned short)(STACK_DUMP_SIZE), (void*)raw, FIRST_PRIORITY, xDumpHandle);
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
void cmd_reset_delayed_command_list(Ack_type* type, ERR_type* err)
{
	*type = ACK_RESET_DELAYED_CMD;
	reset_delayCommand(FALSE);
	*err = ERR_SUCCESS;
}
void cmd_reset_APRS_list(Ack_type* type, ERR_type* err)
{
	*type = ACK_RESET_APRS_LIST;
	reset_APRS_list(FALSE);
	*err = ERR_SUCCESS;
}
void cmd_reset_FRAM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_FRAM_RESET;
	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	*err = ERR_SUCCESS;
	subSystem_indx sub = (subSystem_indx)cmd.data[0];
	switch (sub)
	{
		case EPS:
			reset_FRAM_EPS();
			break;
		case TRXVU:
			reset_FRAM_TRXVU();
			break;
		case ADCS:
			break;
		case OBC:
			//todo
			//reset_FRAM_MAIN();
			break;
		case CAMMERA:
			break;
		case everything:
			reset_FRAM_MAIN();
			reset_EPS_voltages();
			reset_FRAM_TRXVU();
			//reset ADCS, cammera
			//grecful reset
			break;
		default:
			*err = ERR_PARAMETERS;
			break;
	}
}
//ADCS
void cmd_ADCS_Set_Boot_Index(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_SET_BOOT_LODER;
	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	int error = 0;

	if (error == -35)
	{
		*err = ERR_ERROR;
	}
	if (error == -4)
	{
		*err = ERR_BOOT_LOADER_STUCK;
	}
	if (error == 4)
	{
		*err = ERR_SYSTEM_OFF;
	}
	else
	{
		*err = ERR_FAIL;
	}
}
void cmd_ADCS_Run_Boot_Index(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_RUN_BOOT_LOADER;
	if (cmd.length != 0)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	int error = (0);

	if (error == -35)
	{
		*err = ERR_ERROR;
	}
	if (error == -4)
	{
		*err = ERR_BOOT_LOADER_STUCK;
	}
	if (error == 4)
	{
		*err = ERR_SYSTEM_OFF;
	}
	else
	{
		*err = ERR_FAIL;
	}
}
void cmd_ADCS_TLE_Inclination(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;
	if (cmd.length != DOUBLE_SIZE)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	BigEnE_raw_value(cmd.data, DOUBLE_SIZE);

	int error = 0;

	if (error == -35)
	{
		*err = ERR_ERROR;
	}
	if (error == -4)
	{
		*err = ERR_BOOT_LOADER_STUCK;
	}
	if (error == 4)
	{
		*err = ERR_SYSTEM_OFF;
	}
	else
	{
		*err = ERR_FAIL;
	}
}
void cmd_ADCS_Cashe_enable(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_CONFIG;
	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	int error = 0;

	if (error == -35)
	{
		*err = ERR_ERROR;
	}
	if (error == -4)
	{
		*err = ERR_BOOT_LOADER_STUCK;
	}
	if (error == 4)
	{
		*err = ERR_SYSTEM_OFF;
	}
	else
	{
		*err = ERR_FAIL;
	}
}
void cmd_ADCS_Deploy_BOOM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_BOOM_DEPLOY;
	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}
	int error = 0;
#ifndef TESTING
	//error = cspaceADCS_deployMagBoomADCS(0,cmd.data);
#endif
	if (error == -35)
	{
		*err = ERR_ERROR;
	}
	if (error == -4)
	{
		*err = ERR_BOOT_LOADER_STUCK;
	}
	if (error == 4)
	{
		*err = ERR_SYSTEM_OFF;
	}
	else
	{
		*err = ERR_FAIL;
	}
}
void cmd_ADCS_set_SGP4(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	if (cmd.length != DOUBLE_SIZE * 8)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	for (int i = 0; i < 8 * DOUBLE_SIZE; i+= DOUBLE_SIZE)
	{
		BigEnE_raw_value(cmd.data + i, DOUBLE_SIZE);
	}

	int error = 0;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_set_SGP4_0(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	if (cmd.length != DOUBLE_SIZE)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	BigEnE_raw_value(cmd.data, DOUBLE_SIZE);

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_set_SGP4_1(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	if (cmd.length != DOUBLE_SIZE)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	BigEnE_raw_value(cmd.data, DOUBLE_SIZE);

	int error = 0;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_set_SGP4_2(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	if (cmd.length != DOUBLE_SIZE)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	BigEnE_raw_value(cmd.data, DOUBLE_SIZE);

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_set_SGP4_3(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	if (cmd.length != DOUBLE_SIZE)
	{
		*err = ERR_PARAMETERS;
	}

	BigEnE_raw_value(cmd.data, DOUBLE_SIZE);

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_set_SGP4_4(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	if (cmd.length != DOUBLE_SIZE)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	BigEnE_raw_value(cmd.data, DOUBLE_SIZE);

	int error = 0;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_set_SGP4_5(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	if (cmd.length != DOUBLE_SIZE)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	BigEnE_raw_value(cmd.data, DOUBLE_SIZE);

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_set_SGP4_6(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	if (cmd.length != DOUBLE_SIZE)
	{
		*err = ERR_PARAMETERS;
	}

	BigEnE_raw_value(cmd.data, DOUBLE_SIZE);

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_set_SGP4_7(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	if (cmd.length != DOUBLE_SIZE)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	BigEnE_raw_value(cmd.data, DOUBLE_SIZE);

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_reset_boot_reg(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_SET_BOOT_LODER;

	if (cmd.length != 0)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_save_config(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_SAVE_CONFIG;

	if (cmd.length != 0)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_save_Orbit_param(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_SAVE_CONFIG;

	if (cmd.length != 0)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_save_time(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_SAVE_CONFIG;

	if (cmd.length != 6)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	BigEnE_raw_value(cmd.data, INT_SIZE);
	BigEnE_raw_value(cmd.data + INT_SIZE, SHORT_SIZE);

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_cache_enable_state(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_SAVE_CONFIG;

	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_run_mode(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_SAVE_CONFIG;

	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_set_magnetoquer_config(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_SAVE_CONFIG;

	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
void cmd_ADCS_aet_magnetometer_mount(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	*type = ACK_ADCS_SAVE_CONFIG;

	if (cmd.length != 1)
	{
		*err = ERR_PARAMETERS;
		return;
	}

	int error;

	switch (error)
	{
		case -35:
			*err = ERR_ERROR;
			break;
		case -4:
			*err = ERR_BOOT_LOADER_STUCK;
			break;
		case 4:
			*err = ERR_SYSTEM_OFF;
			break;
		default:
			*err = ERR_FAIL;
			break;
	}
}
