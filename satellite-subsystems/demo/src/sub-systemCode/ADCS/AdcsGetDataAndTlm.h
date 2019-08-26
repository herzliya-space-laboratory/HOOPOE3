#ifndef ADCSGETDATAANDTLM_H_
#define ADCSGETDATAANDTLM_H_

#include <hcc/api_fat.h>
#include "sub-systemCode/Global/Global.h"
#include "AdcsTroubleShooting.h"

//TODO: update this number to the correct number of telemetries
#define NUM_OF_ADCS_TLM 12						//<! states the maximum number of telemetries the ADCS can save

#ifndef TLM_SAVE_VECTOR_START_ADDR
	#define TLM_SAVE_VECTOR_START_ADDR (0x6242)
#endif

#ifndef TLM_SAVE_VECTOR_END_ADDR
	#define TLM_SAVE_VECTOR_END_ADDR ((TLM_SAVE_VECTOR_START_ADDR) + (NUM_OF_ADCS_TLM) * sizeof(Boolean8bit))
#endif

#define ADCS_MAX_TLM_SIZE 272

#define ADCS_STATE_TLM_FILENAME 		("StTlm")
#define ADCS_EST_META_DATA				("EstMtDt")
#define ADCS_COARSE_SUN_VEC_FILENAME	("CrsSnVec")
#define ADCS_FINE_SUN_VEC_FILENAME		("FnSnVec")
#define ADCS_SENSOR_FILENAME			("Snsr")
#define ADCS_WHEEL_SPEED_FILENAME 		("WlSpd")
#define ADCS_RAW_MAG_FILENAME 			("RawMag")
#define ADCS_MAG_FIELD_VEC_FILENAME 	("MgFldVc")
#define ADCS_RAW_CSS_FILENAME_1_6 		("RwCss16")
#define ADCS_RAW_CSS_FILENAME_7_10		("RwCss710")
#define ADCS_POWER_TEMP_FILENAME		("PowTemp")
#define	ADCS_MISC_CURR_FILENAME 		("MscCurr")

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
 * @brief 	allows the ground to command which telemetries will be saved to the SD
 * 			and which will not.
 * @param[in] tlm_to_save a boolean array stating which TLM will be saved.
 * @note	default is save all TLM
 * @return	TODO: fill return values
 */
int UpdateTlmToSaveVector(Boolean8bit tlm_to_save[NUM_OF_ADCS_TLM]);

/*!
 * @brief updates the Tlm element at index with the input parameters
 * @param[in] 	index index at which to update in the elements array
 * @param[in] 	TlmElementSize	size of the telemetry
 * @param[in]	ToSave		save telemetry flag(TRUE = save; FALSE = don't save)
 * @param[in]	file_name	the file name to be updated
 * @param[in] 	Period 		TLM save period- time in seconds between TLM saves
 * @note if you don't want to update an element put NULL in it
 */
void UpdateTlmElementAtIndex(int index,AdcsTlmCollectFunc func,unsigned char TlmElementSize,
		Boolean8bit ToSave, char file_name[FN_MAXNAME], char Period);

/*!
 * @brief restores the TLM element array to its default value.
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

#endif /* ADCSGETDATAANDTLM_H_ */
