/*
 * GenericTaskSave.h
 *
 *  Created on: Sep 17, 2019
 *      Author: elain
 */

#ifndef GENERICTASKSAVE_H_
#define GENERICTASKSAVE_H_

#include "TLM_management.h"
#include "Global.h"

typedef struct __attribute__ ((__packed__))
{
	char file_name[MAX_F_FILE_NAME_SIZE];
	int elementSize;
	byte
}saveRequest_task;

#endif /* GENERICTASKSAVE_H_ */
