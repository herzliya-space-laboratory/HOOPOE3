
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



typedef enum __attribute__ ((__packed__)) AdcsFunctionType{
	fType_TLM = 0,
	fType_Get_Config = 1,
	fType_Set_Config = 2,
	fType_Misc = 3
}AdcsFunctionType;

typedef struct __attribute__ ((__packed__)) AdcsCmdTestProtocol_t{
	unsigned char subtype;
	AdcsFunctionType func_type;
	Boolean8bit DefaultDataField;
	short CrspdngSt;
	unsigned short dataLength;
	unsigned char data[80];		//excluding the 272 ADCS config TLM/CMD
	unsigned char I2cData[70];
}AdcsCmdTestProtocol_t;


AdcsCmdTestProtocol_t TestCmd[] = {
		{}
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

