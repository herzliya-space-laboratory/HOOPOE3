#include <stdio.h>
#include <stdlib.h>

#include <hcc/api_fat.h>
#include <hal/boolean.h>
#include <hal/errors.h>
#include <satellite-subsystems/cspaceADCS.h>
#include <satellite-subsystems/cspaceADCS_types.h>
#include "sub-systemCode/Global/TLM_management.h"
#include "sub-systemCode/Global/FRAMadress.h"

#include "sub-systemCode/Global/Global.h"
#include "AdcsCommands.h"
#include "AdcsGetDataAndTlm.h"

Boolean OverrideSaveTLM = TRUE;
AdcsTlmElement_t TlmElements[NUM_OF_ADCS_TLM];

#define ADCS_TLM_DEFAULT_COLLECT_PERIOD 10	// (1/10) Hz - once every 10 seconds

// Estimated Attitude Angles Telemetry(TLM ID 146)
typedef union __attribute__((__packed__)){
	unsigned char raw[6];
	struct __attribute__((__packed__)){
		unsigned short est_roll_angle;
		unsigned short est_pitch_angle;
		unsigned short est_yaw_angle;
	}fields;
}cspace_adcs_estAng_t;
int cspaceADCS_getEstimatedAngles(int adcs_id, cspace_adcs_estAng_t *est_ang){
	adcs_i2c_cmd i2c_cmd;
	int err = 0;
	(void)adcs_id;
	if(NULL == est_ang){
		return E_INPUT_POINTER_NULL;
	}
	i2c_cmd.id = GET_ADCS_EST_ANG_CMD_ID;
	i2c_cmd.length = GET_ADCS_EST_ANG_DATA_LENGTH;
	err = AdcsGenericI2cCmd(&i2c_cmd);
	if(0 != err){
		return err;
	}
	memcpy(est_ang,i2c_cmd.data,sizeof(*est_ang));
	return i2c_cmd.ack;
}

// Estimated Angular Rates Telemetry(TLM ID 147)
typedef union __attribute__((__packed__)){
	unsigned char raw[6];
	struct __attribute__((__packed__)){
		unsigned short est_X_ang_rate;
		unsigned short est_Y_ang_rate;
		unsigned short est_Z_ang_rate;
	}fields;
}cspace_adcs_estAngRates_t;
int cspaceADCS_getEstimatedAnglesRates(int adcs_id, cspace_adcs_estAngRates_t *est_ang){
	adcs_i2c_cmd i2c_cmd;
	(void)adcs_id;
	int err = 0;
	if(NULL == est_ang){
		return E_INPUT_POINTER_NULL;
	}
	i2c_cmd.id = GET_ADCS_EST_ANG_RATE_CMD_ID;
	i2c_cmd.length = GET_ADCS_EST_ANG_RATE_DATA_LENGTH;
	err = AdcsGenericI2cCmd(&i2c_cmd);
	if(0 != err){
		return err;
	}
	memcpy(est_ang,i2c_cmd.data,sizeof(*est_ang));
	return i2c_cmd.ack;
}

//  Satellite Position (LLH) Telemetry(TLM ID 150)
typedef union __attribute__((__packed__)){
	unsigned char raw[6];
	struct __attribute__((__packed__)){
		unsigned short latitude;
		unsigned short longitude;
		unsigned short altitude;
	}fields;
}cspace_adcs_satellitePosition_t;
int cspaceADCS_getSatellitePosition(int adcs_id, cspace_adcs_satellitePosition_t *est_ang){
	adcs_i2c_cmd i2c_cmd;
	(void)adcs_id;
	int err = 0;
	if(NULL == est_ang){
		return E_INPUT_POINTER_NULL;
	}
	i2c_cmd.id = ADCS_GET_SATELLITE_POSITION_CMD_ID;
	i2c_cmd.length = ADCS_GET_SATELLITE_POSITION_DATA_LENGTH;
	err = AdcsGenericI2cCmd(&i2c_cmd);
	if(0 != err){
		return err;
	}
	memcpy(est_ang,i2c_cmd.data,sizeof(*est_ang));
	return i2c_cmd.ack;
}

#define ADCS_DEFAULT_TLM_VECTOR	\
		{{TRUE_8BIT,	sizeof(cspace_adcs_unixtm_t),	(AdcsTlmCollectFunc)cspaceADCS_getCurrentTime,				\
		ADCS_UNIX_TIME_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_estAng_t),	(AdcsTlmCollectFunc)cspaceADCS_getEstimatedAngles,				\
		ADCS_EST_ANGLES_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_estAngRates_t),	(AdcsTlmCollectFunc)cspaceADCS_getEstimatedAnglesRates,		\
		ADCS_EST_ANG_RATE_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_satellitePosition_t),	(AdcsTlmCollectFunc)cspaceADCS_getSatellitePosition,\
		ADCS_SAT_POSITION_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_magfieldvec_t),	(AdcsTlmCollectFunc)cspaceADCS_getMagneticFieldVec,			\
		ADCS_MAG_FIELD_VEC_FILENAME,ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_sunvec_t),		(AdcsTlmCollectFunc)cspaceADCS_getCoarseSunVec,				\
		ADCS_COARSE_SUN_VEC_FILENAME,ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_sunvec_t),		(AdcsTlmCollectFunc)cspaceADCS_getFineSunVec,				\
		ADCS_FINE_SUN_VEC_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_angrate_t),		(AdcsTlmCollectFunc)cspaceADCS_getSensorRates,				\
		ADCS_RATE_SENSOR_FILENAME,		ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},				\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_wspeed_t),		(AdcsTlmCollectFunc)cspaceADCS_getWheelSpeed,				\
		ADCS_WHEEL_SPEED_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_magtorqcmd_t),	(AdcsTlmCollectFunc)cspaceADCS_getMagnetorquerCmd,			\
		ADCS_MAG_CMD_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},						\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_rawcss1_6_t),	(AdcsTlmCollectFunc)cspaceADCS_getRawCss1_6Measurements,	\
		ADCS_RAW_CSS_FILENAME_1_6,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_rawcss7_10_t),	(AdcsTlmCollectFunc)cspaceADCS_getRawCss7_10Measurements,	\
		ADCS_RAW_CSS_FILENAME_7_10,ADCS_TLM_DEFAULT_COLLECT_PERIOD,		(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_rawmagmeter_t),	(AdcsTlmCollectFunc)cspaceADCS_getRawMagnetometerMeas,		\
		ADCS_RAW_MAG_FILENAME,		ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_cctrlcurrms_t),	(AdcsTlmCollectFunc)cspaceADCS_getCControlCurrentMeasurements,\
		ADCS_CUBECTRL_CURRENTS_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},			\
		\
		{TRUE_8BIT,sizeof(cspace_adcs_statetlm_t),		(AdcsTlmCollectFunc)cspaceADCS_getStateTlm,					\
		ADCS_STATE_TLM_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_estmetadata_t),	(AdcsTlmCollectFunc)cspaceADCS_getEstimationMetadata,		\
		ADCS_EST_META_DATA,			ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_pwtempms_t),	(AdcsTlmCollectFunc)cspaceADCS_getPowTempMeasTLM,				\
		ADCS_POWER_TEMP_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		\
		{TRUE_8BIT,	sizeof(cspace_adcs_misccurr_t),	(AdcsTlmCollectFunc)cspaceADCS_getMiscCurrentMeas,				\
		ADCS_MISC_CURR_FILENAME,	ADCS_TLM_DEFAULT_COLLECT_PERIOD,	(time_unix)0,FALSE_8BIT},					\
		};

TroubleErrCode InitTlmElements()
{
	TroubleErrCode err = TRBL_SUCCESS;
	AdcsTlmElement_t default_elements[] = ADCS_DEFAULT_TLM_VECTOR;

	if (NULL ==  memcpy(TlmElements, default_elements, sizeof(default_elements))){
		//TODO: log err
		return TRBL_FAIL;
	}

	Boolean8bit save_tlm_flag[NUM_OF_ADCS_TLM] ={0};

	unsigned char Periods[NUM_OF_ADCS_TLM] = {0};

	err = FRAM_read(save_tlm_flag, ADCS_TLM_SAVE_VECTOR_START_ADDR,
			(NUM_OF_ADCS_TLM) * sizeof(*save_tlm_flag));
	if (0 != err) {
		//TODO: log err
		return TRBL_FRAM_READ_ERR;
	}
	err = FRAM_read((unsigned char*)Periods, ADCS_TLM_PERIOD_VECTOR_START_ADDR,
			NUM_OF_ADCS_TLM* sizeof(*Periods));
	if(0 != err){
		//TODO: log err
		return TRBL_FRAM_READ_ERR;
	}
	for (unsigned int i = 0; i < NUM_OF_ADCS_TLM; i++) {
		TlmElements[i].ToSave = save_tlm_flag[i];
		TlmElements[i].SavePeriod = Periods[i];
	}

	if(0 != FRAM_read((unsigned char*)&OverrideSaveTLM,ADCS_OVERRIDE_SAVE_TLM_ADDR,sizeof(OverrideSaveTLM))){
		OverrideSaveTLM = TRUE;
	}
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

TroubleErrCode SaveElementTlmAtIndex(unsigned int index)
{
	if (index >= NUM_OF_ADCS_TLM) {
		return TRBL_INPUT_PARAM_ERR;
	}

	int err = 0;
	FileSystemResult res = 0;
	unsigned char adcs_tlm[ADCS_MAX_TLM_SIZE] = { 0 };

	if ((NULL 	== TlmElements[index].TlmCollectFunc||
		 NULL	== TlmElements[index].TlmFileName )){
		return TRBL_NULL_DATA;
	}
	if((TRUE_8BIT != TlmElements[index].ToSave) && (!OverrideSaveTLM)	){
		return TRBL_SUCCESS;
	}

	if(TlmElements[index].SavePeriod == 0){ // in case of Period error
		TlmElements[index].SavePeriod = ADCS_TLM_DEFAULT_COLLECT_PERIOD;
	}
	time_unix curr_time = 0;
	Time_getUnixEpoch(&curr_time);
	if(curr_time < TlmElements[index].LastSaveTime){ // in case of past time(B.C of time update).
		TlmElements[index].LastSaveTime = 0;
	}
	if((curr_time - TlmElements[index].LastSaveTime) >= TlmElements[index].SavePeriod ){
		err = TlmElements[index].TlmCollectFunc(0, adcs_tlm);
		if (0 != err && (E_COMMAND_NACKED != err)) {
			// TODO: log error
			return err;
		}

		res = c_fileWrite(TlmElements[index].TlmFileName, adcs_tlm);
		if (FS_SUCCSESS != res) {
			return TRBL_FS_WRITE_ERR;
		}
		TlmElements[index].LastSaveTime = curr_time;
	}
	return TRBL_SUCCESS;
}

TroubleErrCode AdcsGetMeasAngSpeed(cspace_adcs_angrate_t* sen_rates)
{
	if(NULL == sen_rates){
		return TRBL_NULL_DATA;
	}
	if(0 != cspaceADCS_getSensorRates(0,sen_rates)){
		return TRBL_FAIL;
	}
	return TRBL_SUCCESS;
}

TroubleErrCode AdcsGetCssVector(unsigned char raw_css[10]){
	int err = cspaceADCS_getRawCss1_6Measurements(ADCS_ID,(cspace_adcs_rawcss1_6_t*)raw_css);
	if(0 != err){
		return TRBL_FAIL;
	}
	err = cspaceADCS_getRawCss7_10Measurements(ADCS_ID,(cspace_adcs_rawcss7_10_t*)(raw_css+sizeof(cspace_adcs_rawcss1_6_t)));
	if(0 != err){
		return TRBL_FAIL;
	}
	return TRBL_SUCCESS;
}

int UpdateTlmElementAtIndex(int index, Boolean8bit ToSave, char Period)
{
	TroubleErrCode err = TRBL_SUCCESS;
	if(index >= NUM_OF_ADCS_TLM){
		return TRBL_INPUT_PARAM_ERR;
	}
	TlmElements[index].ToSave = ToSave;

	ToSave = ToSave ? TRUE_8BIT : FALSE_8BIT;
	err = FRAM_write(&ToSave,ADCS_TLM_SAVE_VECTOR_START_ADDR + index,sizeof(ToSave));
	if(TRBL_SUCCESS != err){
		return TRBL_FRAM_WRITE_ERR;
	}

	if(0 != Period){
		TlmElements[index].SavePeriod = Period;
		err = FRAM_write((unsigned char*)&Period,ADCS_TLM_PERIOD_VECTOR_START_ADDR + index,sizeof(ToSave));
		if(TRBL_SUCCESS != err){
			return TRBL_FRAM_WRITE_ERR;
		}
	}
	return TRBL_SUCCESS;
	//TODO: log update
}

int UpdateTlmToSaveVector(Boolean8bit save_tlm_flag[NUM_OF_ADCS_TLM])
{
	if (NULL == save_tlm_flag) {
		//TODO: log error
		return (int) TRBL_NULL_DATA;
	}
	int err = 0;

	err = FRAM_write(save_tlm_flag, ADCS_TLM_SAVE_VECTOR_START_ADDR,
			NUM_OF_ADCS_TLM * sizeof(*save_tlm_flag));
	if (0 != err) {
		return err;
	}

	for (unsigned int i = 0; i < NUM_OF_ADCS_TLM; ++i) {
		TlmElements[i].ToSave = save_tlm_flag[i];
	}
	return 0;
}

int UpdateTlmPeriodVector(unsigned char periods[NUM_OF_ADCS_TLM])
{
	if (NULL == periods) {
		//TODO: log error
		return (int) TRBL_NULL_DATA;
	}
	int err = 0;
	for(unsigned int i = 0; i < NUM_OF_ADCS_TLM; i++ ){
		if(0 == periods[i]){
			periods[i] = ADCS_TLM_DEFAULT_COLLECT_PERIOD;
		}
	}
	err = FRAM_write(periods, ADCS_TLM_PERIOD_VECTOR_START_ADDR,
			NUM_OF_ADCS_TLM * sizeof(*periods));
	if (0 != err) {
		return err;
	}

	for (unsigned int i = 0; i < NUM_OF_ADCS_TLM; ++i) {
		TlmElements[i].SavePeriod = periods[i];
	}
	return 0;
}

int AdcsGetTlmOverrideFlag(Boolean *override_flag){
	if(NULL == override_flag){
		return TRBL_NULL_DATA;
	}

	int trbl;
	trbl = FRAM_read((unsigned char*)override_flag,ADCS_OVERRIDE_SAVE_TLM_ADDR,ADCS_OVERRIDE_SAVE_TLM_SIZE);
	if(0 != trbl){
		return TRBL_FRAM_READ_ERR;
	}
	return TRBL_SUCCESS;
}

int AdcsSetTlmOverrideFlag(Boolean override_flag){
	Boolean temp = TRUE;
	int trbl;
	if(override_flag){
		temp = TRUE;
		trbl = FRAM_write((unsigned char*)&temp,ADCS_OVERRIDE_SAVE_TLM_ADDR,ADCS_OVERRIDE_SAVE_TLM_SIZE);
	}else{
		temp = FALSE;
		trbl = FRAM_write((unsigned char*)&temp,ADCS_OVERRIDE_SAVE_TLM_ADDR,ADCS_OVERRIDE_SAVE_TLM_SIZE);
	}
	if(0 != trbl){
		return TRBL_FRAM_WRITE_ERR;
	}
	OverrideSaveTLM = temp;
	return TRBL_SUCCESS;

}

TroubleErrCode RestoreDefaultTlmElement(){
	AdcsTlmElement_t def_tlm[] = ADCS_DEFAULT_TLM_VECTOR;
	if(NULL == memcpy(TlmElements,def_tlm,sizeof(def_tlm))){
		return TRBL_FAIL;
	}
	unsigned char temp[NUM_OF_ADCS_TLM] = {0};
	for(int i = 0;i < NUM_OF_ADCS_TLM;i++){
		temp[i] = def_tlm[i].ToSave;
	}
	int err = FRAM_write(temp,ADCS_TLM_SAVE_VECTOR_START_ADDR,ADCS_TLM_SAVE_VECTOR_END_ADDR);
	if(0 != err){
		return TRBL_FRAM_WRITE_ERR;
	}

	for(int i = 0;i < NUM_OF_ADCS_TLM;i++){
		temp[i] = def_tlm[i].SavePeriod;
	}

	err = FRAM_write(temp,ADCS_TLM_PERIOD_VECTOR_START_ADDR,ADCS_TLM_PERIOD_VECTOR_END_ADDR);
	if(0 != err){
		return TRBL_FRAM_WRITE_ERR;
	}

	return TRBL_SUCCESS;
}

void GetTlmElementAtIndex(AdcsTlmElement_t *elem,unsigned int index){
	if(NULL == elem || index >= NUM_OF_ADCS_TLM){
		return;
	}
	memcpy(elem,&TlmElements[index],sizeof(*elem));
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
