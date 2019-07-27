#include "AdcsGetDataAndTlm.h"


/*
 * AdcsGetDataAndTlm.c
 *
 *  Created on: Jul 21, 2019
 *      Author: Michael
 */

#include <stdio.h>
#include <stdlib.h>

#include <hcc/api_fat.h>

#include "../ADCS.h"
#include "AdcsGetDataAndTlm.h"

int SaveData(Gather_TM_Data Data)
{
	if(Data.File_Name == NULL || Data.FuncPointer == NULL)
	{
		return -1;
	}
	int err;
	unsigned char Get[MAX_TELEMTRY_SIZE];
	Hold_Data ADCS_Data;
	//call Telemtry Function
	err = Data.FuncPointer(ADCS_ID,Get);
	if(err != 0 && err != -35)
	{
		//Save log
		return err;
	}

	memccpy(ADCS_Data.raw,Get[Data.Start_Pos],DATA_SIZE);
	short temp;
	// if needed chnges the XYZ system from the ADCS to the Satllighte (x,y,z) --> (z,-y,x)
	if(Data.Change_Cord != FALSE)
	{
		temp = ADCS_Data.fields.a;
		ADCS_Data.fields.a = ADCS_Data.fields.c;
		ADCS_Data.fields.c = temp;
		ADCS_Data.fields.b = -1*ADCS_Data.fields.b;
	}
	//Saves data at the file system
	FileSystemResult FileErr =  c_fileWrite(Data.File_Name,ADCS_Data.raw);
	if(FileErr != FS_SUCCSESS)
	{
		//log File error
		return FileErr;
	}
	return err;
}

int GatherTlmAndData(Gather_TM_Data Data[])
{
	int i;
	int err;
	for(i = 0;i < 24;i++)
	{
		if(Data[i].Save == 0)
		{
			err = SaveData(Data[i]);
			if(err != 0 && err != -35)
			{
				//Save log
			}
		}
	}
	cspace_adcs_pwtempms_t * DataADCS;
	err = cspaceADCS_getPowTempMeasTLM(0, DataADCS);
	c_fileWrite(ADCS_HK,DataADCS);
	free(DataADCS->raw);
	if(err != 0 && err != -35)
	{
		//Save log
	}
	return err;
}

void InitData(Gather_TM_Data Data[])
{
	stageTable ST = Get_ST();
	Data[0].Change_Cord = 1;
	Data[0].File_Name = ESTIMATED_ANGLES_FILE;
	Data[0].FuncPointer = &cspaceADCS_getStateTlm;
	Data[0].Save = Get_TM(ST).EstimatedAttitudeAnglesCheckSaveId146;
	Data[0].Start_Pos = 6;

	Data[1].Change_Cord = 1;
	Data[1].File_Name = ESTIMATED_ANGULAR_RATE_FILE;
	Data[1].FuncPointer = &cspaceADCS_getStateTlm;
	Data[1].Save = Get_TM(ST).EstimatedAngularRatesCheckSaveId147;
	Data[1].Start_Pos = 18;

	Data[2].Change_Cord = 1;
	Data[2].File_Name = NAME_OF_SATALLITE_VELOCITY_DATA_FILE;
	Data[2].FuncPointer = &cspaceADCS_getStateTlm;
	Data[2].Save = Get_TM(ST).SatellitePositionECICheckSaveId148;
	Data[2].Start_Pos = 24;

	Data[3].Change_Cord = 1;
	Data[3].File_Name = NAME_OF_SATALLITE_VELOCITY_DATA_FILE;
	Data[3].FuncPointer = &cspaceADCS_getStateTlm;
	Data[3].Save = Get_TM(ST).SatelliteVelocityECICheckSaveId149;
	Data[3].Start_Pos = 30;

	Data[4].Change_Cord = FALSE;
	Data[4].File_Name = NAME_OF_LLH_POSTION_DATA_FILE;
	Data[4].FuncPointer = &cspaceADCS_getStateTlm;
	Data[4].Save = Get_TM(ST).SatellitePositionLLHCheckSaveId150;
	Data[4].Start_Pos = 36;

	Data[5].Change_Cord = 1;
	Data[5].File_Name = NAME_OF_MAGNETIC_FIELD_DATA_FILE;
	Data[5].FuncPointer = &cspaceADCS_getMagneticFieldVec;
	Data[5].Save = Get_TM(ST).MagneticFieldVectorCheckSaveId151;
	Data[5].Start_Pos = 0;

	Data[6].Change_Cord = 1;
	Data[6].File_Name = CSS_SUN_VECTOR_DATA_FILE;
	Data[6].FuncPointer = &cspaceADCS_getCoarseSunVec;
	Data[6].Save = Get_TM(ST).CoarseSunVectorCheckSaveId152;
	Data[6].Start_Pos = 0;

	Data[7].Change_Cord = 1;
	Data[7].File_Name = SENSOR_RATE_DATA_FILE;
	Data[7].FuncPointer = &cspaceADCS_getSensorRates;
	Data[7].Save = Get_TM(ST).RateSensorRatesCheckSaveId155;
	Data[7].Start_Pos = 0;

	Data[8].Change_Cord = 1;
	Data[8].File_Name = WHEEL_SPEED_DATA_FILE;
	Data[8].FuncPointer = &cspaceADCS_getWheelSpeed;
	Data[8].Save = Get_TM(ST).WheelSpeedCheckSaveId156;
	Data[8].Start_Pos = 0;

	Data[9].Change_Cord = 1;
	Data[9].File_Name = MAGNETORQUER_CMD_FILE;
	Data[9].FuncPointer = &cspaceADCS_getMagnetorquerCmd;
	Data[9].Save = Get_TM(ST).MagnetorquerCommandSaveId157;
	Data[9].Start_Pos = 0;

	Data[10].Change_Cord = 1;
	Data[10].File_Name = WHEEL_SPEED_CMD_FILE;
	Data[10].FuncPointer = &cspaceADCS_getWheelSpeedCmd;
	Data[10].Save = Get_TM(ST).WheelSpeedCommandSaveId158;
	Data[10].Start_Pos = 0;

	Data[11].Change_Cord = 1;
	Data[11].File_Name = IGRF_MODEL_MAGNETIC_FIELD_VECTOR_FILE;
	Data[11].FuncPointer = &cspaceADCS_getEstimationMetadata;
	Data[11].Save = Get_TM(ST).IGRFModelledMagneticFieldVectorCheckSaveId159;
	Data[11].Start_Pos = 0;

	Data[12].Change_Cord = 1;
	Data[12].File_Name = GYRO_BIAS_FILE;
	Data[12].FuncPointer = &cspaceADCS_getEstimationMetadata;
	Data[12].Save = Get_TM(ST).EstimatedGyroBiasCheckSaveId161;
	Data[12].Start_Pos = 12;

	Data[13].Change_Cord = 1;
	Data[13].File_Name = INNOVATION_VECTOR_FILE;
	Data[13].FuncPointer = &cspaceADCS_getEstimationMetadata;
	Data[13].Save = Get_TM(ST).EstimationInnovationVectorCheckSaveId162;
	Data[13].Start_Pos = 18;

	Data[14].Change_Cord = FALSE;
	Data[14].File_Name = ERROR_VECTOR_FILE;
	Data[14].FuncPointer = &cspaceADCS_getEstimationMetadata;
	Data[14].Save = Get_TM(ST).QuaternionErrorVectorCheckSaveId163;
	Data[14].Start_Pos = 24;

	Data[15].Change_Cord = FALSE;
	Data[15].File_Name = QUATERNION_COVARIANCE_FILE;
	Data[15].FuncPointer = &cspaceADCS_getEstimationMetadata;
	Data[15].Save = Get_TM(ST).QuaternionCovarianceCheckSaveId164;
	Data[15].Start_Pos = 30;

	Data[16].Change_Cord = 1;
	Data[16].File_Name = ANGULAR_RATE_COVARIANCE_FILE;
	Data[16].FuncPointer = &cspaceADCS_getEstimationMetadata;
	Data[16].Save = Get_TM(ST).AngularRateCovarianceCheckSaveId165;
	Data[16].Start_Pos = 36;

	Data[17].Change_Cord = FALSE;
	Data[17].File_Name = NAME_OF_CSS_DATA_FILE;
	Data[17].FuncPointer = &cspaceADCS_getRawCss1_6Measurements;
	Data[17].Save = Get_TM(ST).RawCSSCheckSaveId168;
	Data[17].Start_Pos = 0;

	Data[18].Change_Cord = 1;
	Data[18].File_Name = RAW_MAGNETOMETER_FILE;
	Data[18].FuncPointer = &cspaceADCS_getRawMagnetometerMeas;
	Data[18].Save = Get_TM(ST).RawMagnetometerCheckSaveId170;
	Data[18].Start_Pos = 0;

	Data[19].Change_Cord = FALSE;
	Data[19].File_Name = ESTIMATED_QUATERNION_FILE;
	Data[19].FuncPointer = &cspaceADCS_getStateTlm;
	Data[19].Save = Get_TM(ST).EstimatedQuaternionCheckSaveId218;
	Data[19].Start_Pos = 12;

	Data[20].Change_Cord = 1;
	Data[20].File_Name = NAME_OF_ECEF_POSITION_DATA_FILE;
	Data[20].FuncPointer = &cspaceADCS_getStateTlm;
	Data[20].Save = Get_TM(ST).ECEFPositionCheckSaveId219;
	Data[20].Start_Pos = 42;

	Data[21].Change_Cord = 1;
	Data[21].File_Name = SUN_MODELLE_FILE;
	Data[21].FuncPointer = &cspaceADCS_getEstimationMetadata;
	Data[21].Save = Get_TM(ST).Sun_Modelle;
	Data[21].Start_Pos = 6;

	Data[22].Change_Cord = 1;
	Data[22].File_Name = NULL_FILE;
	Data[22].FuncPointer = &cspaceADCS_getEstimationMetadata;
	Data[22].Save = FALSE;
	Data[22].Start_Pos = 0;

	Data[23].Change_Cord = 1;
	Data[23].File_Name = NULL_FILE;
	Data[23].FuncPointer = &cspaceADCS_getEstimationMetadata;
	Data[23].Save = FALSE;
	Data[23].Start_Pos = 0;


}
