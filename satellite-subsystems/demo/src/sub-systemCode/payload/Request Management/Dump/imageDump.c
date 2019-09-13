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
#include "../../../Global/TLM_management.h"

#include "Butchering.h"
#include "../../Misc/Macros.h"
#include "../../Misc/FileSystem.h"

#include "imageDump.h"

uint16_t chunk_width;
uint16_t chunk_height;

ImageDataBaseResult readChunkDimentionsFromFRAM(void)
{
	int error = 0;
	error = FRAM_read((unsigned char*)&chunk_width, IMAGE_CHUNK_WIDTH_ADDR, IMAGE_CHUNK_WIDTH_SIZE);
	CMP_AND_RETURN(error, 0, DataBaseFramFail);
	error = FRAM_read((unsigned char*)&chunk_height, IMAGE_CHUNK_HEIGHT_ADDR, IMAGE_CHUNK_HEIGHT_SIZE);
	CMP_AND_RETURN(error, 0, DataBaseFramFail);

	return DataBaseSuccess;
}

ImageDataBaseResult setChunkDimensions_inFRAM(uint16_t width, uint16_t height)
{
	if (width * height > MAX_CHUNK_SIZE)
		return Butcher_Parameter_Value;

	int error = 0;
	error = FRAM_write((unsigned char*)&width, IMAGE_CHUNK_WIDTH_ADDR, IMAGE_CHUNK_WIDTH_SIZE);
	CMP_AND_RETURN(error, 0, DataBaseFramFail);
	error = FRAM_write((unsigned char*)&height, IMAGE_CHUNK_HEIGHT_ADDR, IMAGE_CHUNK_HEIGHT_SIZE);
	CMP_AND_RETURN(error, 0, DataBaseFramFail);

	error = readChunkDimentionsFromFRAM();
	CMP_AND_RETURN(error, 0, DataBaseFramFail);

	CMP_AND_RETURN(chunk_width, width, DataBaseFail);
	CMP_AND_RETURN(chunk_height, height, DataBaseFail);

	return DataBaseSuccess;
}

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
ImageDataBaseResult buildAndSend_chunck(pixel_t* chunk_data, unsigned short chunk_index, fileType comprasionType, imageid id, uint32_t image_size, Boolean8bit isImage)
{
	TM_spl packet;

	if (isImage)
	{
		packet.type = IMAGE_DUMP_T;
		packet.length = IMAGE_PACKET_DATA_FIELD_SIZE(CHUNK_SIZE(chunk_height, chunk_width));
	}
	else
	{
		packet.type = IMAGE_DATABASE_DUMP_T;
		packet.length = IMAGE_DB_PACKET_DATA_FIELD_SIZE(CHUNK_SIZE(chunk_height, chunk_width));
	}

	packet.subType = find_subType_ofImage(comprasionType, chunk_index, isImage);
	Time_getUnixEpoch(&packet.time);

	uint32_t offset = 0;

	if(isImage)
	{
		memcpy(packet.data + offset, &id, sizeof(imageid));
		offset += sizeof(imageid);
	}

	memcpy(packet.data + offset, &chunk_index, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	if (packet.subType == IMAGE_DUMP_JPG_FIRST_CHUNK_ST || packet.subType == IMAGE_DATABASE_DUMP_FIRST_CHUNK_ST)
	{
		memcpy(packet.data + offset, &image_size, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		packet.length += 4;
	}

	memcpy(packet.data + offset, chunk_data, CHUNK_SIZE(chunk_height, chunk_width));
	offset += CHUNK_SIZE(chunk_height, chunk_width);

	int rawPacket_length = 0;
	byte rawPacket[packet.length + SPL_TM_HEADER_SIZE];
	encode_TMpacket(rawPacket, &rawPacket_length, packet);

	return TRX_sendFrame(rawPacket, rawPacket_length);
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
ImageDataBaseResult bitField_imageDump(imageid image_id, fileType comprasionType, command_id cmdId, unsigned int firstChunk_index, byte packetsToSend[BIT_FIELD_SIZE])
{
	char file_name[FILE_NAME_SIZE];
	GetImageFileName(image_id, comprasionType, file_name);

	F_FILE *file = NULL;
	int error = f_managed_open(file_name, "r", &file);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	uint32_t image_size = f_filelength(file_name);
	byte* buffer = imageBuffer;

	error = ReadFromFile(file, buffer, image_size, 1);
	check_int("f_read in bitField_imageDump", error);

	error = f_managed_close(&file);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	pixel_t chunk[CHUNK_SIZE(chunk_height, chunk_width)];
	for (unsigned int i = 0; i < NUMBER_OF_CHUNKS_IN_CMD; i++)
	{
		if (getBitValueByIndex(packetsToSend, BIT_FIELD_SIZE, i))
		{
			error = GetChunkFromImage(chunk, chunk_width, chunk_height, i + firstChunk_index, imageBuffer, comprasionType, image_size);
			CMP_AND_RETURN(error, BUTCHER_SUCCSESS, Butcher_Success + error);

			error = buildAndSend_chunck(chunk, (unsigned short)(i + firstChunk_index), comprasionType, image_id, image_size, TRUE_8BIT);
			CMP_AND_RETURN(error, 0, -1);

			lookForRequestToDelete_dump(cmdId);
			vTaskDelay(SYSTEM_DEALY);
		}
	}

	return 0;
}

ImageDataBaseResult imageDataBase_Dump(Camera_Request request, byte buffer[], uint32_t size)
{
	int error;

	uint32_t image_packet_data_field_size = CHUNK_SIZE(chunk_height, chunk_width);

	byte* chunk;
	for (unsigned short j = 0; (unsigned int)(j*image_packet_data_field_size) < size; j++)
	{
		chunk = SimpleButcher(buffer, size, image_packet_data_field_size, (unsigned int)j);
		CHECK_FOR_NULL(chunk, Butcher_Null_Pointer);

		error = buildAndSend_chunck(chunk, j, 0, 0, size, FALSE_8BIT);
		CMP_AND_RETURN(error, 0, -1);

		free(chunk);

		lookForRequestToDelete_dump(request.cmd_id);
	}

	return 0;
}

ImageDataBaseResult chunkField_imageDump(Camera_Request request, imageid image_id, fileType comprasionType, uint16_t firstIndex, uint16_t lastIndex)
{
	char file_name[FILE_NAME_SIZE];
	GetImageFileName(image_id, comprasionType, file_name);

	F_FILE* current_file = NULL;
	int error = f_managed_open(file_name, "r", &current_file);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	uint32_t image_size = f_filelength(file_name);
	byte* buffer = imageBuffer;

	error = ReadFromFile(current_file, buffer, image_size, 1);
	check_int("f_read in bitField_imageDump", error);

	error = f_managed_close(&current_file);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	pixel_t chunk[CHUNK_SIZE(chunk_width, chunk_height)];
	for (unsigned short i = firstIndex; i < lastIndex; i++)
	{
		GomEpsResetWDT(0);

		error = GetChunkFromImage(chunk, chunk_width, chunk_height, i, buffer, comprasionType, image_size);
		CMP_AND_RETURN(error, BUTCHER_SUCCSESS, Butcher_Success + error);

		error = buildAndSend_chunck(chunk, i, comprasionType, image_id, image_size, TRUE_8BIT);
		CMP_AND_RETURN(error, 0, -1);

		lookForRequestToDelete_dump(request.cmd_id);
	}

	return DataBaseSuccess;
}

void imageDump_task(void* param)
{
	Camera_Request request;
	memcpy(&request, param, sizeof(Camera_Request));

	int f_error = f_managed_enterFS();
	check_int("error in Dump_task, f_managed_enterFS - data abort exeption\n", f_error);
	if (f_error == COULD_NOT_TAKE_SEMAPHORE_ERROR)
	{
		f_error = f_managed_enterFS();
		if (f_error != 0)
		{
			vTaskDelete(NULL);
		}
	}
	else
	{
		vTaskDelete(NULL);
	}

	if (get_system_state(dump_param))
	{
		//	exit dump task and saves ACK
		save_ACK(ACK_DUMP, ERR_TASK_EXISTS, request.cmd_id);
		terminateTask();
	}
	else
	{
		set_system_state(dump_param, SWITCH_ON);
	}
	// ToDo: error log

	vTaskDelay(SYSTEM_DEALY);

	xQueueReset(xDumpQueue);

	int error = readChunkDimentionsFromFRAM();

	if (request.id == Image_Dump_bitField)
	{
		imageid image_id;
		memcpy(&image_id, request.data, sizeof(imageid));
		fileType comprasionType;
		memcpy(&comprasionType, request.data + sizeof(imageid), sizeof(byte));
		uint16_t firstIndex;
		memcpy(&firstIndex, request.data + sizeof(imageid) + sizeof(byte), sizeof(uint16_t));

		byte packetsToSend[BIT_FIELD_SIZE];
		memcpy(packetsToSend, request.data + sizeof(imageid) + sizeof(byte) + sizeof(uint16_t), BIT_FIELD_SIZE);

		printf("\n\n");
		unsigned int i;
		for (i = 0; i < BIT_FIELD_SIZE; i++)
		{
			printf("%u ", packetsToSend[i]);
		}
		printf("\n\n");

		error = bitField_imageDump(image_id, comprasionType, request.cmd_id, firstIndex, packetsToSend);
	}
	else if (request.id == Image_Dump_chunkField)
	{
		imageid image_id;
		memcpy(&image_id, request.data, sizeof(imageid));
		fileType comprasionType;
		memcpy(&comprasionType, request.data + sizeof(imageid), sizeof(byte));
		unsigned short firstIndex;
		memcpy(&firstIndex, request.data + sizeof(imageid)+sizeof(byte), sizeof(short));
		unsigned short lastIndex;
		memcpy(&lastIndex, request.data + sizeof(imageid)+sizeof(byte)+sizeof(short), sizeof(short));

		error = chunkField_imageDump(request, image_id, comprasionType, firstIndex, lastIndex);
	}
	else if (request.id == DataBase_Dump)
	{
		time_unix startingTime = *((time_unix*)request.data);
		memcpy(&startingTime, request.data, sizeof(time_unix));
		time_unix endTime = *((time_unix*)request.data + 4);
		memcpy(&endTime, request.data + 4, sizeof(time_unix));

		byte DataBaseBuffer[getDataBaseSize()];
		uint32_t database_size = 0;
		error = getImageDataBaseBuffer(startingTime, endTime, DataBaseBuffer, &database_size);

		if(error == DataBaseSuccess)
		{
			error = imageDataBase_Dump(request, DataBaseBuffer, database_size);
		}
		else
		{
			error = DataBaseNullPointer;
			// ToDo: log error!
		}
	}

	// ToDo: error log!
	printf("\n-IMAGE DUMP- ERROR (%u), type (%u)\n\n", error, request.id);

	set_system_state(dump_param, SWITCH_OFF);
	terminateTask();
}
