/*
 *
 * EPS.c
 *
 *  Created on: 23  2018
 *      Author: I7COMPUTER
 */
#include <freertos/FreeRTOS.h>
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

#include <satellite-subsystems/IsisSolarPanelv2.h>

#include <string.h>
#include "../Global/Global.h"
#include "../Global/GlobalParam.h"
#include "../EPS.h"

#define CAM_STATE	 0x0F
#define ADCS_STATE	 0xF0

#define CALCAVARAGE3(vbatt_prev) (((vbatt_prev)[0] + (vbatt_prev)[1] + (vbatt_prev)[2])/3)
#define CALCAVARAGE2(vbatt_prev, vbatt) ((vbatt_prev + vbatt) / 2)

#define CHECK_CHANNEL_0(preState, currState) ((unsigned char)preState.fields.channel3V3_1 != currState.fields.output[0])
#define CHECK_CHANNEL_3(preState, currState) ((unsigned char)preState.fields.channel5V_1 != currState.fields.output[3])
#define CHECK_CHANNEL_CHANGE(preState, currState) CHECK_CHANNEL_0(preState, currState) || CHECK_CHANNEL_3(preState, currState)

voltage_t VBatt_previous;
float alpha = EPS_ALPHA_DEFFAULT_VALUE;
gom_eps_channelstates_t switches_states;
EPS_mode_t batteryLastMode;
EPS_enter_mode_t enterMode[NUM_BATTERY_MODE];

#define DEFULT_VALUES_VOL_TABLE	{ {6700, 7000, 7400}, {7500, 7100, 6800}}

voltage_t round_vol(voltage_t vol)
{
	int rounding_mul2 = EPS_ROUNDING_FACTOR * 2;
	if (vol % rounding_mul2 > EPS_ROUNDING_FACTOR)
	{
		return (voltage_t)((double)(vol) / rounding_mul2) * rounding_mul2 + rounding_mul2;
	}
	else
	{
		return (voltage_t)((double)(vol) / rounding_mul2) * rounding_mul2;
	}
}

Boolean8bit  get_shut_ADCS()
{
	Boolean8bit mode;
	int error = FRAM_read(&mode, SHUT_ADCS_ADDR, 1);
	check_int("shut_ADCS, FRAM_read", error);
	return mode;
}
void shut_ADCS(Boolean mode)
{
	int error = FRAM_write((byte*)&mode, SHUT_ADCS_ADDR, 1);
	check_int("shut_ADCS, FRAM_write", error);
}

Boolean8bit  get_shut_CAM()
{
	Boolean8bit mode;
	int error = FRAM_read(&mode, SHUT_CAM_ADDR, 1);
	check_int("shut_CAM, FRAM_read", error);
	return mode;
}
void shut_CAM(Boolean mode)
{
	int error = FRAM_write((byte*)&mode, SHUT_CAM_ADDR, 1);
	check_int("shut_CAM, FRAM_write", error);
}


Boolean update_powerLines(gom_eps_channelstates_t newState)
{
	gom_eps_hk_t eps_tlm;
	int i_error = GomEpsGetHkData_general(0, &eps_tlm);
	check_int("can't get gom_eps_hk_t for vBatt in EPS_Conditioning", i_error);
	if (i_error != 0)
		return FALSE;

	if (CHECK_CHANNEL_CHANGE(newState, eps_tlm))
	{
		i_error = GomEpsSetOutput(0, switches_states);
		check_int("can't set channel state in EPS_Conditioning", i_error);

		i_error = FRAM_write(&switches_states.raw, EPS_STATES_ADDR, 1);
		check_int("EPS_Conditioning, FRAM_write", i_error);

		return TRUE;
	}
	else
	{
		return FALSE;

	}
}

EPS_mode_t get_EPS_mode_t()
{
	return batteryLastMode;
}

void init_enterMode()
{
	enterMode[0].fun = EnterCriticalMode;
	enterMode[0].type = critical_mode;
	enterMode[1].fun = EnterSafeMode;
	enterMode[1].type = safe_mode;
	enterMode[2].fun = EnterCruiseMode;
	enterMode[2].type = cruise_mode;
	enterMode[3].fun = EnterFullMode;
	enterMode[3].type = full_mode;
}

void EPS_Init()
{
	unsigned char eps_i2c_address = EPS_I2C_ADDR;
	int error = GomEpsInitialize(&eps_i2c_address, 1);
	if(0 != error)
	{
		printf("error in GomEpsInitialize = %d\n",error);
	}

	error = IsisSolarPanelv2_initialize(slave0_spi);
	check_int("EPS_Init, IsisSolarPanelv2_initialize", error);
	IsisSolarPanelv2_sleep();

	voltage_t voltage_table[2][EPS_VOLTAGE_TABLE_NUM_ELEMENTS / 2] = DEFULT_VALUES_VOL_TABLE;
	error = FRAM_read((byte*)voltage_table, EPS_VOLTAGES_ADDR, EPS_VOLTAGES_SIZE_RAW);
	check_int("FRAM_read, EPS_Init", error);

	gom_eps_hk_t eps_tlm;
	error = GomEpsGetHkData_general(0, &eps_tlm);
	check_int("GomEpsGetHkData_general, EPS_init", error);
	voltage_t current_vbatt = round_vol(eps_tlm.fields.vbatt);

	init_enterMode();

	Boolean changeMode = FALSE;
	printf("EPS valtage: %u\n", current_vbatt);
	for (int i = 0; i < NUM_BATTERY_MODE - 1; i++)
	{
		if (current_vbatt < voltage_table[0][i])
		{
			enterMode[i].fun(&switches_states, &batteryLastMode);
			update_powerLines(switches_states);
			changeMode = TRUE;
		}
	}

	if (!changeMode)
	{
		enterMode[NUM_BATTERY_MODE - 1].fun(&switches_states, &batteryLastMode);
		update_powerLines(switches_states);
	}

	set_Vbatt(eps_tlm.fields.vbatt);
	VBatt_previous = current_vbatt;
}


void reset_FRAM_EPS()
{
	int i_error;

	reset_EPS_voltages();

	//reset EPS_STATES_ADDR
	byte data = 0;
	i_error = FRAM_write(&data, EPS_STATES_ADDR, 1);
	check_int("reset_FRAM_EPS, FRAM_write", i_error);
	data = SWITCH_OFF;
	i_error = FRAM_write(&data, SHUT_ADCS_ADDR, 1);
	check_int("reset_FRAM_EPS, FRAM_write", i_error);
	i_error = FRAM_write(&data, SHUT_CAM_ADDR, 1);
	check_int("reset_FRAM_EPS, FRAM_write", i_error);
	alpha = EPS_ALPHA_DEFFAULT_VALUE;
	i_error = FRAM_write((byte*)&alpha, EPS_ALPHA_ADDR, 4);
	check_int("can't FRAM_write(EPS_ALPHA_ADDR), reset_FRAM_EPS", i_error);
}

void reset_EPS_voltages()
{
	int i_error;
	//reset EPS_VOLTAGES_ADDR
	voltage_t voltages[2][EPS_VOLTAGE_TABLE_NUM_ELEMENTS / 2] = DEFULT_VALUES_VOL_TABLE;
	voltage_t comm_voltage  = 7250;

	i_error = FRAM_write((byte*)voltages, EPS_VOLTAGES_ADDR, EPS_VOLTAGES_SIZE_RAW);
	check_int("reset_FRAM_EPS, FRAM_read", i_error);

	i_error = FRAM_write((byte*)&comm_voltage, BEACON_LOW_BATTERY_STATE_ADDR, 2);
	check_int("reset_FRAM_EPS, FRAM_read", i_error);

	i_error = FRAM_write((byte*)&comm_voltage, TRANS_LOW_BATTERY_STATE_ADDR, 2);
	check_int("reset_FRAM_EPS, FRAM_read", i_error);
}


void battery_downward(voltage_t current_VBatt, voltage_t previuosVBatt)
{
	voltage_t voltage_table[2][EPS_VOLTAGE_TABLE_NUM_ELEMENTS / 2] = DEFULT_VALUES_VOL_TABLE;
	int i_error = FRAM_read((byte*)voltage_table, EPS_VOLTAGES_ADDR, EPS_VOLTAGES_SIZE_RAW);
	check_int("FRAM_read, EPS_Init", i_error);

	for (int i = 0; i < EPS_VOLTAGE_TABLE_NUM_ELEMENTS / 2; i++)
		if (current_VBatt < voltage_table[0][i])
			if (previuosVBatt > voltage_table[0][i])
				enterMode[i].fun(&switches_states, &batteryLastMode);
}

void battery_upward(voltage_t current_VBatt, voltage_t previuosVBatt)
{
	voltage_t voltage_table[2][EPS_VOLTAGE_TABLE_NUM_ELEMENTS / 2] = DEFULT_VALUES_VOL_TABLE;
	int i_error = FRAM_read((byte*)voltage_table, EPS_VOLTAGES_ADDR, EPS_VOLTAGES_SIZE_RAW);
	check_int("FRAM_read, EPS_Init", i_error);

	for (int i = 0; i < EPS_VOLTAGE_TABLE_NUM_ELEMENTS / 2; i++)
		if (current_VBatt > voltage_table[1][i])
			if (previuosVBatt < voltage_table[1][i])
				enterMode[NUM_BATTERY_MODE - 1 - i].fun(&switches_states, &batteryLastMode);
}


void EPS_Conditioning()
{
	gom_eps_hk_t eps_tlm;
	int i_error = GomEpsGetHkData_general(0, &eps_tlm);
	check_int("can't get gom_eps_hk_t for vBatt in EPS_Conditioning", i_error);
	if (i_error != 0)
		return;
	set_Vbatt(eps_tlm.fields.vbatt);

	i_error = FRAM_read((byte*)&alpha, EPS_ALPHA_ADDR, 4);
	check_int("can't FRAM_read(EPS_ALPHA_ADDR) for vBatt in EPS_Conditioning", i_error);
	if (!CHECK_EPS_ALPHA_VALUE(alpha))
	{
		alpha = EPS_ALPHA_DEFFAULT_VALUE;
	}

	voltage_t current_VBatt = round_vol(eps_tlm.fields.vbatt);
	voltage_t VBatt_filtered = (voltage_t)((float)current_VBatt * alpha + (1 - alpha) * (float)VBatt_previous);

	//printf("\nsystem Vbatt: %u,\nfiltered Vbatt: %u \npreviuos Vbatt: %u\n", eps_tlm.fields.vbatt, VBatt_filtered, VBatt_previous);
	//printf("last state: %d, channels state-> 3v3_0:%d 5v_0:%d\n\n", batteryLastMode, eps_tlm.fields.output[0], eps_tlm.fields.output[3]);

	if (VBatt_filtered < VBatt_previous)
	{
		battery_downward(VBatt_filtered, VBatt_previous);
	}
	else if (VBatt_filtered > VBatt_previous)
	{
		battery_upward(VBatt_filtered, VBatt_previous);
	}

	update_powerLines(switches_states);
	//printf("last state: %d\n", batteryLastMode);
	//printf("channels state-> 3v3_0:%d 5v_0:%d\n\n", eps_tlm.fields.output[0], eps_tlm.fields.output[3]);
	VBatt_previous = VBatt_filtered;
}


Boolean overRide_ADCS(gom_eps_channelstates_t* switches_states)
{
	if(get_shut_ADCS())
	{
		switches_states->raw = 0;
		set_system_state(ADCS_param, SWITCH_OFF);

		return FALSE;
	}

	return TRUE;
}

Boolean overRide_Camera()
{
	if(get_shut_CAM())
	{
		set_system_state(cam_operational_param, SWITCH_OFF);

		return FALSE;
	}

	return TRUE;
}

//EPS modes
void EnterFullMode(gom_eps_channelstates_t* switches_states, EPS_mode_t* mode)
{
	printf("Enter Full Mode\n");
	*mode = full_mode;
	set_system_state(Tx_param, SWITCH_ON);

	if (overRide_ADCS(switches_states))
	{
		switches_states->fields.quadbatSwitch = 0;
		switches_states->fields.quadbatHeater = 0;
		switches_states->fields.channel3V3_1 = 1;
		switches_states->fields.channel3V3_2 = 0;
		switches_states->fields.channel3V3_3 = 0;
		switches_states->fields.channel5V_1 = 1;
		switches_states->fields.channel5V_2 = 0;
		switches_states->fields.channel5V_3 = 0;

		set_system_state(ADCS_param, SWITCH_ON);
	}

	if (overRide_Camera())
		set_system_state(cam_operational_param, SWITCH_ON);
}

void EnterCruiseMode(gom_eps_channelstates_t* switches_states, EPS_mode_t* mode)
{
	printf("Enter Cruise Mode\n");
	*mode = cruise_mode;
	set_system_state(Tx_param, SWITCH_ON);

	if (overRide_ADCS(switches_states))
	{
		switches_states->fields.quadbatSwitch = 0;
		switches_states->fields.quadbatHeater = 0;
		switches_states->fields.channel3V3_1 = 1;
		switches_states->fields.channel3V3_2 = 0;
		switches_states->fields.channel3V3_3 = 0;
		switches_states->fields.channel5V_1 = 1;
		switches_states->fields.channel5V_2 = 0;
		switches_states->fields.channel5V_3 = 0;

		set_system_state(ADCS_param, SWITCH_ON);
	}

	set_system_state(cam_operational_param, SWITCH_OFF);
}

void EnterSafeMode(gom_eps_channelstates_t* switches_states, EPS_mode_t* mode)
{
	printf("Enter Safe Mode\n");
	*mode = safe_mode;
	set_system_state(Tx_param, SWITCH_OFF);

	if (overRide_ADCS(switches_states))
	{
		switches_states->fields.quadbatSwitch = 0;
		switches_states->fields.quadbatHeater = 0;
		switches_states->fields.channel3V3_1 = 1;
		switches_states->fields.channel3V3_2 = 0;
		switches_states->fields.channel3V3_3 = 0;
		switches_states->fields.channel5V_1 = 1;
		switches_states->fields.channel5V_2 = 0;
		switches_states->fields.channel5V_3 = 0;

		set_system_state(ADCS_param, SWITCH_ON);
	}

	set_system_state(cam_operational_param, SWITCH_OFF);
}

void EnterCriticalMode(gom_eps_channelstates_t* switches_states, EPS_mode_t* mode)
{
	printf("Enter Critical Mode\n");
	*mode = critical_mode;
	switches_states->fields.quadbatSwitch = 0;
	switches_states->fields.quadbatHeater = 0;
	switches_states->fields.channel3V3_1 = 0;
	switches_states->fields.channel3V3_2 = 0;
	switches_states->fields.channel3V3_3 = 0;
	switches_states->fields.channel5V_1 = 0;
	switches_states->fields.channel5V_2 = 0;
	switches_states->fields.channel5V_3 = 0;

	set_system_state(cam_operational_param, SWITCH_OFF);
	set_system_state(Tx_param, SWITCH_OFF);
	set_system_state(ADCS_param, SWITCH_OFF);
}

//Write gom_eps_k_t
void WriteCurrentTelemetry(gom_eps_hk_t telemetry)
{
	/*printf("vbatt: %d\nvboost: %d, %d, %d\ncurin: %d, %d, %d\ntemp: %d, %d, %d, %d, %d, %d\ncursys: %d\ncursun: %d\nstates: %d\n",
		telemetry.fields.vbatt, telemetry.fields.vboost[0], telemetry.fields.vboost[1], telemetry.fields.vboost[2],
		telemetry.fields.curin[0], telemetry.fields.curin[1], telemetry.fields.curin[2],
		telemetry.fields.temp[0], telemetry.fields.temp[1], telemetry.fields.temp[2], telemetry.fields.temp[3], telemetry.fields.temp[4], telemetry.fields.temp[5],
		telemetry.fields.cursys, telemetry.fields.cursun, states);*/

	printf("Battery Voltage: %d mV\n", telemetry.fields.vbatt);
	printf("Boost Converters: %d, %d, %d mV\n", telemetry.fields.vboost[0], telemetry.fields.vboost[1], telemetry.fields.vboost[2]);
	printf("Currents: %d, %d, %d mA\n", telemetry.fields.curin[0], telemetry.fields.curin[1], telemetry.fields.curin[2]);
	printf("Temperatures: %d, %d, %d, %d, %d, %d C\n", telemetry.fields.temp[0], telemetry.fields.temp[1], telemetry.fields.temp[2], telemetry.fields.temp[3], telemetry.fields.temp[4], telemetry.fields.temp[5]);
	printf("Current out of Battery: %d mA\n", telemetry.fields.cursys);
	printf("Current of Sun Sensor: %d mA\n", telemetry.fields.cursun);
}

void convert_raw_voltage(byte raw[EPS_VOLTAGES_SIZE_RAW], voltage_t voltages[EPS_VOLTAGE_TABLE_NUM_ELEMENTS])
{
	int i;
	int l = 0;
	for (i = 0; i < EPS_VOLTAGE_TABLE_NUM_ELEMENTS; i++)
	{
		voltages[i] = (voltage_t)raw[l];
		l++;
		voltages[i] += (voltage_t)(raw[l] << 8);
		l++;
	}
}

Boolean check_EPSTableCorrection(voltage_t table[2][NUM_BATTERY_MODE - 1])
{
	if (table[0][0] < EPS_VOL_LOGIC_MIN || table[0][0] >= table[1][NUM_BATTERY_MODE - 2])
		return FALSE;
	if (table[1][0] > EPS_VOL_LOGIC_MAX)
		return FALSE;
	for(int i = 1; i < NUM_BATTERY_MODE - 1; i++)
	{
		if (table[0][i] <= table[1][NUM_BATTERY_MODE - 1 - i] ||
				table[0][i] >= table[1][NUM_BATTERY_MODE - 2 - i])
			return FALSE;
	}

	return TRUE;
}
