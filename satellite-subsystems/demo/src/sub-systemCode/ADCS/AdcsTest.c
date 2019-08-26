
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <satellite-subsystems/cspaceADCS.h>
#include <satellite-subsystems/cspaceADCS_types.h>
#include <hal/Utility/util.h>
#include <at91/utility/exithandler.h>

#include <stdlib.h>

#include "AdcsGetDataAndTlm.h"
#include "AdcsCommands.h"
#include "AdcsMain.h"
#include "AdcsTest.h"

#include "../Main/CMD/General_CMD.h"
#include "sub-systemCode/Main/CMD/ADCS_CMD.h"

#include "../EPS.h"
#define MAX_ADCS_QUEUE_LENGTH 42
#define ADCS_ID 0
#define CMD_FOR_TEST_AMUNT 34
#define TastDelay 600000
#define AMUNT_OF_STFF_TO_ADD_TO_THE_Q 3
#define TEST_NUM 1


void TestUpdateTlmToSaveVector(){
	int err = 0;
	Boolean8bit ToSaveVec[NUM_OF_ADCS_TLM] = {0};
	Boolean8bit temp[NUM_OF_ADCS_TLM] = {0};
	AdcsTlmElement_t elem = {0};
	memset(ToSaveVec,0xFF,sizeof(NUM_OF_ADCS_TLM * sizeof(Boolean8bit)));

	UpdateTlmToSaveVector(ToSaveVec);
	err = FRAM_read(temp,ADCS_TLM_SAVE_VECTOR_START_ADDR,ADCS_TLM_SAVE_VECTOR_END_ADDR);	// check update in FRAM
	if(0 != err){
		printf("Error in FRAM_read\n");
		return;
	}
	for(int i = 0; i < NUM_OF_ADCS_TLM; i++){
		GetTlmElementAtIndex(&elem,i);	// check Update in array
		if(ToSaveVec[i] != temp[i] || ToSaveVec[i] != elem.ToSave){
			printf("\n\t-----Error in 'UpdateTlmToSaveVector'\n\n");
			return;
		}
	}
}

void TestSaveTlm(){

}

void TestGatherTlmAndData(){
	TroubleErrCode trbl = 0;
	for(int i = 0; i < 60; i++){
		trbl = GatherTlmAndData();
		vTaskDelay(1000);
	}
}

void TestUpdateTlmElemdntAtIndex(){
	int err = 0;
	for(int i = 0; i<NUM_OF_ADCS_TLM;i++){
		UpdateTlmElementAtIndex(i,TRUE_8BIT,6);
	}

}

void TestRestoreDefaultTlmElement()
{
	TroubleErrCode trbl = RestoreDefaultTlmElement();
	if(trbl != TRBL_SUCCESS)
		printf("\t----- Error in 'RestoreDefaultTlmElement'\n");
}

void TestAdcsTLM()
{
	printf("Test 'UpdateTlmToSaveVector'\n");
	TestUpdateTlmToSaveVector();

	printf("Test 'GatherTlmAndData' for 60 sec\n");
	TestGatherTlmAndData();

	printf("Test 'UpdateTlmElemdntAtIndex' (change tlm save freq to 1/6 Hz)\n");
	TestUpdateTlmElemdntAtIndex();

	printf("Test 'GatherTlmAndData' for 60 sec with updated array\n");
	TestGatherTlmAndData();

	printf("Test 'RestoreDefaultTlmElement'");
	RestoreDefaultTlmElement();

	printf("Test 'GatherTlmAndData' for 60 sec\n");
	TestGatherTlmAndData();
}

void Test()
{
}

void Lupos_Test(byte Data[CMD_FOR_TEST_AMUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE], int setLength[CMD_FOR_TEST_AMUNT])
{
	int i,j;

	memset(setLength,0,CMD_FOR_TEST_AMUNT);
	memset(Data,0,CMD_FOR_TEST_AMUNT*(SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE));

	//data 0
	cspace_adcs_unixtm_t Time;
	Time.fields.unix_time_sec = 1566304200;
	Time.fields.unix_time_millsec = 0;
	memcpy(Data[0],Time.raw,sizeof(cspace_adcs_unixtm_t));
	setLength[0] = 6;

	//data 1
	cspace_adcs_attctrl_mod_t CT;
	CT.fields.ctrl_mode = 1;
	CT.fields.override_flag = 0;
	CT.fields.timeout = 10;
	memcpy(Data[1],CT.raw,sizeof(cspace_adcs_unixtm_t));
	setLength[1] = sizeof(cspace_adcs_attctrl_mod_t);

	//data 2
	Data[2][0] = 1;
	setLength[2] = 1;

	//data 3
	Data[3][0] = 1;
	setLength[3] = 1;

	//data 4
	cspace_adcs_magnetorq_t MT;
	MT.fields.magduty_x = 0.8 * 1000;
	MT.fields.magduty_y = 0.8 * 1000;
	MT.fields.magduty_z = 0.8 * 1000;
	memcpy(Data[4],MT.raw,sizeof(cspace_adcs_magnetorq_t));
	setLength[4] = sizeof(cspace_adcs_magnetorq_t);

	//data 5
	cspace_adcs_wspeed_t wheelConfig;
	wheelConfig.fields.speed_y = 0;
	memcpy(Data[5],wheelConfig.raw,sizeof(cspace_adcs_magnetorq_t));
	setLength[5] = sizeof(cspace_adcs_magnetorq_t);

	//data 6
	for(int i = 0; i<3; i++)
	{
		Data[6][i] = 6;
	}
	setLength[6] = 3;

	//data 7
	for(int i = 0; i < 4; i++)
	{
		Data[7][i] = 6;
	}
	setLength[7] = 4;

	//data 8
	for(int i = 0; i<10; i++)
	{
		Data[8][i] = 6;
	}
	setLength[8] = 10;

	//data 23
	double SGP4_Init[23] = {0,0,0,0,0,0,0,0};
	memcpy(Data[23],SGP4_Init,sizeof(double)*8);
	setLength[23] = sizeof(double)*8;

	//data 24-32
	for(i = 24; i < 32; i++)
	{
		memcpy(Data[i], &SGP4_Init[i-24], sizeof(double));
		setLength[i] = sizeof(double);
	}

}

void AdcsTestTask()
{
//	testInit();
	TC_spl get;
	TC_spl set;
	TC_spl get;

	TaskMamagTest(); // currently restarts the sat' right after

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
			255,
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
	int setLength[CMD_FOR_TEST_AMUNT];
	int getLength[CMD_FOR_TEST_AMUNT] = {0};
	byte GetData[CMD_FOR_TEST_AMUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE] = {0};

	adcs_i2c_cmd i2c_cmd;
	i2c_cmd.id = 131;
	i2c_cmd.length = 1;
	i2c_cmd.ack = 3;
	getLength[1] = sizeof(adcs_i2c_cmd);
	memcpy(GetData[1],&i2c_cmd,getLength[1]);

	//function to test constructor
	uint8_t setSubType[CMD_FOR_TEST_AMUNT] = {0};//function to test sst	
	int setLength[CMD_FOR_TEST_AMUNT] = {0};
	byte setData[CMD_FOR_TEST_AMUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE] = {{0}};
	
	BuildTests(getSubType, getLength, GetData, setSubType, setLength, setData);
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
			memcpy(set.data,SetData[TEST_NUM],set.length);

			get.subType = getSubType[TEST_NUM];
			get.length = getLength[TEST_NUM];
			memcpy(get.data,GetData[TEST_NUM],get.length);

			err = AdcsCmdQueueAdd(&get);
			printf("\nsst:%d\n err:%d\n",get.subType, err);
			err = AdcsCmdQueueAdd(&set);
			printf("\nsst:%d\n err:%d\n data:%s\n",set.subType, err ,set.data);
			for(int i = 0; i < set.length; i++)
			{
				printf("%X, ",set.data[i]);
			}
			err = AdcsCmdQueueAdd(&get);
			printf("\nsst:%d\t err:%d\n",get.subType, err);
			vTaskDelay(TastDelay);
//		}
	}
}

void ErrFlagTest()
{
	Test();

	double sgp4_orbit_params[] = { 0,0,0,0,0,0,0,0 };
	cspaceADCS_setSGP4OrbitParameters(ADCS_ID, sgp4_orbit_params);
	vTaskDelay(5000);
	restart();
}

void printErrFlag()
{
	Test();
	cspace_adcs_statetlm_t data;
	cspaceADCS_getStateTlm(ADCS_ID, &data);
	printf("\nSGP ERR FLAG = %d \n", data.fields.curr_state.fields.orbitparam_invalid);
}

void Mag_Test()
{
	Test();

	//generic i2c tm id 170 size 6 and print them
	cspace_adcs_rawmagmeter_t Mag;
	cspaceADCS_getRawMagnetometerMeas(0, &Mag);
	printf("mag raw x %d", Mag.fields.magnetic_x);
	printf("mag raw y %d", Mag.fields.magnetic_y);
	printf("mag raw z %d", Mag.fields.magnetic_z);


}

void AddCommendToQ()
{
	Test();

	int input;
	int err;
	TC_spl adcsCmd;
	adcsCmd.id = TC_ADCS_T;
	if (UTIL_DbguGetIntegerMinMax((unsigned int *)&(adcsCmd.subType), 0, 666) != 0) {
		printf("Enter ADCS command data\n");
		UTIL_DbguGetString((char*)&(adcsCmd.data), SIZE_OF_COMMAND + 1);
		err = AdcsCmdQueueAdd(&adcsCmd);
		printf("ADCS command error = %d\n\n\n", err);
		printf("Send new command\n");
		printf("Enter ADCS sub type\n");
	}

	if (UTIL_DbguGetIntegerMinMax((unsigned int *)&input, 900, 1000) != 0) {
		printf("Print the data\n");
	}

}

void TestStartAdcs()
{
	TC_spl cmd;
	int err = 0;
	cmd.type = TM_ADCS_T;
	cmd.subType = ADCS_RUN_MODE_ST;

	cmd.data[0] = 1;

	err = AdcsCmdQueueAdd(&cmd);
	if(0 != err){
		printf("ADCS test init command error = %d\n", err);
	}
	vTaskDelay(10);

	cmd.subType = ADCS_SET_PWR_CTRL_DEVICE_ST;
	cspace_adcs_powerdev_t pwr_dev;
	pwr_dev.fields.signal_cubecontrol = 1;
	pwr_dev.fields.motor_cubecontrol = 1;
	pwr_dev.fields.pwr_motor = 1;
	memcpy(cmd.data,pwr_dev.raw,sizeof(pwr_dev));
	err = AdcsCmdQueueAdd(&cmd);
	if(0 != err){
		printf("ADCS set pwr mode error = %d\n", err);
	}

	cmd.subType = ADCS_SET_EST_MODE_ST;
	cmd.data[0] = 1;
	err = AdcsCmdQueueAdd(cmd);
	if(0 != err){
		printf("ADCS set est mode error = %d\n", err);
	}

	cmd.subType = ADCS_SET_ATT_CTRL_MODE_ST;
	cspace_adcs_attctrl_mod_t ctrl = {0};
	ctrl.fields.ctrl_mode = 1;
	memcpy(cmd.data,&ctrl,sizeof(ctrl));
	err = AdcsCmdQueueAdd(cmd);
	if(0 != err){
		printf("ADCS set ctrl mode error = %d\n", err);
	}

}

void TaskMamagTest()
{
	TestStartAdcs();

	TestAdcsTLM();

	while(TRUE){
		vTaskDelay(1000);

	}

}
