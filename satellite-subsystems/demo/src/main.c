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

#define PRINT_IF_NO_ERROR(err,function) 	\
if(0 != err){\
printf("error in " #function "= %d\n",err);return;\
}

void TestSetAdcsModes(){
	int err = 0;
	cspace_adcs_runmode_t runmode = runmode_enabled;
	err = cspaceADCS_setRunMode(ADCS_ID,  runmode);
	vTaskDelay(2000);
	if(0 != err){
		printf("error in 'cspaceADCS_setAttEstMode'=%d\n",err);
		return;
	}else{
		printf("runmode = %d\n",runmode);
	}

	cspace_adcs_powerdev_t pwr = {.raw = {0}};
	pwr.fields.signal_cubecontrol = 1;
	pwr.fields.pwr_motor = 1;
	pwr.fields.motor_cubecontrol = 1;

	vTaskDelay(1000);

	err = cspaceADCS_setPwrCtrlDevice(ADCS_ID,&pwr);
	PRINT_IF_NO_ERROR(err,cspaceADCS_setPwrCtrlDevice);

	cspace_adcs_attctrl_mod_t ctrl_mode;
	printf("\nchoose control mode mode:\n");
	while(UTIL_DbguGetIntegerMinMax((unsigned int*)&ctrl_mode.fields.ctrl_mode,0,13) == 0);
	err = cspaceADCS_setAttCtrlMode(ADCS_ID,&ctrl_mode);
	PRINT_IF_NO_ERROR(err,cspaceADCS_setAttCtrlMode);
	vTaskDelay(2000);

	cspace_adcs_estmode_sel estimation_mode;
	printf("\nchoose estimation mode:\n");
	while(UTIL_DbguGetIntegerMinMax((unsigned int*)&estimation_mode,0,6) == 0);

	err = cspaceADCS_setAttEstMode(ADCS_ID,estimation_mode);
	if(0 != err){
		printf("error in 'cspaceADCS_setAttEstMode'=%d\n",err);
		return;
	}else{
		printf("estimation_mode = %d\n",estimation_mode);
	}
}

void TestAdcsPrintTlm()
{
	int err = 0;
	adcs_i2c_cmd i2c_cmd = {0};

	cspace_adcs_magfieldvec_t vec;
	cspace_adcs_rawmagmeter_t raw_mag;
	cspace_adcs_estmetadata_t metadata;

	printf("\n--- USING ADCS DRIVERS\n");

	err = cspaceADCS_getMagneticFieldVec(ADCS_ID,&vec);
	PRINT_IF_NO_ERROR(err,cspaceADCS_getMagneticFieldVec)
	printf("magfield_x: %d\n",vec.fields.magfield_x);
	printf("magfield_y: %d\n",vec.fields.magfield_y);
	printf("magfield_z: %d\n",vec.fields.magfield_z);

	printf("\n\n");

	err = cspaceADCS_getRawMagnetometerMeas(ADCS_ID, &raw_mag);
	PRINT_IF_NO_ERROR(err,cspaceADCS_getRawMagnetometerMeas)
	printf("magnetic_x: %d\n",raw_mag.fields.magnetic_x);
	printf("magnetic_y: %d\n",raw_mag.fields.magnetic_y);
	printf("magnetic_z: %d\n",raw_mag.fields.magnetic_z);

	err = cspaceADCS_getEstimationMetadata(ADCS_ID, &metadata);
	PRINT_IF_NO_ERROR(err,cspaceADCS_getEstimationMetadata)


	printf("\n--- USING GNERING I2C\n");

	i2c_cmd.id = 151; //cspaceADCS_getMagneticFieldVec
	i2c_cmd.length = 6;
	err = AdcsGenericI2cCmd(&i2c_cmd);
	PRINT_IF_NO_ERROR(err,AdcsGenericI2cCmd)

	memcpy(&vec,i2c_cmd.data,sizeof(vec));
	printf("magfield_x: %d\n",vec.fields.magfield_x);
	printf("magfield_y: %d\n",vec.fields.magfield_y);
	printf("magfield_z: %d\n",vec.fields.magfield_z);


	i2c_cmd.id = 170; //cspaceADCS_getRawMagnetometerMeas
	i2c_cmd.length = 6;
	err = AdcsGenericI2cCmd(&i2c_cmd);
	PRINT_IF_NO_ERROR(err,AdcsGenericI2cCmd)

	memcpy(&raw_mag,i2c_cmd.data,sizeof(raw_mag));
	printf("magnetic_x: %d\n",raw_mag.fields.magnetic_x);
	printf("magnetic_y: %d\n",raw_mag.fields.magnetic_y);
	printf("magnetic_z: %d\n",raw_mag.fields.magnetic_z);

	i2c_cmd.id = 178; //cspaceADCS_getEstimationMetadata
	err = AdcsGenericI2cCmd(&i2c_cmd);
	PRINT_IF_NO_ERROR(err,AdcsGenericI2cCmd)

	memcpy(&metadata,i2c_cmd.data,sizeof(metadata));
	//... print tlm
}

void taskMain()
{
	WDT_startWatchdogKickTask(10 / portTICK_RATE_MS, FALSE);

	InitSubsystems();
	printf("init\n");

	vTaskDelay(100);

	SubSystemTaskStart();
	printf("Task Main start: ADCS test mode\n");

	//portTickType xLastWakeTime = xTaskGetTickCount();
	//const portTickType xFrequency = 1000;
	

	gom_eps_hk_t eps_tlm;
	while(1)
	{
		GomEpsGetHkData_general(0,&eps_tlm);

		vTaskDelay(1000);
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
