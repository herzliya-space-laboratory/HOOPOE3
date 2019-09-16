#include "AdcsCommissioningTest.h"
#include "sub-systemCode/Global/Global.h"
#include "sub-systemCode/COMM/splTypes.h"
#include "sub-systemCode/COMM/GSC.h"
#include <hal/Utility/util.h>
#include "sub-systemCode/Main/CMD/ADCS_CMD.h"

typedef struct __attribute__ ((__packed__))
{
	unsigned char subtype;
	unsigned char data[20];
	unsigned char length;
	unsigned int delay_duration;
}AdcsComsnCmd_t;



AdcsComsnCmd_t InitialAngularRateEstimation[] =
{
	{.subtype = ADCS_UPDATE_TLM_SAVE_VEC_ST,	.data =
	{0xFF,0x00,0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.length = 18, .delay_duration = 100},

	{.subtype = ADCS_UPDATE_TLM_PERIOD_VEC_ST,	.data =
	{10,0,10,0,10,0,0,10,0,0,0,0,0,0,0,0,0,0},
	.length = 18, .delay_duration = 100},

	{.subtype = ADCS_SET_ADCS_LOOP_PARAMETERS_ST,	.data = {0x00,0xE8,0x03,0x00,0x00},
	.length = 5, .delay_duration = 100},

	{.subtype = ADCS_RUN_MODE_ST,	.data = {0x01},	.length = 1, .delay_duration = 100},

	{.subtype = ADCS_SET_PWR_CTRL_DEVICE_ST,	.data = {0x05,0x40,0x00}, .length = 3, .delay_duration = 100},

	{.subtype = ADCS_SET_EST_MODE_ST,	.data = {0x02},	.length = 1, .delay_duration = 100}

};

AdcsComsnCmd_t Detumbling[] =
{
	{.subtype = ADCS_UPDATE_TLM_SAVE_VEC_ST,	.data =
	{0xFF,0x00,0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	.length = 1, .delay_duration = 100},

	{.subtype = ADCS_UPDATE_TLM_PERIOD_VEC_ST,	.data =
	{10,0,10,0,10,0,0,10,0,10,0,0,0,0,0,0,0,0},
	.length = 18, .delay_duration = 100},

	{.subtype = ADCS_SET_ADCS_LOOP_PARAMETERS_ST,	.data = {0x00,0xE8,0x03,0x00,0x00},
	.length = 5, .delay_duration = 100},

	{.subtype = ADCS_RUN_MODE_ST,	.data = {0x01},	.length = 1, .delay_duration = 100},

	{.subtype = ADCS_SET_PWR_CTRL_DEVICE_ST,	.data = {0x05,0x40,0x00}, .length = 3,	.delay_duration = 100},

	{.subtype = ADCS_SET_EST_MODE_ST,	.data = {0x02},	.length = 1, .delay_duration = 6000},

	{.subtype = ADCS_SET_ATT_CTRL_MODE_ST,	.data = {0x01,0x00,0x58,0x02},	.length = 4, .delay_duration = (5*60*1000)}		// 1'st activation
	//	{.subtype = ADCS_SET_ATT_CTRL_MODE_ST,	.data = {0x02,0x00,0x58,0x02},	.length = 4, .delay_duration = (6*60*1000)}	// 2'nd activation
	//	{.subtype = ADCS_SET_ATT_CTRL_MODE_ST,	.data = {0x02,0x00,0x00,0x00},	.length = 4, .delay_duration = 100)}		// final activation
};

AdcsComsnCmd_t MagnetometerDeployment[] =
{
	{.subtype = ADCS_UPDATE_TLM_SAVE_VEC_ST,	.data =
	{0xFF,0x00,0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00},
	.length = 18, .delay_duration = 100},

	{.subtype = ADCS_UPDATE_TLM_PERIOD_VEC_ST,	.data =
	{1,0,1,0,1,0,0,1,0,0,0,0,1,1,0,0,0,0},
	.length = 18, .delay_duration = 100},

	{.subtype = ADCS_SET_ADCS_LOOP_PARAMETERS_ST,	.data = {0x00,0x64,0x00,0x00,0x00},
	.length = 5, .delay_duration = 100},

	{.subtype = ADCS_RUN_MODE_ST,	.data = {0x01},	.length = 1, .delay_duration = 100},

	{.subtype = ADCS_SET_PWR_CTRL_DEVICE_ST,	.data = {0x05,0x40,0x00}, .length = 3,	.delay_duration = 100},

	{.subtype = ADCS_SET_EST_MODE_ST,	.data = {0x02},	.length = 1, .delay_duration = 6000},

	{.subtype = ADCS_SET_ATT_CTRL_MODE_ST,	.data = {0x00,0x00,0x00,0x00},	.length = 4, .delay_duration = 100},

	{.subtype = ADCS_NOP_ST, .data = {0},	.length = 0, .delay_duration = 100},

	{.subtype = ADCS_SET_MAGNETMTR_MOUNT_ST,	.data = {0x28,0x23,0xDC,0xD8,0x00,0x00}, .length = 6, .delay_duration = 100}

};

AdcsComsnCmd_t MagnetometerCalibration[] =
{
		{.subtype = ADCS_UPDATE_TLM_SAVE_VEC_ST,	.data =
		{0xFF,0x00,0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
		.length = 1, .delay_duration = 100},

		{.subtype = ADCS_UPDATE_TLM_PERIOD_VEC_ST,	.data =
		{10,0,10,0,10,0,0,10,0,0,0,0,0,0,0,0,0,0},
		.length = 18, .delay_duration = 100},

		{.subtype = ADCS_SET_ADCS_LOOP_PARAMETERS_ST,	.data = {0x00,0x64,0x00,0x00,0x00},
		.length = 5, .delay_duration = 100},

		{.subtype = ADCS_RUN_MODE_ST,	.data = {0x01},	.length = 1, .delay_duration = 100},

		{.subtype = ADCS_SET_PWR_CTRL_DEVICE_ST,	.data = {0x05,0x40,0x00}, .length = 3,	.delay_duration = 100},

		{.subtype = ADCS_SET_EST_MODE_ST,	.data = {0x02},	.length = 1, .delay_duration = 6000},

		{.subtype = ADCS_SET_ATT_CTRL_MODE_ST,	.data = {0x02,0x00,0x00,0x00},	.length = 4, .delay_duration = 6000},

		{.subtype = ADCS_SET_MAGNETMTR_OFFSET_ST,	.data = {0x28,0x23,0xDC,0xD8,0x00,0x00}, .length = 6, .delay_duration = 100},

		{.subtype = ADCS_SET_MAGNETMTR_SENSTVTY_ST,	.data = {0x28,0x23,0xDC,0xD8,0x00,0x00}, .length = 6, 	.delay_duration = 100}

};

AdcsComsnCmd_t AngularRateAndPitchAngleEstimation[] = {
		{.subtype = ADCS_UPDATE_TLM_SAVE_VEC_ST,	.data =
		{0xFF,0xFF,0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
		.length = 1, .delay_duration = 100},

		{.subtype = ADCS_UPDATE_TLM_PERIOD_VEC_ST,	.data =
		{10,10,10,0,10,0,0,10,0,0,0,0,0,0,0,0,0,0},
		.length = 18, .delay_duration = 100},

		{.subtype = ADCS_SET_ADCS_LOOP_PARAMETERS_ST,	.data = {0x00,0xE8,0x03,0x00,0x00},
		.length = 5, .delay_duration = 100},

		{.subtype = ADCS_RUN_MODE_ST,	.data = {0x01},	.length = 1, .delay_duration = 100},

		{.subtype = ADCS_SET_PWR_CTRL_DEVICE_ST,	.data = {0x05,0x40,0x00}, .length = 3,	.delay_duration = 100},

		{.subtype = ADCS_SET_EST_MODE_ST,	.data = {0x03},	.length = 1, .delay_duration = 6000},

		{.subtype = ADCS_SET_ATT_CTRL_MODE_ST,	.data = {0x02,0x00,0x00,0x00},	.length = 4, .delay_duration = 100}

	};
AdcsComsnCmd_t YWheelRampUpTest[] = {
		{.subtype = ADCS_UPDATE_TLM_SAVE_VEC_ST,	.data =
		{0xFF,0xFF,0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
		.length = 1, .delay_duration = 100},

		{.subtype = ADCS_UPDATE_TLM_PERIOD_VEC_ST,	.data =
		{1,1,1,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0},
		.length = 18, .delay_duration = 100},

		{.subtype = ADCS_SET_ADCS_LOOP_PARAMETERS_ST,	.data = {0x00,0x64,0x00,0x00,0x00},
		.length = 5, .delay_duration = 100},

		{.subtype = ADCS_RUN_MODE_ST,	.data = {0x01},	.length = 1, .delay_duration = 100},

		{.subtype = ADCS_SET_PWR_CTRL_DEVICE_ST,	.data = {0x05,0x04,0x00}, .length = 3,	.delay_duration = 100},

		{.subtype = ADCS_SET_EST_MODE_ST,	.data = {0x03},	.length = 1, .delay_duration = 6000},

		{.subtype = ADCS_SET_ATT_CTRL_MODE_ST,	.data = {0x00,0x00,0x00,0x00},	.length = 4, .delay_duration = 6000},

		{.subtype = ADCS_SET_WHEEL_SPEED_ST,	.data = {0x00,0x00,0xD0,0xF8,0x00,0x00},	.length = 6, .delay_duration = (2*60*1000)}

	};
AdcsComsnCmd_t YMomentumModeCommissioning[] = {
		{.subtype = ADCS_UPDATE_TLM_SAVE_VEC_ST,	.data =
		{0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
		.length = 1, .delay_duration = 100},

		{.subtype = ADCS_UPDATE_TLM_PERIOD_VEC_ST,	.data =
		{10,10,10,10,10,0,0,10,10,0,0,0,0,0,0,0,0,0},
		.length = 18, .delay_duration = 100},

		{.subtype = ADCS_SET_ADCS_LOOP_PARAMETERS_ST,	.data = {0x00,0xE8,0x03,0x00,0x00},
		.length = 5, .delay_duration = 100},

		{.subtype = ADCS_RUN_MODE_ST,	.data = {0x01},	.length = 1, .delay_duration = 100},

		{.subtype = ADCS_SET_CURR_UNIX_TIME_ST,	.data = {0x39,0x77,0x77,0x5D,0x00,0x00},	.length = 6, .delay_duration = 100},

		{.subtype = ADCS_SET_SGP4_ORBIT_PARAMS_ST,	.data = {0},	.length = 64, .delay_duration = 100},

		{.subtype = ADCS_SAVE_ORBIT_PARAM_ST,	.data = {0},	.length = 0, .delay_duration = 100},

		{.subtype = ADCS_SET_ATT_CTRL_MODE_ST,	.data = {0x03,0x00,0x02,0x80}, .length = 4,	.delay_duration = 100},

		{.subtype = ADCS_SET_EST_MODE_ST,	.data = {0x05},	.length = 1, .delay_duration = 6000},

		{.subtype = ADCS_SET_ATT_CTRL_MODE_ST,	.data = {0x00,0x00,0x00,0x00},	.length = 4, .delay_duration = 6000},

		{.subtype = ADCS_SET_WHEEL_SPEED_ST,	.data = {0x00,0x00,0xD0,0xF8,0x00,0x00},	.length = 6, .delay_duration = (2*60*1000)}

	};

void CommissionAdcsMode(AdcsComsnCmd_t *modeCmd, unsigned int length){

	unsigned int choice = 0;
	TC_spl cmd;

	printf("Continuous testing - no stop between commands?\n(1 = Yes\t 0 = No)\n");
	while(UTIL_DbguGetIntegerMinMax(&choice,0,1)==0);

	for(unsigned int i = 0; i < length; i++){

		if(0 == choice){// Discrete Testing
			printf("Continue to Next Command?\n0 = Exit\n1 = Continue\n");
			while(UTIL_DbguGetIntegerMinMax(&choice,0,2)==0);

			switch(choice){
			case 0:
				return;
			case 1:
				break;
			}
		}
		cmd.id = i;
		cmd.type = TC_ADCS_T;
		cmd.subType = modeCmd[i].subtype;
		cmd.length = modeCmd[i].length;
		cmd.id++;
		memcpy(cmd.data,modeCmd[i].data,modeCmd[i].length);
		printf("Sending command to execution.\t\t---subtype = %d",cmd.subType);
		AdcsCmdQueueAdd(&cmd);
		vTaskDelay(modeCmd[i].delay_duration);
		vTaskDelay(1000);
	}
}


void TestAdcsCommissioningTask(){

	unsigned int coms_mode = 0;
	unsigned int length = 0;
	unsigned int temp = 0;
	TC_spl cmd = {0};
	vTaskDelay(10000);
	while(1){
		printf("\nChoose Commissioning mode:\n");
		printf("\t%d) %s\n",	0,"Costume Command");
		printf("\t%d) %s\n",	1,"Initial Angular Rate Estimation");
		printf("\t%d) %s\n",	2,"Detumbling");
		printf("\t%d) %s\n",	3,"Magnetometer Deployment");
		printf("\t%d) %s\n",	4,"Magnetometer Calibration");
		printf("\t%d) %s\n",	5,"Angular Rate And Pitch Angle Estimation");
		printf("\t%d) %s\n",	6,"Y-Wheel Ramp-Up Test");
		printf("\t%d) %s\n",	7,"Y-Momentum Mode Commissioning");
		printf("\t%d) %s\n",	8,"Delay for 2 minute");
		while(UTIL_DbguGetIntegerMinMax(&coms_mode,0,8)==0);

		AdcsComsnCmd_t *mode = NULL;
		switch(coms_mode){
		case 0:
			cmd.type = TM_ADCS_T;
			printf("Choose ADCS command subtype:\n");
			while(UTIL_DbguGetIntegerMinMax((unsigned int*)&temp,0,255)==0);
			cmd.subType = temp;

			printf("Command Length:\n");
			while(UTIL_DbguGetIntegerMinMax((unsigned int*)&temp,0,sizeof(cmd.data))==0);
			cmd.length = temp;

			if(cmd.length >0){
				printf("Insert Data(in decimal):\n");
				for(unsigned int i = 0; i< cmd.length; i++){
					printf("data[%d]:",i);
					while(UTIL_DbguGetIntegerMinMax((unsigned int*)&temp,0,255) == 0);
					cmd.data[i] = temp;
					printf("\n");
				}
			}
			AdcsCmdQueueAdd(&cmd);
			vTaskDelay(1000);
			continue;
		case 1:
			mode = InitialAngularRateEstimation;
			length = (sizeof(InitialAngularRateEstimation)/sizeof(AdcsComsnCmd_t));
			printf("\t-------Initial Angular Rate Estimation\n");
			break;
		case 2:
			mode = Detumbling;
			length = (sizeof(Detumbling)/sizeof(AdcsComsnCmd_t));
			printf("\t-------Detumbling\n");
			break;
		case 3:
			mode = MagnetometerDeployment;
			length = (sizeof(MagnetometerDeployment)/sizeof(AdcsComsnCmd_t));
			printf("\t-------Y-Magnetometer Deployment\n");
			break;
		case 4:
			mode = MagnetometerCalibration;
			length = (sizeof(MagnetometerCalibration)/sizeof(AdcsComsnCmd_t));
			printf("\t-------Magnetometer Calibration\n");
			break;
		case 5:
			mode = AngularRateAndPitchAngleEstimation;
			length = (sizeof(AngularRateAndPitchAngleEstimation)/sizeof(AdcsComsnCmd_t));
			printf("\t-------Angular Rate And Pitch Angle Estimation\n");
			break;
		case 6:
			mode = YWheelRampUpTest;
			length = (sizeof(YWheelRampUpTest)/sizeof(AdcsComsnCmd_t));
			printf("\t-------Y-Wheel Ramp Up Test\n");
			break;
		case 7:
			mode = YMomentumModeCommissioning;
			length = (sizeof(YMomentumModeCommissioning)/sizeof(AdcsComsnCmd_t));
			printf("\t-------Y-Momentum Mode Commissioning\n");
			break;
		case 8:
			vTaskDelay(2*60*1000);
			break;
		}
		CommissionAdcsMode(mode,length);
	}
}
