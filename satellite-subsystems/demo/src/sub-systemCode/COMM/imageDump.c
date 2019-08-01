/*
 * imageDump.c
 *
 *  Created on: Jun 23, 2019
 *      Author: elain
 */
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <Math.h>

#include <hcc/api_fat.h>

#include "imageDump.h"
#include "GSC.h"
#include "../TRXVU.h"
#include "../payload/Butchering.h"
#include "../payload/DataBase.h"
#include "../Main/HouseKeeping.h"
#include "../Global/TM_managment.h"

image_t imageBuffer;

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
		case t32:
			return IMAGE_DUMP_THUMBNAIL5_ST;
			break;
		case t64:
			return IMAGE_DUMP_THUMBNAIL6_ST;
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
int buildAndSend_chunck(chunk_t chunk_data, unsigned short chunk_index, fileType comprasionType, imageid id)
{
	TM_spl packet;

	packet.type = IMAGE_DUMP_T;
	packet.subType = find_subType_ofImage(comprasionType);
	packet.time = id;
	packet.length = IMAGE_DATA_FIELD_PACKET_SIZE;
	memcpy(packet.data, &chunk_index, sizeof(uint16_t));
	memcpy(packet.data + 2, chunk_data, CHUNK_SIZE);

	int rawPacket_length = 0;
	byte rawPacket[IMAGE_PACKET_SIZE];
	encode_TMpacket(rawPacket, &rawPacket_length, packet);
#ifdef TESTING
	return 0;
#else
	return TRX_sendFrame(rawPacket, rawPacket_length, trxvu_bitrate_9600);
#endif
}

/*
 * @brief		send image according to a bit field from ground
 * @param[in]	the id of the image in the database
 * @param[in]	the type of image requested
 * @param[in]	the id of the command who started the dump, just in case the dump stopped in the middle
 * @param[in]	the first chunk in the bit field array
 * @param[in]	the bit field. every bit represent a chunk, 1 means send this chunk 0 means skip this chunk.
 * 				the first bit is for @firstChunk_index.
 */
int bitField_imageDump(imageid image_id, fileType comprasionType, command_id cmdId, unsigned int firstChunk_index, byte packetsToSend[NUMBER_OF_CHUNKS_IN_CMD / 8])
{
	int error;

	char file_name[FILE_NAME_SIZE];
	GetImageFileName(image_id, comprasionType, file_name);

	F_FILE* current_file = f_open(file_name, "r");;

	int image_size = f_filelength(file_name);

	error = f_read(imageBuffer,(size_t)image_size,(size_t)1,current_file);
	check_int("f_read in bitField_imageDump", error);

	chunk_t chunk_data;
	for (unsigned int i = 0; i < NUMBER_OF_CHUNKS_IN_CMD; i++)
	{
		if (getBitValueByIndex(packetsToSend + i, NUMBER_OF_CHUNKS_IN_CMD / 8, i))
		{
			error = GetChunkFromImage(chunk_data, i + firstChunk_index, imageBuffer, comprasionType);
			if (error != BUTCHER_SUCCSESS)
				return error;

			error = buildAndSend_chunck(chunk_data, (unsigned short)(i + firstChunk_index), comprasionType, image_id);
			if (error < 0)
				return -1;

			lookForRequestToDelete_dump(cmdId);
			vTaskDelay(SYSTEM_DEALY);
		}
	}

	return 0;
}

int thumbnail_Dump(imageid* ID_list, command_id cmdId)
{
	unsigned int lastIndex = ceil((IMAGE_SIZE / ((2^DEFALT_REDUCTION_LEVEL)^2)) / CHUNK_SIZE);

	imageid image_id = 0;

	unsigned int numberOfIDs = 0;
	memcpy(&numberOfIDs, ID_list, sizeof(int));

	for (unsigned int j = 0; j < numberOfIDs; j++)
	{
		memcpy(&image_id, ID_list + ((j+1)*sizeof(imageid)), sizeof(imageid));

		char file_name[FILE_NAME_SIZE];
		GetImageFileName(image_id, DEFALT_REDUCTION_LEVEL, file_name);

		F_FILE* current_file = f_open(file_name, "r");;

		int image_size = f_filelength(file_name);

		f_read(imageBuffer,(size_t)image_size,(size_t)1,current_file);

		chunk_t chunks[lastIndex];
		GetChunksFromRange(chunks, 0, lastIndex, imageBuffer, DEFALT_REDUCTION_LEVEL);

		for (unsigned int i = 0; i < lastIndex; i++)
		{
			buildAndSend_chunck(chunks[i], (unsigned short)(i), DEFALT_REDUCTION_LEVEL, image_id);
			lookForRequestToDelete_dump(cmdId);
			vTaskDelay(SYSTEM_DEALY);
		}
	}

	free(ID_list);

	return 0;
}

int imageDataBase_Dump(byte* DataBase_buffer, command_id cmdId)
{
	unsigned int bufferSize = 0;
	memcpy(&bufferSize, DataBase_buffer, sizeof(int));

	for (unsigned short j = 0; (unsigned int)(j*IMAGE_DATA_FIELD_PACKET_SIZE) < bufferSize; j++)
	{
		byte* chunk = SimpleButcher(DataBase_buffer, bufferSize, IMAGE_DATA_FIELD_PACKET_SIZE, (unsigned int)j);

		TM_spl packet;

		packet.type = IMAGE_DATABASE_DUMP_T;
		packet.subType = j;
		Time_getUnixEpoch(&packet.time);
		packet.length = IMAGE_DATA_FIELD_PACKET_SIZE;	// ToDo: = 225 bytes, but where?
		memcpy(packet.data, chunk, IMAGE_DATA_FIELD_PACKET_SIZE);

		int rawPacket_length = 0;
		byte rawPacket[IMAGE_PACKET_SIZE];
		encode_TMpacket(rawPacket, &rawPacket_length, packet);

		TRX_sendFrame(rawPacket, rawPacket_length, trxvu_bitrate_9600);

		lookForRequestToDelete_dump(cmdId);
		vTaskDelay(SYSTEM_DEALY);
	}

	free(DataBase_buffer);

	return 0;
}

void imageDump_task(void* param)
{
	request_image request;
	memcpy(&request, param, sizeof(request_image));

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
		imageid image_id;
		memcpy(&image_id, request.command_parameters, sizeof(imageid));
		fileType comprasionType;
		memcpy(&comprasionType, request.command_parameters + 4, sizeof(byte));
		unsigned short firstIndex;
		memcpy(&firstIndex, request.command_parameters + 5, sizeof(short));

		byte packetsToSend[BIT_FIELD_SIZE];
		memcpy(packetsToSend, request.command_parameters + 7, BIT_FIELD_SIZE);

		bitField_imageDump(image_id, comprasionType, request.cmdId, firstIndex, packetsToSend);
	}

	else if (request.type == chunkField_imageDump_)
	{
		imageid image_id;
		memcpy(&image_id, request.command_parameters, sizeof(imageid));
		fileType comprasionType;
		memcpy(&comprasionType, request.command_parameters + 4, sizeof(byte));
		unsigned short firstIndex;
		memcpy(&firstIndex, request.command_parameters + 5, sizeof(short));
		unsigned short lastIndex;
		memcpy(&lastIndex, request.command_parameters + 7, sizeof(short));

		char file_name[FILE_NAME_SIZE];
		GetImageFileName(image_id, comprasionType, file_name);

		F_FILE* current_file = f_open(file_name, "r");;

		int image_size = f_filelength(file_name);

		f_read(imageBuffer,(size_t)image_size,(size_t)1,current_file);

		chunk_t chunks[lastIndex - firstIndex];
		GetChunksFromRange(chunks, firstIndex, lastIndex, imageBuffer, comprasionType);

		for (unsigned int i = 0; i < (unsigned int)(lastIndex - firstIndex); i++)
		{
			buildAndSend_chunck(chunks[i], (unsigned short)(i + firstIndex), comprasionType, image_id);
			lookForRequestToDelete_dump(request.cmdId);
		}
	}

	else if (request.type == thumbnail_imageDump_)
	{
		imageid* ID_list;
		memcpy(&ID_list, request.command_parameters, sizeof(imageid*));

		thumbnail_Dump(ID_list, request.cmdId);
	}
	else if (request.type == imageDataBaseDump_)
	{
		byte* DataBaseBuffer;
		memcpy(&DataBaseBuffer, request.command_parameters, sizeof(byte*));

		imageDataBase_Dump(DataBaseBuffer, request.cmdId);
	}

	set_system_state(dump_param, SWITCH_OFF);

	vTaskDelete(NULL);
}
