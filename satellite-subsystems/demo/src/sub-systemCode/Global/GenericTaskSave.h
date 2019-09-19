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

#define MAX_ELEMENT_SIZE	20

typedef struct __attribute__ ((__packed__))
{
	char file_name[MAX_F_FILE_NAME_SIZE];
	int elementSize;
	byte buffer[MAX_ELEMENT_SIZE];
}saveRequest_task;

void init_GenericSaveQueue();

int add_GenericElement_queue(saveRequest_task item);

void GenericSave_Task();

#endif /* GENERICTASKSAVE_H_ */
