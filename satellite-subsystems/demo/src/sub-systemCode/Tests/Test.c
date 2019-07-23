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
#include "../Main/HouseKeeping.h"
#include "../COMM/GSC.h"
#include "../TRXVU.h"
#include "../EPS.h"
#include "../ADCS/Stage_Table.h"
#include "../Ants.h"

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
