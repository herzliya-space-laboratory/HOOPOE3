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
#define NUMBER_OF_ONLIME_TM_PACKETS	23
#define MAX_ITEMS_OFFLINE_LIST	20
#define SETTING_LIST_SIZE	MAX_ITEMS_OFFLINE_LIST * OFFLINE_FRAM_STRUCT_SIZE

typedef struct __attribute__ ((__packed__)) {
	void* TM_param;
	int TM_param_length;
	char name[MAX_F_FILE_NAME_SIZE];
	int (*fn)(unsigned char, void*);
}onlineTM_param;

typedef struct __attribute__ ((__packed__)) {
	onlineTM_param *type;
	time_unix stopTime;
	time_unix lastSave;
	uint period;
}saveTM;

onlineTM_param get_item_by_index(int TMIndex);

void init_onlineParam();

void reset_offline_TM_list();

int get_online_packet(int TM_index, TM_spl* packet);

int get_offlineSettingPacket(TM_spl* setPacket);

int delete_onlineTM_param_from_offline(int TM_index);

int add_onlineTM_param_to_save_list(int TM_index, uint period, time_unix stopTime);

void save_onlineTM_task();
#endif /* ONLINETM_H_ */
