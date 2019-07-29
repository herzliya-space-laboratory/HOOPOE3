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
//! the file name of the CSS
#define NAME_OF_CSS_DATA_FILE ("Css_data")
//! the file name of the magnetic field
#define NAME_OF_MAGNETIC_FIELD_DATA_FILE ("Magnetic_Feild")
//! the file name of the css sun vector
#define CSS_SUN_VECTOR_DATA_FILE ("CSS_Sun_vector")
//! the file name of the wheel speed
#define WHEEL_SPEED_DATA_FILE ("Wheel_Speed")
//! the file name of the sensor rate
#define	SENSOR_RATE_DATA_FILE ("Sensor_Rate")
//! the file name of the magnetorquer cmd
#define MAGNETORQUER_CMD_FILE ("MAG_CMD")
//! the file name of the wheel speed cmd
#define WHEEL_SPEED_CMD_FILE ("Whell_CMD")
//! the file name of the taw magnetometer
#define RAW_MAGNETOMETER_FILE ("MAG_Raw")
//! the file name of the UGRF model
#define IGRF_MODEL_MAGNETIC_FIELD_VECTOR_FILE ("IGRF_MODEL")
//! the file name of the Gyro bias
#define GYRO_BIAS_FILE ("Gyro_BIAS")
//! the file name of the innovation vector
#define INNOVATION_VECTOR_FILE ("Inno_Vextor")
//! the file name of the error vector
#define ERROR_VECTOR_FILE ("Error_Vec")
//! the file name of the quaternion covariance
#define QUATERNION_COVARIANCE_FILE ("QUATERNION_COVARIANCE")
//! the file name of the angular rate covariance
#define ANGULAR_RATE_COVARIANCE_FILE ("ANGULAR_RATE_COVARIANCE")
//! the file name of the estimated angles
#define ESTIMATED_ANGLES_FILE ("ESTIMATED_ANGLES")
//! the file name of the angular rates
#define ESTIMATED_ANGULAR_RATE_FILE ("Estimated_AR")
//! the file name of the eci postion
#define NAME_OF_ECI_POSITION_DATA_FILE ("ECI_POS")
//! the file name of the satallite velocity
#define NAME_OF_SATALLITE_VELOCITY_DATA_FILE ("SAV_Vel")
//! the file name of the ecef position
#define NAME_OF_ECEF_POSITION_DATA_FILE ("ECEF_POS")
//! the file name of the LLH postion
#define NAME_OF_LLH_POSTION_DATA_FILE ("LLH_POS")
//! the file name of the estimated quaternion
#define ESTIMATED_QUATERNION_FILE ("EST_QUATERNION")

#define SUN_MODELLE_FILE ("Sun_Mode")


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
