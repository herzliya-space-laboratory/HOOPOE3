#include <stdio.h>
#include <stdlib.h>

#include <hcc/api_fat.h>
#include <hal/boolean.h>
#include <satellite-subsystems/cspaceADCS.h>
#include <satellite-subsystems/cspaceADCS_types.h>

#include "sub-systemCode/Global/TM_managment.h"
#include "AdcsGetDataAndTlm.h"


#define TLM_ELEMENT_SIZE		(1+1+4+FN_MAXNAME+1+1+1) //TODO: check if needed

typedef struct __attribute__ ((__packed__)){
	Boolean8bit ToSave;					//<! A flag stating whether to save this specific telemetry
	unsigned char TlmElementeSize;		//<! size of the telemetry to be collected
	AdcsTlmCollectFunc TlmCollectFunc;	//<! A function that collects the TLM (An ISIS driver function)
	char TlmFileName[FN_MAXNAME];		//<! The filename in which the TLM will be saved
	char SavePeriod;					//<! Time period between two TLM save points in time. In units of 1[sec]
	time_unix LastSaveTime;				//<! Last time the TLM was saved
	Boolean8bit OperatingFlag;			//<! A flag stating if the TLM is working correctly and no errors occuredd in the file creation or TLM collection
}AdcsTlmElement_t;

AdcsTlmElement_t TlmElements[NUM_OF_ADCS_TLM];

#define ADCS_DEFAULT_TLM_VECTOR	\
		{(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_statetlm_t),		cspaceADCS_getStateTlm,				ADCS_STATE_TLM_FILENAME,		1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_magfieldvec_t),		cspaceADCS_getMagneticFieldVec,		ADCS_MAG_FIELD_VEC_FILENAME,	1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_sunvec_t),			cspaceADCS_getCoarseSunVec,			ADCS_CSS_FILENAME,				1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_angrate_t),			cspaceADCS_getSensorRates,			ADCS_SENSOR_FILENAME,			1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_wspeed_t),			cspaceADCS_getWheelSpeed,			ADCS_WHEEL_SPEED_FILENAME,		1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_magtorqcmd_t),		cspaceADCS_getMagnetorquerCmd,		ADCS_MAG_CMD_FILENAME,			1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_estmetadata_t),		cspaceADCS_getEstimationMetadata,	ADCS_EST_META_DATA,				1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_rawcss1_6_t),		cspaceADCS_getRawCss1_6Measurements,ADCS_RAW_CSS_FILENAME,			1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_rawmagmeter_t),		cspaceADCS_getRawMagnetometerMeas,	ADCS_RAW_MAG_FILENAME,			1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_wspeed_t),			cspaceADCS_getWheelSpeedCmd,		ADCS_WHEEL_SPEED_CMD_FILENAME,	1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_pwtempms_t),		cspaceADCS_getPowTempMeasTLM,		ADCS_POWER_TEMP_FILENAME,		1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_exctm_t),			cspaceADCS_getADCSExcTimes,			ADCS_ESC_TIME_FILENAME,			1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_misccurr_t),		cspaceADCS_getMiscCurrentMeas,		ADCS_MISC_CURR_FILENAME,		1,0,	FALSE_8BIT},\
		(AdcsTlmElement_t){TRUE_8BIT,sizeof(cspace_adcs_msctemp_t),			cspaceADCS_getADCSTemperatureTlm,	ADCS_TEMPERATURE_FILENAME,		1,0,	FALSE_8BIT}};

TroubleErrCode SaveElementTlmAtIndex(unsigned int index);

TroubleErrCode GetTlmElements(unsigned char *data)
{
	if(NULL == data){
		return TRBL_NULL_DATA;
	}
	if(NULL == memcpy(data,TlmElements,sizeof(TlmElements))){
		return TRBL_FAIL;
		//TODO: log err
	}
	return TRBL_SUCCESS;
}

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

	if ( NULL != TlmElements[index].TlmCollectFunc &&
	NULL != TlmElements[index].TlmFileName &&
	TRUE_8BIT == TlmElements[index].ToSave) {
		//TODO: change 0 to a define
		time_unix curr_time = 0;
		Time_getUnixEpoch(&curr_time);
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
		}
	}
	return TRBL_SUCCESS;
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
			(TlmElements[index].TlmElementeSize != TlmElementSize || TlmElements[index].TlmElementeSize == 0) ?
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
	err = FRAM_read(Periods, TLM_PERIOD_VECTOR_START_ADDR,
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