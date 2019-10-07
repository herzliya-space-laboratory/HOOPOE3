/*
 * FileDump.c
 *
 *  Created on: Oct 7, 2019
 *      Author: Roy's Laptop
 */

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <satellite-subsystems/IsisTRXVU.h>
#include <satellite-subsystems/GomEPS.h>

#include "../COMM/GSC.h"
#include "../TRXVU.h"
#include "../Main/HouseKeeping.h"
#include "TLM_management.h"

#include "logger.h"

#include "FileDump.h"

void DownloadFile_Dump(void* param)
{
	TC_spl command;
	memcpy(&command, param, sizeof(TC_spl));

	if (get_system_state(dump_param))
	{
		//	exit dump task and saves ACK
		save_ACK(ACK_TASK, ERR_ALLREADY_EXIST, command.id);
		vTaskDelete(NULL);
	}
	else
	{
		int f_error = f_managed_enterFS();// task enter 5
		check_int("enter FS, in imageDump_task", f_error);
		set_system_state(dump_param, SWITCH_ON);
	}

	vTaskDelay(SYSTEM_DEALY);

	xQueueReset(xDumpQueue);

	uint32_t chunk_size;
	memcpy(&chunk_size, command.data, sizeof(uint32_t));
	uint32_t fileName_size;
	memcpy(&fileName_size, command.data + sizeof(uint32_t), sizeof(uint32_t));
	char fileName[fileName_size];
	memcpy(fileName, command.data + sizeof(uint32_t) + sizeof(uint32_t), fileName_size);

	F_FILE* file = NULL;
	f_managed_open(fileName, "r", &file);

	f_seek(file, 0, SEEK_END);
	uint32_t size_of_file = f_tell(file);
	f_seek(file, 0, SEEK_SET);

	int error;

	byte chunk[chunk_size];
	uint32_t number_of_bytes_to_read = chunk_size;
	for (uint16_t i = 0; i < size_of_file/chunk_size; i++)
	{
		memset(chunk, 0, chunk_size);

		if (i > size_of_file)
			number_of_bytes_to_read = chunk_size - (i*chunk_size - size_of_file);
		else
			number_of_bytes_to_read = chunk_size;

		error = f_read(chunk, number_of_bytes_to_read, 1, file);
		if (error != 0)
			break;

		TM_spl packet;
		packet.type = TM_GENERAL_T;
		packet.subType = FILE_DUMP_ST;
		Time_getUnixEpoch(&packet.time);

		packet.length = chunk_size + sizeof(uint32_t) + sizeof(uint16_t);
		memcpy(packet.data, &size_of_file, sizeof(uint32_t));
		memcpy(packet.data + sizeof(uint32_t), &i, sizeof(uint16_t));
		memcpy(packet.data + sizeof(uint32_t) + sizeof(uint16_t), chunk, chunk_size);

		int rawPacket_length = 0;
		byte rawPacket[packet.length + SPL_TM_HEADER_SIZE];
		encode_TMpacket(rawPacket, &rawPacket_length, packet);

		error = TRX_sendFrame(rawPacket, rawPacket_length);
		if (error != 0)
			break;
	}

	f_managed_close(&file);

	set_system_state(dump_param, SWITCH_OFF);
	f_managed_releaseFS();
	vTaskDelete(NULL);
}

void DownloadFile(TC_spl command)
{
	xTaskCreate(DownloadFile_Dump, (const signed char*)"File Download", 8192, &command, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, NULL);
	vTaskDelay(SYSTEM_DEALY);
}
