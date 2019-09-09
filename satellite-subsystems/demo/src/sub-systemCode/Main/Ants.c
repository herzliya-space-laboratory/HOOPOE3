#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../Ants.h"

#include <hal/Storage/FRAM.h>
#include <hal/errors.h>
#include <stdio.h>

#include <satellite-subsystems/GomEPS.h>

static ISISantsSide nextDeploy;

void init_Ants()
{
    int retValInt = 0;

	ISISantsI2Caddress myAntennaAddress[2];
	myAntennaAddress[0].addressSideA = 0x31;
	myAntennaAddress[0].addressSideB = 0x32;

	//Initialize the AntS system
	retValInt = IsisAntS_initialize(myAntennaAddress, 1);
	check_int("init_Ants, IsisAntS_initialize", retValInt);

	nextDeploy = FIRST_DEPLOY_SIDE;
}

int ARM_ants()
{
	int error;
	//2. changing state in side A
	ISISantsSide side = isisants_sideA;
	error = IsisAntS_setArmStatus(I2C_BUS_ADDR, side, isisants_arm);
	check_int("ARM_ants, IsisAntS_setArmStatus", error);
	if (error == E_NOT_INITIALIZED)
	{
		return -2;
	}
	else if(error != 0)
	{
		return -1;
	}
	//3. changing state in side B
	side = isisants_sideB;
	IsisAntS_setArmStatus(I2C_BUS_ADDR, side, isisants_arm);
	if (error == E_NOT_INITIALIZED)
	{
		return -2;
	}
	else if(error != 0)
	{
		return -1;
	}

	return 0;
}

int DISARM_ants()
{
	int error;
	//2. changing state in side A
	ISISantsSide side = isisants_sideA;
	error = IsisAntS_setArmStatus(I2C_BUS_ADDR, side, isisants_disarm);
	check_int("ARM_ants, IsisAntS_setArmStatus", error);
	if (error == E_NOT_INITIALIZED)
	{
		return -2;
	}
	else if(error != 0)
	{
		return -1;
	}
	//3. changing state in side B
	side = isisants_sideB;
	IsisAntS_setArmStatus(I2C_BUS_ADDR, side, isisants_disarm);
	if (error == E_NOT_INITIALIZED)
	{
		return -2;
	}
	else if(error != 0)
	{
		return -1;
	}

	return 0;
}

int deploye_ants(ISISantsSide side)
{
#ifndef ANTS_DO_NOT_DEPLOY
	int error = ARM_ants();

	if (error != 0)
	{
		printf("error in Arm sequence: %d\n", error);
		return error;
	}

	error = IsisAntS_autoDeployment(0, side, DEFFULT_DEPLOY_TIME);
	check_int("IsisAntS_autoDeployment, side A", error);

	return 0;
#else
	printf("Are you crazy, please do NOT try to deploy ants in the lab\n");
	return 666;
#endif
}


void update_nextDeploy()
{
	if (nextDeploy == isisants_sideA)
		nextDeploy = isisants_sideB;
	else
		nextDeploy = isisants_sideA;
}

Boolean checkTime_nextDeploy(int deployCount)
{
	if (deployCount > 2)
		return FALSE;

	deploy_attempt attempt;
	int i_error = FRAM_read(&attempt, DEPLOY_ANTS_ATTEMPTS_ADDR, SIZE_DEPLOY_ATTEMPT_UNION);
	check_int("FRAM_read, checkTime_nextDeploy", i_error);

	if (attempt.isAtemptDone == ATTEMPT_DONE)
		return FALSE;

	time_unix time_now;
	i_error = Time_getUnixEpoch(&time_now);
	check_int("Time_getUnixEpoch, checkTime_nextDeploy", i_error);

	if (time_now <= attempt.timeToDeploy)
	{
		deploye_ants(nextDeploy);
		update_nextDeploy();
		deploy_attempt = ATTEMPT_DONE;
		int i_error = FRAM_write(&attempt, DEPLOY_ANTS_ATTEMPTS_ADDR, SIZE_DEPLOY_ATTEMPT_UNION);
		check_int("FRAM_write, checkTime_nextDeploy", i_error);
		return TRUE;
	}

	return FALSE;
}

void Auto_Deploy()
{
	deploy_attempt deploy_status;
	time_unix time_now;
	for (int i = 0; i < 10; i++)
	{

	}
	int i_error;
}

Boolean activeDeploySequence(Boolean firstActivationFalg)
{
	if (firstActivationFalg)
	{
		Auto_Deploy();
		return TRUE;
	}
}
