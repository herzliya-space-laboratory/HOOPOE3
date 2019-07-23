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

#include <hal/Timing/WatchDogTimer.h>
#include <hal/Timing/Time.h>

#include <hal/Drivers/I2C.h>
#include <hal/Drivers/LED.h>
#include <hal/Drivers/I2C.h>
#include <hal/Drivers/SPI.h>

#include <hal/boolean.h>

#include <hal/version/version.h>

#include <hal/Storage/FRAM.h>

#include <hal/Utility/util.h>

#include "../Global/Global.h"
#include "../Global/GlobalParam.h"
#include "../Global/TM_managment.h"
#include "../EPS.h"
#include "../Ants.h"
#include "../ADCS.h"
#include "../ADCS/Stage_Table.h"
#include "../TRXVU.h"
#include "../payload/CameraManeger.h"
#include "HouseKeeping.h"
#include "commands.h"

#ifdef TESTING
#include "../Tests/Test.h"
#endif

#define I2c_SPEED_Hz 100000
#define I2c_Timeout 10
#define I2c_TimeoutTest portMAX_DELAY
#define RESET_COUNT_FRAM_ID 666
// will Boot- deploy all  appropriate subsytems

//extern unsigned short* Vbat_Prv;
stageTable ST;

#ifdef TESTING
void test_menu()
{
	byte dataFRAM = FALSE_8BIT;
	FRAM_write(&dataFRAM, FIRST_ACTIVATION_ADDR, 1);

	Boolean exit = FALSE;
	unsigned int selection;
	while (!exit)
	{
		printf( "\n\r Select a test to perform: \n\r");
		printf("\t 0) continue to code\n\r");
		printf("\t 1) set first activation flag to TRUE\n\r");
		printf("\t 2) write data to FRAM \n");

		exit = FALSE;
		while(UTIL_DbguGetIntegerMinMax(&selection, 0, 1) == 0);

		switch(selection)
		{
		case 0:
			exit = TRUE;
			break;
		case 1:
			reset_FIRST_activation();
			break;
		}
	}
}
#endif

void numberOfRestarts()
{
	byte raw[4];
	int err = FRAM_read(raw, RESTART_FLAG_ADDR, 4);
	check_int("numberOfRestarts, FRAM_write()", err);
	unsigned int *num = (unsigned int*)raw;
	(*num)++;
	err = FRAM_write(raw, RESTART_FLAG_ADDR, 4);
	check_int("numberOfRestarts, FRAM_write(++)", err);
	set_numOfResets(*num);
}

void StartFRAM()
{
	int error = FRAM_start();
	if(0 != error )
	{
		printf("error in FRAM_start(); err = %d\n",error);
	}
}

void StartI2C()
{
	int error = I2C_start(I2c_SPEED_Hz, I2c_Timeout);
	if(0 != error )
	{
		printf("error in I2C_start; err = %d\n",error);
	}
}

void StartSPI()
{
	int error = SPI_start(bus1_spi, slave1_spi);
	if(0 != error )
	{
		printf("error in SPI_start; err = %d\n",error);
	}
}

void StartTIME()
{
	// starts time
	int error = 0;
	Time initial_time_jan2000={0,0,0,1,1,1,0,0};
	error = Time_start(&initial_time_jan2000,0);
	if(0 != error )
	{
		printf("error in Time_start; err = %d\n",error);
	}
	time_unix time_now;
	error = Time_getUnixEpoch(&time_now);
	check_int("StartTIME, Time_getUnixEpoch", error);
	// get last saved time before reset from FRAM
	time_unix set_time;
	byte raw_time[TIME_SIZE];
	FRAM_read(raw_time, TIME_ADDR, TIME_SIZE);
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
	FRAM_read(&dataFRAM, FIRST_ACTIVATION_ADDR, 1);
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
	// 3. cahnge the first activation to false
	dataFRAM = FALSE_8BIT;

	FRAM_write(&dataFRAM, FIRST_ACTIVATION_ADDR, 1);
	return TRUE;
}

int InitSubsystems()
{
	StartI2C();

	StartSPI();

	StartFRAM();

#ifdef TESTING
	test_menu();
#endif

	Boolean activation = first_activation();

	StartTIME();

	init_GP();

	numberOfRestarts();

	InitializeFS(activation);

	create_files(activation);

	EPS_Init();

#ifdef ANTS_ON
	init_Ants();

	printf("before deploy ");
	readAntsState();

	Auto_Deploy();

	readAntsState();
	printf("after deploy ");
#endif

	ADCS_startLoop(activation);

	init_trxvu();

	initCamera(activation);

	init_command();

	return 0;
}

void task_delay()
{
	while(1)
	{
		vTaskDelay(SYSTEM_DEALY);
	}
}

// this function initializes all neccesary subsystem tasks in main
int SubSystemTaskStart()
{


	xTaskCreate(TRXVU_task, (const signed char*)("TRXVU_TASK_Debug"), 8192, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), NULL);
	vTaskDelay(100);

	xTaskCreate(HouseKeeping_Task, (const signed char*)("HouseKeeping_Task"), 8192, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), NULL);
	xTaskCreate(HouseKeeping_secondTask, (const signed char*)("HouseKeeping2_Task"), 8192, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), NULL);
	vTaskDelay(100);

	xTaskGenericCreate(ADCS_Task, (const signed char*)"ADCS_TASK_Debug", 8192, (void *) ST, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), NULL, NULL, NULL);
	vTaskDelay(100);

	//KickStartCamera();

/*#ifdef TESTING
	xTaskCreate(test_task, (const signed char*)("test_Task"), 8192, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), NULL);
	vTaskDelay(100);
#endif*/
	return 0;
}




