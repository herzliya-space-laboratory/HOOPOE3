/*
 * imageDump.c
 *
 *  Created on: Jun 23, 2019
 *      Author: elain
 */
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>>
#include <freertos/task.h>

#include "imageDump.h"
#include "../COMM/GSC.h"
#include "../TRXVU.h"
#include "../payload/Butchering.h"
#include "../payload/DataBase.h"
#include "../Main/HouseKeeping.h"

byte imageBuffer[IMAGE_SIZE];

/*
 * @brief		find the subType for the image
 * @param[in]	the type of image
 * @return		the number of subType for the image type
 */
uint8_t find_subType_ofImage(fileType comprasionType)
{
	switch (comprasionType) {
		case raw:
			return IMAGE_DUMP_RAW_ST;
			break;
		case jpg:
			return IMAGE_DUMP_JPG_ST;
			break;
		case t02:
			return IMAGE_DUMP_THUMBNAIL1_ST;
			break;
		case t04:
			return IMAGE_DUMP_THUMBNAIL2_ST;
			break;
		case t08:
			return IMAGE_DUMP_THUMBNAIL3_ST;
			break;
		case t16:
			return IMAGE_DUMP_THUMBNAIL4_ST;
			break;
		default:
			return 0xff;
			break;
	}

	return 0;
}

/*
 * @brief		build a spl packet from the chunk and send it to the ground
 * @param[in]	the chunk to prepar to ground
 * @param[in]	the index of the chunk
 * @param[in]	the type of image
 * @param[in] 	the id of the image
 */
int buildAndSend_chunck(chunk_t chunk_data, unsigned short chunk_index, fileType comprasionType, time_unix id)
{
	TM_spl packet;

	packet.type = IMAGE_DUMP_T;
	packet.subType = find_subType_ofImage(comprasionType);
	packet.time = id;
	packet.length = IMAGE_DATA_FIELD_PACKET_SIZE;
	packet.data[0] = (uint8_t)(chunk_index);
	packet.data[1] = (uint8_t)(chunk_index >> 8);
	memcpy(packet.data + 2, chunk_data, CHUNK_SIZE);

	int rawPacket_length = 0;
	byte rawPacket[IMAGE_PACKET_SIZE];
	encode_TMpacket(rawPacket, &rawPacket_length, packet);
	return TRX_sendFrame(rawPacket, rawPacket_length, trxvu_bitrate_9600);
}

int bitField_imageDump(time_unix image_id, fileType comprasionType, command_id cmdId,
		unsigned int firstChunk_index, byte packetsToSend[NUMBER_OF_CHUNKS_IN_CMD / 8])
{
	int error;

	//todo: read image to image buffer

	chunk_t chunk_data;
	for (unsigned int i = 0; i < NUMBER_OF_CHUNKS_IN_CMD; i++)
	{
		if (getBitValueByIndex(packetsToSend + i, NUMBER_OF_CHUNKS_IN_CMD / 8, i))
		{
			GetChunkFromImage(chunk_data, i + firstChunk_index, imageBuffer, comprasionType);
			error = buildAndSend_chunck(chunk_data, (unsigned short)(i + firstChunk_index), comprasionType, image_id);
			if (error < 0)
				return -1;

			lookForRequestToDelete_dump(cmdId);
			vTaskDelay(SYSTEM_DEALY);
		}
	}

	return 0;
}

int thumbnail4_Dump(time_unix startingTime, time_unix endTime)
{
	return 0;
}

void imageDump_task(void* param)
{
	request_image request = *((request_image*)param);

	if (get_system_state(dump_param))
	{
		//	exit dump task and saves ACK
		save_ACK(ACK_DUMP, ERR_TASK_EXISTS, request.cmdId);
		vTaskDelete(NULL);
	}
	else
	{
		set_system_state(dump_param, SWITCH_ON);
	}

	int ret = f_enterFS();
	check_int("Dump task enter FileSystem", ret);
	vTaskDelay(SYSTEM_DEALY);
	xQueueReset(xDumpQueue);

	if (request.type == bitField_imageDump_)
	{
		time_unix image_id = *((time_unix*)request.command_parameters);
		fileType comprasionType = (fileType)(request.command_parameters[4]);
		unsigned short firstIndex = *((unsigned short*)request.command_parameters + 5);
		byte packetsToSend[NUMBER_OF_CHUNKS_IN_CMD / 8];
		memcpy(packetsToSend, request.command_parameters + 7, NUMBER_OF_CHUNKS_IN_CMD / 8);

		bitField_imageDump(image_id, comprasionType, request.cmdId, firstIndex, packetsToSend);
	}

	set_system_state(dump_param, SWITCH_OFF);
	vTaskDelete(NULL);
}
