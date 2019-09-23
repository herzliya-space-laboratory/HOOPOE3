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

#include "../../Global/logger.h"

void cmd_reset_delayed_command_list(Ack_type* type, ERR_type* err)
{
	reset_delayCommand(FALSE);
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
			//todo:
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
