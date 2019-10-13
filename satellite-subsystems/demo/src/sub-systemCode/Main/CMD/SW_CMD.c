/*
 * SW_CMD.c
 *
 *  Created on: Jun 22, 2019
 *      Author: Hoopoe3n
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "SW_CMD.h"

#include "../../TRXVU.h"
#include "../../EPS.h"
#include "../../Main/Main_Init.h"
#include "../../COMM/APRS.h"
#include "../../COMM/DelayedCommand_list.h"
#include "../../Ants.h"

#include "../../payload/Request Management/CameraManeger.h"

#include "../../Global/logger.h"

void cmd_reset_delayed_command_list(Ack_type* type, ERR_type* err)
{
	reset_delayCommand();
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
}
void cmd_reset_APRS_list(Ack_type* type, ERR_type* err)
{
	reset_APRS_list(FALSE);
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
}
void cmd_reset_FRAM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 1)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
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
		case Ants:
			reset_FRAM_ants();
			break;
		case ADCS:
			break;
		case OBC:
			reset_FRAM_MAIN();
			break;
		case CAMMERA:
			send_request_to_reset_database();
			break;
		case everything:
			reset_FRAM_MAIN();
			reset_EPS_voltages();
			reset_FRAM_TRXVU();
			reset_FRAM_ants();
			//reset ADCS, cammera
			//grecful reset
			break;
		default:
			*type = ACK_CMD_FAIL;
			*err = ERR_PARAMETERS;
			break;
	}
}

void cmd_FRAM_read(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 5)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
	}

	unsigned int read_size = (unsigned int)cmd.data[0];
	unsigned int address;
	memcpy(&address, cmd.data + 1, 4);
	if (read_size > SIZE_TXFRAME - SPL_TM_HEADER_SIZE || address + read_size > FRAM_MAX_ADDRESS)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
	}

	TM_spl packet;
	packet.length = (unsigned short)(SIZE_TXFRAME - SPL_TM_HEADER_SIZE);
	packet.type = SOFTWARE_T;
	packet.subType = FRAM_READ_ST;
	Time_getUnixEpoch(&packet.time);
	memset(packet.data, 0, SIZE_TXFRAME - SPL_TM_HEADER_SIZE);
	int error = FRAM_read_exte(packet.data, address, read_size);
	if (error != 0)
	{
		*type = ACK_FRAM;
		*err = ERR_READ_FAIL;
	}

	byte raw[SIZE_TXFRAME];
	int raw_size;
	encode_TMpacket(raw, &raw_size, packet);
	error = TRX_sendFrame(raw, (uint8_t)raw_size);
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
void cmd_FRAM_write(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length > 5)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
	}

	unsigned int write_size = (unsigned int)cmd.data[0];
	unsigned int address;
	memcpy(&address, cmd.data + 1, 4);
	if (write_size > SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE || address + write_size > FRAM_MAX_ADDRESS)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
	}

	TM_spl packet;
	int error = FRAM_write_exte(packet.data, address, write_size);
	if (error != 0)
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
