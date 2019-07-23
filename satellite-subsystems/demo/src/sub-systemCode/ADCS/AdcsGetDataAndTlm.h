/*
 * AdcsGetDataAndTlm.h
 *
 *  Created on: Jul 21, 2019
 *      Author: איתי
 */


#ifndef ADCSGETDATAANDTLM_H_
#define ADCSGETDATAANDTLM_H_

#define MAX_TELEMTRY_SIZE 272
#define DATA_SIZE 6

#include "AdcsTroubleShooting.h"


void IniTData(Gather_TM_Data Data[]);

/*!
 * @get all ADCS data and TLM and save if need to
 * @get the data from the ADCS computer.
 * @return:
 * @	err code by TroubleErrCode enum
 */
int GatherTlmAndData();

#endif /* ADCSGETDATAANDTLM_H_ */
