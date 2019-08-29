/*
 * OnlineTM.h
 *
 *  Created on: Aug 3, 2019
 *      Author: elain
 */

#ifndef ONLINETM_H_
#define ONLINETM_H_

#include "Global.h"
#include "TLM_management.h"
#include "../COMM/GSC.h"
#include "../COMM/splTypes.h"
#include "../Main/HouseKeeping.h"

#define DEFULT_INDEX	0
#define NUMBER_OF_ONLIME_TM_PACKETS	25
#define MAX_ITEMS_OFFLINE_LIST	10
#define SETTING_LIST_SIZE	MAX_ITEMS_OFFLINE_LIST * OFFLINE_FRAM_STRUCT_SIZE

typedef enum{
	TM_EPS_HK = 0,
	TM_COMM_HK = 1,
	TM_CAM_HK = 2,
	TM_SP_HK = 3,
	TM_ADCS_HK = 4,
	TM_GomEPS_HK_param = 5,
	TM_GomEPS_HK_general = 6,
	TM_GomEPS_HK_vi = 7,
	TM_GomEPS_HK_out = 8,
	TM_GomEPS_HK_wdt = 9,
	TM_GomEPS_HK_basic= 10,
	TM_GomEPS_HK_config = 11,
	TM_GomEPS_HK_config2 = 12,
	TM_IsisTrxvu_rcGetTelemetryAll = 13,
	TM_IsisTrxvu_rcGetTelemetryAll_revC = 14,
	TM_IsisTrxvu_tcGetTelemetryAll = 15,
	TM_IsisTrxvu_tcGetTelemetryAll_revC = 16,
	TM_IsisAntS_getStatusData_sideA = 17,
	TM_IsisAntS_getAlltelemetry_sideA = 18,
	TM_IsisAntS_getTemperature_sideA = 19,
	TM_IsisAntS_getStatusData_sideB = 20,
	TM_IsisAntS_getAlltelemetry_sideB = 21,
	TM_IsisAntS_getTemperature_sideB = 22,
	TM_FS_Space_A = 23,
	TM_FS_Space_B = 24,
	TM_emptySpace = 255,
}TM_struct_types;

typedef struct __attribute__ ((__packed__)) {
	void* TM_param;
	int TM_param_length;
	char name[MAX_F_FILE_NAME_SIZE];
	int (*fn)(unsigned char, void*);
}onlineTM_param;

typedef struct __attribute__ ((__packed__)) {
	TM_struct_types type;
	time_unix stopTime;
	time_unix lastSave;
	uint period;
}saveTM;

onlineTM_param get_item_by_index(TM_struct_types TMIndex);

void init_onlineParam();

void reset_offline_TM_list();

int get_online_packet(int TM_index, TM_spl* packet);

int get_offlineSettingPacket(TM_spl* setPacket);

int delete_onlineTM_param_from_offline(TM_struct_types TM_index);

int add_onlineTM_param_to_save_list(TM_struct_types TM_index, uint period, time_unix stopTime);

void save_onlineTM_task();
#endif /* ONLINETM_H_ */
