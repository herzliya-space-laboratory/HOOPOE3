/*
 * AdcsGetDataAndTlm.h
 *
 *  Created on: Jul 21, 2019
 *      Author: איתי
 */

#include "Stage_Table.h"

#ifndef ADCSGETDATAANDTLM_H_
#define ADCSGETDATAANDTLM_H_

#define MAX_TELEMTRY_SIZE 272
#define DATA_SIZE 6

#include "AdcsTroubleShooting.h"

/*!
 *@init the adcs get and data main function
 *@using a arr holding the basic data of all the ADCS data we use
 *@get:
 *@		the arr of data to init
 */
void InitData(Gather_TM_Data Data[]);

/*!
 * @get all ADCS data and TLM and save if need to
 * @get the data from the ADCS computer.
 * @return:
 * @	err code by TroubleErrCode enum
 */
int GatherTlmAndData(Gather_TM_Data *Data);

#endif /* ADCSGETDATAANDTLM_H_ */
