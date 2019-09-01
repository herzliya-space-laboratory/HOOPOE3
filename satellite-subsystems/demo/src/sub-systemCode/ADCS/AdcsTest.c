
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
#ifndef ADCS_ID
	#define ADCS_ID 0
#endif



typedef enum __attribute__ ((__packed__)){
	fType_TLM = 0,
	fType_Get_Config = 1,
	fType_Set_Config = 2,
	fType_Misc = 3
}AdcsFunctionType;

typedef struct __attribute__ ((__packed__)){
	unsigned char subtype;
	AdcsFunctionType func_type;
	Boolean8bit DefaultDataField;
	short CrspdngSt;
	unsigned short dataLength;
	unsigned char data[ADCS_FULL_CONFIG_DATA_LENGTH];
}AdcsCmdTestProtocol_t;

AdcsCmdTestProtocol_t TestCmd[] = {
		{.subtype = ADCS_I2C_GENRIC_ST,					.func_type = fType_Misc,		.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 0, 	.data = {0}} ,
		{.subtype = ADCS_COMPONENT_RESET_ST,			.func_type = fType_Misc,		.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 0, 	.data = {0}} ,
		{.subtype = ADCS_SET_CURR_UNIX_TIME_ST,			.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 6, 	.data = {0}},
		{.subtype = ADCS_CACHE_ENABLE_ST,				.func_type = fType_Set_Config,	.DefaultDataField = FALSE_8BIT,	.CrspdngSt = -1, .dataLength = 1,	.data = {0x01}},
		{.subtype = ADCS_RESET_BOOT_REGISTER_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 0, 	.data = {0}},
		{.subtype = ADCS_RUN_MODE_ST,					.func_type = fType_Set_Config,	.DefaultDataField = FALSE_8BIT,	.CrspdngSt = -1, .dataLength = 1, 	.data = {0x01}},
		{.subtype = ADCS_SET_PWR_CTRL_DEVICE_ST,		.func_type = fType_Set_Config,	.DefaultDataField = FALSE_8BIT,	.CrspdngSt = -1, .dataLength = 0, 	.data = {0x05,0x01,0x00}},
		{.subtype = ADCS_CLEAR_ERRORS_ST,				.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 1, 	.data = {0}},
		{.subtype = ADCS_SET_ATT_CTRL_MODE_ST,			.func_type = fType_Set_Config,	.DefaultDataField = FALSE_8BIT,	.CrspdngSt = -1, .dataLength = 3, 	.data = {0x01,0x00,0x00}},
		{.subtype = ADCS_SET_EST_MODE_ST,				.func_type = fType_Set_Config,	.DefaultDataField = FALSE_8BIT,	.CrspdngSt = -1, .dataLength = 1, 	.data = {0x01}},
		{.subtype = ADCS_SET_MAG_OUTPUT_ST,				.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 6, 	.data = {0}},
		{.subtype = ADCS_SET_WHEEL_SPEED_ST,			.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 6, 	.data = {0}},
		{.subtype = ADCS_SET_MTQ_CONFIG_ST,				.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 3, 	.data = {0}},
		{.subtype = ADCS_RW_CONFIG_ST,					.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 3, 	.data = {0}},
		{.subtype = ADCS_GYRO_CONFIG_ST,				.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 3, 	.data = {0}},
		{.subtype = ADCS_CSS_CONFIG_ST,					.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 10, 	.data = {0}},
		{.subtype = ADCS_CSS_RELATIVE_SCALE_ST,			.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 11, 	.data = {0}},
		{.subtype = ADCS_SET_MAGNETMTR_MOUNT_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 6,  	.data = {0}},
		{.subtype = ADCS_SET_MAGNETMTR_OFFSET_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 12, 	.data = {0}},
		{.subtype = ADCS_SET_MAGNETMTR_SENSTVTY_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 12, 	.data = {0}},
		{.subtype = ADCS_RATE_SENSOR_OFFSET_ST,			.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 6,	.data = {0}},
		{.subtype = ADCS_SET_DETUMB_CTRL_PARAM_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 14, 	.data = {0}},
		{.subtype = ADCS_SET_YWHEEL_CTRL_PARAM_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 20, 	.data = {0}},
		{.subtype = ADCS_SET_RWHEEL_CTRL_PARAM_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 8, 	.data = {0}},
		{.subtype = ADCS_SET_MOMENT_INTERTIA_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 12, 	.data = {0}},
		{.subtype = ADCS_PROD_INERTIA_ST,				.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 12, 	.data = {0}},
		{.subtype = ADCS_ESTIMATION_PARAM1_ST,			.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 16, 	.data = {0}},
		{.subtype = ADCS_ESTIMATION_PARAM2_ST,			.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 14, 	.data = {0}},
		{.subtype = ADCS_SET_SGP4_ORBIT_PARAMS_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 64, 	.data = {0}},
		{.subtype = ADCS_SET_SGP4_ORBIT_INC_ST,			.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 8, 	.data = {0}},
		{.subtype = ADCS_SET_SGP4_ORBIT_ECC_ST,			.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 8, 	.data = {0}},
		{.subtype = ADCS_SET_SGP4_ORBIT_RAAN_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 8, 	.data = {0}},
		{.subtype = ADCS_SET_SGP4_ORBIT_ARG_OF_PER_ST,	.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 8, 	.data = {0}},
		{.subtype = ADCS_SET_SGP4_ORBIT_BSTAR_DRAG_ST,	.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 8, 	.data = {0}},
		{.subtype = ADCS_SET_SGP4_ORBIT_MEAN_MOT_ST,	.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 8, 	.data = {0}},
		{.subtype = ADCS_SET_SGP4_ORBIT_MEAN_ANOM_ST,	.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 8, 	.data = {0}},
		{.subtype = ADCS_SET_SGP4_ORBIT_EPOCH_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 8, 	.data = {0}},
		{.subtype = ADCS_SET_MAGNETOMETER_MODE_ST,		.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 1, 	.data = {0}},
		{.subtype = ADCS_SAVE_CONFIG_ST,				.func_type = fType_Misc,		.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 0, 	.data = {0}},
		{.subtype = ADCS_SAVE_ORBIT_PARAM_ST,			.func_type = fType_Misc,		.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 0, 	.data = {0}},
		{.subtype = ADCS_SAVE_IMAGE_ST,					.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 2, 	.data = {0}},
		{.subtype = ADCS_SET_BOOT_INDEX_ST,				.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 1, 	.data = {0}},
		{.subtype = ADCS_RUN_BOOT_PROGRAM_ST,			.func_type = fType_Set_Config,	.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 0, 	.data = {0}},
		{.subtype = ADCS_UPDATE_TLM_ELEMENT_AT_INDEX_ST,.func_type = fType_Misc,		.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 3, 	.data = {0}},
		{.subtype = ADCS_RESET_TLM_ELEMENTS_ST,			.func_type = fType_Misc,		.DefaultDataField = TRUE_8BIT,	.CrspdngSt = -1, .dataLength = 0, 	.data = {0}}
};



void TestUpdateTlmToSaveVector(){
	int err = 0;
	Boolean8bit ToSaveVec[NUM_OF_ADCS_TLM] = {0};
	Boolean8bit temp[NUM_OF_ADCS_TLM] = {0};
	AdcsTlmElement_t elem = {0};
	memset(ToSaveVec,0xFF,NUM_OF_ADCS_TLM * sizeof(Boolean8bit));

	AdcsSelectWhichTlmToSave(ToSaveVec);
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

void TestInitCmdTestProtocol(AdcsCmdTestProtocol_t *cmd){
	if(NULL == cmd){
		return;
	}
	if(TRUE_8BIT == cmd->DefaultDataField){
		for(unsigned int i = 1; i <= cmd->dataLength; i++){
			cmd->data[i] = i;
		}
	}
}

void GetNextTestData(unsigned int index, TC_spl *next_cmd)
{
	if(NULL == next_cmd){
		return;
	}
	next_cmd->id = 0x12345678 + index;
	next_cmd->length = TestCmd[index].dataLength;
	next_cmd->time = 0;

	TestInitCmdTestProtocol(&TestCmd[index]);
	memcpy(next_cmd->data,TestCmd[index].data,TestCmd[index].dataLength);

	next_cmd->type = 154;
	next_cmd->subType = TestCmd[index].subtype;

}


static unsigned int test_cmd_index = 0;

void TestTaskAdcsNew()
{
	TroubleErrCode trbl = 0;
	TC_spl cmd = {0};

	vTaskDelay(20000);
	//unsigned int num_of_cmd_tests = sizeof(TestCmd) / sizeof(TestCmd[0]);

	GetNextTestData(test_cmd_index,&cmd);

	printf("\nStart ADCS Testing Task\n");
	while(1){
		trbl = AdcsCmdQueueAdd(&cmd);
		if(TRBL_SUCCESS != trbl){
			printf("----Error in AdcsCmdQueueAdd\n");
		}
		test_cmd_index++;

		vTaskDelay(5000);
	}
}

