/*


 * HouseKeeping.c
 *
 *  Created on: Jan 18, 2019
 *      Author: elain
 */
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
#include <hcc/api_fat.h>

#include <hal/Storage/FRAM.h>

#include <string.h>

#include <satellite-subsystems/GomEPS.h>
#include <satellite-subsystems/SCS_Gecko/gecko_driver.h>
#include <satellite-subsystems/IsisAntS.h>
#include <satellite-subsystems/IsisTRXVU.h>
#include <satellite-subsystems/IsisSolarPanelv2.h>
#include <satellite-subsystems/cspaceADCS.h>

#include "HouseKeeping.h"

#include "sub-systemCode/ADCS/AdcsGetDataAndTlm.h"
#include "../EPS.h"
#include "../COMM/splTypes.h"
#include "../Global/Global.h"
#include "../Global/sizes.h"
#include "../Global/FRAMadress.h"
#include "../Global/GlobalParam.h"
#include "../Global/logger.h"
#include "../Global/OnlineTM.h"


int save_ACK(Ack_type type, ERR_type err, command_id ACKcommandId)
{
	FileSystemResult error;
	byte raw_ACK[ACK_DATA_LENGTH];
	build_data_field_ACK(type, err, ACKcommandId, raw_ACK);
	error = c_fileWrite(ACK_FILE_NAME, raw_ACK);
	if (error == FS_NOT_EXIST)
	{
		error = c_fileCreate(ACK_FILE_NAME, ACK_DATA_LENGTH);
		if (error != FS_SUCCSESS)
			printf("could not create ACK file, error %d", error);
		error = c_fileWrite(ACK_FILE_NAME, raw_ACK);
		if (error != FS_SUCCSESS)
			printf("could not save ACK after creating one, error %d", error);
	}
	else if (error != FS_SUCCSESS)
	{
		printf("could not save ACK, error %d", error);
		//if theres errors
	}

#ifdef TESTING
	printf("ACK///id: %u, type: %u, err: %u\n", ACKcommandId, type, err);
#endif
	return 0;
}

//update the global_params
void set_GP_EPS(EPS_HK hk_in)
{
	set_Vbatt(hk_in.fields.VBatt);
	set_curBat(hk_in.fields.Total_system_current);
	for (int i = 0; i < 4; i++)
	{
		set_tempEPS(i, hk_in.fields.temp[i]);
	}
	for (int i = 0; i < 2; i++)
	{
		set_tempBatt(i, hk_in.fields.temp[4 + i]);
	}
	set_cur3V3(hk_in.fields.currentChanel[0]);
	set_cur5V(hk_in.fields.currentChanel[3]);
}
void set_GP_COMM(ISIStrxvuRxTelemetry_revC hk_in)
{
	set_tempComm_LO(hk_in.fields.locosc_temp);
	set_tempComm_PA(hk_in.fields.pa_temp);
	set_RxDoppler(hk_in.fields.rx_doppler);
	set_RxRSSI(hk_in.fields.rx_rssi);
	ISIStrxvuTxTelemetry_revC tx_tm;
	int err = IsisTrxvu_tcGetTelemetryAll_revC(0, &tx_tm);
	check_int("set_GP_COMM, IsisTrxvu_tcGetTelemetryAll_revC", err);
	set_TxForw(tx_tm.fields.tx_fwrdpwr);
	set_TxRefl(tx_tm.fields.tx_reflpwr);
}

// collect from the drivers telemetry for each of the HK types
int SP_HK_collect(SP_HK* hk_out)
{
	if (get_system_state(cam_param))
		return -33;
	int errors[NUMBER_OF_SOLAR_PANNELS];
	int error_combine = 1;
	IsisSolarPanelv2_wakeup();
	int32_t paneltemp;
	uint8_t status = 0;
	for(int i = 0; i < NUMBER_OF_SOLAR_PANNELS; i++)
	{
		errors[i] = IsisSolarPanelv2_getTemperature(i, &paneltemp, &status);
		check_int("EPS_HK_collect, IsisSolarPanelv2_getTemperature", errors[i]);
		hk_out->fields.SP_temp[i] = paneltemp;
		error_combine &= errors[i];
	}
	IsisSolarPanelv2_sleep();
	return error_combine;
}
int EPS_HK_collect(EPS_HK* hk_out)
{
	gom_eps_hk_t gom_hk;
	int error = GomEpsGetHkData_general(0, &gom_hk);
	check_int("EPS_HK_collect, GomEpsGetHkData_param", error);

	hk_out->fields.photoVoltaic3 = gom_hk.fields.vboost[2];
	hk_out->fields.photoVoltaic2 = gom_hk.fields.vboost[1];
	hk_out->fields.photoVoltaic1 = gom_hk.fields.vboost[0];
	hk_out->fields.photoCurrent3 = gom_hk.fields.curin[2];
	hk_out->fields.photoCurrent2 = gom_hk.fields.curin[1];
	hk_out->fields.photoCurrent1 = gom_hk.fields.curin[0];
	hk_out->fields.Total_photo_current = gom_hk.fields.cursun;
	hk_out->fields.VBatt = gom_hk.fields.vbatt;
	hk_out->fields.Total_system_current = gom_hk.fields.cursys;
	int i;
	for (i = 0; i < 6; i++)
	{
		hk_out->fields.currentChanel[i] = gom_hk.fields.curout[i];
		hk_out->fields.temp[i] = gom_hk.fields.temp[i];
	}
	hk_out->fields.Cause_of_last_reset = gom_hk.fields.bootcause;
	hk_out->fields.Number_of_EPS_reboots = gom_hk.fields.counter_boot;
	hk_out->fields.pptMode = gom_hk.fields.pptmode;
	hk_out->fields.channelStatus = 0;
	for (i = 0; i < 8; i++)
	{
		if (gom_hk.fields.output[i])
		{
			hk_out->fields.channelStatus |= 1 << i;
		}
	}
	hk_out->fields.EPSSateNumber = (byte)get_EPS_mode_t();
	hk_out->fields.systemState = get_systems_state_param();

	set_GP_EPS(*hk_out);
	return error;
}
int CAM_HK_collect(CAM_HK* hk_out)
{
	if (get_system_state(cam_param) == SWITCH_OFF)
		return -1;
	hk_out->fields.VoltageInput5V = (voltage_t)(GECKO_GetVoltageInput5V() * 1000);
	hk_out->fields.CurrentInput5V = (current_t)(GECKO_GetCurrentInput5V() * 1000);
	hk_out->fields.VoltageFPGA1V = (voltage_t)(GECKO_GetVoltageFPGA1V() * 1000);
	hk_out->fields.CurrentFPGA1V = (current_t)(GECKO_GetCurrentFPGA1V() * 1000);
	hk_out->fields.VoltageFPGA1V8 = (voltage_t)(GECKO_GetVoltageFPGA1V8() * 1000);
	hk_out->fields.CurrentFPGA1V8 = (current_t)(GECKO_GetCurrentFPGA1V8() * 1000);
	hk_out->fields.VoltageFPGA2V5 = (voltage_t)(GECKO_GetVoltageFPGA2V5() * 1000);
	hk_out->fields.CurrentFPGA2V5 = (current_t)(GECKO_GetCurrentFPGA2V5() * 1000);
	hk_out->fields.VoltageFPGA3V3 = (voltage_t)(GECKO_GetVoltageFPGA3V3() * 1000);
	hk_out->fields.CurrentFPGA3V3 = (current_t)(GECKO_GetCurrentFPGA3V3() * 1000);
	hk_out->fields.VoltageFlash1V8 = (voltage_t)(GECKO_GetVoltageFlash1V8() * 1000);
	hk_out->fields.CurrentFlash1V8 = (current_t)(GECKO_GetCurrentFlash1V8() * 1000);
	hk_out->fields.VoltageFlash3V3 = (voltage_t)(GECKO_GetVoltageFlash3V3() * 1000);
	hk_out->fields.CurrentFlash3V3 = (current_t)(GECKO_GetCurrentFlash3V3() * 1000);
	hk_out->fields.VoltageSNSR1V8 = (voltage_t)(GECKO_GetVoltageSNSR1V8() * 1000);
	hk_out->fields.CurrentSNSR1V8 = (current_t)(GECKO_GetCurrentSNSR1V8() * 1000);
	hk_out->fields.VoltageSNSRVDDPIX = (voltage_t)(GECKO_GetVoltageSNSRVDDPIX() * 1000);
	hk_out->fields.CurrentSNSRVDDPIX = (current_t)(GECKO_GetCurrentSNSRVDDPIX() * 1000);
	hk_out->fields.VoltageSNSR3V3 = (voltage_t)(GECKO_GetVoltageSNSR3V3() * 1000);
	hk_out->fields.CurrentSNSR3V3 = (current_t)(GECKO_GetCurrentSNSR3V3() * 1000);
	hk_out->fields.VoltageFlashVTT09 = (voltage_t)(GECKO_GetVoltageFlashVTT09() * 1000);

	hk_out->fields.TempSMU3AB = GECKO_GetTempSMU3AB();
	hk_out->fields.TempSMU3BC = GECKO_GetTempSMU3BC();
	hk_out->fields.TempREGU6 = GECKO_GetTempREGU6();
	hk_out->fields.TempREGU8 = GECKO_GetTempREGU8();
	hk_out->fields.TempFlash = GECKO_GetTempFlash();

	return 0;
}
int COMM_HK_collect(COMM_HK* hk_out)
{
	ISIStrxvuRxTelemetry_revC telemetry_Rx;
	int error_Rx = -1, error_antA = -1, error_antB = -1, error_Tx = -1;
	error_Rx = IsisTrxvu_rcGetTelemetryAll_revC(0, &telemetry_Rx);
	check_int("COMM_HK_collect, IsisTrxvu_rcGetTelemetryAll_revC", error_Rx);
	hk_out->fields.bus_vol = telemetry_Rx.fields.bus_volt;
	hk_out->fields.total_curr = telemetry_Rx.fields.total_current;
	hk_out->fields.pa_temp = telemetry_Rx.fields.pa_temp;
	hk_out->fields.locosc_temp = telemetry_Rx.fields.locosc_temp;
	hk_out->fields.rx_rssi = telemetry_Rx.fields.rx_rssi;

	ISIStrxvuTxTelemetry_revC telemetry_Tx;
	error_Tx = IsisTrxvu_tcGetTelemetryAll_revC(0, &telemetry_Tx);
	hk_out->fields.tx_fwrdpwr = telemetry_Tx.fields.tx_fwrdpwr;
	hk_out->fields.tx_reflpwr = telemetry_Tx.fields.tx_reflpwr;
#ifdef ANTS_ON
	error_antA = IsisAntS_getTemperature(0, isisants_sideA, &(hk_out->fields.ant_A_temp));
	check_int("COMM_HK_collect ,IsisAntS_getTemperature", error_antA);

	error_antB = IsisAntS_getTemperature(0, isisants_sideB, &(hk_out->fields.ant_B_temp));
	check_int("COMM_HK_collect ,IsisAntS_getTemperature", error_antB);
#endif

	set_GP_COMM(telemetry_Rx);

	return (error_Rx && error_antA && error_antB && error_Tx);
}
int FS_HK_collect(FS_HK* hk_out, int SD_num)
{
	F_SPACE parameter;
	int error = f_getfreespace(SD_num, &parameter);
	if (error != 0)
		return error;
	hk_out->fields.bad = parameter.bad;
	hk_out->fields.free = parameter.free;
	hk_out->fields.total = parameter.total;
	hk_out->fields.used = parameter.used;
	return 0;
}

void HK_find_ADCSTM_fileName(HK_AdcsTlmTypes type, char* fileName)
{
	if(NULL == fileName){
		return;
	}

	switch(type){
	case AdcsTlm_UnixTime:
		strcpy(fileName,ADCS_UNIX_TIME_FILENAME);
		break;
	case AdcsTlm_EstimatedAngles:
		strcpy(fileName,ADCS_EST_ANGLES_FILENAME);
		break;
	case AdcsTlm_EstimatedRates:
		strcpy(fileName,ADCS_EST_ANG_RATE_FILENAME);
		break;
	case AdcsTlm_SatellitePosition:
		strcpy(fileName,ADCS_SAT_POSITION_FILENAME);
		break;
	case AdcsTlm_MagneticField:
		strcpy(fileName,ADCS_MAG_FIELD_VEC_FILENAME);
		break;
	case AdcsTlm_CoarseSunVec:
		strcpy(fileName,ADCS_COARSE_SUN_VEC_FILENAME);
		break;
	case AdcsTlm_FineSunVec:
		strcpy(fileName,ADCS_FINE_SUN_VEC_FILENAME);
		break;
	case AdcsTlm_RateSensor:
		strcpy(fileName,ADCS_RATE_SENSOR_FILENAME);
		break;
	case AdcsTlm_WheelSpeed:
		strcpy(fileName,ADCS_WHEEL_SPEED_FILENAME);
		break;
	case AdcsTlm_MagnetorquerCommand:
		strcpy(fileName,ADCS_MAG_CMD_FILENAME);
		break;
	case AdcsTlm_RawCss1_6:
		strcpy(fileName,ADCS_RAW_CSS_FILENAME_1_6);
		break;
	case AdcsTlm_RawCss7_10:
		strcpy(fileName,ADCS_RAW_CSS_FILENAME_7_10);
		break;
	case AdcsTlm_RawMagnetic:
		strcpy(fileName,ADCS_RAW_MAG_FILENAME);
		break;
	case AdcsTlm_CubeCtrlCurrents:
		strcpy(fileName,ADCS_CUBECTRL_CURRENTS_FILENAME);
		break;
	case AdcsTlm_AdcsState:
		strcpy(fileName,ADCS_STATE_TLM_FILENAME);
		break;
	case AdcsTlm_EstimatedMetaData:
		strcpy(fileName,ADCS_EST_META_DATA);
		break;
	case AdcsTlm_PowerTemperature:
		strcpy(fileName,ADCS_POWER_TEMP_FILENAME);
		break;
	case AdcsTlm_MiscCurrents:
		strcpy(fileName,ADCS_MISC_CURR_FILENAME);
		break;
	}
}
int HK_find_fileName(HK_types type, char* fileName)
{
	if (fileName == NULL)
		return -12;
	if (type == ACK_T){
		strcpy(fileName, ACK_FILE_NAME);
		return 0;
	}

	if (type == log_files_erorrs_T){
		strcpy(fileName, ERROR_LOG_FILENAME);
		return 0;
	}

	if (type == log_files_events_T){
		strcpy(fileName, EVENT_LOG_FILENAME);
		return 0;
	}

	if (offlineTM_T <= type && type < ADCS_science_T){
		onlineTM_param OnlineTM_type = get_item_by_index(type - offlineTM_T);
		strcpy(fileName, OnlineTM_type.name);
		return 0;
	}

	if (type >= ADCS_science_T){
		HK_find_ADCSTM_fileName(type - ADCS_science_T,fileName);
	}
	return -13;
}

int HK_find_ADCSTM_ElementSize(HK_AdcsTlmTypes type)
{
	switch(type){
	case AdcsTlm_UnixTime:
		return	6;
		break;
	case AdcsTlm_EstimatedAngles:
		return	6;
		break;
	case AdcsTlm_EstimatedRates:
		return	6;
		break;
	case AdcsTlm_SatellitePosition:
		return	6;
		break;
	case AdcsTlm_MagneticField:
		return	6;
		break;
	case AdcsTlm_CoarseSunVec:
		return	6;
		break;
	case AdcsTlm_FineSunVec:
		return	6;
		break;
	case AdcsTlm_RateSensor:
		return	6;
		break;
	case AdcsTlm_WheelSpeed:
		return	6;
		break;
	case AdcsTlm_MagnetorquerCommand:
		return	6;
		break;
	case AdcsTlm_RawCss1_6:
		return	6;
		break;
	case AdcsTlm_RawCss7_10:
		return	4;
		break;
	case AdcsTlm_RawMagnetic:
		return	6;
		break;
	case AdcsTlm_CubeCtrlCurrents:
		return	6;
		break;
	case AdcsTlm_AdcsState:
		return	48;
		break;
	case AdcsTlm_EstimatedMetaData:
		return	42;
		break;
	case AdcsTlm_PowerTemperature:
		return	34;
		break;
	case AdcsTlm_MiscCurrents:
		return	4;
		break;
	default:
		return -13;
	}
	return -13;
}
int HK_findElementSize(HK_types type)
{
	if (type == ACK_T)
	{
		return ACK_DATA_LENGTH;
	}
	if (type == log_files_erorrs_T){
		return LOG_STRUCT_ELEMENT_SIZE;
	}

	if (type == log_files_events_T){
		return LOG_STRUCT_ELEMENT_SIZE;
	}
	if (offlineTM_T <= type && type < ADCS_science_T)
	{
		onlineTM_param OnlineTM_type = get_item_by_index(type - offlineTM_T);
		return OnlineTM_type.TM_param_length;
	}
	if (type >= ADCS_science_T){
		return HK_find_ADCSTM_ElementSize(type - ADCS_science_T);
	}
	return -13;
}


int build_HK_spl_packet(HK_types type, byte* raw_data, TM_spl* packet)
{
	if (raw_data == NULL || packet == NULL){
		return -12;
	}
	memcpy(&packet->time, raw_data, TIME_SIZE);
	if (type == ACK_T){
		packet->type = ACK_TYPE;
		packet->subType = ACK_ST;
		packet->length = ACK_DATA_LENGTH;
		memcpy(packet->data, raw_data + TIME_SIZE, ACK_DATA_LENGTH);
		return 0;
	}

	if(type == log_files_events_T)
	{
		packet->type = LOG_T;
		packet->subType = LOG_EVENTS_ST;
		packet->length = LOG_STRUCT_ELEMENT_SIZE;
		memcpy(packet->data, raw_data + TIME_SIZE, ACK_DATA_LENGTH);
		return 0;
	}

	if(type == log_files_erorrs_T)
	{
		packet->type = LOG_T;
		packet->subType = LOG_ERROR_ST;
		packet->length = LOG_STRUCT_ELEMENT_SIZE;
		memcpy(packet->data, raw_data + TIME_SIZE, ACK_DATA_LENGTH);
		return 0;
	}

	if (type >= offlineTM_T && type < ADCS_science_T){
		onlineTM_param OnlineTM_type = get_item_by_index(type - offlineTM_T);
		packet->type = TM_ONLINE_TM_T;
		packet->subType = (byte)type;
		packet->length = (unsigned short)OnlineTM_type.TM_param_length;
		memcpy(packet->data, raw_data + TIME_SIZE, OnlineTM_type.TM_param_length);
		return 0;
	}

	if (type >= ADCS_science_T){
		packet->type = TM_ADCS_T;
		packet->subType = (byte)type;
		packet->length = (unsigned short)HK_findElementSize(type);
		memcpy(packet->data, raw_data + TIME_SIZE, packet->length);
		return 0;
	}
	return -13;
}
