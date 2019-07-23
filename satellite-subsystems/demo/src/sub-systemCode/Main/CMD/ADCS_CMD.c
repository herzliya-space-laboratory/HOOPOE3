#include "../TRXVU.h"
#include "Stage_Table.h"
#include "../ADCS.h"
#include "ADCS_CMD.h"
#include "../COMM/GSC.h"
#include "../COMM/splTypes.h"
#include "../Main/HouseKeeping.h"
#include <hal/Drivers/I2C.h>
#include <stdlib.h>
#include <string.h>

//TODO: change 'decode' into a pointer to 'TC_spl' to save space on the stack
void ADCS_CMD(TC_spl decode)
{
	Ack_type type = ACK_ADCS_TEST;
	ERR_type err;
	int SubType = decode.subType - startIndex;

	if (decode.length != (ADCS_cmd[SubType]).Length)
	{
		*err = ERR_PARAMETERS;
		#ifndef NOT_USE_ACK_HK
		save_ACK(type, err, decode.id);
		#endif
		return;
	}

	int error = (ADCS_cmd[SubType]).Command(decode.data);

	if (error == SUCCESS)
	{
		*err = ERR_SUCCESS;
	}
	else if (error == SYSTEM_OFF)
	{
		*err = ERR_SYSTEM_OFF;
	}
	else if (error == ERROR)
	{
		*err = ERR_ERROR;
	}
	else {
		*err = ERR_FAIL;
	}
	#ifndef NOT_USE_ACK_HK
		save_ACK(type, err, decode.id);
	#endif
}

void Set_ADCS_CMD(ADCS_CMD_t ADCS_cmd[])
{
	ADCS_cmd[98-startIndex].Command = SetBootIndex;
	ADCS_cmd[98-startIndex].Length = 1;

	ADCS_cmd[99-startIndex].Command = Run_Selected_Boot_Program;
	ADCS_cmd[99-startIndex].Length = 0;

	ADCS_cmd[100-startIndex].Command = SGP4_Oribt_Param;
	ADCS_cmd[100-startIndex].Length = 64;

	ADCS_cmd[101-startIndex].Command = SGP4_Oribt_Inclination;
	ADCS_cmd[101-startIndex].Length = 8;

	ADCS_cmd[102-startIndex].Command = SGP4_Oribt_Eccentricy;
	ADCS_cmd[102-startIndex].Length = 8;

	ADCS_cmd[103-startIndex].Command = SGP4_Oribt_RAAN;
	ADCS_cmd[103-startIndex].Length = 8;

	ADCS_cmd[104-startIndex].Command = SGP4_Oribt_Argument;
	ADCS_cmd[104-startIndex].Length = 8;

	ADCS_cmd[105-startIndex].Command = SGP4_Oribt_B_Star;
	ADCS_cmd[105-startIndex].Length = 8;

	ADCS_cmd[106-startIndex].Command = SGP4_Oribt_Mean_Motion;
	ADCS_cmd[106-startIndex].Length = 8;

	ADCS_cmd[107-startIndex].Command = SGP4_Oribt_Mean_Anomaly;
	ADCS_cmd[107-startIndex].Length = 8;

	ADCS_cmd[108-startIndex].Command = SGP4_Oribt_Epoc;
	ADCS_cmd[108-startIndex].Length = 8;

	ADCS_cmd[109-startIndex].Command = Reset_Boot_Registers;
	ADCS_cmd[109-startIndex].Length = 0;

	ADCS_cmd[110-startIndex].Command = Save_Config;
	ADCS_cmd[110-startIndex].Length = 0;

	ADCS_cmd[111-startIndex].Command = Save_Orbit_Param;
	ADCS_cmd[111-startIndex].Length = 0;

	ADCS_cmd[112-startIndex].Command = setUnixTime;
	ADCS_cmd[112-startIndex].Length = 6;

	ADCS_cmd[113-startIndex].Command = Cashe_State;
	ADCS_cmd[113-startIndex].Length = 1;

	ADCS_cmd[114-startIndex].Command = DeployMagnetometerBoom;
	ADCS_cmd[114-startIndex].Length = 1;

	ADCS_cmd[115-startIndex].Command = setRunMode;
	ADCS_cmd[115-startIndex].Length = 1;

	ADCS_cmd[116-startIndex].Command = Mag_Config;
	ADCS_cmd[116-startIndex].Length = 3;

	ADCS_cmd[117-startIndex].Command = Set_Mag_Mount;
	ADCS_cmd[117-startIndex].Length = 6;

	ADCS_cmd[118-startIndex].Command = Set_Mag_Offest_Scale;
	ADCS_cmd[118-startIndex].Length = 12;

	ADCS_cmd[119-startIndex].Command = Set_Mag_Sense;
	ADCS_cmd[119-startIndex].Length = 12;

	ADCS_cmd[120-startIndex].Command = set_Detumbling_Param;
	ADCS_cmd[120-startIndex].Length = 14;

	ADCS_cmd[121-startIndex].Command = Set_Y_Wheel_Param;
	ADCS_cmd[121-startIndex].Length = 16;

	ADCS_cmd[122-startIndex].Command = Set_REACTION_Wheel_Param;
	ADCS_cmd[122-startIndex].Length = 8;

	ADCS_cmd[123-startIndex].Command = Set_Moments_Inertia;
	ADCS_cmd[123-startIndex].Length = 12;

	ADCS_cmd[124-startIndex].Command = Set_Products_Inertia;
	ADCS_cmd[124-startIndex].Length = 12;

	ADCS_cmd[125-startIndex].Command = cspaceADCS_setEstimationParam1;
	ADCS_cmd[125-startIndex].Length = 16;

	ADCS_cmd[126-startIndex].Command = cspaceADCS_setEstimationParam2;
	ADCS_cmd[126-startIndex].Length = 14;

	ADCS_cmd[127-startIndex].Command = ADCS_translateCommandFULL;
	ADCS_cmd[127-startIndex].Length = 9;

	ADCS_cmd[128-startIndex].Command = ADCS_translateCommandDelay;
	ADCS_cmd[128-startIndex].Length = 3;

	ADCS_cmd[129-startIndex].Command = ADCS_translateCommandControlMode;
	ADCS_cmd[129-startIndex].Length = 1;

	ADCS_cmd[130-startIndex].Command = ADCS_translateCommandPower;
	ADCS_cmd[130-startIndex].Length = 1;

	ADCS_cmd[131-startIndex].Command = ADCS_translateCommandEstimationMode;
	ADCS_cmd[131-startIndex].Length = 1;

	ADCS_cmd[132-startIndex].Command = ADCS_translateCommandTelemtryADCS;
	ADCS_cmd[132-startIndex].Length = 3;

	ADCS_cmd[133-startIndex].Command = ADCS_Config;
	ADCS_cmd[133-startIndex].Length = 0;

	ADCS_cmd[134-startIndex].Command = Magnetorquer_Configuration;
	ADCS_cmd[134-startIndex].Length = 3;

	ADCS_cmd[135-startIndex].Command = RW_Configuration;
	ADCS_cmd[135-startIndex].Length = 4;

	ADCS_cmd[136-startIndex].Command = Gyro_Configuration;
	ADCS_cmd[136-startIndex].Length = 3;

	ADCS_cmd[137-startIndex].Command = CSS_Configuration;
	ADCS_cmd[137-startIndex].Length = 10;

	ADCS_cmd[138-startIndex].Command = CSS_Relative_Scale;
	ADCS_cmd[138-startIndex].Length = 10;

	ADCS_cmd[139-startIndex].Command = CSS_Threshold;
	ADCS_cmd[139-startIndex].Length = 1;

	ADCS_cmd[140-startIndex].Command = Magnetometer_Mounting_Transform_Angles;
	ADCS_cmd[140-startIndex].Length = 6;

	ADCS_cmd[141-startIndex].Command = Magnetometer_Channel_1_3_Offset;
	ADCS_cmd[141-startIndex].Length = 6;

	ADCS_cmd[142-startIndex].Command = Magnetometer_Sensitivity_Matrix;
	ADCS_cmd[142-startIndex].Length = 18;

	ADCS_cmd[143-startIndex].Command = Rate_Sensor_Offset;
	ADCS_cmd[143-startIndex].Length = 6;

	ADCS_cmd[144-startIndex].Command = Rate_Sensor_Mult;
	ADCS_cmd[144-startIndex].Length = 1;

	ADCS_cmd[145-startIndex].Command = Detumbling_Gain_Config;
	ADCS_cmd[145-startIndex].Length = 14;

	ADCS_cmd[146-startIndex].Command = Y_Momentum_Gain_Config;
	ADCS_cmd[146-startIndex].Length = 16;

	ADCS_cmd[147-startIndex].Command = Reference_Wheel_Momentum;
	ADCS_cmd[147-startIndex].Length = 4;

	ADCS_cmd[148-startIndex].Command = RWheel_Gain;
	ADCS_cmd[148-startIndex].Length = 8;

	ADCS_cmd[149-startIndex].Command = Moment_of_Inertia;
	ADCS_cmd[149-startIndex].Length = 24;

	ADCS_cmd[150-startIndex].Command = Noise_Configuration;
	ADCS_cmd[150-startIndex].Length = 28;

	ADCS_cmd[151-startIndex].Command = Use_in_EKF;
	ADCS_cmd[151-startIndex].Length = 1;



}

int SetBootIndex(TC_spl cmd )
{
	unsigned char temp;
	get_Boot_Index(&temp);
	int error = Set_Boot_Index(cmd.data);
	get_Boot_Index(&temp);
	if (temp != cmd.data)
	{
		return ERR_FAIL;
	}
	else {
		return error;
	}
}

int SetSGP4OrbitPramatars(Ack_type* type, TC_spl cmd)
{
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;
	unsigned char temp[SGP4FullLen];

	GetSG4OrbitPrams(temp);
	int error = SGP4_Oribt_Param(cmd.data);
	GetSG4OrbitPrams(temp);

	return error;
}

int SetSGP4OrbitInclination(Ack_type* type, TC_spl cmd) {
	if ( NULL == type) {
		return -1;
	}

	char temp[SGP4UnitLen];
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	GetSG4SubPram(temp, Inclination);
	int error = SGP4_Oribt_Inclination(cmd.data);
	GetSG4SubPram(temp, Inclination);

	if (memcmp(cmd.data, temp, SGP4UnitLen) != 0)
	{
		return ERR_FAIL;
	}
	return error;
	
}

int SetSGP4OrbitEccentricy(Ack_type* type, TC_spl cmd) {
		if ( NULL == type) {
			return -1;
		}

		char temp[SGP4UnitLen];
		*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

		GetSG4SubPram(temp, Eccentricity);
		int error = SGP4_Oribt_Eccentricy(cmd.data);
		GetSG4SubPram(temp, Eccentricity);

		if (memcmp(cmd.data, temp, SGP4UnitLen) != 0)
		{
			return ERR_FAIL;
		}
		return error;

	}

int SetSGP4OrbitEpoch(Ack_type *type, TC_spl cmd)
{
	if ( NULL == type) {
		return ERR_FAIL;
	}

	char temp[SGP4UnitLen];
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;
	

	GetSG4SubPram(temp, Epoch);
	int error = SGP4_Oribt_Epoc(cmd.data);
	GetSG4SubPram(temp, Epoch);
	if (memcmp(cmd.data, temp, SGP4UnitLen) != 0)
	{
		return ERR_FAIL;
	}
	return error;

}

int SetSGP4OrbitRAAN(Ack_type *type,  TC_spl cmd)
{
	if ( NULL == type) {
		return ERR_FAIL;
	}


	char temp[SGP4UnitLen];
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	GetSG4SubPram(temp, RAAN);
	int error = SGP4_Oribt_RAAN(cmd.data);
	GetSG4SubPram(temp, RAAN);

	if (memcmp(cmd.data, temp, SGP4UnitLen) != 0)
	{
		return ERR_FAIL;
	}
	return error;
}

int SetSGP4OrbitArgumentofPerigee(Ack_type *type,  TC_spl cmd)
{
	if ( NULL == type) {
		return ERR_FAIL;
	}

	char temp[SGP4UnitLen];
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER

	GetSG4SubPram(temp, ArgumentofPerigee);
	int error = SGP4_Oribt_RAAN(cmd.data);
	GetSG4SubPram(temp, ArgumentofPerigee);

	if (memcmp(cmd.data, temp, SGP4UnitLen) != 0)
		{
			return ERR_FAIL;
		}
	return error;

}

int SetSGP4OrbitBStar(Ack_type *type, TC_spl cmd)
{
	if ( NULL == type) {
		return ERR_FAIL;
	}

	char temp[SGP4UnitLen];
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	GetSG4SubPram(temp, BStar);
	int error = SGP4_Oribt_RAAN(cmd.data);
	GetSG4SubPram(temp, BStar);
	if (memcmp(cmd.data, temp, SGP4UnitLen) != 0)
	{
		return ERR_FAIL;
	}
	return error;
}

int SetSGP4OrbitMeanMotion(Ack_type *type, TC_spl cmd)
{
	if ( NULL == type) {
		return ERR_FAIL;
	}

	char temp[SGP4UnitLen];
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	GetSG4SubPram(temp, MeanMotion);
	int error = SGP4_Oribt_RAAN(cmd.data);
	GetSG4SubPram(temp, MeanMotion);

	if (memcmp(cmd.data, temp, SGP4UnitLen) != 0)
	{
		return ERR_FAIL;
	}
	return error;

}

int SetSGP4OrbitMeanAnomaly(Ack_type *type, TC_spl cmd)
{
	if ( NULL == type) {
		return ERR_FAIL;
	}

	char temp[SGP4UnitLen];
	*type = ACK_ADCS_UPFATE_TLE_PARAMETER;

	GetSG4SubPram(temp, MeanAnomaly);
	int error = SGP4_Oribt_RAAN(cmd.data);
	GetSG4SubPram(temp, MeanAnomaly);

	if ( memcmp(cmd.data, temp, SGP4UnitLen) != 0)
	{
		return ERR_FAIL;
	}
	return error;

}

int ResetBootRegisters(Ack_type *type, TC_spl cmd)
{
	if ( NULL == type) {
		return return;
	}

	unsigned char temp;
	*type = ACK_ADCS_CONFIG;

	get_Boot_Index(&temp);
	int error = Reset_Boot_Registers();
	get_Boot_Index(&temp);
	if(temp == 1)
	{
		return ERR_FAIL;
	}

	return error;
}

int CurrentUnixTime(Ack_type *type, TC_spl cmd)
{
	if ( NULL == type) {
		return ERR_FAIL;
	}

	*type = ACK_ADCS_SET_TIME;
	
	int error = setUnixTime(cmd.data);
	return error;

}

int CacheEnabledState(Ack_type *type, TC_spl cmd)
{
	if ( NULL == type) {
		return ERR_FAIL;
	}

	*type = ACK_ADCS_CONFIG;

	int error = Cashe_State(cmd.data);
	return error;

}

int DeployMagnetometerBoom(Ack_type *type,TC_spl cmd)
{
	#ifndef TESTING
	int error = BOOMStickDeploy(cmd.data);
	#else
	int error = SUCCESS;
	printf("are you nuts?!");
	#endif// !TESTING
	return error;
}

int ADCSRunMode(Ack_type *type, TC_spl cmd)
{
	if ( NULL == type) {
		return ERR_FAIL;
	}

	*type = ACK_ADCS_CONFIG;

	

	int error = setRunMode(cmd.data);
	return error;


}

int SetMagnetorquerConfiguration(Ack_type *type, TC_spl cmd)
{

	*type = ACK_ADCS_CONFIG;
	int error = Mag_Config(cmd.data);
	return error;

}

int SetMagnetometerMounting(Ack_type *type, TC_spl cmd)
{

	*type = ACK_ADCS_CONFIG;

	int error = Set_Mag_Mount(cmd.data);
	return error;

}

int SetMagnetometerOffsetAndScalingConfiguration(Ack_type *type,TC_spl cmd)
{
	if (NULL == type) {
		return ERR_FAIL;
	}

	*type = ACK_ADCS_CONFIG;

	int error = Set_Mag_Mount(cmd.data);

	return error;

}





































































































































