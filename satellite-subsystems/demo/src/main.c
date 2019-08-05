/*
 * main.c
 *      Author: Akhil
 */
#include <satellite-subsystems/version/version.h>

#include <at91/utility/exithandler.h>
#include <at91/commons.h>
#include <at91/utility/trace.h>
#include <at91/peripherals/cp15/cp15.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

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

#include "sub-systemCode/Global/OnlineTM.h"

#define DEBUGMODE

#ifndef DEBUGMODE
	#define DEBUGMODE
#endif


#define HK_DELAY_SEC 10
#define MAX_SIZE_COMMAND_Q 20

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

void online_test_menu()
{
	printf( "\n\r Select a test to perform: \n\r");
	printf("\t 0) continue to code\n\r");
	printf("\t 1) add online TM to save off line\n\r");
	printf("\t 2) delete from off line list\n\r");
	printf("\t 3) delay for 10000\n\r");

	int error;
	unsigned int selection;
	while(UTIL_DbguGetIntegerMinMax(&selection, 0, 3) == 0);
	switch (selection) {
		case 0:
			for (int i = 0; i < 18; i++)
			{
				TM_spl packet;
				error = get_online_packet(i, &packet);
				if (error != 0)
					printf("error from get_online_packet(%d): %d\n", i, error);
				else
					print_TM_spl_packet(packet);
				vTaskDelay(700);
			}
			break;
		case 1:
			printf("add second:\n");
			while(UTIL_DbguGetIntegerMinMax(&selection, 0, 100) == 0);
			time_unix stopTime;
			Time_getUnixEpoch(&stopTime);
			stopTime += selection;
			printf("period:\n");
			while(UTIL_DbguGetIntegerMinMax(&selection, 0, 10000) == 0);
			uint period = selection;
			printf("index:\n");
			while(UTIL_DbguGetIntegerMinMax(&selection, 0, 18) == 0);
			error = add_onlineTM_param_to_save_list((int)selection, period, stopTime);
			if (error != 0)
				printf("error in add_onlineTM_param_to_save_list: %d\n", error);
			break;
		case 2:
			printf("index:\n");
			while(UTIL_DbguGetIntegerMinMax(&selection, 0, 18) == 0);
			error = delete_onlineTM_param_to_save_list((int)selection);
			if (error != 0)
				printf("error in delete_onlineTM_param_to_save_list: %d\n", error);
			break;
		case 3:
			vTaskDelay(10000);
			break;
		default:
			vTaskDelay(1000);
			break;
	}
}

void taskMain()
{
	WDT_startWatchdogKickTask(10 / portTICK_RATE_MS, FALSE);
	InitSubsystems();

	vTaskDelay(100);
	printf("init\n");
	SubSystemTaskStart();
	printf("Task Main start\n");

	portTickType xLastWakeTime = xTaskGetTickCount();
	const portTickType xFrequency = 1000;

	while(1)
	{
		EPS_Conditioning();
		Command_logic();
		save_time();
		online_test_menu();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

int main()
{
	TRACE_CONFIGURE_ISP(DBGU_STANDARD, 2000000, BOARD_MCK);
	// Enable the Instruction cache of the ARM9 core. Keep the MMU and Data Cache disabled.
	CP15_Enable_I_Cache();

	WDT_start();

	printf("Task Main 2121\n");
	xTaskGenericCreate(taskMain, (const signed char *)("taskMain"), 2048, NULL, configMAX_PRIORITIES - 2, NULL, NULL, NULL);
	printf("start sch\n");
	vTaskStartScheduler();
	return 0;
}
