/*
 * Test.c
 *
 *  Created on: Mar 3, 2019
 *      Author: Hoopoe3n
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
	byte raw[SIZE_TXFRAME - SPL_TM_HEADER_SIZE];
	struct __attribute__ ((__packed__))
	{
			unsigned int id;
			unsigned int ChunkNum;
			unsigned char Compress;
			chunk_t data;
	}fields;
}Chunk_Protocol;

void ButcheringTest(unsigned int StartIndex, unsigned int EndIndex, unsigned int id, fileType CompressionType)
{
	pixel_t arr[IMAGE_SIZE] = {0};
	F_FILE *fp = f_open("gecko.raw", "r");
	f_read(arr, IMAGE_SIZE, 1, fp);
	//TODO: return  value checking...

	//TODO: fix the values that he send
	TM_spl packet;
	packet.length =  SIZE_TXFRAME - SPL_TM_HEADER_SIZE;
	packet.time = 0;
	packet.type = 16;
	packet.subType = 55;

	Chunk_Protocol Chunk;
	for(unsigned int i = StartIndex; i <= EndIndex; i++)
	{
		//divides into chunks
		GetChunkFromImage(Chunk.fields.data, i, arr, CompressionType);
		Chunk.fields.ChunkNum = i;
		Chunk.fields.id = id;
		Chunk.fields.Compress = (unsigned char)CompressionType;
		//packs and sends chunks
		TRX_sendFrame(Chunk.raw, CHUNK_SIZE, trxvu_bitrate_9600);
	}
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
		ButcheringTest(StartIndex, EndIndex, id, CompressionType);
	}

	set_system_state(dump_param, SWITCH_OFF);
	vTaskDelete(NULL);
}
