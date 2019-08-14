/*
 * imageDump.c
 *
 *  Created on: Jun 23, 2019
 *      Author: elain
 */
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <satellite-subsystems/IsisTRXVU.h>
#include <satellite-subsystems/GomEPS.h>

#include <Math.h>

#include <hcc/api_fat.h>

#include "../../../COMM/GSC.h"
#include "../../../TRXVU.h"
#include "../../../Main/HouseKeeping.h"
#include "../../../Global/TM_managment.h"

#include "Butchering.h"
#include "../../DataBase/DataBase.h"
#include "../../Misc/Macros.h"

#include "imageDump.h"

image_t imageBuffer;

/*
 * @brief		find the subType for the image
 * @param[in]	the type of image
 * @return		the number of subType for the image type
 */
uint8_t find_subType_ofImage(fileType comprasionType, unsigned short chunk_index, Boolean8bit isImage)
{
	if (!isImage)
	{
		if (chunk_index == 0)
			return IMAGE_DATABASE_DUMP_FIRST_CHUNK_ST;
		else
			return IMAGE_DATABASE_DUMP_ST;
	}
	else
	{
		switch (comprasionType)
		{
			case raw:
				return IMAGE_DUMP_RAW_ST;
				break;
			case jpg:
				if (chunk_index == 0)
					return IMAGE_DUMP_JPG_FIRST_CHUNK_ST;
				else
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
int buildAndSend_chunck(chunk_t chunk_data, unsigned short chunk_index, fileType comprasionType, imageid id, uint32_t image_size, Boolean8bit isImage)
{
	TM_spl packet;

	if (isImage)
	{
		packet.type = IMAGE_DUMP_T;
		packet.length = IMAGE_DATA_FIELD_PACKET_SIZE;
	}
	else
	{
		packet.type = IMAGE_DATABASE_DUMP_T;
		packet.length = IMAGE_DB_DATA_FIELD_PACKET_SIZE;
	}

	packet.subType = find_subType_ofImage(comprasionType, chunk_index, isImage);
	Time_getUnixEpoch(&packet.time);

	uint32_t offset = 0;

	memcpy(packet.data + offset, &id, sizeof(imageid));
	offset += sizeof(imageid);
	memcpy(packet.data + offset, &chunk_index, sizeof(uint16_t));
	offset += sizeof(uint16_t);
	if (packet.subType == IMAGE_DUMP_JPG_FIRST_CHUNK_ST || packet.subType == IMAGE_DATABASE_DUMP_FIRST_CHUNK_ST)
	{
		memcpy(packet.data + offset, &image_size, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		packet.length += 4;
	}
	memcpy(packet.data + offset, chunk_data, CHUNK_SIZE);
	offset += CHUNK_SIZE;

	int rawPacket_length = 0;
	byte rawPacket[packet.length + SPL_TM_HEADER_SIZE];
	encode_TMpacket(rawPacket, &rawPacket_length, packet);

	return TRX_sendFrame(rawPacket, rawPacket_length, trxvu_bitrate_9600);
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

	uint32_t image_size = f_filelength(file_name);

	error = f_read(imageBuffer,(size_t)image_size,(size_t)1,current_file);
	check_int("f_read in bitField_imageDump", error);

	chunk_t chunk_data;
	for (unsigned int i = 0; i < NUMBER_OF_CHUNKS_IN_CMD; i++)
	{
		if (getBitValueByIndex(packetsToSend + i, NUMBER_OF_CHUNKS_IN_CMD / 8, i))
		{
			error = GetChunkFromImage(chunk_data, i + firstChunk_index, imageBuffer, comprasionType, image_size);
			if (error != BUTCHER_SUCCSESS)
				return error;

			error = buildAndSend_chunck(chunk_data, (unsigned short)(i + firstChunk_index), comprasionType, image_id, image_size, TRUE_8BIT);
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
	unsigned int lastIndex = ceil(IMAGE_SIZE / pow( pow(2, DEFALT_REDUCTION_LEVEL), 2) / CHUNK_SIZE);

	imageid image_id = 0;

	unsigned int numberOfIDs = 0;
	memcpy(&numberOfIDs, ID_list, sizeof(int));

	for (unsigned int j = 0; j < numberOfIDs; j++)
	{
		memcpy(&image_id, ID_list + ((j+1)*sizeof(imageid)), sizeof(imageid));

		char file_name[FILE_NAME_SIZE];
		GetImageFileName(image_id, DEFALT_REDUCTION_LEVEL, file_name);

		F_FILE* current_file = f_open(file_name, "r");;

		uint32_t image_size = f_filelength(file_name);

		f_read(imageBuffer,(size_t)image_size,(size_t)1,current_file);

		chunk_t chunks[lastIndex];
		GetChunksFromRange(chunks, 0, lastIndex, imageBuffer, DEFALT_REDUCTION_LEVEL, image_size);

		for (unsigned int i = 0; i < lastIndex; i++)
		{
			buildAndSend_chunck(chunks[i], (unsigned short)(i), DEFALT_REDUCTION_LEVEL, image_id, image_size, TRUE_8BIT);
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

	chunk_t chunk;
	byte* chunk_;

	for (unsigned short j = 0; (unsigned int)(j*IMAGE_DATA_FIELD_PACKET_SIZE) < bufferSize; j++)
	{
		chunk_ = SimpleButcher(DataBase_buffer, bufferSize, IMAGE_DATA_FIELD_PACKET_SIZE, (unsigned int)j);
		memcpy(chunk, chunk_, CHUNK_SIZE);
		free(chunk_);

		buildAndSend_chunck(chunk, j, 0, 0, bufferSize, FALSE_8BIT);
	}

	free(DataBase_buffer);

	return 0;
}

ImageDataBaseResult chunkField_imageDump(request_image request, imageid image_id, fileType comprasionType, uint16_t firstIndex, uint16_t lastIndex)
{
	char file_name[FILE_NAME_SIZE];
	int result = GetImageFileName(image_id, comprasionType, file_name);
	CMP_AND_RETURN(result, DataBaseSuccess, result);

	F_FILE* current_file;
	OPEN_FILE(current_file, file_name, "r", DataBaseFileSystemError);

	uint32_t image_size = f_filelength(file_name);

	uint8_t* buffer = imageBuffer;

	READ_FROM_FILE(current_file, buffer, image_size, 1, DataBaseFileSystemError);

	chunk_t chunk;

	for (unsigned short i = firstIndex; i < lastIndex; i++)
	{
		GomEpsResetWDT(0);

		result = GetChunkFromImage(chunk, i, imageBuffer, comprasionType, image_size);
		CMP_AND_RETURN(result, BUTCHER_SUCCSESS, Butcher_Success + result);

		buildAndSend_chunck(chunk, i, comprasionType, image_id, image_size, TRUE_8BIT);
		lookForRequestToDelete_dump(request.cmdId);
	}

	return DataBaseSuccess;
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

	int err = IsisTrxvu_tcSetAx25Bitrate(0, trxvu_bitrate_9600);
	check_int("IsisTrxvu_tcSetAx25Bitrate, image dump", err);

	if (request.type == bitField_imageDump_)
	{
		imageid image_id;
		memcpy(&image_id, request.command_parameters, sizeof(imageid));
		fileType comprasionType;
		memcpy(&comprasionType, request.command_parameters + 2, sizeof(byte));
		uint16_t firstIndex;
		memcpy(&firstIndex, request.command_parameters + 3, sizeof(uint16_t));

		byte packetsToSend[BIT_FIELD_SIZE];
		memcpy(packetsToSend, request.command_parameters + 5, BIT_FIELD_SIZE);

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

		chunkField_imageDump(request, image_id, comprasionType, firstIndex, lastIndex);
	}
	else if (request.type == thumbnail_imageDump_)
	{
		time_unix startingTime = *((time_unix*)request.command_parameters);
		memcpy(&startingTime, request.command_parameters, sizeof(time_unix));
		time_unix endTime = *((time_unix*)request.command_parameters + 4);
		memcpy(&endTime, request.command_parameters + 4, sizeof(time_unix));

		imageid* ID_list = get_ID_list_withDefaltThumbnail(startingTime, endTime);

		thumbnail_Dump(ID_list, request.cmdId);
	}
	else if (request.type == imageDataBaseDump_)
	{
		time_unix startingTime = *((time_unix*)request.command_parameters);
		memcpy(&startingTime, request.command_parameters, sizeof(time_unix));
		time_unix endTime = *((time_unix*)request.command_parameters + 4);
		memcpy(&endTime, request.command_parameters + 4, sizeof(time_unix));

		byte* DataBaseBuffer = getImageDataBaseBuffer(startingTime, endTime);

		imageDataBase_Dump(DataBaseBuffer, request.cmdId);
	}

	set_system_state(dump_param, SWITCH_OFF);

	vTaskDelete(NULL);
}
