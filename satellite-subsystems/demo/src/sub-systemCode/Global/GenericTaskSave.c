/*
 * GenericTaskSave.c
 *
 *  Created on: Sep 17, 2019
 *      Author: elain
 */
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "GenericTaskSave.h"

#include <hal/errors.h>

#define QUEUE_LENGTH	100

xQueueHandle saveQueue;

void init_GenericSaveQueue()
{
	saveQueue = xQueueCreate(QUEUE_LENGTH, sizeof(saveRequest_task));
	if (saveQueue == NULL)
		return;
}

int add_GenericElement_queue(saveRequest_task item)
{
	if (saveQueue == NULL)
		return E_NOT_INITIALIZED;

	if (xQueueSend(saveQueue, &item, MAX_DELAY) == pdTRUE)
		return 0;
	else
		return -1;
}

void GenericSave_Task()
{
	saveRequest_task removeItem;
	int f_error = f_managed_enterFS();// task 6 enter FS
	check_int("enter FS, in GenericSave task", f_error);

	portBASE_TYPE ret;
	while (1)
	{
		if (uxQueueMessagesWaiting(saveQueue) > 0)
		{
			ret = xQueueReceive(saveQueue, &removeItem, 1000);
			if (ret == pdTRUE)
			{
				FileSystemResult fsResult = c_fileWrite(removeItem.file_name, removeItem.buffer);
				if (fsResult == FS_NOT_EXIST)
				{
					fsResult = c_fileCreate(removeItem.file_name, removeItem.elementSize);
					fsResult = c_fileWrite(removeItem.file_name, removeItem.buffer);
				}
			}
			else
			{
				if (saveQueue == NULL)
					init_GenericSaveQueue();
				else
					xQueueReset(saveQueue);
			}
		}
		vTaskDelay(20);
	}
}
