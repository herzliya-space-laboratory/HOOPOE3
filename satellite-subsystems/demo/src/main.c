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

void TestGenericI2cTelemetry()
{
	int err = 0;
	cspace_adcs_powerdev_t device_ctrl;
	device_ctrl.fields.motor_cubecontrol= TRUE;
	device_ctrl.fields.pwr_cubesense = TRUE;
	device_ctrl.fields.signal_cubecontrol = TRUE;
	err = cspaceADCS_setPwrCtrlDevice(ADCS_ID, &device_ctrl);
	if(0!=err){
		printf("'cspaceADCS_setPwrCtrlDevice' = %d\n",err);
		return;
	}

	unsigned int tlm_id = 0;
	unsigned int tlm_length = 0;
	printf("choose from driver telemetry? Y=1/N=0\n");
	while(UTIL_DbguGetIntegerMinMax(&err,0,1)==0);

	printf("please choose TLM id(0 to cancel):\n");
	while(UTIL_DbguGetIntegerMinMax(&tlm_id,0,200)==0);
	if(tlm_id == 0) return;

	printf("please choose TLM length(0 to cancel):\n");
	while(UTIL_DbguGetIntegerMinMax(&tlm_length,0,300)==0);
	if(tlm_length == 0) return;

	byte buffer[300] ={0};
	AdcsI2cCmdReadTLM(tlm_id,buffer,tlm_length);

	if(err == 1){
		cspace_adcs_magfieldvec_t info_data;
		switch(tlm_id){
		case 151:
			err = cspaceADCS_getMagneticFieldVec(ADCS_ID, &info_data);
			err = err*1;
			memcpy(&info_data,buffer,sizeof(info_data));
			err = 0;
		break;
		}
	}
	else{
		for(int i=0;i<tlm_length;i++)
		{
			printf("%X\t",buffer[i]);
		}
	}

}

void taskMain()
{
	WDT_startWatchdogKickTask(10 / portTICK_RATE_MS, FALSE);
	InitSubsystems();

	vTaskDelay(100);
	printf("init finished\n");
	SubSystemTaskStart();
	printf("Task Main start: ADCS test mode\n");

	portTickType xLastWakeTime = xTaskGetTickCount();
	const portTickType xFrequency = 1000;
	
	TC_spl adcsCmd;
	adcsCmd.id = TC_ADCS_T;

	uint input;
	int err;
	while(1)
	{
		// EPS_Conditioning();
		// Command_logic();
		// save_time();



		printf("Send new command\n");
		printf("Enter ADCS sub type\n");
		UTIL_DbguGetIntegerMinMax(&input, 0,1000);
		if (input !=0)
		{
			if (input < 200)
			{
				adcsCmd.subType = input;
				printf("Enter ADCS command data\n");
				while (UTIL_DbguGetIntegerMinMax(&(adcsCmd.data), 0,200) == 0);
				err = AdcsCmdQueueAdd(&adcsCmd);
				printf("ADCS command error = %d\n\n\n", err);

			}
			else
			{
				TestGenericI2cTelemetry();
				//printf("Print the data\n\n");
			}
		}

		vTaskDelay(1000);
	}
}

int main()
{
	TRACE_CONFIGURE_ISP(DBGU_STANDARD, 2000000, BOARD_MCK);
	// Enable the Instruction cache of the ARM9 core. Keep the MMU and Data Cache disabled.
	CP15_Enable_I_Cache();

	WDT_start();

	printf("Task Main 2121\n");
	xTaskGenericCreate(taskMain, (const signed char *)("taskMain"), 4000, NULL, configMAX_PRIORITIES - 2, NULL, NULL, NULL);
	printf("start sch\n");
	vTaskStartScheduler();
	while(1){
		printf("should not be here\n");
		vTaskDelay(2000);
	}
	return 0;
}
