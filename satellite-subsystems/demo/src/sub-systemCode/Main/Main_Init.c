/*
 * Main_Init.c
 *
 *  Created on: 15 2018
 *      Author: I7COMPUTER
 *
 *      purpose of module: this module will declare the main functions
 *      regarding the "BOOT" and "INIT" of the main;
 */
#include <satellite-subsystems/GomEPS.h>
#include <satellite-subsystems/IsisAntS.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <at91/utility/exithandler.h>
#include <at91/commons.h>
#include <at91/utility/trace.h>
#include <at91/peripherals/cp15/cp15.h>

#include <hcc/api_fat.h>

#include <hal/Timing/WatchDogTimer.h>
#include <hal/Timing/Time.h>

#include <hal/Drivers/I2C.h>
#include <hal/Drivers/LED.h>
#include <hal/Drivers/I2C.h>
#include <hal/Drivers/SPI.h>
#include <hal/errors.h>
#include <hal/boolean.h>

#include <hal/version/version.h>

#include <hal/Storage/FRAM.h>

#include <hal/Utility/util.h>

#include "../Global/Global.h"
#include "../Global/GlobalParam.h"
#include "../Global/TLM_management.h"
#include "../Global/OnlineTM.h"
#include "../Global/GenericTaskSave.h"
#include "../EPS.h"
#include "../Ants.h"
#include "../CUF/CUF.h"

#include "../ADCS/AdcsMain.h"

#include "../TRXVU.h"
#include "../payload/Request Management/CameraManeger.h"
#include "../payload/Request Management/AutomaticImageHandler.h"
#include "sub-systemCode/ADCS/AdcsMain.h"
#include "HouseKeeping.h"
#include "commands.h"

#include "../payload/Compression/ImageConversion.h"
#include "../payload/Compression/jpeg/ImgCompressor.h"
#include "../payload/Request Management/AutomaticImageHandler.h"

#define I2c_SPEED_Hz 100000
#define I2c_Timeout 10
#define I2c_TimeoutTest portMAX_DELAY
#define RESET_COUNT_FRAM_ID 666
// will Boot- deploy all  appropriate subsytems

void numberOfRestarts()
{
	byte raw[4];
	int err = FRAM_read_exte(raw, RESTART_FLAG_ADDR, 4);
	check_int("numberOfRestarts, FRAM_write_exte()", err);
	unsigned int *num = (unsigned int*)raw;
	(*num)++;
	err = FRAM_write_exte(raw, RESTART_FLAG_ADDR, 4);
	check_int("numberOfRestarts, FRAM_write_exte(++)", err);
	set_numOfResets(*num);
}

void StartFRAM()
{
	int error = FRAM_start();
}

void StartI2C()
{
	int error = I2C_start(I2c_SPEED_Hz, I2c_Timeout);
}

void StartSPI()
{
	int error = SPI_start(bus1_spi, slave1_spi);
}

void StartTIME()
{
	// starts time
	int error = 0;
	Time initial_time_jan2000={0,0,0,1,13,10,19,0};
	error = Time_start(&initial_time_jan2000,0);
	vTaskDelay(100);
	time_unix time_now;
	error = Time_getUnixEpoch(&time_now);
	check_int("StartTIME, Time_getUnixEpoch", error);
	// get last saved time before reset from FRAM
	time_unix set_time;
	byte raw_time[TIME_SIZE];
	FRAM_read_exte(raw_time, TIME_ADDR, TIME_SIZE);
	set_time = (time_unix)raw_time[0];
	set_time += (time_unix)(raw_time[1] << 8);
	set_time += (time_unix)(raw_time[2] << 16);
	set_time += (time_unix)(raw_time[3] << 24);
	//sets set_time in the time unit
	if(set_time > time_now)
	{
		error = Time_setUnixEpoch(set_time);
		check_int("StartTIME; Time_setUnixEpoch",error);
	}

}

Boolean first_activation()
{
	byte dataFRAM = FALSE_8BIT;
	FRAM_read_exte(&dataFRAM, FIRST_ACTIVATION_ADDR, 1);
	if (!dataFRAM)
	{
		return FALSE;
	}
	// 1. reset global FRAM adrees
	reset_FRAM_MAIN();
	// 2 reset TRXVU FRAM adress
	reset_FRAM_TRXVU();
	// 3. reset TRXVU FRAM adress
	reset_FRAM_EPS();

	reset_offline_TM_list();
	// 3. cahnge the first activation to false
	dataFRAM = FALSE_8BIT;

	FRAM_write_exte(&dataFRAM, FIRST_ACTIVATION_ADDR, 1);
	return TRUE;
}

void resetSD()
{
	F_FIND find;
	unsigned int selection = 0;
	if (!f_findfirst("A:/*.*",&find))
	{
		do
		{
			printf ("filename:%s\n ",find.filename);
		} while (!f_findnext(&find));
	}
	if (!f_findfirst("A:/*.*",&find))
	{
		do
		{
			gom_eps_hk_t eps_tlm;
			GomEpsGetHkData_general(0, &eps_tlm);

			printf ("filename:%s\n ",find.filename);
			printf("\t 0) next file \n\r");
			printf("\t 1) delete and next file \n\r");
			while(UTIL_DbguGetIntegerMinMax(&selection, 0, 13) == 0);
			if(selection == 1)
			{
				f_delete(find.filename);
			}
			selection =0;
		} while (!f_findnext(&find));
	}
}

int InitSubsystems()
{
	StartI2C();

	StartSPI();

	StartFRAM();

	init_onlineParam();

	init_GenericSaveQueue();

	Boolean activation = first_activation();

	StartTIME();

	init_GP();

	initAutomaticImageHandlerTask();

	numberOfRestarts();

	EPS_Init();

	InitializeFS(activation);

	CUFManageRestart(activation);

#ifdef ANTS_ON
	init_Ants(activation);
#endif

	AdcsInit();

	init_trxvu();

	initCamera(activation);

	init_command();

	return 0;
}

// this function initializes all neccesary subsystem tasks in main
int SubSystemTaskStart()
{
	xTaskCreate(TRXVU_task, (const signed char*)("TRX"), 8192, NULL, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, NULL);
	vTaskDelay(100);

	xTaskCreate(save_onlineTM_task, (const signed char*)("OnlineTM"), 8192, NULL, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, NULL);
	vTaskDelay(100);

	xTaskCreate(GenericSave_Task, (const signed char*)("generic save task"), 8192, NULL, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, NULL);
	vTaskDelay(100);

	KickStartCamera();
	vTaskDelay(100);

	KickStartAutomaticImageHandlerTask();
	vTaskDelay(100);

	xTaskCreate(AdcsTask, (const signed char*)("ADCS"), 8192, NULL, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, NULL);
	return 0;
}




