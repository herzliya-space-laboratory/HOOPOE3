
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
#define ADCS_ID 0
#define CMD_FOR_TEST_AMOUNT 34
#define TestDelay (2*1000)
#define TEST_NUM 1

void TaskMamagTest();

void testInit()
{
	TC_spl cmd;
	int err = 0;
	cmd.type = TM_ADCS_T;
	cmd.subType = ADCS_RUN_MODE_ST;

	cmd.data[0] = 1;

	err = AdcsCmdQueueAdd(&cmd);
	if(0 != err){
		printf("\t---ADCS test init command error = %d\n", err);
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
		printf("\t---ADCS set pwr mode error = %d\n", err);
	}

	cmd.subType = ADCS_SET_EST_MODE_ST;
	cmd.data[0] = 1;
	err = AdcsCmdQueueAdd(&cmd);
	if(0 != err){
		printf("\t---ADCS set est mode error = %d\n", err);
	}

	cmd.subType = ADCS_SET_ATT_CTRL_MODE_ST;
	cspace_adcs_attctrl_mod_t ctrl = {.raw = {0}};
	ctrl.fields.ctrl_mode = 1;
	memcpy(cmd.data,&ctrl,sizeof(ctrl));
	err = AdcsCmdQueueAdd(&cmd);
	if(0 != err){
		printf("\t---ADCS set ctrl mode error = %d\n", err);
	}

}


void TestUpdateTlmToSaveVector(){
	int err = 0;
	Boolean8bit ToSaveVec[NUM_OF_ADCS_TLM] = {0};
	Boolean8bit temp[NUM_OF_ADCS_TLM] = {0};
	AdcsTlmElement_t elem = {0};
	memset(ToSaveVec,0xFF,NUM_OF_ADCS_TLM * sizeof(Boolean8bit));

	UpdateTlmToSaveVector(ToSaveVec);
	err = FRAM_read(temp,ADCS_TLM_SAVE_VECTOR_START_ADDR,NUM_OF_ADCS_TLM * sizeof(*temp));	// check update in FRAM
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
	for(int i = 0; i < 60; i++){
		GatherTlmAndData();
		vTaskDelay(1000);
	}
}

void TestUpdateTlmElemdntAtIndex(){
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
	printf("Test 'RestoreDefaultTlmElement'");
	RestoreDefaultTlmElement();
	vTaskDelay(100);

	printf("Test 'UpdateTlmToSaveVector'\n");
	TestUpdateTlmToSaveVector();
	vTaskDelay(100);

	printf("Test 'GatherTlmAndData' for 60 sec\n");
	TestGatherTlmAndData();
	vTaskDelay(100);

	printf("Test 'UpdateTlmElemdntAtIndex' (change tlm save freq to 1/6 Hz)\n");
	TestUpdateTlmElemdntAtIndex();
	vTaskDelay(100);

	printf("Test 'GatherTlmAndData' for 60 sec with updated array\n");
	TestGatherTlmAndData();
	vTaskDelay(100);

	printf("Test 'RestoreDefaultTlmElement'");
	RestoreDefaultTlmElement();
	vTaskDelay(100);

	printf("Test 'GatherTlmAndData' for 60 sec\n");
	TestGatherTlmAndData();
	vTaskDelay(100);
}

void BuildTests(uint8_t getSubType[CMD_FOR_TEST_AMOUNT], int getLength[CMD_FOR_TEST_AMOUNT], byte getData[CMD_FOR_TEST_AMOUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE], uint8_t setSubType[CMD_FOR_TEST_AMOUNT], int setLength[CMD_FOR_TEST_AMOUNT], byte setData[CMD_FOR_TEST_AMOUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE])
{
	adcs_i2c_cmd i2c_cmd;

	memset(setLength,0,CMD_FOR_TEST_AMOUNT * sizeof(*setLength));
	memset(getLength,0,CMD_FOR_TEST_AMOUNT * sizeof(*getLength));
	memset(setData,0,(SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE) * sizeof(*setLength));
	memset(getData,0,(SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE) * sizeof(*getLength));

	int testNum = 0;

	//test #0 data
	getSubType[testNum] = ADCS_GET_CURR_UNIX_TIME_ST;
	setSubType[testNum] = ADCS_SET_CURR_UNIX_TIME_ST;
	setLength[testNum] = 6;
	cspace_adcs_unixtm_t time;
	time.fields.unix_time_sec = 1566304200;
	time.fields.unix_time_millsec = 0;
	memcpy(setData[testNum],time.raw,sizeof(cspace_adcs_unixtm_t));
	testNum++;

	//test #1 data
	getSubType[testNum] = ADCS_I2C_GENRIC_ST; //generic I2C command #131
	getLength[testNum] = sizeof(adcs_i2c_cmd);
	i2c_cmd.id = 131;
	i2c_cmd.length = 1;
	i2c_cmd.ack = 3;
	memcpy(getData[testNum],&i2c_cmd,getLength[testNum]);
	setSubType[testNum] = ADCS_CACHE_ENABLE_ST;
	setLength[testNum] = sizeof(cspace_adcs_attctrl_mod_t);
	testNum++;

	//test #2 data
	getSubType[testNum] = ADCS_GET_CURRENT_STATE_ST;
	setSubType[testNum] = ADCS_SET_ATT_CTRL_MODE_ST;
	setData[testNum][0] = 1;
	setLength[testNum] = 1;
	testNum++;

	//test #3 data
	getSubType[testNum] = ADCS_GET_CURRENT_STATE_ST;
	setSubType[testNum] = ADCS_SET_EST_MODE_ST;
	setData[testNum][0] = 1;
	setLength[testNum] = 1;
	testNum++;

	//test #4 data
	getSubType[testNum] = ADCS_GET_MAGNETORQUER_CMD_ST;
	setSubType[testNum] = ADCS_SET_MAG_OUTPUT_ST;
	setLength[testNum] = sizeof(cspace_adcs_magnetorq_t);
	cspace_adcs_magnetorq_t MT;
	MT.fields.magduty_x = 0.8 * 1000;
	MT.fields.magduty_y = 0.8 * 1000;
	MT.fields.magduty_z = 0.8 * 1000;
	memcpy(setData[testNum],MT.raw,setLength[testNum]);
	testNum++;

	//test #5 data
	getSubType[testNum] = ADCS_GET_WHEEL_SPEED_CMD_ST;
	setSubType[testNum] = ADCS_SET_WHEEL_SPEED_ST;
	setLength[testNum] = sizeof(cspace_adcs_magnetorq_t);
	cspace_adcs_wspeed_t wheelConfig;
	wheelConfig.fields.speed_y = 0;
	memcpy(setData[testNum],wheelConfig.raw,setLength[testNum]);
	testNum++;

	//test #6 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MTQ_CONFIG_ST;
	setLength[testNum] = 3;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 6;
	}
	testNum++;

	//test #7 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_RW_CONFIG_ST;
	setLength[testNum] = 4;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 6;
	}
	testNum++;

	//test #8 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_GYRO_CONFIG_ST;
	setLength[testNum] = 10;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 6;
	}
	testNum++;

	//test #9 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_CSS_CONFIG_ST;
	testNum++;

	//test #10 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_CSS_RELATIVE_SCALE_ST;
	testNum++;

	//test #11 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MAGNETMTR_MOUNT_ST;
	testNum++;

	//test #12 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MAGNETMTR_OFFSET_ST;
	testNum++;

	//test #13 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MAGNETMTR_SENSTVTY_ST;
	testNum++;

	//test #14 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_RATE_SENSOR_OFFSET_ST;
	testNum++;

	//test #15 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_STAR_TRACKER_CONFIG_ST;
	testNum++;

	//test #16 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_DETUMB_CTRL_PARAM_ST;
	testNum++;

	//test #17 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_YWHEEL_CTRL_PARAM_ST;
	testNum++;

	//test #18 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_RWHEEL_CTRL_PARAM_ST;
	testNum++;

	//test #19 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MOMENT_INTERTIA_ST;
	testNum++;

	//test #20 data
	getSubType[testNum] = 255;
	setSubType[testNum] = ADCS_PROD_INERTIA_ST;
	testNum++;

	//test #21 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_ESTIMATION_PARAM1_ST;
	testNum++;

	//test #22 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_ESTIMATION_PARAM2_ST;
	testNum++;

	//tests #23-32 data all SGP4 commands

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

	for(testNum = 24; testNum < 32; testNum++){
		SGP4_Init[testNum-24] = 1; //change from 0 there from last test
		getSubType[testNum] = ADCS_GET_SGP4_ORBIT_PARAMETERS_ST;
		setLength[testNum] = sizeof(double);
		memcpy(setData[testNum], &SGP4_Init[testNum-24], setLength[testNum]);
	}
	testNum++;

	//test #32 data
	getSubType[testNum] = ADCS_GET_FULL_CONFIG_ST;
	setSubType[testNum] = ADCS_SET_MAGNETOMETER_MODE_ST;
	testNum++;
}

void AdcsTestTask()
{
	testInit();
	TC_spl get;
	TC_spl set;

	//TaskMamagTest(); // currently in infinite loop

	uint8_t getSubType[CMD_FOR_TEST_AMOUNT] = {0};
	int getLength[CMD_FOR_TEST_AMOUNT] = {0};
	byte GetData[CMD_FOR_TEST_AMOUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE] = {{0}};

	//function to test constructor
	uint8_t setSubType[CMD_FOR_TEST_AMOUNT] = {0};//function to test sst
	int setLength[CMD_FOR_TEST_AMOUNT] = {0};
	byte setData[CMD_FOR_TEST_AMOUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE] = {{0}};
	
	BuildTests(getSubType, getLength, GetData, setSubType, setLength, setData);
	printf("start test");
	int err;
	int continue_flag = 0;
	unsigned int test_num = 0;
	do{
		printf("which test would you like to perform?(0 to %d)\n",CMD_FOR_TEST_AMOUNT);
		while(UTIL_DbguGetIntegerMinMax(&test_num,0,CMD_FOR_TEST_AMOUNT) == 0);

		set.subType = setSubType[test_num];
		set.length = setLength[test_num];
		memcpy(set.data,setData[test_num],set.length);

		get.subType = getSubType[test_num];
		get.length = getLength[test_num];
		memcpy(get.data,GetData[test_num],get.length);

		err = AdcsCmdQueueAdd(&get);
		printf("\nsst:%d\n err:%d\n",get.subType, err);

		err = AdcsCmdQueueAdd(&set);
		printf("\nsst:%d\n err:%d\n data:%s\n",set.subType, err ,set.data);

		for(int i = 0; i < set.length; i++){
			printf("%X, ",set.data[i]);
		}
		err = AdcsCmdQueueAdd(&get);
		printf("\nsst:%d\t err:%d\n",get.subType, err);
		vTaskDelay(TestDelay);

		printf("Do you want to continue?(1 = Yes / 0 = No)\n");
		while(UTIL_DbguGetIntegerMinMax((unsigned int*)&continue_flag,0,1) == 0);
		if(continue_flag == 0){
			vTaskDelete(NULL);
			return;
		}
	}while(TRUE);
}

/*
//void ErrFlagTest()
//{
//	double sgp4_orbit_params[] = { 0,0,0,0,0,0,0,0 };
//	cspaceADCS_setSGP4OrbitParameters(ADCS_ID, sgp4_orbit_params);
//	vTaskDelay(5000);
//	restart();
//}

//void printErrFlag()
//{
//
//	cspace_adcs_statetlm_t data;
//	cspaceADCS_getStateTlm(ADCS_ID, &data);
//	printf("\nSGP ERR FLAG = %d \n", data.fields.curr_state.fields.orbitparam_invalid);
//}

//void Mag_Test()
//{
//	//generic i2c tm id 170 size 6 and print them
//	cspace_adcs_rawmagmeter_t Mag;
//	cspaceADCS_getRawMagnetometerMeas(0, &Mag);
//	printf("mag raw x %d", Mag.fields.magnetic_x);
//	printf("mag raw y %d", Mag.fields.magnetic_y);
//	printf("mag raw z %d", Mag.fields.magnetic_z);
//}

//void AddCommendToQ()
//{
//	Test();
//
//	int input;
//	int err;
//	TC_spl adcsCmd;
//	adcsCmd.id = TC_ADCS_T;
//	if (UTIL_DbguGetIntegerMinMax((unsigned int *)&(adcsCmd.subType), 0, 666) != 0) {
//		printf("Enter ADCS command data\n");
//		UTIL_DbguGetString((char*)&(adcsCmd.data), SIZE_OF_COMMAND + 1);
//		err = AdcsCmdQueueAdd(&adcsCmd);
//		printf("ADCS command error = %d\n\n\n", err);
//		printf("Send new command\n");
//		printf("Enter ADCS sub type\n");
//	}
//
//	if (UTIL_DbguGetIntegerMinMax((unsigned int *)&input, 900, 1000) != 0) {
//		printf("Print the data\n");
//	}
//
//}
*/


void TestMagnetometer()
{
	//TODO: collect TLM, print + transmit
}

void TaskMamagTest()
{
	testInit();

	TestAdcsTLM();

	while(TRUE){

		TestMagnetometer();

		vTaskDelay(1000);
	}

}
