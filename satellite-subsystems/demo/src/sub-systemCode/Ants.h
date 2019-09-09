/*
 * Ants.h
 *
 *  Created on: Mar 27, 2019
 *      Author: Hoopoe3n
 */

#ifndef ANTS_H_
#define ANTS_H_

#include "Global/Global.h"
#include <satellite-subsystems/IsisAntS.h>

#define ATTEMPT_DONE FALSE

#define DEFFULT_DEPLOY_TIME 10
#define START_MUTE_TIME_MIN		10
#define NUMBER_OF_ANTS			4

#define DEPLOY_ATTEMPT_NUMBER_1 0
#define DEPLOY_ATTEMPT_NUMBER_2 1
#define DEPLOY_ATTEMPT_NUMBER_3 2

#define FIRST_DEPLOY_SIDE isisants_sideA
#define SIZE_DEPLOY_ATTEMPT_UNION sizeof(deploy_attempt)

typedef union __attribute__ ((__packed__))
{
	Boolean isAtemptDone;
	time_unix timeToDeploy;
}deploy_attempt;

int ARM_ants();
int DISARM_ants();
int deploye_ants(ISISantsSide side);
void init_Ants();

void Auto_Deploy();

#endif /* ANTS_H_ */
