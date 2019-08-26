/*
 * AdcsTest.c
 *
 *  Created on: Aug 11, 2019
 *      Author: ����
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "AdcsCommands.h"
#include "AdcsMain.h"
#include "AdcsTest.h"
#include "sub-systemCode/Main/CMD/ADCS_CMD.h"
#include "../Main/CMD/General_CMD.h"
#include <satellite-subsystems/cspaceADCS.h>
#include <satellite-subsystems/cspaceADCS_types.h>
#include "../EPS.h"
#include <hal/Utility/util.h>

#define MAX_ADCS_QUEUE_LENGTH 42
#define ADCS_ID 0
#define CMD_FOR_TEST_AMUNT 34
#define TastDelay 600000
#define AMUNT_OF_STFF_TO_ADD_TO_THE_Q 3
#define TEST_NUM 1

void testInit();

void BuildTests(uint8_t getSubType[CMD_FOR_TEST_AMUNT], int getLength[CMD_FOR_TEST_AMUNT], byte getData[CMD_FOR_TEST_AMUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE], uint8_t setSubType[CMD_FOR_TEST_AMUNT], int setLength[CMD_FOR_TEST_AMUNT], byte setData[CMD_FOR_TEST_AMUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE])
{
	int i,j;
	adcs_i2c_cmd i2c_cmd;

	for(i = 0; i < CMD_FOR_TEST_AMUNT; i++)
	{
		setLength[i] = 0;
		getLength[i] = 0;
		for(j = 0; j < SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE; j++)
		{
			setData[i][j] = 0;
			getData[i][j] = 0;
		}
	}
	int testNum;

	//test #0 data
	testNum = 0;
	getSubType[testNum] = ADCS_GET_CURR_UNIX_TIME_ST;
	setSubType[testNum] = ADCS_SET_CURR_UNIX_TIME_ST;
	setLength[testNum] = 6;
	cspace_adcs_unixtm_t time;
	time.fields.unix_time_sec = 1566304200;
	time.fields.unix_time_millsec = 0;
	memcpy(setData[testNum],time.raw,sizeof(cspace_adcs_unixtm_t));

	//test #1 data
	testNum = 1;
	getSubType[testNum] = ADCS_I2C_GENRIC_ST; //generic I2C command #131
	getLength[testNum] = sizeof(adcs_i2c_cmd);
	i2c_cmd.id = 131;
	i2c_cmd.length = 1;
	i2c_cmd.ack = 3;
	memcpy(getData[testNum],&i2c_cmd,getLength[testNum]);
	setSubType[testNum] = ADCS_CACHE_ENABLE_ST;
	setLength[testNum] = sizeof(cspace_adcs_attctrl_mod_t);
	cspace_adcs_attctrl_mod_t CT;
	CT.fields.ctrl_mode = 1;
	CT.fields.override_flag = 0;
	CT.fields.timeout = 10;
	memcpy(setData[testNum],CT.raw,setLength[testNum]);

	//test #2 data
	testNum = 2;
	getSubType[testNum] = ADCS_GET_CURRENT_STATE_ST;
	setSubType[testNum] = ADCS_SET_ATT_CTRL_MODE_ST;
	setData[testNum][0] = 1;
	setLength[testNum] = 1;

	//test #3 data
	testNum = 3;
	getSubType[testNum] = ADCS_GET_CURRENT_STATE_ST;
	setSubType[testNum] = ADCS_SET_EST_MODE_ST;
	setData[testNum][0] = 1;
	setLength[testNum] = 1;

	//test #4 data
	testNum = 4;
	getSubType[testNum] = ADCS_GET_MAGNETORQUER_CMD_ST;
	setSubType[testNum] = ADCS_SET_MAG_OUTPUT_ST;
	setLength[testNum] = sizeof(cspace_adcs_magnetorq_t);
	cspace_adcs_magnetorq_t MT;
	MT.fields.magduty_x = 0.8 * 1000;
	MT.fields.magduty_y = 0.8 * 1000;
	MT.fields.magduty_z = 0.8 * 1000;
	memcpy(setData[testNum],MT.raw,setLength[testNum]);

	//test #5 data
	testNum = 5;
	getSubType[testNum] = ADCS_GET_WHEEL_SPEED_CMD_ST;
	setSubType[testNum] = ADCS_SET_WHEEL_SPEED_ST;
	setLength[testNum] = sizeof(cspace_adcs_magnetorq_t);
	cspace_adcs_wspeed_t wheelConfig;
	wheelConfig.fields.speed_y = 0;
	memcpy(setData[testNum],wheelConfig.raw,setLength[testNum]);

	//test #6 data
	testNum = 6;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MTQ_CONFIG_ST;
	setLength[testNum] = 3;
	for(int i = 0; i<setLength[testNum]; i++)
	{
		setData[testNum][i] = 6;
	}

	//test #7 data
	testNum = 7;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_RW_CONFIG_ST;
	setLength[testNum] = 4;
	for(int i = 0; i<setLength[testNum]; i++)
	{
		setData[testNum][i] = 6;
	}

	//test #8 data
	testNum = 8;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_GYRO_CONFIG_ST;
	setLength[testNum] = 10;
	for(int i = 0; i<setLength[testNum]; i++)
	{
		setData[testNum][i] = 6;
	}

	//test #9 data
	testNum = 9;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_CSS_CONFIG_ST;

	//test #10 data
	testNum = 10;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_CSS_RELATIVE_SCALE_ST;

	//test #11 data
	testNum = 11;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MAGNETMTR_MOUNT_ST;

	//test #12 data
	testNum = 12;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MAGNETMTR_OFFSET_ST;

	//test #13 data
	testNum = 13;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MAGNETMTR_SENSTVTY_ST;

	//test #14 data
	testNum = 14;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_RATE_SENSOR_OFFSET_ST;

	//test #15 data
	testNum = 15;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_STAR_TRACKER_CONFIG_ST;

	//test #16 data
	testNum = 16;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_DETUMB_CTRL_PARAM_ST;

	//test #17 data
	testNum = 17;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_YWHEEL_CTRL_PARAM_ST;

	//test #18 data
	testNum = 18;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_RWHEEL_CTRL_PARAM_ST;

	//test #19 data
	testNum = 19;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MOMENT_INTERTIA_ST;

	//test #20 data
	testNum = 20;
	getSubType[testNum] = 255;
	setSubType[testNum] = ADCS_PROD_INERTIA_ST;

	//test #21 data
	testNum = 21;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_ESTIMATION_PARAM1_ST;

	//test #22 data
	testNum = 22;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_ESTIMATION_PARAM2_ST;

	//tests #23-32 data all SGP4 commands
	testNum = 23;
	double SGP4_Init[8] = {0,0,0,0,0,0,0,0};
	getSubType[testNum] = ADCS_GET_SGP4_ORBIT_PARAMETERS_ST;
	setSubType[testNum] = ADCS_SET_SGP4_ORBIT_PARAMS_ST;
	setLength[testNum] = sizeof(double)*8;
	memcpy(setData[testNum],SGP4_Init,setLength[testNum]);

	//data 24-32
	setSubType[testNum+1] = ADCS_SET_SGP4_ORBIT_INC_ST;
	setSubType[testNum+2] = ADCS_SET_SGP4_ORBIT_ECC_ST;
	setSubType[testNum+3] = ADCS_SET_SGP4_ORBIT_RAAN_ST;
	setSubType[testNum+4] = ADCS_SET_SGP4_ORBIT_ARG_OF_PER_ST;
	setSubType[testNum+5] = ADCS_SET_SGP4_ORBIT_BSTAR_DRAG_ST;
	setSubType[testNum+6] = ADCS_SET_SGP4_ORBIT_MEAN_MOT_ST;
	setSubType[testNum+7] = ADCS_SET_SGP4_ORBIT_MEAN_ANOM_ST;
	setSubType[testNum+9] = ADCS_SET_SGP4_ORBIT_EPOCH_ST;

	for(testNum = 24; testNum < 32; i++)
	{
		SGP4_Init[testNum-24] = 1; //change from 0 there from last test
		getSubType[testNum] = ADCS_GET_SGP4_ORBIT_PARAMETERS_ST;
		setLength[testNum] = sizeof(double);
		memcpy(setData[testNum], &SGP4_Init[testNum-24], setLength[testNum]);
	}

	//test #32 data
	testNum = 32;
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MAGNETOMETER_MODE_ST;
}


void AdcsTestTask()
{
	testInit();
	TC_spl get;
	TC_spl set;

	uint8_t getSubType[CMD_FOR_TEST_AMUNT];
	int getLength[CMD_FOR_TEST_AMUNT];
	byte getData[CMD_FOR_TEST_AMUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE];

	//function to test constructor
	uint8_t setSubType[CMD_FOR_TEST_AMUNT];//function to test sst	
	int setLength[CMD_FOR_TEST_AMUNT];
	byte setData[CMD_FOR_TEST_AMUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE];
	
	BuildTests(getSubType, getLength, getData, setSubType, setLength, setData);
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
			set.length = setLength[TEST_NUM];
			memcpy(set.data,setData[TEST_NUM],set.length);

			get.subType = getSubType[TEST_NUM];
			get.length = getLength[TEST_NUM];
			memcpy(get.data,getData[TEST_NUM],get.length);

			err = AdcsCmdQueueAdd(&get);
			printf("\nsst:%d\t err:%d\n",get.subType, err);
			err = AdcsCmdQueueAdd(&set);
			printf("sst:%d\t err:%d\t data:",set.subType, err ,set.data);
			for(int i = 0; i < set.length; i++)
			{
				printf("%x, ",set.data[i]);
			}
			err = AdcsCmdQueueAdd(&get);
			printf("\nsst:%d\t err:%d\n",get.subType, err);
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
	int err = AdcsCmdQueueAdd(&InitCommand);
	printf("ADCS test init command error = %d\n", err);

	vTaskDelay(10);

	InitCommand.subType = ADCS_SET_PWR_CTRL_DEVICE_ST;
	cspace_adcs_powerdev_t device_ctrl;
	device_ctrl.fields.signal_cubecontrol = 1;
	device_ctrl.fields.motor_cubecontrol = 1;
	device_ctrl.fields.pwr_motor = 1;
	memcpy(InitCommand.data,device_ctrl.raw,sizeof(device_ctrl));
	err = AdcsCmdQueueAdd(&InitCommand);
	printf("ADCS test init command error = %d\n", err);

}

