/*
 * OnlineTM.h
 *
 *  Created on: Aug 3, 2019
 *      Author: elain
 */

#ifndef ONLINETM_H_
#define ONLINETM_H_

#include "Global.h"
#include "TM_managment.h"
#include "../COMM/GSC.h"
#include "../COMM/splTypes.h"

#define USE_DIFFERENT_TASK_ONLINE_TM

#define DEFULT_INDEX	0

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

void init_onlineParam();

void reset_offline_TM_list();

int get_online_packet(int TM_index, TM_spl* packet);

int delete_onlineTM_param_to_save_list(int TM_index);

int add_onlineTM_param_to_save_list(int TM_index, uint period, time_unix stopTime);

#ifndef USE_DIFFERENT_TASK_ONLINE_TM
void save_onlineTM_logic();
#else
void save_onlineTM_task();
#endif
#endif /* ONLINETM_H_ */
