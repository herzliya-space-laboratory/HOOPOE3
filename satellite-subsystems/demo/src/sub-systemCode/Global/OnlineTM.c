/*
 * OnlineTM.c
 *
 *  Created on: Aug 3, 2019
 *      Author: elain
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <satellite-subsystems/GomEPS.h>
#include <satellite-subsystems/IsisTRXVU.h>
#include <satellite-subsystems/IsisAntS.h>

#include "OnlineTM.h"

int IsisAntS_getStatusData_sideA(unsigned char index, ISISantsStatus* status)
{
	return IsisAntS_getStatusData(index, isisants_sideA, status);
}
int IsisAntS_getAlltelemetry_sideA(unsigned char index, ISISantsTelemetry* alltelemetry)
{
	return IsisAntS_getAlltelemetry(index, isisants_sideA, alltelemetry);
}
int IsisAntS_getTemperature_sideA(unsigned char index, unsigned short* temperature)
{
	return IsisAntS_getTemperature(index, isisants_sideA, temperature);
}
int IsisAntS_getStatusData_sideB(unsigned char index, ISISantsStatus* status)
{
	return IsisAntS_getStatusData(index, isisants_sideB, status);
}
int IsisAntS_getAlltelemetry_sideB(unsigned char index, ISISantsTelemetry* alltelemetry)
{
	return IsisAntS_getAlltelemetry(index, isisants_sideB, alltelemetry);
}
int IsisAntS_getTemperature_sideB(unsigned char index, unsigned short* temperature)
{
	return IsisAntS_getTemperature(index, isisants_sideB, temperature);
}

#define NUMBER_OF_ONLIME_TM_PACKETS	18
onlineTM_param onlineTM_list[NUMBER_OF_ONLIME_TM_PACKETS];

#define MAX_ITEMS_OFFLINE_LIST	10
saveTM offline_TM_list[MAX_ITEMS_OFFLINE_LIST];

void init_onlineParam()
{
	onlineTM_list[0].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_param;
	onlineTM_list[0].TM_param_length = 45;
	onlineTM_list[0].TM_param = malloc(onlineTM_list[0].TM_param_length);
	strcpy(onlineTM_list[0].name, "GEA");

	onlineTM_list[1].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_general;
	onlineTM_list[1].TM_param_length = 133;
	onlineTM_list[1].TM_param = malloc(onlineTM_list[1].TM_param_length);
	strcpy(onlineTM_list[1].name, "GEB");

	onlineTM_list[2].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_vi;
	onlineTM_list[2].TM_param_length = 22;
	onlineTM_list[2].TM_param = malloc(onlineTM_list[2].TM_param_length);
	strcpy(onlineTM_list[2].name, "GEC");

	onlineTM_list[3].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_out;
	onlineTM_list[3].TM_param_length = 66;
	onlineTM_list[3].TM_param = malloc(onlineTM_list[3].TM_param_length);
	strcpy(onlineTM_list[3].name, "GED");

	onlineTM_list[4].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_wdt;
	onlineTM_list[4].TM_param_length = 28;
	onlineTM_list[4].TM_param = malloc(onlineTM_list[4].TM_param_length);
	strcpy(onlineTM_list[4].name, "GEE");

	onlineTM_list[5].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_basic;
	onlineTM_list[5].TM_param_length = 23;
	onlineTM_list[5].TM_param = malloc(onlineTM_list[5].TM_param_length);
	strcpy(onlineTM_list[5].name, "GEF");

	onlineTM_list[6].fn = (int (*)(unsigned char, void*))GomEpsConfigGet;
	onlineTM_list[6].TM_param_length = 60;
	onlineTM_list[6].TM_param = malloc(onlineTM_list[6].TM_param_length);
	strcpy(onlineTM_list[6].name, "GEG");

	onlineTM_list[7].fn = (int (*)(unsigned char, void*))GomEpsConfig2Get;
	onlineTM_list[7].TM_param_length = 22;
	onlineTM_list[7].TM_param = malloc(onlineTM_list[7].TM_param_length);
	strcpy(onlineTM_list[7].name, "GEJ");

	onlineTM_list[8].fn = (int (*)(unsigned char, void*))IsisTrxvu_rcGetTelemetryAll;
	onlineTM_list[8].TM_param_length = TRXVU_ALL_RXTELEMETRY_SIZE;
	onlineTM_list[8].TM_param = malloc(onlineTM_list[8].TM_param_length);
	strcpy(onlineTM_list[8].name, "TRA");

	onlineTM_list[9].fn = (int (*)(unsigned char, void*))IsisTrxvu_rcGetTelemetryAll_revC;
	onlineTM_list[9].TM_param_length = TRXVU_ALL_RXTELEMETRY_REVC_SIZE;
	onlineTM_list[9].TM_param = malloc(onlineTM_list[9].TM_param_length);
	strcpy(onlineTM_list[9].name, "TRB");

	onlineTM_list[10].fn = (int (*)(unsigned char, void*))IsisTrxvu_tcGetTelemetryAll;
	onlineTM_list[10].TM_param_length = TRXVU_ALL_TXTELEMETRY_SIZE;
	onlineTM_list[10].TM_param = malloc(onlineTM_list[10].TM_param_length);
	strcpy(onlineTM_list[10].name, "TRC");

	onlineTM_list[11].fn = (int (*)(unsigned char, void*))IsisTrxvu_tcGetTelemetryAll_revC;
	onlineTM_list[11].TM_param_length = TRXVU_ALL_TXTELEMETRY_REVC_SIZE;
	onlineTM_list[11].TM_param = malloc(onlineTM_list[11].TM_param_length);
	strcpy(onlineTM_list[11].name, "TRD");

	onlineTM_list[12].fn = (int (*)(unsigned char, void*))IsisAntS_getStatusData_sideA;
	onlineTM_list[12].TM_param_length = 2;
	onlineTM_list[12].TM_param = malloc(onlineTM_list[12].TM_param_length);
	strcpy(onlineTM_list[12].name, "ANA");

	onlineTM_list[13].fn = (int (*)(unsigned char, void*))IsisAntS_getAlltelemetry_sideA;
	onlineTM_list[13].TM_param_length = 8;
	onlineTM_list[13].TM_param = malloc(onlineTM_list[13].TM_param_length);
	strcpy(onlineTM_list[13].name, "ANB");

	onlineTM_list[14].fn = (int (*)(unsigned char, void*))IsisAntS_getTemperature_sideA;
	onlineTM_list[14].TM_param_length = sizeof(unsigned short);
	onlineTM_list[14].TM_param = malloc(onlineTM_list[14].TM_param_length);
	strcpy(onlineTM_list[14].name, "ANC");

	onlineTM_list[15].fn = (int (*)(unsigned char, void*))IsisAntS_getStatusData_sideB;
	onlineTM_list[15].TM_param_length = 2;
	onlineTM_list[15].TM_param = malloc(onlineTM_list[15].TM_param_length);
	strcpy(onlineTM_list[15].name, "AND");

	onlineTM_list[16].fn = (int (*)(unsigned char, void*))IsisAntS_getAlltelemetry_sideB;
	onlineTM_list[16].TM_param_length = 8;
	onlineTM_list[16].TM_param = malloc(onlineTM_list[16].TM_param_length);
	strcpy(onlineTM_list[16].name, "ANE");

	onlineTM_list[17].fn = (int (*)(unsigned char, void*))IsisAntS_getTemperature_sideB;
	onlineTM_list[17].TM_param_length = sizeof(unsigned short);
	onlineTM_list[17].TM_param = malloc(onlineTM_list[5].TM_param_length);
	strcpy(onlineTM_list[17].name, "ANF");

	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
		reset_offline_TM_list();
}

void reset_offline_TM_list()
{
	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
	{
		offline_TM_list[i].type = NULL;
	}
}

int get_online_packet(int TM_index, TM_spl* packet)
{
	if (TM_index >= NUMBER_OF_ONLIME_TM_PACKETS || TM_index < 0)
		return -1;
	if (packet == NULL)
		return -2;

	int error = onlineTM_list[TM_index].fn(DEFULT_INDEX, onlineTM_list[TM_index].TM_param);
	if (error != 0)
		return error;

	packet->type = TM_ONLINE_TM_T;
	packet->subType = TM_index;
	packet->length = onlineTM_list[TM_index].TM_param_length;
	Time_getUnixEpoch(&(packet->time));

	memcpy(packet->data, onlineTM_list[TM_index].TM_param, onlineTM_list[TM_index].TM_param_length);

	return 0;
}

int save_onlineTM_param(saveTM param)
{
	if (param.type == NULL)
		return -2;

	int error = param.type->fn(DEFULT_INDEX, param.type->TM_param);
	if (error != 0)
		return error;

	FileSystemResult FS_result= c_fileWrite(param.type->name, param.type->TM_param);
	if (FS_result == FS_NOT_EXIST)
	{
		FS_result = c_fileCreate(param.type->name, param.type->TM_param_length);
		if (FS_result != FS_SUCCSESS)
			return 2;
		FS_result= c_fileWrite(param.type->name, param.type->TM_param);
		if (FS_result != FS_SUCCSESS)
			return 3;
	}
	else if (FS_result != FS_SUCCSESS)
		return 3;

	return 0;
}

int add_onlineTM_param_to_save_list(int TM_index, uint period, time_unix stopTime)
{
	if (TM_index >= NUMBER_OF_ONLIME_TM_PACKETS || TM_index < 0)
		return -1;
	if (period == 0)
		return -1;

	time_unix time_now;
	Time_getUnixEpoch(&time_now);

	if (time_now >= stopTime)
		return -1;

	Boolean addedToLit = FALSE;
	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
	{
		if (offline_TM_list[i].type == NULL && !addedToLit)
		{
			offline_TM_list[i].type = onlineTM_list + TM_index;
			offline_TM_list[i].period = period;
			offline_TM_list[i].stopTime = stopTime;
			offline_TM_list[i].lastSave = 0;
			addedToLit = TRUE;
		}
		else
		{
			if (offline_TM_list[i].type == onlineTM_list + TM_index)
			{
				if (addedToLit)
				{
					offline_TM_list[i].type = NULL;
				}
				else
				{
					offline_TM_list[i].period = period;
					offline_TM_list[i].stopTime = stopTime;
				}
			}
		}
	}

	if (addedToLit)
		return 0;
	else
		return -3;
}

int delete_onlineTM_param_to_save_list(int TM_index)
{
	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
	{
		if (offline_TM_list[i].type == NULL)
			continue;
		else if (offline_TM_list[i].type == onlineTM_list + TM_index)
		{
			offline_TM_list[i].type = NULL;
			return 0;
		}
	}

	return -1;
}

void save_onlineTM_logic()
{
	time_unix time_now;
	Time_getUnixEpoch(&time_now);
	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
	{
		if (offline_TM_list[i].type == NULL)
			continue;
		else if (offline_TM_list[i].stopTime <= time_now)
		{
			offline_TM_list[i].type = NULL;
			offline_TM_list[i].lastSave = 0;
			offline_TM_list[i].period = 0;
			offline_TM_list[i].stopTime = 0;
		}
		else if (offline_TM_list[i].period + offline_TM_list[i].lastSave <= time_now)
		{
			save_onlineTM_param(offline_TM_list[i]);
			offline_TM_list[i].lastSave = time_now;
		}
	}
}

void save_onlineTM_task()
{
	portTickType xLastWakeTime = xTaskGetTickCount();
	const portTickType xFrequency = 1000;

	while(TRUE)
	{
		save_onlineTM_logic();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}
