/*
 * HouseKeeping.h
 *
 *  Created on: Jan 18, 2019
 *      Author: elain
 */

#ifndef HOUSEKEEPING_H_
#define HOUSEKEEPING_H_

#include "../Global/Global.h"
#include "../Global/GlobalParam.h"
#include "../COMM/GSC.h"
#include "../Global/TLM_management.h"

#include <satellite-subsystems/cspaceADCS_types.h>

#define TASK_HK_HIGH_RATE_DELAY 	1000
#define TASK_HK_LOW_RATE_DELAY 	10000

#define NUMBER_OF_SOLAR_PANNELS	6

#define ACK_HK_SIZE ACK_DATA_LENGTH
#define EPS_HK_SIZE 51
#define SP_HK_SIZE	FLOAT_SIZE * NUMBER_OF_SOLAR_PANNELS
#define CAM_HK_SIZE 70
#define COMM_HK_SIZE 18
#define ADCS_HK_SIZE 34
#define FS_HK_SIZE 4*4

#define ADCS_SC_SIZE 6

#define ACK_FILE_NAME "ACKf"
#define EPS_HK_FILE_NAME "EPSf"
#define	SP_HK_FILE_NAME	"SPF"
#define CAM_HK_FILE_NAME "CAMf"
#define COMM_HK_FILE_NAME "COMMf"// TRX and ANTS HK
#define FS_HK_FILE_NAME "FSf"
#define ADCS_HK_FILE_NAME "ADCf"// ADCS

#define COLLECTING_HK_CODE_ERROR 333

typedef enum HK_dump_types{
	ACK_T = 0,
	log_files_events_T = 1,
	log_files_erorrs_T = 2,
	this_is_not_the_file_you_are_looking_for = 18,
	offlineTM_T = 50,
	ADCS_science_T = 128,

}HK_types;

typedef enum __attribute__ ((__packed__)) HK_AdcsTlmTypes{
	AdcsTlm_UnixTime = 			140 - ADCS_science_T,
	AdcsTlm_EstimatedAngles = 	146 - ADCS_science_T,
	AdcsTlm_EstimatedRates = 	147 - ADCS_science_T,
	AdcsTlm_SatellitePosition = 150 - ADCS_science_T,
	AdcsTlm_MagneticField = 	151 - ADCS_science_T,
	AdcsTlm_CoarseSunVec = 		152 - ADCS_science_T,
	AdcsTlm_FineSunVec =		153 - ADCS_science_T,
	AdcsTlm_RateSensor = 		155 - ADCS_science_T,
	AdcsTlm_WheelSpeed = 		156 - ADCS_science_T,
	AdcsTlm_MagnetorquerCommand = 157 - ADCS_science_T,
	AdcsTlm_RawCss1_6 = 		168 - ADCS_science_T,
	AdcsTlm_RawCss7_10 = 		169 - ADCS_science_T,
	AdcsTlm_RawMagnetic = 		170 - ADCS_science_T,
	AdcsTlm_CubeCtrlCurrents =	172 - ADCS_science_T,
	AdcsTlm_AdcsState = 		190 - ADCS_science_T,
	AdcsTlm_EstimatedMetaData = 193 - ADCS_science_T,
	AdcsTlm_PowerTemperature = 	195  - ADCS_science_T,
	AdcsTlm_MiscCurrents =		198 - ADCS_science_T
}HK_AdcsTlmTypes;
/*

 */
typedef union __attribute__ ((__packed__))
{
	byte raw[EPS_HK_SIZE];
	struct __attribute__((packed))
	{
		voltage_t photoVoltaic3;
		voltage_t photoVoltaic2;
		voltage_t photoVoltaic1;
		current_t photoCurrent3;
		current_t photoCurrent2;
		current_t photoCurrent1;
		current_t Total_photo_current;
		voltage_t VBatt;
		current_t Total_system_current;
		current_t currentChanel[6];
		short temp[6];
		unsigned int Number_of_EPS_reboots;
		unsigned char Cause_of_last_reset;
		unsigned char pptMode;
		byte channelStatus;
		systems_state systemState;
		byte EPSSateNumber;
	}fields;
}EPS_HK;

typedef union __attribute__ ((__packed__))
{
	byte raw[CAM_HK_SIZE];
	struct __attribute__((packed))
	{
		voltage_t VoltageInput5V;
		current_t CurrentInput5V;
		voltage_t VoltageFPGA1V;
		current_t CurrentFPGA1V;
		voltage_t VoltageFPGA1V8;
		current_t CurrentFPGA1V8;
		voltage_t VoltageFPGA2V5;
		current_t CurrentFPGA2V5;
		voltage_t VoltageFPGA3V3;
		current_t CurrentFPGA3V3;
		voltage_t VoltageFlash1V8;
		current_t CurrentFlash1V8;
		voltage_t VoltageFlash3V3;
		current_t CurrentFlash3V3;
		voltage_t VoltageSNSR1V8;
		current_t CurrentSNSR1V8;
		voltage_t VoltageSNSRVDDPIX;
		current_t CurrentSNSRVDDPIX;
		voltage_t VoltageSNSR3V3;
		current_t CurrentSNSR3V3;
		voltage_t VoltageFlashVTT09;
		temp_t TempSMU3AB;
		temp_t TempSMU3BC;
		temp_t TempREGU6;
		temp_t TempREGU8;
		temp_t TempFlash;
		voltage_t voltage_5V_DB;
		current_t current_5V_DB;
		voltage_t voltage_3V3_DB;
		current_t current_3V3_DB;
	}fields;
}CAM_HK;

typedef union __attribute__ ((__packed__))
{
	byte raw[COMM_HK_SIZE];
	struct __attribute__((packed))
	{
		voltage_t bus_vol;
		current_t total_curr;
        unsigned short tx_reflpwr;
        unsigned short tx_fwrdpwr;
        unsigned short rx_rssi;
		unsigned short pa_temp;
		unsigned short locosc_temp;
		unsigned short ant_A_temp;
		unsigned short ant_B_temp;
	}fields;
}COMM_HK;

typedef union __attribute__ ((__packed__))
{
	byte raw[SP_HK_SIZE];
	struct __attribute__((packed))
	{
		int32_t SP_temp[NUMBER_OF_SOLAR_PANNELS];
	}fields;
}SP_HK;

typedef union __attribute__ ((__packed__))
{
	byte raw[FS_HK_SIZE];
	struct __attribute__((packed))
	{
		  unsigned long  total;
		  unsigned long  free;
		  unsigned long  used;
		  unsigned long  bad;
	}fields;
}FS_HK;

typedef cspace_adcs_pwtempms_t ADCS_HK;


/*
 * @brief		save a new execution ACK in ACK file with the online time
 * @param[in]	ACK type (according to GSC.h)
 * @param[in]	err type (according to GSC.h)
 * @param[in]	id of the command the execution ACK is for
 * return 		0
 */
int save_ACK(Ack_type type, ERR_type err, command_id ACKcommandId);

int SP_HK_collect(SP_HK* hk_out);
int EPS_HK_collect(EPS_HK* hk_out);
int CAM_HK_collect(CAM_HK* hk_out);
int COMM_HK_collect(COMM_HK* hk_out);
int FS_HK_collect(FS_HK* hk_out, int SD_num);

int HK_find_fileName(HK_types type, char* fileName);
int HK_findElementSize(HK_types type);
int build_HK_spl_packet(HK_types type, byte* raw_data, TM_spl* packet);
#endif /* HOUSEKEEPING_H_ */
