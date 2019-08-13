#ifndef ADCSGETDATAANDTLM_H_
#define ADCSGETDATAANDTLM_H_

#include <hcc/api_fat.h>
#include "AdcsTroubleShooting.h"

//TODO: update this number to the correct number of telemetries
#define NUM_OF_ADCS_TLM 42 							//<! states the maximum number of telemetries the ADCS can save
#define TLM_SAVE_VECTOR_START_ADDR 	(0x4242)		//<! FRAM start address
#define TLM_SAVE_VECTOR_END_ADDR 	(TLM_SAVE_VECTOR_START_ADDR + NUM_OF_ADCS_TLM) //<! FRAM end address

#define TLM_PERIOD_VECTOR_START_ADDR 	(TLM_SAVE_VECTOR_END_ADDR+1)		//<! FRAM start address
#define TLM_PERIOD_VECTOR_END_ADDR 		(TLM_PERIOD_VECTOR_START_ADDR + NUM_OF_ADCS_TLM) //<! FRAM end address


#define ADCS_MAX_TLM_SIZE 272

#define ADCS_STATE_TLM_FILENAME 		("StateTlm")
#define ADCS_MAG_FIELD_VEC_FILENAME 	("MagFldVec")
#define ADCS_MAG_CMD_FILENAME 			("MagCmd")
#define ADCS_CSS_FILENAME				("CSSTlm")
#define ADCS_SENSOR_FILENAME			("Snsr")
#define ADCS_WHEEL_SPEED_FILENAME 		("WhlSpd")
#define ADCS_WHEEL_SPEED_CMD_FILENAME 	("WhlSpdCmd")
#define ADCS_EST_META_DATA				("EstMetaData")
#define ADCS_RAW_CSS_FILENAME 			("RawCss")
#define ADCS_RAW_MAG_FILENAME 			("RawMag")
#define ADCS_POWER_TEMP_FILENAME		("PowTemp")
#define ADCS_ESC_TIME_FILENAME			("ExcTime")
#define	ADCS_MISC_CURR_FILENAME 		("MiscCurr")
#define ADCS_TEMPERATURE_FILENAME 		("AdcsTemp")

typedef int(*AdcsTlmCollectFunc)(int,void*);

#define TLM_ELEMENT_SIZE		(1+1+4+FN_MAXNAME+1+1+1)

/*!
 * @brief 	allows the ground to command which telemetries will be saved to the SD
 * 			and which will not.
 * @param[in] tlm_to_save a boolean array stating which TLM will be saved.
 * @note	default is save all TLM
 * @return	TODO: fill return values
 */
int UpdateTlmSaveVector(Boolean8bit tlm_to_save[NUM_OF_ADCS_TLM]);


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
