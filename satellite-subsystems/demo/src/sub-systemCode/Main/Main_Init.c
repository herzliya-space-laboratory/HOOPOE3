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
#include "../EPS.h"
#include "../Ants.h"

#include "../ADCS/AdcsMain.h"

#include "../TRXVU.h"
#include "../payload/Request Management/CameraManeger.h"
#include "sub-systemCode/ADCS/AdcsMain.h"
#include "HouseKeeping.h"
#include "commands.h"

#include "../payload/Compression/ImageConversion.h"
#include "../payload/Compression/jpeg/ImgCompressor.h"

#include "sub-systemCode/Main/CMD/ADCS_CMD.h"

#define I2c_SPEED_Hz 100000
#define I2c_Timeout 10
#define I2c_TimeoutTest portMAX_DELAY
#define RESET_COUNT_FRAM_ID 666
// will Boot- deploy all  appropriate subsytems

//extern unsigned short* Vbat_Prv;


#ifdef TESTING
void reset_FIRST_activation(Boolean8bit dataFRAM)
{
	FRAM_write(&dataFRAM, FIRST_ACTIVATION_ADDR, 1);
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
	Time initial_time_jan2000={0,0,0,1,30,6,19,0};
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

	reset_offline_TM_list();
	// 3. cahnge the first activation to false
	dataFRAM = FALSE_8BIT;

	FRAM_write(&dataFRAM, FIRST_ACTIVATION_ADDR, 1);
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

	Boolean activation = first_activation();

	StartTIME();

	init_GP();

	numberOfRestarts();

	InitializeFS(activation);

	EPS_Init();

#ifdef ANTS_ON
	init_Ants();

	Auto_Deploy();
#endif

	AdcsInit();

	init_trxvu();

	initCamera(activation);

	init_command();

	return 0;
}

typedef struct __attribute__ ((__packed__))
{
	unsigned char subtype;
	unsigned char data[20];
	unsigned char length;
	unsigned short delay_duration;
}AdcsComsnCmd_t;

AdcsComsnCmd_t InitialAngularRateEstimation[] =
{
		{.subtype = 10,	.data = {0x01},	.length = 1, .delay_duration = 100},

		{.subtype = 11,	.data = {0x05,0x01,0x00}, .length = 3, .delay_duration = 100},

		{.subtype = 14,	.data = {0x02},	.length = 1, .delay_duration = 100},

		{.subtype = 95,	.data =
		{0xFF,0x00,0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
		.length = 18, .delay_duration = 100},

		{.subtype = 94,	.data =
		{10,0,10,0,10,0,0,10,0,0,0,0,0,0,0,0,0,0},
		.length = 18, .delay_duration = 100},
};

AdcsComsnCmd_t Detumbling[] =
{
	{.subtype = 10,	.data = {0x01},	.length = 1, .delay_duration = 100},

	{.subtype = 11,	.data = {0x05,0x01,0x00}, .length = 3,	.delay_duration = 100},

	{.subtype = 14,	.data = {0x02},	.length = 1, .delay_duration = 6000},

	{.subtype = 13,	.data = {0x01,0x00,0x58,0x02},	.length = 4, .delay_duration = 100},

	{.subtype = 95,	.data =
	{0xFF,0x00,0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.length = 1, .delay_duration = 100},

	{.subtype = 94,	.data =
	{10,0,10,0,10,0,0,10,0,10,0,0,0,0,0,0,0,0},
	.length = 18, .delay_duration = 100},

};
void CommissionAdcsMode(AdcsComsnCmd_t *modeCmd, unsigned int length){

	unsigned int choice = 0;

	TC_spl cmd;
	cmd.id = 0x00000000;
	cmd.type = TC_ADCS_T;

	for(unsigned int i = 0; i < length; i++){
		printf("Continue to Next Command?\n(0 = Exit,1 = Continue; 2 = Costume Command)\n");
		while(UTIL_DbguGetIntegerMinMax(&choice,0,1));

		switch(choice){
		case 0:
			return;
		case 1:
			break;
		case 2:
				printf("subtype:(0 to 255)\n");
				while(UTIL_DbguGetIntegerMinMax(&cmd.subType,0,255));
				printf("cmd length:(0 to %d)\n",sizeof(cmd.data));
				while(UTIL_DbguGetIntegerMinMax(&cmd.length,0,sizeof(cmd.data)));
				printf("Insert Data:\n");
				for(unsigned int i = 0; i< cmd.length; i++){
					printf("data[%d]:",i);
					while(UTIL_DbguGetIntegerMinMax(&cmd.data[i],0,255));
					printf("\n");
				}
				AdcsCmdQueueAdd(&cmd);
				i--;
				continue;
			break;
		}

		cmd.subType = modeCmd[i].subtype;
		cmd.length = modeCmd[i].length;
		cmd.id++;
		memcpy(cmd.data,modeCmd[i].data,modeCmd[i].length);
		AdcsCmdQueueAdd(&cmd);
		vTaskDelay(1000);
	}


}

void TestAdcsCommissioningTask(){

	unsigned int coms_mode = 1;
	vTaskDelay(10000);
	unsigned int length = 0;
	while(1){
		printf("\nChoose Commissioning mode:\n");
		printf("\t%d) %s\n",	coms_mode++,"Initial Angular Rate Estimation");
		printf("\t%d) %s\n",	coms_mode++,"Detumbling");
		printf("\t%d) %s\n",	coms_mode++,"Magnetometer Deployment");
		printf("\t%d) %s\n",	coms_mode++,"Magnetometer Calibration");
		printf("\t%d) %s\n",	coms_mode++,"Angular Rate And Pitch Angle Estimation");
		printf("\t%d) %s\n",	coms_mode++,"Y-Wheel Ramp-Up Test");
		printf("\t%d) %s\n",	coms_mode++,"Y-Momentum Mode Commissioning");
		while(UTIL_DbguGetIntegerMinMax(&coms_mode,1,coms_mode));

		AdcsComsnCmd_t *mode = NULL;

		switch(coms_mode){
		case 1:
			mode = &InitialAngularRateEstimation;
			break;
		case 2:
			mode = &Detumbling;
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 6:
			break;
		case 7:
			break;
		}
		length = (sizeof(*mode)/sizeof(AdcsComsnCmd_t));
		CommissionAdcsMode(mode,length);
	}
}

// this function initializes all neccesary subsystem tasks in main
int SubSystemTaskStart()
{
	xTaskCreate(TRXVU_task, (const signed char*)("TRX"), 8192, NULL, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, NULL);
	vTaskDelay(100);

	xTaskCreate(save_onlineTM_task, (const signed char*)("OnlineTM"), 8192, NULL, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, NULL);
	vTaskDelay(100);

	KickStartCamera();
	vTaskDelay(100);

	xTaskCreate(AdcsTask, (const signed char*)("ADCS"), 8192, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), NULL);
	vTaskDelay(100);

	xTaskCreate(TestAdcsCommissioningTask, (const signed char*)("AdcsTestTask"), 8192, NULL, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, NULL);
	vTaskDelay(100);
	return 0;
}




