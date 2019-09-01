
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
#define TestDelay (10*1000)
#define INIT_DELAY (20*1000)
#define TEST_NUM 1


void AdcsConfigPramTest()
{
	printf("config test\n");
	TC_spl test = {0};
	test.type = TM_ADCS_T;
	test.subType = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	test.length = 4;
	test.data[0] = 0;
	test.data[1] = 0;
	test.data[2] = 10;
	test.data[3] = 0;
	AdcsCmdQueueAdd(&test);
	test.subType = ADCS_GET_FULL_CONFIG_ST;
	test.length = 4;
	test.data[0] = 0;
	test.data[1] = 0;
	test.data[2] = 0;
	test.data[3] = 0;
	AdcsCmdQueueAdd(&test);
	vTaskDelay(TestDelay);
}
void testInit()
{
	TC_spl cmd;
	int err = 0;
	cmd.type = TM_ADCS_T;
	cmd.subType = ADCS_RUN_MODE_ST;
	cmd.length =1;
	cmd.data[0] = 1;

	err = AdcsCmdQueueAdd(&cmd);
	if(0 != err){
		printf("\t---ADCS test init command error = %d\n", err);
	}
	vTaskDelay(INIT_DELAY);

	cmd.subType = ADCS_SET_PWR_CTRL_DEVICE_ST;
	cmd.length = sizeof(cspace_adcs_powerdev_t);
	cspace_adcs_powerdev_t pwr_dev = {.raw = {0}};
	pwr_dev.fields.signal_cubecontrol = 1;
	pwr_dev.fields.motor_cubecontrol = 1;
	pwr_dev.fields.pwr_motor = 1;
	memcpy(cmd.data,pwr_dev.raw,cmd.length);
	err = AdcsCmdQueueAdd(&cmd);
	if(0 != err){
		printf("\t---ADCS set pwr mode error = %d\n", err);
	}

	cmd.subType = ADCS_SET_EST_MODE_ST;
	cmd.length = 1;
	cmd.data[0] = 1;
	err = AdcsCmdQueueAdd(&cmd);
	if(0 != err){
		printf("\t---ADCS set est mode error = %d\n", err);
	}

	cmd.subType = ADCS_SET_ATT_CTRL_MODE_ST;
	cmd.length = sizeof(cspace_adcs_attctrl_mod_t);
	cspace_adcs_attctrl_mod_t ctrl = {.raw = {0}};
	ctrl.fields.ctrl_mode = 1;
	memcpy(cmd.data,&ctrl,cmd.length);
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
	i2c_cmd.id = 131;
	i2c_cmd.length = 1;
	i2c_cmd.ack = 3;
	getLength[testNum] = sizeof(adcs_i2c_cmd) - ADCS_CMD_MAX_DATA_LENGTH + i2c_cmd.length;
	memcpy(getData[testNum],&i2c_cmd,getLength[testNum]);
	setSubType[testNum] = ADCS_CACHE_ENABLE_ST;
	setData[testNum][0] = 1;
	setLength[testNum] = 1;
	testNum++;

	//test #2 data
	getSubType[testNum] = ADCS_GET_CURRENT_STATE_ST;
	setSubType[testNum] = ADCS_SET_ATT_CTRL_MODE_ST;
	setLength[testNum] = sizeof(cspace_adcs_attctrl_mod_t);
	cspace_adcs_attctrl_mod_t CT;
	CT.fields.ctrl_mode = 1;
	CT.fields.override_flag = 0;
	CT.fields.timeout = 10;
	memcpy(setData[testNum],CT.raw,setLength[testNum]);
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
	wheelConfig.fields.speed_y = 4000;
	wheelConfig.fields.speed_z = 0;
	wheelConfig.fields.speed_x = 0;
	memcpy(setData[testNum],wheelConfig.raw,setLength[testNum]);
	testNum++;

	//test #6 data
	setSubType[testNum] = ADCS_SET_MTQ_CONFIG_ST;
	setLength[testNum] = MTQ_CONFIG_CMD_DATA_LENGTH;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 6;
	}
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 0;
	getData[testNum][2] = 3;
	testNum++;

	//test #7 data
	setSubType[testNum] = ADCS_RW_CONFIG_ST;
	setLength[testNum] = SET_WHEEL_CONFIG_DATA_LENGTH;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 6;
	}
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 3;
	getData[testNum][2] = 4;
	testNum++;

	//test #8 data
	setSubType[testNum] = ADCS_GYRO_CONFIG_ST;
	setLength[testNum] = SET_GYRO_CONFIG_DATA_LENGTH;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 6;
	}
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 7;
	getData[testNum][2] = 3;
	testNum++;

	//test #9 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 10;
	getData[testNum][2] = 10;
	setSubType[testNum] = ADCS_CSS_CONFIG_ST;
	setLength[testNum] = SET_CSS_CONFIG_DATA_LENGTH;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 0;
	}
	testNum++;

	//test #10 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 20;
	getData[testNum][2] = 11;
	setSubType[testNum] = ADCS_CSS_RELATIVE_SCALE_ST;
	setLength[testNum] = SET_CSS_SCALE_DATA_LENGTH;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 0;
	}
	testNum++;

	//test #11 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 101;
	getData[testNum][2] = 6;
	setSubType[testNum] = ADCS_SET_MAGNETMTR_MOUNT_ST;
	setLength[testNum] = sizeof(cspace_adcs_magmountcfg_t);
	cspace_adcs_magmountcfg_t magMount;
	magMount.fields.mounttrans_alpha_ang = 0;
	magMount.fields.mounttrans_beta_ang = 0;
	magMount.fields.mounttrans_gamma_ang = 0;
	memcpy(setData[testNum],magMount.raw,setLength[testNum]);
	testNum++;

	//test #12 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 107;
	getData[testNum][2] = 12;
	setSubType[testNum] = ADCS_SET_MAGNETMTR_OFFSET_ST;
	setLength[testNum] = sizeof(cspace_adcs_offscal_cfg_t);
	cspace_adcs_offscal_cfg_t magOffset;
	magOffset.fields.mag_ch1off = 1;
	magOffset.fields.mag_ch2off = 2;
	magOffset.fields.mag_ch3off = 3;
	magOffset.fields.mag_sensmatrix11 = 4;
	magOffset.fields.mag_sensmatrix22 = 5;
	magOffset.fields.mag_sensmatrix33 = 6;
	memcpy(setData[testNum],magOffset.raw,setLength[testNum]);
	testNum++;

	//test #13 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 119;
	getData[testNum][2] = 12;
	setSubType[testNum] = ADCS_SET_MAGNETMTR_SENSTVTY_ST;
	setLength[testNum] = sizeof(cspace_adcs_magsencfg_t);
	cspace_adcs_magsencfg_t magSens;
	magSens.fields.mag_sensmatrix_s12 = 1;
	magSens.fields.mag_sensmatrix_s13 = 2;
	magSens.fields.mag_sensmatrix_s21 = 3;
	magSens.fields.mag_sensmatrix_s23 = 4;
	magSens.fields.mag_sensmatrix_s31 = 5;
	magSens.fields.mag_sensmatrix_s32 = 6;
	memcpy(setData[testNum],magSens.raw,setLength[testNum]);
	testNum++;

	//test #14 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 131;
	getData[testNum][2] = 7;
	setSubType[testNum] = ADCS_RATE_SENSOR_OFFSET_ST;
	setLength[testNum] = sizeof(cspace_adcs_ratesencfg_t);
	cspace_adcs_ratesencfg_t rateOffset;
	rateOffset.fields.rate_senmult = 1;
	rateOffset.fields.ratesen_off_x = 2;
	rateOffset.fields.ratesen_off_y = 3;
	rateOffset.fields.ratesen_off_z = 4;
	memcpy(setData[testNum],rateOffset.raw,setLength[testNum]);
	testNum++;

	//test #15 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 138;
	getData[testNum][2] = 26;
	setSubType[testNum] = ADCS_SET_STAR_TRACKER_CONFIG_ST;
	setLength[testNum] = sizeof(cspace_adcs_startrkcfg_t);
	//cspace_adcs_startrkcfg_t starConfig;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = i+1;
	}
	testNum++;

	//test #16 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 164;
	getData[testNum][2] = 14;
	setSubType[testNum] = ADCS_SET_DETUMB_CTRL_PARAM_ST;
	setLength[testNum] = DETUMB_CTRL_PARAM_DATA_LENGTH;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 0;
	}
	testNum++;

	//test #17 data //TODO
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 0;
	getData[testNum][2] = 20;
	setSubType[testNum] = ADCS_SET_YWHEEL_CTRL_PARAM_ST;
	setLength[testNum] = SET_YWHEEL_CTRL_PARAM_DATA_LENGTH;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 0;
	}
	testNum++;

	//test #18 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 198;
	getData[testNum][2] = 8;
	setSubType[testNum] = ADCS_SET_RWHEEL_CTRL_PARAM_ST;
	setLength[testNum] = SET_RWHEEL_CTRL_PARAM_DATA_LENGTH;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 0;
	}
	testNum++;

	//test #19 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 218;
	getData[testNum][2] = 24; //TODO: set is only 12
	setSubType[testNum] = ADCS_SET_MOMENT_INTERTIA_ST;
	setLength[testNum] = SET_MOMENT_INERTIA_DATA_LENGTH;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 0;
	}
	testNum++;

	//test #20 data
	getSubType[testNum] = 255; //NO GET
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 0;
	getData[testNum][2] = 0;
	setSubType[testNum] = ADCS_PROD_INERTIA_ST;
	setLength[testNum] = SET_PROD_INERTIA_DATA_LENGTH;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 0;
	}
	testNum++;

	//test #21 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0;
	getData[testNum][1] = 242;
	getData[testNum][2] = sizeof(cspace_adcs_estparam1_t);
	setSubType[testNum] = ADCS_ESTIMATION_PARAM1_ST;
	setLength[testNum] = 16;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 0;
	}
	testNum++;

	//test #22 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 3;
	getData[testNum][0] = 0x01;
	getData[testNum][1] = 0x02;
	getData[testNum][2] = sizeof(cspace_adcs_estparam2_t);
	setSubType[testNum] = ADCS_ESTIMATION_PARAM2_ST;
	setLength[testNum] = 14;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 0;
	}
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
		memcpy(&(setData[testNum][0]), &SGP4_Init[testNum-24], setLength[testNum]);
	}
	testNum++;

	//test #32 data
	getSubType[testNum] = ADCS_GET_ADCS_CONFIG_PARAM_ST;
	getLength[testNum] = 0;
	getData[testNum][0] = 0x0f;
	getData[testNum][1] = 0x01;
	getData[testNum][2] = 1;
	setSubType[testNum] = ADCS_SET_MAGNETOMETER_MODE_ST;
	setLength[testNum] = 3;
	for(int i = 0; i<setLength[testNum]; i++){
		setData[testNum][i] = 0;
	}
	testNum++;

	//test #33 data
	getSubType[testNum] = ADCS_NOP_ST;
	getLength[testNum] = 0;
	setSubType[testNum] = ADCS_RESET_BOOT_REGISTER_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	memset(setData[testNum],0,setLength[testNum]);
	testNum++;

	//test #34 data
	getSubType[testNum] = ADCS_NOP_ST;
	getLength[testNum] = 0;
	setSubType[testNum] = ADCS_CLEAR_ERRORS_ST;
	setLength[testNum] = 1;
	setData[testNum][0] = 0xFF;
	//memset(setData[testNum],0,setLength[testNum]);
	testNum++;

	//test #35 data
	getSubType[testNum] = ADCS_NOP_ST;
	getLength[testNum] = 0;
	setSubType[testNum] = ADCS_SAVE_CONFIG_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0xFF;
	testNum++;

	//test #36 data
	getSubType[testNum] = ADCS_NOP_ST;
	getLength[testNum] = 0;
	setSubType[testNum] = ADCS_SAVE_ORBIT_PARAM_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0xFF;
	testNum++;

	//test #37 data
	getSubType[testNum] = ADCS_NOP_ST;
	getLength[testNum] = 0;
	setSubType[testNum] = ADCS_SAVE_IMAGE_ST;
	setLength[testNum] = 2;
	setData[testNum][0] = 0x01;
	setData[testNum][1] = 0x02;
	testNum++;


	//test #38 data
	getSubType[testNum] = ADCS_I2C_GENRIC_ST;
	i2c_cmd.id = 130;
	i2c_cmd.length = 2;
	getLength[testNum] = sizeof(adcs_i2c_cmd) - ADCS_CMD_MAX_DATA_LENGTH + i2c_cmd.length;
	memcpy(getData[testNum],&i2c_cmd,getLength[testNum]);

	setSubType[testNum] = ADCS_SET_BOOT_INDEX_ST;
	setLength[testNum] = 1;
	setData[testNum][0] = 0;
	testNum++;

	//test #39 data
	getSubType[testNum] = ADCS_NOP_ST;
	getLength[testNum] = 0

	setSubType[testNum] = ADCS_RUN_BOOT_PROGRAM_ST;
	setLength[testNum] = 0;
	testNum++;

	//test #40 data
	getSubType[testNum] = ADCS_GET_GENERAL_INFO_ST;
	getLength[testNum] = 8;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #41 data
	getSubType[testNum] = ADCS_GET_BOOT_PROGRAM_INFO_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #42 data
	getSubType[testNum] = ADCS_GET_SRAM_LATCHUP_COUNTERS_ST;
	getLength[testNum] = 4;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;


	//test #43 data
	getSubType[testNum] = ADCS_GET_EDAC_COUNTERS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #44 data
	getSubType[testNum] = ADCS_GET_COMM_STATUS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #45 data
	getSubType[testNum] = ADCS_GET_MAGNETIC_FIELD_VEC_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;


	//test #46 data
	getSubType[testNum] = ADCS_GET_COARSE_SUN_VEC_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #47 data
	getSubType[testNum] = ADCS_GET_FINE_SUN_VEC_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #48 data
	getSubType[testNum] = ADCS_GET_NADIR_VECTOR_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #49 data
	getSubType[testNum] = ADCS_GET_SENSOR_RATES_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #50 data
	getSubType[testNum] = ADCS_GET_ADC_RAW_RED_MAG_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #51 data
	getSubType[testNum] = ADCS_GET_ACP_EXECUTION_STATE_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #52 data
	getSubType[testNum] = ADCS_GET_RAW_CSS_1_6_MEASURMENTS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #53 data
	getSubType[testNum] = ADCS_GET_RAW_CSS_7_10_MEASURMENTS_ST;
	getLength[testNum] = 4;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #54 data
	getSubType[testNum] = ADCS_GET_RAW_MAGNETOMETER_MEAS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #55 data
	getSubType[testNum] = ADCS_GET_C_SENSE_CURRENT_MEASUREMENTS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #56 data
	getSubType[testNum] = ADCS_GET_C_CONTROL_CURRENT_MEASUREMENTS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #57 data
	getSubType[testNum] = ADCS_GET_WHEEL_CURRENTS_TLM_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #58 data
	getSubType[testNum] = ADCS_GET_ADCS_TEMPERATURE_TLM_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;


	//test #59 data
	getSubType[testNum] = ADCS_GET_RATE_SENSOR_TEMP_TLM_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;


	//test #60 data
	getSubType[testNum] = ADCS_GET_RAW_GPS_STATUS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;


	//test #61 data
	getSubType[testNum] = ADCS_GET_RAW_GPS_TIME_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;


	//test #62 data
	getSubType[testNum] = ADCS_GET_RAW_GPS_X_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;


	//test #63 data
	getSubType[testNum] = ADCS_GET_RAW_GPS_Y_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;


	//test #64 data
	getSubType[testNum] = ADCS_GET_RAW_GPS_Z_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #65 data
	getSubType[testNum] = ADCS_GET_STATE_TLM_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++

	//test #66 data
	getSubType[testNum] = ADCS_GET_ADCS_MEASUREMENTS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #67 data
	getSubType[testNum] = ADCS_GET_ESTIMATION_METADATA_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #68 data
	getSubType[testNum] = ADCS_GET_SENSOR_MEASUREMENTS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #69 data
	getSubType[testNum] = ADCS_GET_POW_TEMP_MEAS_TLM_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #70 data
	getSubType[testNum] = ADCS_GET_ADCS_EXC_TIMES_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #71 data
	getSubType[testNum] = ADCS_GET_PWR_CTRL_DEVICE_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #72 data
	getSubType[testNum] = ADCS_GET_MISC_CURRENT_MEAS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #73 data
	getSubType[testNum] = ADCS_GET_COMMANDED_ATTITUDE_ANGLES_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;


	//test #74 data
	getSubType[testNum] = ADCS_GET_ADCS_RAW_GPS_MEAS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #75 data
	getSubType[testNum] = ADCS_GET_STAR_TRACKER_TLM_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;

	//test #76 data
	getSubType[testNum] = ADCS_RESET_TLM_ELEMENTS_ST;
	getLength[testNum] = 6;

	setSubType[testNum] = ADCS_NOP_ST;
	setLength[testNum] = 0;
	setData[testNum][0] = 0;
	testNum++;
}

void AdcsTestTask()
{
	testInit();
	TC_spl get;
	TC_spl set;
	get.type = TM_ADCS_T;
	set.type = TM_ADCS_T;


	uint8_t getSubType[CMD_FOR_TEST_AMOUNT] = {0};
	int getLength[CMD_FOR_TEST_AMOUNT] = {0};
	byte GetData[CMD_FOR_TEST_AMOUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE] = {{0}};

	//function to test constructor
	uint8_t setSubType[CMD_FOR_TEST_AMOUNT] = {0};//function to test sst
	int setLength[CMD_FOR_TEST_AMOUNT] = {0};
	byte setData[CMD_FOR_TEST_AMOUNT][SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE] = {{0}};

	BuildTests(getSubType, getLength, GetData, setSubType, setLength, setData);
	printf("start test\n");
	int err;
	int continue_flag = 0;
	unsigned int test_num = 0;
	vTaskDelay(10);
	AdcsConfigPramTest();
	vTaskDelay(10);
	cspace_adcs_currstate_t State;
	cspace_adcs_powerdev_t PowerADCS;
	do{
		printf("which test would you like to perform?(0 to %d)\n",CMD_FOR_TEST_AMOUNT);
		while(UTIL_DbguGetIntegerMinMax(&test_num,0,CMD_FOR_TEST_AMOUNT) == 0);
		cspaceADCS_getPwrCtrlDevice(0,&PowerADCS);
		cspaceADCS_getCurrentState(0,&State);
		printf("run:%d\t",State.fields.run_mode);
		printf("signel:%d\t",PowerADCS.fields.signal_cubecontrol);
		printf("control:%d\t",PowerADCS.fields.motor_cubecontrol);
		printf("moter:%d\n",PowerADCS.fields.pwr_motor);
		set.subType = setSubType[test_num];
		set.length = setLength[test_num];
		memcpy(set.data,&setData[test_num][0],set.length);
		vTaskDelay(10);
		get.id = 0;
		get.subType = getSubType[test_num];
		get.length = getLength[test_num];
		memcpy(get.data,&GetData[test_num][0],get.length);
		vTaskDelay(10);
		err = AdcsCmdQueueAdd(&get);
		vTaskDelay(10);
		printf("\nsst:%d\terr:%d\n",get.subType, err);

		err = AdcsCmdQueueAdd(&set);
		vTaskDelay(10);
		printf("sst: %d\terr: %d\tdata: ",set.subType, err);

		for(int i = 0; i < set.length; i++){
			printf("%X, ",set.data[i]);
		}
		printf("\n");
		err = AdcsCmdQueueAdd(&get);
		vTaskDelay(10);
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

