/*
 * COMM_CMD.c
 *
 *  Created on: Jun 22, 2019
 *      Author: Hoopoe3n
 */
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <hal/Storage/FRAM.h>

#include "COMM_CMD.h"
#include "../../TRXVU.h"
#include "../../COMM/APRS.h"


#define create_task(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask) xTaskCreate( (pvTaskCode) , (pcName) , (usStackDepth) , (pvParameters), (uxPriority), (pxCreatedTask) ); vTaskDelay(10);

void cmd_mute(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	//1. send ACK before mutes satellite
	*type = ACK_NOTHING;
	if (cmd.length != 2)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	//2. mute satellite
	unsigned short param = 	BigEnE_raw_to_uShort(cmd.data);

	int error = set_mute_time(param);
	if (error == 666)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
	}
	else if (error != 0)
	{
		*type = ACK_FRAM;
		*err = ERR_WRITE_FAIL;
	}

	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
}
void cmd_unmute(Ack_type* type, ERR_type* err)
{
	int error = set_mute_time(0);
	if (error == 0)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_FAIL;
	}
	else if (error != 0)
	{
		*type = ACK_FRAM;
		*err = ERR_WRITE_FAIL;
	}
	unmute_Tx();
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
}
void cmd_active_trans(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 1)
	{
		*type = ACK_CMD_FAIL;
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
	create_task(Transponder_task, (const signed char * const)"Transponder_Task", 1024, &raw, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, xTransponderHandle);
	//no ACK
}
void cmd_shut_trans(Ack_type* type, ERR_type* err)
{
	//1. shut down the transponder and returning the TRAX to regular transmitting
	int error = sendRequestToStop_transponder();
	if (error == 1)
	{
		*type = ACK_SYSTEM;
		*err = ERR_NOT_RUNNING;
	}
	else if (error == 2)
	{
		*type = ACK_QUEUE;
		*err = ERR_FULL;
	}
	else
	{
		*type = ACK_NOTHING;
		*type = ERR_SUCCESS;
	}
}
void cmd_change_trans_rssi(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 2)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}

	unsigned short param = cmd.data[1];
	param += cmd.data[0] << 8;
	if (param > MAX_TRANS_RSSI)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
		return;
	}

	change_trans_RSSI(cmd.data);
	int error = FRAM_write_exte(cmd.data, TRANSPONDER_RSSI_ADDR, 2);
	check_int("cmd_change_trans_rssi, FRAM_write", error);
	if (error == 0)
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
void cmd_aprs_dump(Ack_type* type, ERR_type* err)
{
	*type = ACK_NOTHING;
	*err = ERR_EMPTY;
}
void cmd_stop_dump(Ack_type* type, ERR_type* err)
{
	//1. stop dump
	int error = sendRequestToStop_dump();
	if (error == 1)
	{
		*type = ACK_SYSTEM;
		*err = ERR_NOT_RUNNING;
	}
	else if (error == 2)
	{
		*type = ACK_QUEUE;
		*err = ERR_FULL;
	}
	else
	{
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}
}
void cmd_time_frequency(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 1)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	//1. check if parameter in range
	if (cmd.data[0] < MIN_TIME_DELAY_BEACON || cmd.data[0] > MAX_TIME_DELAY_BEACON)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
	}
	//2. update time in FRAM
	else if (FRAM_writeAndVerify_exte(&cmd.data[0], BEACON_TIME_ADDR, 1))
	{
		*type = ACK_FRAM;
		*err = ERR_WRITE_FAIL;
	}
	else
	{
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}
}
void cmd_change_def_bit_rate(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 1)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}

	ISIStrxvuBitrate newParam;
	Boolean validValue = FALSE;
	for (uint8_t i = 1; i < 9; i *= 2)
	{
		if (cmd.data[0] == i)
		{
			newParam = (ISIStrxvuBitrate)cmd.data[0];
			validValue = TRUE;
			break;
		}
	}

	if (validValue)
	{
		int error = FRAM_write_exte(cmd.data, BIT_RATE_ADDR, 1);
		check_int("FRAM_write, cmd_change_def_bit_rate", error);
		if (error)
		{
			*type = ACK_FRAM;
			*err = ERR_WRITE_FAIL;
			return;
		}
		error = IsisTrxvu_tcSetAx25Bitrate(0, newParam);
		check_int("IsisTrxvu_tcSetAx25Bitrate, cmd_change_def_bit_rate", error);
		if (error)
		{
			*type = ACK_TRXVU;
			*err = ERR_DRIVER;
			return;
		}
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}
	else
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
	}
}
