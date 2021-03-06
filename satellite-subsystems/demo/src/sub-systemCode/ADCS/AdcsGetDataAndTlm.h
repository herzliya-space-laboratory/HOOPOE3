#ifndef ADCSGETDATAANDTLM_H_
#define ADCSGETDATAANDTLM_H_

#include <hcc/api_fat.h>
#include <satellite-subsystems/cspaceADCS_types.h>
#include "sub-systemCode/Global/Global.h"
#include "AdcsTroubleShooting.h"

#define NUM_OF_ADCS_TLM 18						//<! states the maximum number of telemetries the ADCS can save

#define ADCS_MAX_TLM_SIZE 272


#define ADCS_UNIX_TIME_FILENAME 		("UxTm")	//TLM ID 140
#define ADCS_EST_ANGLES_FILENAME 		("EAng")	//TLM ID 146
#define ADCS_EST_ANG_RATE_FILENAME 		("EAngR")	//TLM ID 147
#define ADCS_SAT_POSITION_FILENAME 		("Pos")		//TLM ID 150
#define ADCS_MAG_FIELD_VEC_FILENAME 	("MgFld")	//TLM ID 151
#define ADCS_COARSE_SUN_VEC_FILENAME	("CSnVc")	//TLM ID 152
#define ADCS_FINE_SUN_VEC_FILENAME		("FSnVc")	//TLM ID 153
#define ADCS_RATE_SENSOR_FILENAME		("RtS")		//TLM ID 155
#define ADCS_WHEEL_SPEED_FILENAME 		("WlSpd")	//TLM ID 156
#define ADCS_MAG_CMD_FILENAME			("MgCmd")	//TLM ID 157
#define ADCS_RAW_CSS_FILENAME_1_6 		("RC16")	//TLM ID 168
#define ADCS_RAW_CSS_FILENAME_7_10		("RC7A")	//TLM ID 169
#define ADCS_RAW_MAG_FILENAME 			("RwMg")	//TLM ID 170
#define ADCS_CUBECTRL_CURRENTS_FILENAME	("CCrnt")	//TLM ID 172
#define ADCS_STATE_TLM_FILENAME 		("StTlm")	//TLM ID 190
#define ADCS_EST_META_DATA				("EMtD")	//TLM ID 193
#define ADCS_POWER_TEMP_FILENAME		("PwTmp")	//TLM ID 195 (includes 174 inside)
#define	ADCS_MISC_CURR_FILENAME 		("MscCr")	//TLM ID 198

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
 * @brief	returns the measured CSS values
 * @return 	TRBL_FAIL in case of error.
 * 			TRBL_SUCCSESS in case of success.
 * 			TRBL_NULL_DATA in case of NULL input data.
 */
TroubleErrCode AdcsGetCssVector(unsigned char raw_css[10]);

/*!
 * @brief	returns the estimated angles
 * @return 	TRBL_FAIL in case of error.
 * 			TRBL_SUCCSESS in case of success.
 */
TroubleErrCode AdcsGetEstAngles(uint16_t angles[3]);

/*!
 * @brief 	allows the ground to command which telemetries will be saved to the SD
 * 			and which will not.
 * @param[in] tlm_to_save a boolean array stating which TLM will be saved.
 * @note	default is save all TLM
 * @return	Errors According to TroubleErrCode enum.
 */
int UpdateTlmToSaveVector(Boolean8bit tlm_to_save[NUM_OF_ADCS_TLM]);

/*!
 * @brief returns flags indicating which TLM's are saved.
 * @param[out] output buffer
 * @return	Errors According to TroubleErrCode enum.
 */
int GetTlmToSaveVector(Boolean8bit save_tlm_flag[NUM_OF_ADCS_TLM]);

/*!
 * @brief 	allows the ground to command TLM collection period between TLM saves.
 * @param[in] periods time periods array in [sec].
 * @note	default is 10 seconds between saves(1/10 Hz)
 * @return	Errors According to TroubleErrCode enum.
 */
int UpdateTlmPeriodVector(unsigned char periods[NUM_OF_ADCS_TLM]);

/*!
 * @brief returns the currently used TLM save periods.
 * @param[out] output buffer
 * @return	Errors According to TroubleErrCode enum.
 */
int GetTlmPeriodVector(unsigned char periods[NUM_OF_ADCS_TLM]);

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
 * @brief returns flags indicating which TLM is operating and which had errors.
 * @param[out] output buffer
 * @return	Errors According to TroubleErrCode enum.
 */
int GetOperationgFlags(unsigned char operating_flags[NUM_OF_ADCS_TLM]);

/*!
 * @brief returns vector of last time each telemetry has been saved.
 * @param[out] output buffer
 * @return	Errors According to TroubleErrCode enum.
 */
int GetLastSaveTimes( time_unix last_save_times[NUM_OF_ADCS_TLM]);

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
