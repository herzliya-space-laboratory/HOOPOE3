#ifndef ADCS_CMD_H_
#define ADCS_CMD_H_
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "../../ADCS/Stage_Table.h"
#include "../../TRXVU.h"
#include "../../ADCS.h"
#include "../../COMM/GSC.h"
#include "../../COMM/splTypes.h"
#include "../HouseKeeping.h"
#include <hal/Drivers/I2C.h>
#include <stdlib.h>
#include <string.h>

//def
#define SUCCESS 0
#define SYSTEM_OFF 4
#define ERROR -35


#define BootIndixLen 1
#define SGP4FullLen 64
#define SGP4UnitLen 8
#define startIndex 98
#define ADCS_CMD_LEN 255

#define ADCS_SET_BOOT_INDEX 98
#define ADCS_RUN_SELECTED_BOOT_PROGRAM 99
#define SET_SGP4_ORBIT_PRAMATARS 100
#define SET_SGP4_SUB_PARM 101 // -108
#define RESET_BOOT_REGISTERS 109
#define SAVE_CONFIGURATION 110
#define SAVE_ORBIT_PARAMETERS 111
#define CURRENT_UNIX_TIME 112
#define CACHE_ENABLED_STATE 113
#define DEPLOY_MAGNETOMETER_BOOM 114
#define ADCS_RUN_MODE 115
#define SET_MAGNETORQUER_CONFIG 117
#define SET_MAGNETOMETER_MOUNTING 118



typedef struct ADCS_CMD
{
	int(*Command)(void *cmd);
	int Length;

}ADCS_CMD_t;

static ADCS_CMD_t ADCS_cmd[ADCS_CMD_LEN];

static xQueueHandle AdcsCmdTour; //todo:add: AdcsCmdTour =  xQueueCreate(10,sizeof(tcp_spl)); to the ADCS init

//main
int ADCS_CMD(TC_spl decode);
void Set_ADCS_CMD(ADCS_CMD ADCS_cmd[]);


//protcted func call
void SetBootIndex(Ack_type *type, ERR_type *err, TC_spl cmd);
void RunSelectedBootProgram(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetSGP4OrbitPramatars(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetSGP4OrbitInclination(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetSGP4OrbitEccentricity(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetSGP4OrbitEpoch(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetSGP4OrbitRAAN(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetSGP4OrbitArgumentofPerigee(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetSGP4OrbitBStar(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetSGP4OrbitMeanMotion(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetSGP4OrbitMeanAnomaly(Ack_type *type, ERR_type *err, TC_spl cmd);
void ResetBootRegisters(Ack_type *type, ERR_type *err, TC_spl cmd);
void SaveConfiguration(Ack_type *type, ERR_type *err, TC_spl cmd);
void SaveOrbitParameters(Ack_type *type, ERR_type *err, TC_spl cmd);
void CurrentUnixTime(Ack_type *type, ERR_type *err, TC_spl cmd);
void CacheEnabledState(Ack_type *type, ERR_type *err, TC_spl cmd);
void DeployMagnetometerBoom(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetMagnetorquerConfiguration(Ack_type *type, ERR_type *err, TC_spl cmd);
void SetMagnetometerMounting(Ack_type * type, ERR_type * err, TC_spl cmd);
void SetMagnetometerOffsetAndScalingConfiguration(Ack_type * type, ERR_type * err, TC_spl cmd);







#endif // !ADCS_CMD_H_
