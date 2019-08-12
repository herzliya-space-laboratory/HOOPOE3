/*
 * AdcsTest.c
 *
 *  Created on: Aug 11, 2019
 *      Author: איתי
 */
#include "AdcsMain.h"
#include "AdcsTest.h"
#include "../Main/CMD/General_CMD.h"
#define ADCS_ID 0

void ErrFlagTest()
{
	cspaceADCS_setRunMode(0,runmode_enabled);
	double *SGP4 = {0,0,0,0,0,0,0,0};
	cspaceADCS_setSGP4Parameters(ADCS_ID, SGP4);
	restart();
}

void printErrFlag(){
	cspaceADCS_setRunMode(0,runmode_enabled);
	cspace_adcs_statetlm_t data;
	cspaceADCS_getStateTlm(ADCS_ID, &data);
	printf("\nSGP ERR FLAG = %d \n", data.fields.curr_state.fields.orbitparam_invalid);
}

void Mag_Test()
{
	cspaceADCS_setRunMode(0,runmode_enabled);
	cspace_adcs_powerdev_t Power;
	int i = 0;
	for(;i<3;i++)
	{
		Power.raw[i] = 0;
	}
	Power.fields.motor_cubecontrol = 1;
	Power.fields.signal_cubecontrol = 1;
	Power.fields.pwr_motor = 1;
	cspaceADCS_setPwrCtrlDevice(ADCS_ID,Power);
	//generic i2c tm id 170 size 6 and print them
	cspace_adcs_rawmagmeter_t Mag;
	cspaceADCS_getRawMagnetometerMeas(0,&Mag);
	printf("mag raw x %d",Mag.fields.magnetic_x);
	printf("mag raw y %d",Mag.fields.magnetic_y);
	printf("mag raw z %d",Mag.fields.magnetic_z);


}

void AddCommendToQ()
{
	int input;
	int err;
	TC_spl adcsCmd;
	adcsCmd.id = TC_ADCS_T;
	if (UTIL_DbguGetIntegerMinMax(&(adcsCmd.subType), 0,666) != 0){
		printf("Enter ADCS command data\n");
		UTIL_DbguGetString(&(adcsCmd.data), SIZE_OF_COMMAND+1);
		err = AdcsCmdQueueAdd(&adcsCmd);
		printf("ADCS command error = %d\n\n\n", err);
		printf("Send new command\n");
		printf("Enter ADCS sub type\n");
	}

	if (UTIL_DbguGetIntegerMinMax(&input, 900,1000) != 0){
		printf("Print the data\n");
	}

}
