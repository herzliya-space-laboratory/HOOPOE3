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

typedef struct __attribute__ ((__packed__)) {
	void* TM_param;
	int TM_param_length;
	char name[MAX_F_FILE_NAME_SIZE];
	int (*fn)(unsigned char, void*);
}onlineTM_param;

typedef struct __attribute__ ((__packed__)) {
	onlineTM_param *type;
	time_unix stopTime;
	int period;
}saveTM;

void init_onlineParam();

#endif /* ONLINETM_H_ */
