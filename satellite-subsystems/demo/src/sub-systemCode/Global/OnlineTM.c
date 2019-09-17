/*
 * OnlineTM.c
 *
 *  Created on: Aug 3, 2019
 *      Author: elain
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <satellite-subsystems/GomEPS.h>
#include <satellite-subsystems/IsisTRXVU.h>
#include <satellite-subsystems/IsisAntS.h>
#include <satellite-subsystems/cspaceADCS.h>
#include <satellite-subsystems/cspaceADCS_types.h>

#include "freertosExtended.h"
#include "OnlineTM.h"

xSemaphoreHandle xSD_state;
FS_HK SD_A;
FS_HK SD_B;

#define CHECK_TM_INDEX_RANGE(index) ((index >= 0) && (index < NUMBER_OF_ONLIME_TM_PACKETS))

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

int HK_collect_EPS(unsigned char index, void* param)
{
	(void)index;
	return EPS_HK_collect(param);
}
int HK_collect_COMM(unsigned char index, void* param)
{
	(void)index;
	return COMM_HK_collect(param);
}
int HK_collect_CAM(unsigned char index, void* param)
{
	(void)index;
	return CAM_HK_collect(param);
}
int HK_collect_SP(unsigned char index, void* param)
{
	(void)index;
	return SP_HK_collect(param);
}
int HK_collect_FS_A(unsigned char index, void* param)
{
	(void)index;
	if (xSemaphoreTake_extended(xSD_state, 100) == pdTRUE)
	{
		memcpy(param, SD_A.raw, FS_HK_SIZE);
		xSemaphoreGive_extended(xSD_state);
		return 0;
	}
	return -1;
}
int HK_collect_FS_B(unsigned char index, void* param)
{
	(void)index;
	if (xSemaphoreTake_extended(xSD_state, 100) == pdTRUE)
	{
		memcpy(param, SD_B.raw, FS_HK_SIZE);
		xSemaphoreGive_extended(xSD_state);
		return 0;
	}
	return -1;
}

onlineTM_param onlineTM_list[NUMBER_OF_ONLIME_TM_PACKETS];
saveTM offline_TM_list[MAX_ITEMS_OFFLINE_LIST];

typedef struct __attribute__ ((__packed__)) {
	Boolean8bit isEmpty;
	byte type_index;
	time_unix stopTime;
	uint period;
}saveTM_FRAM;

//update the current off line list in the FRAM
void save_offlineSetting_FRAM()
{
	saveTM_FRAM list[MAX_ITEMS_OFFLINE_LIST];
	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
	{
		if (offline_TM_list[i].type == TM_emptySpace)
		{
			list[i].isEmpty = TRUE_8BIT;
			continue;
		}
		list[i].isEmpty = FALSE_8BIT;
		list[i].type_index = (byte)offline_TM_list[i].type;
		list[i].stopTime = offline_TM_list[i].stopTime;
		list[i].period = offline_TM_list[i].period;
	}

	int i_error = FRAM_writeAndVerify((byte*)list, OFFLINE_LIST_SETTINGS_ADDR, OFFLINE_FRAM_STRUCT_SIZE * MAX_ITEMS_OFFLINE_LIST);
	check_int("FRAM_writeAndVerify, save_offlineSettings_FRAM", i_error);
}
//get the current off line list in the FRAM
int get_offlineSetting_FRAM(saveTM_FRAM *list)
{
	if (list == NULL)
		return -4;
	int i_error = FRAM_read((byte*)list, OFFLINE_LIST_SETTINGS_ADDR, OFFLINE_FRAM_STRUCT_SIZE * MAX_ITEMS_OFFLINE_LIST);
	check_int("FRAM_writeAndVerify, save_offlineSettings_FRAM", i_error);
	return 0;
}
//update the current off line list from the FRAM
int update_offlineSetting_FRAM()
{
	saveTM_FRAM list[MAX_ITEMS_OFFLINE_LIST];
	if (get_offlineSetting_FRAM(list))
		return -1;

	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
	{
		if (list[i].isEmpty)
		{
			offline_TM_list[i].type = TM_emptySpace;
			offline_TM_list[i].lastSave = 0;
			offline_TM_list[i].stopTime = 0;
			offline_TM_list[i].period = 0;
		}
		else
		{
			offline_TM_list[i].type = list[i].type_index;
			offline_TM_list[i].lastSave = 0;
			offline_TM_list[i].stopTime = list[i].stopTime;
			offline_TM_list[i].period = list[i].period;
		}
	}
	return 0;
}
//get the current off line list settings from the FRAM to a TM_spl packet
int get_offlineSettingPacket(TM_spl* setPacket)
{
	if (setPacket == NULL)
		return -12;
	setPacket->type = TM_ONLINE_TM_T;
	setPacket->subType = OFFLINE_SETTING_ST;
	setPacket->length = SETTING_LIST_SIZE;
	int i_error = Time_getUnixEpoch(&setPacket->time);
	check_int("Time_getUnixEpoch, get_offlineSettingPacket", i_error);
	saveTM_FRAM FRAM_list[MAX_ITEMS_OFFLINE_LIST];
	i_error = get_offlineSetting_FRAM(FRAM_list);
	if (i_error != 0)
		return i_error;
	memcpy(setPacket->data, FRAM_list, SETTING_LIST_SIZE);
	return 0;
}

onlineTM_param get_item_by_index(TM_struct_types TMIndex)
{
	onlineTM_param item;
	item.TM_param = NULL;
	item.TM_param_length = onlineTM_list[TMIndex].TM_param_length;
	strcpy(item.name, onlineTM_list[TMIndex].name);
	return item;
}
onlineTM_param* get_pointer_item_by_index(TM_struct_types TMIndex)
{
	return &(onlineTM_list[TMIndex]);
}

void init_onlineParam()
{
	vSemaphoreCreateBinary(xSD_state);
	if (xSD_state == NULL)
	{
		printf("xSD_state could not be created\n");
	}

	int index = 0;
	//0
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))HK_collect_EPS;
	onlineTM_list[index].TM_param_length = EPS_HK_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, EPS_HK_FILE_NAME);
	index++;
	//1
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))HK_collect_COMM;
	onlineTM_list[index].TM_param_length = COMM_HK_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, COMM_HK_FILE_NAME);
	index++;
	//2
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))HK_collect_CAM;
	onlineTM_list[index].TM_param_length = CAM_HK_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, CAM_HK_FILE_NAME);
	index++;
	//3
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))HK_collect_SP;
	onlineTM_list[index].TM_param_length = SP_HK_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, SP_HK_FILE_NAME);
	index++;
	//4
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))HK_collect_SP;
	onlineTM_list[index].TM_param_length = SP_HK_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, SP_HK_FILE_NAME);
	index++;
	//5
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_param;
	onlineTM_list[index].TM_param_length = 45;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "GEA");
	index++;
	//6
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_general;
	onlineTM_list[index].TM_param_length = 133;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "GEB");
	index++;
	//7
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_vi;
	onlineTM_list[index].TM_param_length = 22;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "GEC");
	index++;
	//8
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_out;
	onlineTM_list[index].TM_param_length = 66;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "GED");
	index++;
	//9
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_wdt;
	onlineTM_list[index].TM_param_length = 28;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "GEE");
	index++;
	//10
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))GomEpsGetHkData_basic;
	onlineTM_list[index].TM_param_length = 23;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "GEF");
	index++;
	//11
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))GomEpsConfigGet;
	onlineTM_list[index].TM_param_length = 60;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "GEG");
	index++;
	//12
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))GomEpsConfig2Get;
	onlineTM_list[index].TM_param_length = 22;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "GEJ");
	index++;
	//13
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))IsisTrxvu_rcGetTelemetryAll;
	onlineTM_list[index].TM_param_length = TRXVU_ALL_RXTELEMETRY_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "TRA");
	index++;
	//14
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))IsisTrxvu_rcGetTelemetryAll_revC;
	onlineTM_list[index].TM_param_length = TRXVU_ALL_RXTELEMETRY_REVC_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "TRB");
	index++;
	//15
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))IsisTrxvu_tcGetTelemetryAll;
	onlineTM_list[index].TM_param_length = TRXVU_ALL_TXTELEMETRY_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "TRC");
	index++;
	//16
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))IsisTrxvu_tcGetTelemetryAll_revC;
	onlineTM_list[index].TM_param_length = TRXVU_ALL_TXTELEMETRY_REVC_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "TRD");
	index++;
	//17
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))IsisAntS_getStatusData_sideA;
	onlineTM_list[index].TM_param_length = 2;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "ANA");
	index++;
	//18
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))IsisAntS_getAlltelemetry_sideA;
	onlineTM_list[index].TM_param_length = 8;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "ANB");
	index++;
	//19
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))IsisAntS_getTemperature_sideA;
	onlineTM_list[index].TM_param_length = sizeof(unsigned short);
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "ANC");
	index++;
	//20
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))IsisAntS_getStatusData_sideB;
	onlineTM_list[index].TM_param_length = 2;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "AND");
	index++;
	//21
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))IsisAntS_getAlltelemetry_sideB;
	onlineTM_list[index].TM_param_length = 8;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "ANE");
	index++;
	//22
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))IsisAntS_getTemperature_sideB;
	onlineTM_list[index].TM_param_length = sizeof(unsigned short);
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, "ANF");
	index++;
	//23
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))HK_collect_FS_A;
	onlineTM_list[index].TM_param_length = FS_HK_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, FS_HK_FILE_NAME);
	index++;
	//24
	onlineTM_list[index].fn = (int (*)(unsigned char, void*))HK_collect_FS_B;
	onlineTM_list[index].TM_param_length = FS_HK_SIZE;
	onlineTM_list[index].TM_param = malloc(onlineTM_list[index].TM_param_length);
	strcpy(onlineTM_list[index].name, FS_HK_FILE_NAME);
	index++;
}
void reset_offline_TM_list()
{
	for(int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
	{
		offline_TM_list[i].type = TM_emptySpace;
		offline_TM_list[i].lastSave = 0;
		offline_TM_list[i].period = 0;
		offline_TM_list[i].stopTime = 0;
	}

	add_onlineTM_param_to_save_list(TM_EPS_HK, 1, 4294967295u);
	add_onlineTM_param_to_save_list(TM_COMM_HK, 1, 4294967295u);
	add_onlineTM_param_to_save_list(TM_CAM_HK, 1, 4294967295u);
	add_onlineTM_param_to_save_list(TM_SP_HK, 20, 4294967295u);
	add_onlineTM_param_to_save_list(TM_FS_Space_A, 60*30, 4294967295u);
	//add_onlineTM_param_to_save_list(TM_FS_Space_B, 60*30, 4294967295u);

	save_offlineSetting_FRAM();
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
	packet->subType = TM_index + offlineTM_T;
	packet->length = onlineTM_list[TM_index].TM_param_length;
	Time_getUnixEpoch(&(packet->time));

	memcpy(packet->data, onlineTM_list[TM_index].TM_param, onlineTM_list[TM_index].TM_param_length);

	return 0;
}
int save_onlineTM_param(TM_struct_types TMIndex)
{
	if (TMIndex == TM_emptySpace)
		return -2;

	onlineTM_param* TLM_type = get_pointer_item_by_index(TMIndex);
	int error = TLM_type->fn(DEFULT_INDEX, TLM_type->TM_param);
	if (error != 0)
		return error;
	FileSystemResult FS_result = FS_SUCCSESS;
	FS_result = c_fileWrite(TLM_type->name, TLM_type->TM_param);
	if (FS_result == FS_NOT_EXIST)
	{
		FS_result = c_fileCreate(TLM_type->name, TLM_type->TM_param_length);
		FS_result = c_fileWrite(TLM_type->name, TLM_type->TM_param);
	}

	return 0;
}

int add_onlineTM_param_to_save_list(TM_struct_types TM_index, uint period, time_unix stopTime)
{
	if (TM_index >= NUMBER_OF_ONLIME_TM_PACKETS || TM_index == 4)
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
		if (TM_index == offline_TM_list[i].type && !addedToLit)
		{
			addedToLit = TRUE;
			offline_TM_list[i].period = period;
			offline_TM_list[i].stopTime = stopTime;
		}
		else if (TM_index == offline_TM_list[i].type && addedToLit)
		{
			offline_TM_list[i].type = TM_emptySpace;
		}
	}
	if (addedToLit)
	{
		save_offlineSetting_FRAM();
		return 0;
	}

	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
	{
		if (TM_emptySpace == offline_TM_list[i].type)
		{
			addedToLit = TRUE;
			offline_TM_list[i].type = TM_index;
			offline_TM_list[i].lastSave = 0;
			offline_TM_list[i].period = period;
			offline_TM_list[i].stopTime = stopTime;
			break;
		}
	}
	if (addedToLit)
	{
		save_offlineSetting_FRAM();
		return 0;
	}
	else
		return -3;
}
int delete_onlineTM_param_from_offline(TM_struct_types TM_index)
{
	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
	{
		if (offline_TM_list[i].type == TM_emptySpace)
			continue;
		else if (offline_TM_list[i].type == TM_index)
		{
			offline_TM_list[i].type = TM_emptySpace;
			offline_TM_list[i].lastSave = 0;
			offline_TM_list[i].period = 0;
			offline_TM_list[i].stopTime = 0;
			save_offlineSetting_FRAM();
			return 0;
		}
	}

	return -2;
}

void save_onlineTM_logic()
{
	int i_error;
	time_unix time_now;
	i_error = Time_getUnixEpoch(&time_now);
	check_int("Time_getUnixEpoch, save_onlineTM_logic", i_error);
	printf("       time now: %u\n", time_now);
	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
	{
		if (offline_TM_list[i].type == TM_emptySpace)
			continue;
		else if (offline_TM_list[i].stopTime < time_now)
		{
			delete_onlineTM_param_from_offline(offline_TM_list[i].type);
		}
		else if (offline_TM_list[i].period + offline_TM_list[i].lastSave <= time_now)
		{
			i_error = save_onlineTM_param(offline_TM_list[i].type);
			if (i_error)
				printf("error in collecting TM: %d, ERROR: %d\n", offline_TM_list[i].type, i_error);
			else
				offline_TM_list[i].lastSave = time_now;
		}
	}
}

void updateSD_state()
{
	if (xSemaphoreTake_extended(xSD_state, 100) == pdTRUE)
	{
		F_SPACE parameter;
		int error = f_getfreespace(0, &parameter);
		if (error != 0)
			printf("fuck, just fuck\n");
		SD_A.fields.bad = parameter.bad;
		SD_A.fields.free = parameter.free;
		SD_A.fields.total = parameter.total;
		SD_A.fields.used = parameter.used;
		/*error = f_getfreespace(1, &parameter);
		if (error != 0)
			printf("fuck, just fuck\n");
		SD_B.fields.bad = parameter.bad;
		SD_B.fields.free = parameter.free;
		SD_B.fields.total = parameter.total;
		SD_B.fields.used = parameter.used;*/
		xSemaphoreGive_extended(xSD_state);
	}
}

void save_onlineTM_task()
{
	portTickType xLastWakeTime = xTaskGetTickCount();
	const portTickType xFrequency = 1000;
	if (update_offlineSetting_FRAM())
	{
		printf("error from set_offlineSettings_FRAM in init\n");
		reset_offline_TM_list();
	}
	for (int i = 0; i < MAX_ITEMS_OFFLINE_LIST; i++)
		offline_TM_list[i].lastSave = 0;

	int f_error = f_enterFS();//4 enter FS
	check_int("save online TM, enter FS", f_error);
	while(TRUE)
	{
		updateSD_state();
		save_onlineTM_logic();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}
