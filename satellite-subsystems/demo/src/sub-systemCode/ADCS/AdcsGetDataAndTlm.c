#include <stdio.h>
#include <stdlib.h>

#include <hcc/api_fat.h>
#include <hal/boolean.h>
#include <satellite-subsystems/cspaceADCS.h>
#include <satellite-subsystems/cspaceADCS_types.h>

#include "sub-systemCode/Global/TM_managment.h"
#include "sub-systemCode/Global/FRAMadress.h"
#include "AdcsGetDataAndTlm.h"


#define TLM_ELEMENT_SIZE		(1+1+4+FN_MAXNAME+1+1+1) //TODO: check if needed

AdcsTlmElement_t TlmElements[NUM_OF_ADCS_TLM];

#define ADCS_DEFAULT_TLM_VECTOR	\
		{(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_statetlm_t),		cspaceADCS_getStateTlm,				ADCS_STATE_TLM_FILENAME,		1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_estmetadata_t),		cspaceADCS_getEstimationMetadata,	ADCS_EST_META_DATA,				1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_sunvec_t),			cspaceADCS_getCoarseSunVec,			ADCS_COARSE_SUN_VEC_FILENAME,	1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_sunvec_t),			cspaceADCS_getFineSunVec,			ADCS_FINE_SUN_VEC_FILENAME,		1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_angrate_t),			cspaceADCS_getSensorRates,			ADCS_SENSOR_FILENAME,			1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_wspeed_t),			cspaceADCS_getWheelSpeed,			ADCS_WHEEL_SPEED_FILENAME,		1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_rawmagmeter_t),		cspaceADCS_getRawMagnetometerMeas,	ADCS_RAW_MAG_FILENAME,			1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_magfieldvec_t),		cspaceADCS_getMagneticFieldVec,		ADCS_MAG_FIELD_VEC_FILENAME,	1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_rawcss1_6_t),		cspaceADCS_getRawCss1_6Measurements,ADCS_RAW_CSS_FILENAME_1_6,		1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_rawcss7_10_t),		cspaceADCS_getRawCss7_10Measurements,ADCS_RAW_CSS_FILENAME_7_10,	1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_pwtempms_t),		cspaceADCS_getPowTempMeasTLM,		ADCS_POWER_TEMP_FILENAME,		1,(time_unix)0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_misccurr_t),		cspaceADCS_getMiscCurrentMeas,		ADCS_MISC_CURR_FILENAME,		1,(time_unix)0,	FALSE_8BIT},\
		};

/*
cspaceADCS_getGeneralInfo
cspace_adcs_geninfo_t

cspaceADCS_getBootProgramInfo
cspace_adcs_bootinfo_t

cspaceADCS_getSRAMLatchupCounters
cspace_adcs_sramlatchupcnt_t

cspaceADCS_getEDACCounters
cspace_adcs_edaccnt_t

cspaceADCS_getCurrentTime
cspace_adcs_unixtm_t

cspaceADCS_getCommStatus
cspace_adcs_commstat_t

cspaceADCS_getCurrentState
cspace_adcs_currstate_t

cspaceADCS_getMagneticFieldVec
cspace_adcs_magfieldvec_t

cspaceADCS_getCoarseSunVec
cspace_adcs_sunvec_t

cspaceADCS_getFineSunVec
cspace_adcs_sunvec_t

cspaceADCS_getNadirVector
cspace_adcs_nadirvec_t

cspaceADCS_getSensorRates
cspace_adcs_angrate_t

cspaceADCS_getWheelSpeed
cspace_adcs_wspeed_t

cspaceADCS_getMagnetorquerCmd
cspace_adcs_magtorqcmd_t

cspaceADCS_getWheelSpeedCmd
cspace_adcs_wspeed_t

cspaceADCS_getRawCam2Sensor
cspace_adcs_rawcam_t

cspaceADCS_getRawCam1Sensor
cspace_adcs_rawcam_t

cspaceADCS_getRawCss1_6Measurements
cspace_adcs_rawcss1_6_t

cspaceADCS_getRawCss7_10Measurements
cspace_adcs_rawcss7_10_t

cspaceADCS_getRawMagnetometerMeas
cspace_adcs_rawmagmeter_t

cspaceADCS_getCSenseCurrentMeasurements
cspace_adcs_csencurrms_t

cspaceADCS_getCControlCurrentMeasurements
cspace_adcs_cctrlcurrms_t

cspaceADCS_getWheelCurrentsTlm
cspace_adcs_wheelcurr_t

cspaceADCS_getADCSTemperatureTlm
cspace_adcs_msctemp_t

cspaceADCS_getRateSensorTempTlm
cspace_adcs_ratesen_temp_t

cspaceADCS_getStateTlm
cspace_adcs_statetlm_t

cspaceADCS_getADCSMeasurements
cspace_adcs_measure_t

cspaceADCS_getActuatorsCmds
cspace_adcs_actcmds_t

cspaceADCS_getEstimationMetadata
cspace_adcs_estmetadata_t

cspaceADCS_getRawSensorMeasurements
cspace_adcs_rawsenms_t

cspaceADCS_getPowTempMeasTLM
cspace_adcs_pwtempms_t

cspaceADCS_getADCSExcTimes
cspace_adcs_exctm_t

cspaceADCS_getPwrCtrlDevice
cspace_adcs_powerdev_t

cspaceADCS_getMiscCurrentMeas
cspace_adcs_misccurr_t

cspaceADCS_getCommandedAttitudeAngles
cspace_adcs_cmdangles_t

cspaceADCS_getADCSConfiguration
config

cspaceADCS_getSGP4OrbitParameters
parameters
 */


TroubleErrCode SaveElementTlmAtIndex(unsigned int index);


int UpdateTlmToSaveVector(Boolean8bit save_tlm_flag[NUM_OF_ADCS_TLM])
{
	if (NULL == save_tlm_flag) {
		//TODO: log error
		return (int) TRBL_NULL_DATA;
	}

	int err = 0;

	err = FRAM_write(save_tlm_flag, TLM_SAVE_VECTOR_START_ADDR,
			TLM_SAVE_VECTOR_END_ADDR);
	if (0 != err) {
		return err;
	}

	for (unsigned int i = 0; i < NUM_OF_ADCS_TLM; ++i) {
		TlmElements[i].ToSave = save_tlm_flag[i];
	}
	return 0;
}

TroubleErrCode GatherTlmAndData()
{
	TroubleErrCode err = TRBL_SUCCESS;
	TroubleErrCode err_occured = TRBL_SUCCESS;
	for (unsigned int i = 0; i < NUM_OF_ADCS_TLM; ++i) {
		err = SaveElementTlmAtIndex(i);
		if (TRBL_SUCCESS != err) {
#ifdef TESTING
			printf("\t----- Error in 'SaveElementTlmAtIndex': %d", err);
#endif
			err_occured = TRBL_TLM_ERR;
			TlmElements[i].ToSave = FALSE_8BIT;			// stop saving tlm if error
			TlmElements[i].OperatingFlag = FALSE_8BIT;	// raise the 'not operating correctly' flag
			//TODO: log err
		}
	}
	return err_occured;
}

TroubleErrCode SaveElementTlmAtIndex(unsigned int index)
{
	if (index >= NUM_OF_ADCS_TLM) {
		return TRBL_INPUT_PARAM_ERR;
	}

	TroubleErrCode err = 0;
	FileSystemResult res = 0;
	unsigned char adcs_tlm[ADCS_MAX_TLM_SIZE] = { 0 };

	if (NULL 	== TlmElements[index].TlmCollectFunc||
		NULL	== TlmElements[index].TlmFileName 	||
		TRUE_8BIT != TlmElements[index].ToSave)
	{
		return TRBL_SUCCESS;
	}
	//TODO: change 0 to a define
	time_unix curr_time = 0;
	Time_getUnixEpoch(&curr_time);
	if(curr_time > TlmElements[index].LastSaveTime){
		if((curr_time - TlmElements[index].LastSaveTime) % TlmElements[index].SavePeriod == 0){
			err = TlmElements[index].TlmCollectFunc(0, adcs_tlm);
			if (0 != err) {
				// TODO: log error
				return err;
			}

			res = c_fileWrite(TlmElements[index].TlmFileName, adcs_tlm);
			if (FS_SUCCSESS != res) {
				return TRBL_FS_WRITE_ERR;
			}
			TlmElements[index].LastSaveTime = curr_time;
#ifdef TESTING
			printf("Collected Tlm %s\n",TlmElements[index].TlmFileName);
#ifdef PRINTTLM
			if(index == 7){	// magnetic field vector
				printf("\n[");
				for(int i = 0; i< TlmElements[index].TlmElementeSize - 1;i++){
					printf("%X,",adcs_tlm[i]);
				}
				printf("%X]\n",adcs_tlm[TlmElements[index].TlmElementeSize-1]);
			}
#endif
#endif

		}
	}
	return TRBL_SUCCESS;
}

TroubleErrCode RestoreDefaultTlmElement(){
	AdcsTlmElement_t temp[] = ADCS_DEFAULT_TLM_VECTOR;
	if(NULL == memcpy(TlmElements,temp,sizeof(temp))){
		return TRBL_FAIL;
	}
	return TRBL_SUCCESS;
}

void GetTlmElementAtIndex(AdcsTlmElement_t *elem,unsigned int index){
	if(NULL == elem || index >= NUM_OF_ADCS_TLM){
		return;
	}
	memcpy(elem,&TlmElements[index],sizeof(*elem));
}

void UpdateTlmElementAtIndex(int index, AdcsTlmCollectFunc func,
		unsigned char TlmElementSize, Boolean8bit ToSave,
		char file_name[FN_MAXNAME], char Period)
{
	if (NULL != func) {
		TlmElements[index].TlmCollectFunc = func;
	}
	if (NULL != file_name) {
		memcpy(TlmElements[index].TlmFileName, file_name, F_MAXNAME);
	}

	TlmElements[index].TlmElementeSize =
			(TlmElements[index].TlmElementeSize != 0) ?	// if 0 then ignore
			 TlmElementSize : TlmElements[index].TlmElementeSize;

	TlmElements[index].ToSave = ToSave;
	if(0 != Period){
		TlmElements[index].SavePeriod = Period;
	}
	//TODO: log update
}

TroubleErrCode InitTlmElements()
{
	//TODO: chage return error codes to appropriate values
	TroubleErrCode err = 0;
	AdcsTlmElement_t default_elements[] = ADCS_DEFAULT_TLM_VECTOR;

	unsigned int tlm_elements_length = sizeof(default_elements)
			/ sizeof(default_elements[0]);

	if (NULL ==  memcpy(TlmElements, default_elements, tlm_elements_length)) {
		//TODO: change -1 to appropriate error
		//TODO: log err
		return -1;
	}
	Boolean8bit *save_tlm_flag = malloc(
			tlm_elements_length * sizeof(*save_tlm_flag));
	char *Periods = malloc(tlm_elements_length*sizeof(*Periods));

	if (NULL == save_tlm_flag || NULL == Periods)
	{
		//TODO: change -1 to appropriate error
		//TODO: log err
		return -1;
	}

	err = FRAM_read(save_tlm_flag, TLM_SAVE_VECTOR_START_ADDR,
			TLM_SAVE_VECTOR_END_ADDR);
	if (0 != err) {
		//TODO: log err
		free(save_tlm_flag);
		return err;
	}
	err = FRAM_read((unsigned char*)Periods, TLM_PERIOD_VECTOR_START_ADDR,
			TLM_PERIOD_VECTOR_END_ADDR);
	if(0 != err){
		//TODO: log err
		free(save_tlm_flag);
		return err;
	}
	for (unsigned int i = 0; i < NUM_OF_ADCS_TLM; i++) {
		TlmElements[i].ToSave = save_tlm_flag[i];
		TlmElements[i].SavePeriod = Periods[i];
	}
	free(save_tlm_flag);
	free(Periods);
	//TODO: log successful init
	return TRBL_SUCCESS;
}

Boolean CreateTlmElementFiles()
{
	FileSystemResult res = 0;
	Boolean err = TRUE;

	for (int i = 0; i < NUM_OF_ADCS_TLM; i++) {
		if (NULL != TlmElements[i].TlmFileName &&
			0 != TlmElements[i].TlmElementeSize)
		{
			res = c_fileCreate(TlmElements[i].TlmFileName,TlmElements[i].TlmElementeSize);
			TlmElements[i].OperatingFlag = TRUE_8BIT;
			if (FS_SUCCSESS != res) {
				TlmElements[i].ToSave = FALSE_8BIT;
				TlmElements[i].OperatingFlag = FALSE_8BIT;
				err = FALSE;
				//TODO: log error
			}
		}
	}
	return err;
}
