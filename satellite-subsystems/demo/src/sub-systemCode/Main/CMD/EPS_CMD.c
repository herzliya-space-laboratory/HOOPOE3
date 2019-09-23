/*
 * EPS_CMD.c
 *
 *  Created on: Jun 13, 2019
 *      Author: Hoopoe3n
 */
#include <freertos/FreeRTOS.h>
#include <hal/errors.h>
#include <hal/Storage/FRAM.h>

#include "EPS_CMD.h"

#include "../../Global/logger.h"

void cmd_upload_volt_logic(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 8 * 2)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	//convert raw logic to voltage_t[8]
	voltage_t eps_logic[2][NUM_BATTERY_MODE - 1];
	voltage_t comm_vol[2];
	for(int i = 0; i < 2; i++)
	{
		for (int l = 0; l < NUM_BATTERY_MODE - 1; l++)
		{
			eps_logic[i][l] = BigEnE_raw_to_uShort(cmd.data + i*6 + l*2);
		}
	}
	for(int i = 0; i < 2; i++)
	{
		comm_vol[i] = BigEnE_raw_to_uShort(cmd.data + i*2 + 12);
	}

	// check logic
	if (!check_EPSTableCorrection(eps_logic))
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
		return;
	}
	for (int i = 0; i < 2; i++)
	{
		if (eps_logic[1][0] < comm_vol[i] && comm_vol[i] < eps_logic[1][1])
		{
			*type = ACK_CMD_FAIL;
			*err = ERR_PARAMETERS;
			return;
		}
	}

	int FRAM_err = FRAM_writeAndVerify_exte((byte*)eps_logic, EPS_VOLTAGES_ADDR, 12);
	check_int("cmd_upload_volt_logic, FRAM_writeAndVerify_exte(EPS_VOLTAGES_ADDR)", FRAM_err);
	if (FRAM_err)
	{
		*type = ACK_FRAM;
		*err = ERR_WRITE_FAIL;
		return;
	}

	FRAM_err = FRAM_writeAndVerify_exte((byte*)comm_vol, BEACON_LOW_BATTERY_STATE_ADDR, 2);
	check_int("cmd_upload_volt_logic, FRAM_writeAndVerify_exte(BEACON_LOW_BATTERY_STATE_ADDR)", FRAM_err);
	if (FRAM_err)
	{
		reset_EPS_voltages();
		*type = ACK_FRAM;
		*err = ERR_WRITE_FAIL;
		return;
	}

	byte raw[2];
	raw[0] = (byte)(comm_vol[1]);
	raw[1] = (byte)(comm_vol[1] << 8);
	FRAM_err = FRAM_writeAndVerify_exte(raw, TRANS_LOW_BATTERY_STATE_ADDR, 2);
	check_int("cmd_upload_volt_logic, FRAM_writeAndVerify_exte(TRANS_LOW_BATTERY_STATE_ADDR)", FRAM_err);
	if (FRAM_err)
	{
		reset_EPS_voltages();
		*type = ACK_FRAM;
		*err = ERR_WRITE_FAIL;
		return;
	}

	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
}
void cmd_upload_volt_COMM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 2 * 2)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	voltage_t comm_vol[2];
	voltage_t eps_logic[2][NUM_BATTERY_MODE - 1];

	for (int i = 0; i < 2; i++)
	{
		comm_vol[i] = BigEnE_raw_to_uShort(cmd.data + i*2);
	}

	int i_error = FRAM_read_exte((byte*)eps_logic, EPS_VOLTAGES_ADDR, 12);
	check_int("cmd_upload_volt_COMM, FRAM_write_exte(EPS_VOLTAGES_ADDR)", i_error);
	for (int i = 0; i < 2; i++)
	{
		if (eps_logic[1][0] < comm_vol[i] && comm_vol[i] < eps_logic[1][1])
		{
			*type = ACK_CMD_FAIL;
			*err = ERR_PARAMETERS;
			return;
		}
	}

	i_error = FRAM_write_exte((byte*)comm_vol, BEACON_LOW_BATTERY_STATE_ADDR, 2);
	check_int("cmd_upload_volt_COMM, FRAM_write_exte(BEACON_LOW_BATTERY_STATE_ADDR)", i_error);
	voltage_t volll = comm_vol[1];
	i_error = FRAM_write_exte((byte*)&volll, TRANS_LOW_BATTERY_STATE_ADDR, 2);
	check_int("cmd_upload_volt_COMM, FRAM_write_exte(BEACON_LOW_BATTERY_STATE_ADDR)", i_error);
}
void cmd_heater_temp(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	// 1. define type for later ACK
	if (cmd.length != 2)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
		return;
	}
	// 2. sets values in eps_config_t
    eps_config_t config_data;
    config_data.fields.battheater_low = cmd.data[0];
    config_data.fields.battheater_high = cmd.data[1];
    // 3. sets the new values in the EPS beef(controller)
    int error = GomEpsConfigSet(I2C_BUS_ADDR, &config_data);
    // 4. check for errors
    switch (error)
    {
		case E_NO_SS_ERR:
			*type = ACK_NOTHING;
			*err = ERR_SUCCESS;
			break;
		case E_NOT_INITIALIZED:
			*type = ACK_EPS;
			*err = ERR_NOT_INITIALIZED;
			break;
		default:
			*type = ACK_EPS;
			*err = ERR_DRIVER;
			break;
	}
}
void cmd_SHUT_CAM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 0)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
	shut_CAM(SWITCH_ON);
}
void cmd_SHUT_ADCS(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 0)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
	shut_ADCS(SWITCH_ON);
}
void cmd_allow_CAM(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 0)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
	shut_CAM(SWITCH_OFF);
}
void cmd_allow_ADCS(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 0)
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}
	*type = ACK_NOTHING;
	*err = ERR_SUCCESS;
	shut_ADCS(SWITCH_OFF);
}
void cmd_update_alpha(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != sizeof(float))
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_LENGTH;
		return;
	}

	float alpha;
	memcpy(&alpha, cmd.data, 4);
	if (CHECK_EPS_ALPHA_VALUE(alpha))
	{
		int error = FRAM_write_exte((byte*)&alpha, EPS_ALPHA_ADDR, 4);
		if (error)
		{
			*type = ACK_FRAM;
			*err = ERR_WRITE_FAIL;
			return;
		}
		check_int("cmd_update_alpha, FRAM_write_exte(EPS_ALPHA_ADDR)", error);
		*type = ACK_NOTHING;
		*err = ERR_SUCCESS;
	}
	else
	{
		*type = ACK_CMD_FAIL;
		*err = ERR_PARAMETERS;
	}
}
