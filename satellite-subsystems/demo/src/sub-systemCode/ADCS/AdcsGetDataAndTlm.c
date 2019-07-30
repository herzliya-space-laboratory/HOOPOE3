#include <stdio.h>
#include <stdlib.h>

#include <hcc/api_fat.h>
#include <hal/boolean.h>
#include <satellite-subsystems/cspaceADCS.h>
#include <satellite-subsystems/cspaceADCS_types.h>

#include "sub-systemCode/Global/TM_managment.h"

#include "AdcsGetDataAndTlm.h"


typedef struct{
	Boolean8bit ToSave;
	unsigned char TlmElementeSize;
	AdcsTlmCollectFunc TlmCollectFunc;
	char TlmFileName[FN_MAXNAME];
}AdcsTlmElement;


AdcsTlmElement TlmElements[NUM_OF_ADCS_TLM];

#define ADCS_DEFAULT_TLM_VECTOR	\
		{(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_statetlm_t),		cspaceADCS_getStateTlm,				ADCS_STATE_TLM_FILENAME		},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_magfieldvec_t),	cspaceADCS_getMagneticFieldVec,		ADCS_MAG_FIELD_VEC_FILENAME	},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_sunvec_t),		cspaceADCS_getCoarseSunVec,			ADCS_CSS_FILENAME			},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_angrate_t),		cspaceADCS_getSensorRates,			ADCS_SENSOR_FILENAME		},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_wspeed_t),		cspaceADCS_getWheelSpeed,			ADCS_WHEEL_SPEED_FILENAME	},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_magtorqcmd_t),	cspaceADCS_getMagnetorquerCmd,		ADCS_MAG_CMD_FILENAME		},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_estmetadata_t),	cspaceADCS_getEstimationMetadata,	ADCS_EST_META_DATA			},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_rawcss1_6_t),		cspaceADCS_getRawCss1_6Measurements,ADCS_RAW_CSS_FILENAME		},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_rawmagmeter_t),	cspaceADCS_getRawMagnetometerMeas,	ADCS_RAW_MAG_FILENAME		},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_wspeed_t),		cspaceADCS_getWheelSpeedCmd,		ADCS_WHEEL_SPEED_CMD_FILENAME},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_pwtempms_t),		cspaceADCS_getPowTempMeasTLM,		ADCS_POWER_TEMP_FILENAME	},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_exctm_t),			cspaceADCS_getADCSExcTimes,			ADCS_ESC_TIME_FILENAME		},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_misccurr_t),		cspaceADCS_getMiscCurrentMeas,		ADCS_MISC_CURR_FILENAME		},\
		(AdcsTlmElement){TRUE_8BIT,sizeof(cspace_adcs_msctemp_t),		cspaceADCS_getADCSTemperatureTlm,	ADCS_TEMPERATURE_FILENAME	}};


// -- function declarations
TroubleErrCode SaveElementTlmAtIndex(unsigned int index);

// --

int UpdateTlmSaveVector(Boolean8bit save_tlm_flag[NUM_OF_ADCS_TLM])
{
	if(NULL == save_tlm_flag){
		//TODO: log error
		return TRBL_NULL_DATA;
	}

	int err = 0;

	err = FRAM_write(save_tlm_flag,TLM_SAVE_VECTOR_START_ADDR,TLM_SAVE_VECTOR_END_ADDR);
	if(0 != err){
		return err;
	}

	for (unsigned int i = 0; i < NUM_OF_ADCS_TLM; ++i){
		TlmElements[i].ToSave = save_tlm_flag[i];
	}
	return 0;
}


void GatherTlmAndData()
{
	TroubleErrCode err = TRBL_NO_ERROR;
	for (unsigned int i = 0; i < NUM_OF_ADCS_TLM; ++i)
	{
		err = SaveElementTlmAtIndex(i);
		if(TRBL_NO_ERROR != err)
		{
			//TODO: log err
		}
	}
}


TroubleErrCode SaveElementTlmAtIndex(unsigned int index)
{
	if(index >= NUM_OF_ADCS_TLM){
		return TRBL_INPUT_PARAM_ERR;
	}

	unsigned char adcs_tlm[ADCS_MAX_TLM_SIZE] = {0};
	TroubleErrCode err = 0;
	FileSystemResult res = 0;
	if(	NULL != TlmElements[index].TlmCollectFunc	&&
		NULL != TlmElements[index].TlmFileName		&&
		TRUE_8BIT ==  TlmElements[index].ToSave )
	{
		//TODO: change 0 to a define
		err = TlmElements[index].TlmCollectFunc(0,adcs_tlm);
		if(0 != err){
			// TODO: log error
			return err;
		}

		res =  c_fileWrite(TlmElements[index].TlmFileName,adcs_tlm);
		if(FS_SUCCSESS != res){
			return TRBL_FS_WRITE_ERR;
		}
	}
	return TRBL_NO_ERROR;
}


void UpdateTlmElementAtIndex(int index,AdcsTlmCollectFunc func,unsigned char TlmElementSize,
		Boolean8bit ToSave, char file_name[FN_MAXNAME])
{
	if(NULL != func){
		TlmElements[index].TlmCollectFunc = func;
	}
	if(NULL != file_name){
		memcpy(TlmElements[index].TlmFileName,file_name,F_MAXNAME);
	}
	TlmElements[index].ToSave = ToSave;
	if(NULL != file_name){
		TlmElements[index].TlmElementeSize = TlmElementSize;
	}
	//TODO: log update
}


TroubleErrCode InitTlmElements()
{
	//TODO: chage return error codes to appropriate values

	TroubleErrCode err = 0;

	AdcsTlmElement default_elements[NUM_OF_ADCS_TLM] = ADCS_DEFAULT_TLM_VECTOR;

	unsigned int tlm_elements_length = sizeof(default_elements) / sizeof(default_elements[0]);
	void* ptr = memcpy(TlmElements,default_elements,tlm_elements_length);
	if(NULL == ptr){
		//TODO: change -1 to appropriate error
		//TODO: log err
		return -1;
	}
	Boolean8bit *save_tlm_flag = malloc(tlm_elements_length * sizeof(Boolean8bit));
	if(NULL == save_tlm_flag){
		//TODO: change -1 to appropriate error
		//TODO: log err
		return -1;
	}

	err = FRAM_read(save_tlm_flag,TLM_SAVE_VECTOR_START_ADDR,TLM_SAVE_VECTOR_END_ADDR);
	if(0 != err){
		//TODO: log err
		free(save_tlm_flag);
		return err;
	}
	for(unsigned int i = 0; i < NUM_OF_ADCS_TLM; i++)
	{
		TlmElements[i].ToSave = save_tlm_flag[i];
	}
	free(save_tlm_flag);

	//TODO: log successful init
	return TRBL_NO_ERROR;
}


Boolean CreateTlmElementFiles()
{
	FileSystemResult res = 0;
	Boolean err = TRUE;
	for(int i = 0; i < NUM_OF_ADCS_TLM; i++)
	{
		if(NULL != TlmElements[i].TlmFileName && 0 != TlmElements[i].TlmElementeSize){
			res = c_fileCreate(TlmElements[i].TlmFileName,TlmElements[i].TlmElementeSize);
			if(FS_SUCCSESS != res)
			{
				TlmElements[i].ToSave = FALSE_8BIT;
				err = FALSE;
				//TODO: log error
			}
		}
	}
	return err;
}
