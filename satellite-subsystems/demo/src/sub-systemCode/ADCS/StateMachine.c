/*
 * StateMachine.c
 *
 *  Created on: Jul 21, 2019
 *      Author: Michael
 */
#include <stdio.h>
#include <stdlib.h>

#include <hal/Storage/FRAM.h>
#include <hcc/api_fat.h>

#include "Adcs_Main.h"
#include "Adcs_Config.h"
#include "StateMachine.h"
#include "AdcsGetDataAndTlm.h"

#include "sub-systemCode/Global/GlobalParam.h"
#include "sub-systemCode/Global/FRAMadress.h"

#include "sub-systemCode/COMM/GSC.h"
#include "sub-systemCode/COMM/splTypes.h"


 TroubleErrCode InitStateMachine()
 {
	 return TRBL_SUCCESS;
 }

TroubleErrCode UpdateAdcsStateMachine()
{

	return TRBL_SUCCESS;
}




//int Save_Orbit_Param()
//{
//	return cspaceADCS_saveOrbitParam(ADCS_ID);
//}
//
//int Set_Mag_Mount(unsigned char * config_data)
//{
//	if(config_data == NULL){return -1;}
//	return cspaceADCS_setMagMountConfig(ADCS_ID, config_data);
//}
//
//int Set_Mag_Offest_Scale(unsigned char* config_data)
//{
//	if(config_data == NULL){return -1;}
//	return cspaceADCS_setMagOffsetScalingConfig(ADCS_ID, config_data);
//}
//
//int Set_Mag_Sense(unsigned char * config_data)
//{if(config_data == NULL){return -1;}
//	return cspaceADCS_setMagSensConfig(ADCS_ID, config_data);
//}
//
//int setUnixTime(unsigned char * unix_time)
//{
//	if(unix_time == NULL){return -1;}
//	return cspaceADCS_setCurrentTime(ADCS_ID, unix_time);
//}
//
//int Cashe_State(unsigned char state)
//{
//	if(state == NULL){return -1;}
//	return cspaceADCS_cacheStateADCS(ADCS_ID, state);
//}
//



