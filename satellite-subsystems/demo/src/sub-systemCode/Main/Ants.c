#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../Ants.h"

#include "../Global/logger.h"

#include <hal/Storage/FRAM.h>
#include <hal/errors.h>
#include <stdio.h>

#include <satellite-subsystems/GomEPS.h>
#include "../TRXVU.h"
#include "../EPS.h"

static ISISantsSide nextDeploy;

void reset_deployStatusFRAM(int delayForNextAttempt);

void init_Ants(Boolean activation)
{
	if (activation)
	{
		reset_FRAM_ants();
	}

    int retValInt = 0;

	ISISantsI2Caddress myAntennaAddress[2];
	myAntennaAddress[0].addressSideA = 0x31;
	myAntennaAddress[0].addressSideB = 0x32;

	//Initialize the AntS system
	retValInt = IsisAntS_initialize(myAntennaAddress, 1);
	if (retValInt)
		WriteErrorLog((log_errors)LOG_ERR_INIT_ANTS, SYSTEM_ANTS, retValInt);
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
	if (error)
		WriteErrorLog((log_errors)LOG_ERR_ARM_ANTS_A, SYSTEM_ANTS, error);
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
	if (error)
		WriteErrorLog((log_errors)LOG_ERR_ARM_ANTS_A, SYSTEM_ANTS, error);
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
	if (error)
		WriteErrorLog((log_errors)LOG_ERR_DEPLOY_ANTS, SYSTEM_ANTS, error);
	check_int("IsisAntS_autoDeployment, side A", error);

	return 0;
#else
	printf("Are you crazy, please do NOT try to deploy ants in the lab (%u (don't panic just wanted to get rid of an error...))\n", side);
	return 666;
#endif
}


int checkDeployAttempt(int attemptNumber)
{
	if (attemptNumber >= NUMBER_OF_ATTEMPTS)
		return -3;

	deploy_attempt attempt;
	unsigned int addres = DEPLOY_ANTS_ATTEMPTS_ADDR + attemptNumber*SIZE_DEPLOY_ATTEMPT_UNION;
	int i_error = FRAM_read_exte((byte*)&attempt, addres, SIZE_DEPLOY_ATTEMPT_UNION);
	if (i_error)
		WriteErrorLog((log_errors)LOG_ERR_READ_FRAM_ANTS, SYSTEM_ANTS, i_error);
	check_int("FRAM_read, checkDeployAttempt", i_error);

	if (attempt.isAtemptDone == ATTEMPT_DONE)
		return -1;//the deploy been done

	time_unix time;
	i_error = Time_getUnixEpoch(&time);
	if (i_error)
		WriteErrorLog((log_errors)LOG_ERR_GET_TIME_ANTS, SYSTEM_ANTS, i_error);
	check_int("Time_getUnixEpoch, checkDeployAttempt", i_error);
	if (time < attempt.timeToDeploy)
		return -2;// there is time for the deploy

	printf("\n\n\n       deploy ants!!!!!!!\n\n\n");
	deploye_ants(nextDeploy);
	attempt.isAtemptDone = ATTEMPT_DONE;
	i_error = FRAM_write_exte((byte*)&attempt, addres, SIZE_DEPLOY_ATTEMPT_UNION);
	if (i_error)
		WriteErrorLog((log_errors)LOG_ERR_WRITE_FRAM_ANTS, SYSTEM_ANTS, i_error);
	check_int("FRAM_write, checkDeployAttempt", i_error);
	if (nextDeploy == isisants_sideA)
		nextDeploy = isisants_sideB;
	else
		nextDeploy = isisants_sideA;

	return 0;
}

void reset_deployStatusFRAM(int delayForNextAttempt)
{
	time_unix time;
	deploy_attempt attempt;
	int i_error = Time_getUnixEpoch(&time);
	if (i_error)
		WriteErrorLog((log_errors)LOG_ERR_GET_TIME_ANTS, SYSTEM_ANTS, i_error);
	check_int("Time_getUnixEpoch, reset_deployStatusFRAM", i_error);
	for (int i = 0 ; i < NUMBER_OF_ATTEMPTS; i++)
	{
		attempt.timeToDeploy = time + delayForNextAttempt + i * DELAY_BETWEEN_3_ATTEMPTS;
		unsigned int addres = DEPLOY_ANTS_ATTEMPTS_ADDR + i*SIZE_DEPLOY_ATTEMPT_UNION;
		i_error = FRAM_write_exte((byte*)&attempt, addres, SIZE_DEPLOY_ATTEMPT_UNION);
		check_int("FRAM_write, reset_deployStatusFRAM", i_error);
	}
}

void reset_FRAM_ants()
{
	Boolean8bit stopDeploy = FALSE;
	int i_error = FRAM_write_exte(&stopDeploy, STOP_DEPLOY_ATTEMPTS_ADDR, 1);
	if (i_error)
		WriteErrorLog((log_errors)LOG_ERR_RESET_FRAM_ANTS, SYSTEM_ANTS, i_error);
	check_int("FRAM_write, DeployIfNeeded", i_error);

	i_error = FRAM_write_exte(&stopDeploy, ANTS_AUTO_DEPLOY_FINISH_ADDR, 1);
	if (i_error)
		WriteErrorLog((log_errors)LOG_ERR_RESET_FRAM_ANTS, SYSTEM_ANTS, i_error);
	check_int("FRAM_write, DeployIfNeeded", i_error);

	reset_deployStatusFRAM(START_MUTE_TIME_FIRST);

	shut_ADCS(SWITCH_ON);
	set_mute_time((unsigned short)((START_MUTE_TIME_FIRST + 2*60)/60));
}


Boolean DeployIfNeeded()
{
	Boolean8bit stopDeploy, autoDeploy_finish;
	int i_error = FRAM_read_exte(&stopDeploy, STOP_DEPLOY_ATTEMPTS_ADDR, 1);
	if (i_error)
		WriteErrorLog(LOG_ERR_FRAM_READ, SYSTEM_ANTS, i_error);
	check_int("FRAM_read, DeployIfNeeded", i_error);
	i_error = FRAM_read_exte(&autoDeploy_finish, ANTS_AUTO_DEPLOY_FINISH_ADDR, 1);
	if (i_error)
		WriteErrorLog(LOG_ERR_FRAM_READ, SYSTEM_ANTS, i_error);
	check_int("FRAM_read, DeployIfNeeded", i_error);
	if (stopDeploy && autoDeploy_finish)
		return FALSE;

	for (int i = 0; i < NUMBER_OF_ATTEMPTS; i++)
	{
		int deployRet = checkDeployAttempt(i);
		if (deployRet == -2)
			return FALSE;
		if (deployRet == 0 && i == NUMBER_OF_ATTEMPTS-1)
		{
			reset_deployStatusFRAM(DELAY_BETWEEN_ATTEMPTS_NORMAL);
			if (!autoDeploy_finish)
			{
				shut_ADCS(SWITCH_OFF);
				autoDeploy_finish = TRUE_8BIT;
				i_error = FRAM_write_exte(&autoDeploy_finish, ANTS_AUTO_DEPLOY_FINISH_ADDR, 1);
				if (i_error)
					WriteErrorLog(LOG_ERR_FRAM_WRITE, SYSTEM_ANTS, i_error);
				check_int("FRAM_read, DeployIfNeeded", i_error);
			}
		}
	}

	return TRUE;
}


void update_stopDeploy_FRAM()
{
	Boolean8bit stopDeploy = TRUE_8BIT;
	int i_error = FRAM_write_exte(&stopDeploy, STOP_DEPLOY_ATTEMPTS_ADDR, 1);
	if (i_error)
		WriteErrorLog(LOG_ERR_FRAM_WRITE, SYSTEM_ANTS, i_error);
	check_int("FRAM_write, update_stopDeploy_FRAM", i_error);
}
