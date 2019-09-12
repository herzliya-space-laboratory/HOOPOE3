#ifndef ADCSGETDATAANDTLM_H_
#define ADCSGETDATAANDTLM_H_

#include <hcc/api_fat.h>
#include <satellite-subsystems/cspaceADCS_types.h>
#include "sub-systemCode/Global/Global.h"
#include "AdcsTroubleShooting.h"

//TODO: update this number to the correct number of telemetries
#define NUM_OF_ADCS_TLM 18						//<! states the maximum number of telemetries the ADCS can save

#ifndef TLM_SAVE_VECTOR_START_ADDR
	#define TLM_SAVE_VECTOR_START_ADDR (0x6242)
#endif

#ifndef TLM_SAVE_VECTOR_END_ADDR
	#define ADCS_TLM_SAVE_VECTOR_END_ADDR ((ADCS_TLM_SAVE_VECTOR_START_ADDR) + (NUM_OF_ADCS_TLM) * sizeof(Boolean8bit))
#endif

#define ADCS_MAX_TLM_SIZE 272


#define ADCS_UNIX_TIME_FILENAME 		("UnxTm")	//TLM ID 140
#define ADCS_EST_ANGLES_FILENAME 		("EstAng")	//TLM ID 146
#define ADCS_EST_ANG_RATE_FILENAME 		("EAngRt")	//TLM ID 147
#define ADCS_SAT_POSITION_FILENAME 		("SatPos")	//TLM ID 150
#define ADCS_MAG_FIELD_VEC_FILENAME 	("MgFldV")	//TLM ID 151
#define ADCS_COARSE_SUN_VEC_FILENAME	("CrsSnV")	//TLM ID 152
#define ADCS_FINE_SUN_VEC_FILENAME		("FnSnVc")	//TLM ID 153
#define ADCS_RATE_SENSOR_FILENAME		("RtSnsr")	//TLM ID 155
#define ADCS_WHEEL_SPEED_FILENAME 		("WlSpd")	//TLM ID 156
#define ADCS_MAG_CMD_FILENAME			("MagCmd")	//TLM ID 157
#define ADCS_RAW_CSS_FILENAME_1_6 		("RCss16")	//TLM ID 168
#define ADCS_RAW_CSS_FILENAME_7_10		("RCss7A")	//TLM ID 169
#define ADCS_RAW_MAG_FILENAME 			("RawMag")	//TLM ID 170
#define ADCS_CUBECTRL_CURRENTS_FILENAME	("CCCrnt")	//TLM ID 172
#define ADCS_STATE_TLM_FILENAME 		("StTlm")	//TLM ID 190
#define ADCS_EST_META_DATA				("EstMtD")	//TLM ID 193
#define ADCS_POWER_TEMP_FILENAME		("PowTmp")	//TLM ID 195 (includes 174 inside)
#define	ADCS_MISC_CURR_FILENAME 		("MscCur")	//TLM ID 198

typedef int(*AdcsTlmCollectFunc)(int,void*);

typedef struct __attribute__ ((__packed__)) AdcsTlmElement_t{
	Boolean8bit ToSave;					//<! A flag stating whether to save this specific telemetry
	unsigned char TlmElementeSize;		//<! size of the telemetry to be collected
	AdcsTlmCollectFunc TlmCollectFunc;	//<! A function that collects the TLM (An ISIS driver function)
	char TlmFileName[FN_MAXNAME];		//<! The filename in which the TLM will be saved
	char SavePeriod;					//<! Time period between two TLM save points in time. In units of 1[sec]
	time_unix LastSaveTime;				//<! Last time the TLM was saved
	Boolean8bit OperatingFlag;			//<! A flag stating if the TLM is working correctly and no errors occuredd in the file creation or TLM collection
}AdcsTlmElement_t;

/*!
 * 	@brief Initializes the telemetry array.
 *	@return return errors according to TroubleErrCode enum
 */
TroubleErrCode InitTlmElements();

/*!
 * 	@brief Initializes the telemetry array.
 *	@return FALSE if error occured in file creation
 */
Boolean CreateTlmElementFiles();

/*!
 * @brief returns the measured angular rates of the satellite.
 * @param[out] sen_rates the angular rates of axis X, Y, Z
 * @return 	TRBL_FAIL in case of error.
 * 			TRBL_SUCCSESS in case of success.
 * 			TRBL_NULL_DATA in case of NULL input data.
 */
TroubleErrCode AdcsGetMeasAngSpeed(cspace_adcs_angrate_t* sen_rates);

/*!
 * @brief 	allows the ground to command which telemetries will be saved to the SD
 * 			and which will not.
 * @param[in] tlm_to_save a boolean array stating which TLM will be saved.
 * @note	default is save all TLM
 * @return	Errors According to TroubleErrCode enum.
 */
int UpdateTlmToSaveVector(Boolean8bit tlm_to_save[NUM_OF_ADCS_TLM]);

/*!
 * @brief 	allows the ground to command TLM collection period between TLM saves.
 * @param[in] periods time periods array in [sec].
 * @note	default is 10 seconds between saves(1/10 Hz)
 * @return	Errors According to TroubleErrCode enum.
 */
int UpdateTlmPeriodVector(unsigned char periods[NUM_OF_ADCS_TLM]);

/*!
 * @brief updates the Tlm element at index with the input parameters
 * @param[in] 	index index at which to update in the elements array
 * @param[in]	ToSave		save telemetry flag(TRUE = save; FALSE = don't save)
 * @param[in] 	Period 		TLM save period- time in seconds between TLM saves
 * @note if you don't want to update 'Period' put 0 in it
 * @return	Errors According to TroubleErrCode enum.
 */
int UpdateTlmElementAtIndex(int index, Boolean8bit ToSave, char Period);

/*!
 * @breif return the current state of the override flag.
 * @param[out] override_flag output buffer for the flag.
 * @return 	Errors According to TroubleErrCode enum.
 */
int AdcsGetTlmOverrideFlag(Boolean *override_flag);

/*!
 * @brief the override flag forces the system to save telemetry even if errors occure.
 * @param[in] override_flag value to be set to the override flag.
 * @note faulty telemtry will be saved in the file and will be garbage data(all zeros or random figures).
 * @return	Errors According to TroubleErrCode enum.
 */
int AdcsSetTlmOverrideFlag(Boolean override_flag);

/*!
 * @brief restores the TLM element array to its default values and updated the FRAM.
 * @return 	TRBL_FAIL in case of error.
 * 			TRBL_SUCCSESS in case of successful restoration
 */
TroubleErrCode RestoreDefaultTlmElement();

/*!
 * @brief returns the value of an element at a specified index.
 * @param[out] elem the output buffer
 * @param[in] index get element at index.
 */
void GetTlmElementAtIndex(AdcsTlmElement_t * elem,unsigned int index);

/*!
 * @get all ADCS data and TLM and save if need to
 * @get the data from the ADCS computer.
 */
TroubleErrCode GatherTlmAndData();


#endif /* ADCSGETDATAANDTLM_H_ */
