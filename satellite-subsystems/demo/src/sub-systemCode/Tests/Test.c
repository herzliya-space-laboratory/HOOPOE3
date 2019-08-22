/*
 * Test.c
 *
 *  Created on: Mar 3, 2019
 *      Author: elain
 */
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <hal/Timing/Time.h>

#include <hal/Utility/util.h>

#include <hal/Storage/FRAM.h>

#include <hcc/api_fat.h>
#include <hcc/api_hcc_mem.h>
#include <hcc/api_fat_test.h>
#include <hcc/api_mdriver_atmel_mcipdc.h>

#include <string.h>

#include <stdio.h>

#include "Test.h"

#include "../COMM/splTypes.h"
#include "../Global/Global.h"
#include "../Global/GlobalParam.h"
#include "../Main/HouseKeeping.h"
#include "../COMM/GSC.h"
#include "../COMM/imageDump.h"
#include "../TRXVU.h"
#include "../EPS.h"
#include "../ADCS/Stage_Table.h"
#include "../Ants.h"
#include "../payload/DataBase.h"

#include <hal/Drivers/SPI.h>
#include <satellite-subsystems/SCS_Gecko/gecko_driver.h>
#include <satellite-subsystems/SCS_Gecko/gecko_use_cases.h>
#include <satellite-subsystems/IsisAntS.h>

void printDeploymentStatus(unsigned char antenna_id, unsigned char status)
{
	printf("Antenna %d: ", antenna_id);
	if(status == 0)
	{
		printf("deployed\n\r");
	}
	else
	{
		printf("undeployed\n\r");
	}
}

void readAntsState()
{
	ISISantsStatus param;
	IsisAntS_getStatusData(0, isisants_sideA, &param);
	printf("side A:\n");
	printf(" current deployment status 0x%x 0x%x (raw value) \r\n", param.raw[0], param.raw[1]);
	printf("Arm status: %s \r\n", param.fields.armed==0?"disarmed":"armed");
	printDeploymentStatus(1, param.fields.ant1Undeployed);
	printDeploymentStatus(2, param.fields.ant2Undeployed);
	printDeploymentStatus(3, param.fields.ant3Undeployed);
	printDeploymentStatus(4, param.fields.ant4Undeployed);
	printf("Override: %s \r\n", param.fields.ignoreFlag==0?"inactive":"active");
	printf("\n\n");

	IsisAntS_getStatusData(0, isisants_sideB, &param);
	printf("side B:\n");
	printf(" current deployment status 0x%x 0x%x (raw value) \r\n", param.raw[0], param.raw[1]);
	printf("Arm status: %s \r\n", param.fields.armed==0?"disarmed":"armed");
	printDeploymentStatus(1, param.fields.ant1Undeployed);
	printDeploymentStatus(2, param.fields.ant2Undeployed);
	printDeploymentStatus(3, param.fields.ant3Undeployed);
	printDeploymentStatus(4, param.fields.ant4Undeployed);
	printf("Override: %s \r\n", param.fields.ignoreFlag==0?"inactive":"active");
}

void collect_TM_during_deploy()
{
	for (int i = 0; i < NUMBER_OF_ANTS * DEFFULT_DEPLOY_TIME; i++)
	{
		save_HK();
		vTaskDelay(1000);
	}
}

void reset_FIRST_activation()
{
	byte dataFRAM = TRUE_8BIT;
	FRAM_write(&dataFRAM, FIRST_ACTIVATION_ADDR, 1);
}

typedef union __attribute__ ((__packed__)) Chunk_Protocol
{
	byte raw[CHUNK_SIZE + 2];
	struct __attribute__ ((__packed__))
	{
			unsigned short ChunkNum;
			chunk_t data;
	}fields;
}Chunk_Protocol;

pixel_t arr[IMAGE_SIZE] = {0};
void GetChunkFromImageTest(chunk_t chunk, unsigned short index, pixel_t image[IMAGE_SIZE], unsigned int ImageSize);

void ButcheringTest(unsigned int StartIndex, unsigned int EndIndex, unsigned int id)
{
	F_FILE *fp = f_open("gecko.raw", "r");
	f_read(arr, IMAGE_SIZE, 1, fp);
	//TODO: return  value checking...
	byte raw_packet[SIZE_TXFRAME];
	int size_raw_packet;

	TM_spl packet;
	packet.length =  CHUNK_SIZE + 2;
	packet.time = id;
	packet.type = IMAGE_DUMP_T;
	packet.subType = IMAGE_DUMP_THUMBNAIL4_ST;

	Chunk_Protocol Chunk;
	for(unsigned int i = StartIndex; i <= EndIndex; i++)
	{
		//divides into chunks
		GetChunkFromImageTest(Chunk.fields.data, i, arr, 8704);
		Chunk.fields.ChunkNum = (unsigned short)i;
		//packs and sends chunks
		memcpy(packet.data, Chunk.raw, packet.length);
		encode_TMpacket(raw_packet, &size_raw_packet, packet);
		TRX_sendFrame(raw_packet, size_raw_packet, trxvu_bitrate_9600);
	}
}

void GetChunkFromImageTest(chunk_t chunk, unsigned short index, pixel_t image[IMAGE_SIZE], unsigned int ImageSize)
{
	if ((chunk == NULL) || (image == NULL))
	{
		return;
	}
	for (int i = 0; i < CHUNK_SIZE; i++)
	{
		if ((unsigned int)index * CHUNK_SIZE + i > ImageSize)
		{
			chunk[i] = 0;
		}
		else
		{
			chunk[i] = image[index * CHUNK_SIZE + i];
		}
	}
}

void GetChunkFromImageReal(chunk_t chunk, unsigned short index, pixel_t image[IMAGE_SIZE], fileType im_type, unsigned int ChunkHeight, unsigned int ChunkWidth)
{
	if ((chunk == NULL) || (image == NULL))
	{
		return;
	}
	unsigned int factor = 1;
	switch(im_type)
	{
		case raw: factor = 1; break;
		case t02: factor = 2; break;
		case t04: factor = 4; break;
		case t08: factor = 8; break;
		case t16: factor = 16; break;
		case t32: factor = 32; break;
		case t64: factor = 64; break;
		default: return;
	}
	if ((ChunkHeight % factor != 0) || (ChunkWidth % factor != 0) || (IMAGE_WIDTH % ChunkWidth != 0) || (ChunkHeight > IMAGE_HEIGHT) || (ChunkWidth > IMAGE_WIDTH) || (index > IMAGE_SIZE / ChunkHeight / ChunkWidth + 1))
	{
		return;
	}
	unsigned int RemainingChunk = IMAGE_HEIGHT % ChunkHeight;
	unsigned int IndexWidth;
	unsigned int IndexHeight;
	if (index == IMAGE_SIZE / ChunkHeight / ChunkWidth + 1)
	{
		IndexWidth = 0;
		IndexHeight = IMAGE_SIZE - RemainingChunk;
	}
	IndexWidth = index % (IMAGE_WIDTH / ChunkWidth);
	IndexHeight = index / (IMAGE_WIDTH / ChunkWidth);

}

Boolean CompareButchering(pixel_t arr1[IMAGE_SIZE], pixel_t arr2[IMAGE_SIZE])
{
	for (int i = 0; i < IMAGE_SIZE; i++)
	{
		if (arr1[i] != arr2[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}

void imageDump_test(void* param)
{
	unsigned int StartIndex = *((unsigned int*)param);
	unsigned int EndIndex = *((unsigned int*)param + 4);
	unsigned int id = *((unsigned int*)param + 8);
	fileType CompressionType = (fileType)*((unsigned char*)(param + 12));

	vTaskDelay(SYSTEM_DEALY);

	if (get_system_state(dump_param))
	{
		//	exit dump task and saves ACK
		save_ACK(ACK_DUMP, ERR_TASK_EXISTS, id);
		vTaskDelete(NULL);
	}
	else
	{
		set_system_state(dump_param, SWITCH_ON);
	}

	// 2. check if parameters are legal
	if (StartIndex > EndIndex)
	{
		save_ACK(ACK_DUMP, ERR_PARAMETERS, id);
		set_system_state(dump_param, SWITCH_OFF);
	}
	else
	{
		int ret = f_enterFS();
		check_int("Dump task enter FileSystem", ret);
		vTaskDelay(SYSTEM_DEALY);
		xQueueReset(xDumpQueue);
		ButcheringTest(StartIndex, EndIndex, id);
	}

	set_system_state(dump_param, SWITCH_OFF);
	vTaskDelete(NULL);
}
