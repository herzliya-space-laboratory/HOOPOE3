/*
 * AdcsTest.c
 *
 *  Created on: Aug 11, 2019
 *      Author: ����
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "AdcsMain.h"
#include "AdcsTest.h"
#include "../Main/CMD/General_CMD.h"
#include <satellite-subsystems/cspaceADCS.h>
#include <satellite-subsystems/cspaceADCS_types.h>
#include "../EPS.h"

#define MAX_ADCS_QUEUE_LENGTH 42
#define ADCS_ID 0
#define CMD_FOR_TEST_AMUNT 34
#define TastDelay 600000
#define AMUNT_OF_STFF_TO_ADD_TO_THE_Q 3
#define TEST_NUM 0

void Lupos_Test(byte Data[CMD_FOR_TEST_AMUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE])
{
	int i,j;
	for(i = 0; i < CMD_FOR_TEST_AMUNT; i++)
	{
		for(j = 0; j < SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE; j++)
		{
			Data[i][j] = 0;
		}
	}

	//data 0
	cspace_adcs_unixtm_t Time;
	Time.fields.unix_time_sec = 1566304200;
	Time.fields.unix_time_millsec = 0;
	memcpy(Data[0],Time.raw,sizeof(cspace_adcs_unixtm_t));

	//data 1
	cspace_adcs_attctrl_mod_t CT;
	CT.fields.ctrl_mode = 1;
	CT.fields.override_flag = 0;
	CT.fields.timeout = 10;
	memcpy(Data[1],CT.raw,sizeof(cspace_adcs_unixtm_t));


	//data 2
	Data[2][0] = 1;

	//data 3
	cspace_adcs_magnetorq_t MT;
	MT.fields.magduty_x = 0.8 * 1000;
	MT.fields.magduty_y = 0.8 * 1000;
	MT.fields.magduty_z = 0.8 * 1000;
	memcpy(Data[3],MT.raw,sizeof(cspace_adcs_magnetorq_t));

	//data 4
	cspace_adcs_wspeed_t wheelConfig;
	wheelConfig.fields.speed_y = 3586;
	memcpy(Data[4],wheelConfig.raw,sizeof(cspace_adcs_magnetorq_t));

	//data 5
	for(int i = 0; i<3; i++)
	{
		Data[5][i] = 6;
	}

	//data 6
	for(int i = 0; i < 4; i++)
	{
		Data[6][i] = 6;
	}

	//data 7
	for(int i = 0; i<10; i++)
	{
		Data[7][i] = 6;
	}

	//data 23
	double SGP4_Init[23] = {0,0,0,0,0,0,0,0};
	memcpy(Data[23],SGP4_Init,sizeof(double)*8);

	//data 24-32
	for(i = 24; i < 32; i++)
	{
		memcpy(Data[i], &SGP4_Init[i-24], sizeof(double));
	}

}


void AdcsTestTask()
{
	testInit();
	TC_spl set;
	TC_spl get;

	uint8_t getSubType[CMD_FOR_TEST_AMUNT] =
	{
			ADCS_GET_CURR_UNIX_TIME_ST,
			ADCS_I2C_GENRIC_ST, //131
			ADCS_GET_CURRENT_STATE_ST,
			ADCS_GET_CURRENT_STATE_ST,
			ADCS_GET_MAGNETORQUER_CMD_ST,
			ADCS_GET_WHEEL_SPEED_CMD_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			666,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_FULL_CONFIG_ST,
			ADCS_GET_SGP4_ORBIT_PARAMETERS_ST,
			ADCS_GET_SGP4_ORBIT_PARAMETERS_ST,
			ADCS_GET_SGP4_ORBIT_PARAMETERS_ST,
			ADCS_GET_SGP4_ORBIT_PARAMETERS_ST,
			ADCS_GET_SGP4_ORBIT_PARAMETERS_ST,
			ADCS_GET_SGP4_ORBIT_PARAMETERS_ST,
			ADCS_GET_SGP4_ORBIT_PARAMETERS_ST,
			ADCS_GET_SGP4_ORBIT_PARAMETERS_ST,
			ADCS_GET_SGP4_ORBIT_PARAMETERS_ST,
			ADCS_GET_FULL_CONFIG_ST,
	};
	byte GetData[CMD_FOR_TEST_AMUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE];
	int i,j;
	for(i = 0; i < CMD_FOR_TEST_AMUNT; i++)
	{
		for(j = 0; j < SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE; j++)
		{
			GetData[i][j] = 0;
		}
	}
	GetData[1][0] = 131;


	//function to test constructor
	uint8_t setSubType[CMD_FOR_TEST_AMUNT] = {
			ADCS_SET_CURR_UNIX_TIME_ST,
			ADCS_CACHE_ENABLE_ST,
			ADCS_SET_ATT_CTRL_MODE_ST,
			ADCS_SET_EST_MODE_ST,
			ADCS_SET_MAG_OUTPUT_ST,
			ADCS_SET_WHEEL_SPEED_ST,
			ADCS_SET_MTQ_CONFIG_ST,
			ADCS_RW_CONFIG_ST,
			ADCS_GYRO_CONFIG_ST,
			ADCS_CSS_CONFIG_ST,
			ADCS_CSS_RELATIVE_SCALE_ST,
			ADCS_SET_MAGNETMTR_MOUNT_ST,
			ADCS_SET_MAGNETMTR_OFFSET_ST,
			ADCS_SET_MAGNETMTR_SENSTVTY_ST,
			ADCS_RATE_SENSOR_OFFSET_ST,
			ADCS_SET_STAR_TRACKER_CONFIG_ST,
			ADCS_SET_DETUMB_CTRL_PARAM_ST,
			ADCS_SET_YWHEEL_CTRL_PARAM_ST,
			ADCS_SET_RWHEEL_CTRL_PARAM_ST,
			ADCS_SET_MOMENT_INTERTIA_ST,
			ADCS_PROD_INERTIA_ST,
			ADCS_ESTIMATION_PARAM1_ST,
			ADCS_ESTIMATION_PARAM2_ST,
			ADCS_SET_SGP4_ORBIT_PARAMS_ST,
			ADCS_SET_SGP4_ORBIT_INC_ST,
			ADCS_SET_SGP4_ORBIT_ECC_ST,
			ADCS_SET_SGP4_ORBIT_RAAN_ST,
			ADCS_SET_SGP4_ORBIT_ARG_OF_PER_ST,
			ADCS_SET_SGP4_ORBIT_BSTAR_DRAG_ST,
			ADCS_SET_SGP4_ORBIT_MEAN_MOT_ST,
			ADCS_SET_SGP4_ORBIT_MEAN_ANOM_ST,
			ADCS_SET_SGP4_ORBIT_EPOCH_ST,
			ADCS_SET_MAGNETOMETER_MODE_ST
	};//function to test sst


	byte SetData[CMD_FOR_TEST_AMUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE];
	Lupos_Test(SetData);
	printf("start test");
	while(TRUE)
	{
		int err;
//		for(i = 0; i < CMD_FOR_TEST_AMUNT; i++)
//		{
//			if(i % (MAX_ADCS_QUEUE_LENGTH/AMUNT_OF_STFF_TO_ADD_TO_THE_Q) == 0)
//			{
//
//			}
			set.subType = setSubType[TEST_NUM];
			memcpy(set.data,SetData[TEST_NUM],set.length);

			get.subType = getSubType[TEST_NUM];
			memcpy(get.data,GetData[TEST_NUM],set.length);

			err = AdcsCmdQueueAdd(&get);
			printf("sst:%d\t err:%d\n",getSubType[TEST_NUM], err);
			err = AdcsCmdQueueAdd(&set);
			printf("sst:%d\t err:%d\n",setSubType[TEST_NUM], err);
			err = AdcsCmdQueueAdd(&get);
			printf("sst:%d\t err:%d\n",getSubType[TEST_NUM], err);
			vTaskDelay(TastDelay);
//		}
	}
}



void ErrFlagTest()
{
	testInit();

	double* SGP4 = { 0,0,0,0,0,0,0,0 };
	cspaceADCS_setSGP4Parameters(ADCS_ID, SGP4);
	restart();
}

void printErrFlag()
{
	testInit();
	cspace_adcs_statetlm_t data;
	cspaceADCS_getStateTlm(ADCS_ID, &data);
	printf("\nSGP ERR FLAG = %d \n", data.fields.curr_state.fields.orbitparam_invalid);
}

void Mag_Test()
{
	testInit();

	//generic i2c tm id 170 size 6 and print them
	cspace_adcs_rawmagmeter_t Mag;
	cspaceADCS_getRawMagnetometerMeas(0, &Mag);
	printf("mag raw x %d", Mag.fields.magnetic_x);
	printf("mag raw y %d", Mag.fields.magnetic_y);
	printf("mag raw z %d", Mag.fields.magnetic_z);


}

void AddCommendToQ()
{
	testInit();

	int input;
	int err;
	TC_spl adcsCmd;
	adcsCmd.id = TC_ADCS_T;
	if (UTIL_DbguGetIntegerMinMax(&(adcsCmd.subType), 0, 666) != 0) {
		printf("Enter ADCS command data\n");
		UTIL_DbguGetString(&(adcsCmd.data), SIZE_OF_COMMAND + 1);
		err = AdcsCmdQueueAdd(&adcsCmd);
		printf("ADCS command error = %d\n\n\n", err);
		printf("Send new command\n");
		printf("Enter ADCS sub type\n");
	}

	if (UTIL_DbguGetIntegerMinMax(&input, 900, 1000) != 0) {
		printf("Print the data\n");
	}

}

void testInit()
{
	TC_spl InitCommand;
	InitCommand.subType = ADCS_RUN_MODE_ST;
	InitCommand.data[0] = 1;
	cspaceADCS_setRunMode(ADCS_ID, runmode_enabled);
	int err = AdcsCmdQueueAdd(&adcsCmd);
	printf("ADCS test init command error = %d\n", err);

	vTaskDelay(10);

	InitCommand.subType = ADCS_SET_PWR_CTRL_DEVICE_ST;
	cspace_adcs_powerdev_t device_ctrl = { 0 };
	device_ctrl.fields.signal_cubecontrol = 1;
	device_ctrl.fields.motor_cubecontrol = 1;
	device_ctrl.fields.pwr_motor = 1;
	memcpy(InitCommand.data,device_ctrl,sizeof(device_ctrl));
	int err = AdcsCmdQueueAdd(&InitCommand);
	printf("ADCS test init command error = %d\n", err);

}

