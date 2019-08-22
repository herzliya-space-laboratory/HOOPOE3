/*
 * test_CMD.c
 *
 *  Created on: Jun 23, 2019
 *      Author: elain
 */
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "COMM_CMD.h"
#include "../../TRXVU.h"
#include "../../Tests/Test.h"

#define create_task(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask) xTaskCreate( (pvTaskCode) , (pcName) , (usStackDepth) , (pvParameters), (uxPriority), (pxCreatedTask) ); vTaskDelay(10);

void cmd_imageDump_test(Ack_type* type, ERR_type* err, TC_spl cmd)
{
	if (cmd.length != 2 * TIME_SIZE + 5)
	{
		return;
	}
	//1. build combine data with command_id
	unsigned char raw[2 * TIME_SIZE + 5 + 4] = {0};
	// 1.1. copying command id
	BigEnE_uInt_to_raw(cmd.id, &raw[0]);
	// 1.2. copying command data
	memcpy(raw + 4, cmd.data, 2 * TIME_SIZE + 5);
	create_task(imageDump_test, (const signed char * const)"Dump_Task", (unsigned short)(STACK_DUMP_SIZE), (void*)raw, FIRST_PRIORITY, xDumpHandle);

}
