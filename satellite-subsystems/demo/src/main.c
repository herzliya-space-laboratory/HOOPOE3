/*
 * main.c
 *      Author: Akhil
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <satellite-subsystems/version/version.h>

#include <at91/utility/exithandler.h>
#include <at91/commons.h>
#include <at91/utility/trace.h>
#include <at91/peripherals/cp15/cp15.h>


#include <hal/Utility/util.h>
#include <hal/Timing/WatchDogTimer.h>
#include <hal/Timing/Time.h>
#include <hal/Drivers/LED.h>
#include <hal/Drivers/I2C.h>
#include <hal/Drivers/SPI.h>
#include <hal/boolean.h>
#include <hal/version/version.h>

#include <hal/Storage/FRAM.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sub-systemCode/COMM/GSC.h"
#include "sub-systemCode/Main/commands.h"
#include "sub-systemCode/Main/Main_Init.h"
#include "sub-systemCode/Global/Global.h"
#include "sub-systemCode/EPS.h"

#include "sub-systemCode/Main/CMD/ADCS_CMD.h"
#include "sub-systemCode/ADCS/AdcsMain.h"
#include "sub-systemCode/ADCS/AdcsCommands.h"
#include "sub-systemCode/Global/OnlineTM.h"
#include "sub-systemCode/Ants.h"

#define DEBUGMODE

#ifndef DEBUGMODE
	#define DEBUGMODE
#endif

void save_time()
{
	int err;
	// get current time from time module
	time_unix current_time;
	err = Time_getUnixEpoch(&current_time);
	check_int("set time, Time_getUnixEpoch", err);
	byte raw_time[TIME_SIZE];
	// converting the time_unix to raw (small endien)
	raw_time[0] = (byte)current_time;
	raw_time[1] = (byte)(current_time >> 8);
	raw_time[2] = (byte)(current_time >> 16);
	raw_time[3] = (byte)(current_time >> 24);
	// writing to FRAM the current time
	err = FRAM_write(raw_time, TIME_ADDR, TIME_SIZE);
	check_int("set time, FRAM_write", err);
}

void Command_logic()
{
	TC_spl command;
	int error = 0;
	do
	{
		error = get_command(&command);
		if (error == 0)
			act_upon_command(command);
	}
	while (error == 0);
}


void taskMain()
{
	WDT_startWatchdogKickTask(10 / portTICK_RATE_MS, FALSE);

	InitSubsystems();
	printf("init\n");

	vTaskDelay(100);

	SubSystemTaskStart();
	printf("Task Main start: ADCS test mode\n");

	portTickType xLastWakeTime = xTaskGetTickCount();
	const portTickType xFrequency = 1000;
	
	while(1)
	{
		EPS_Conditioning();

		Command_logic();

		save_time();

		DeployIfNeeded();

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

int main()
{
	TRACE_CONFIGURE_ISP(DBGU_STANDARD, 2000000, BOARD_MCK);
	// Enable the Instruction cache of the ARM9 core. Keep the MMU and Data Cache disabled.
	CP15_Enable_I_Cache();

	// The actual watchdog is already started, this only initializes the watchdog-kick interface.
	WDT_start();

	printf("Task Main \n");
	xTaskGenericCreate(taskMain, (const signed char *)("taskMain"), 8196, NULL, configMAX_PRIORITIES - 2, NULL, NULL, NULL);
	printf("start sch\n");
	vTaskStartScheduler();
	while(1){
		printf("should not be here\n");
		vTaskDelay(2000);
	}

	return 0;
}
