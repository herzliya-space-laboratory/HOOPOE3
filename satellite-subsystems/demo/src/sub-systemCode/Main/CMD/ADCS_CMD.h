#ifndef ADCS_CMD_H_
#define ADCS_CMD_H_
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
//#include <freertos/semphr.h>
//#include <freertos/task.h>
//#include "../../ADCS/Stage_Table.h"
//#include "../../TRXVU.h"
//#include "../../ADCS.h"
#include "../../COMM/GSC.h"
//#include "../../COMM/splTypes.h"
//#include "../HouseKeeping.h"
#include "../commands.h"
#include "../../ADCS/AdcsTroubleShooting.h"
//#include <hal/Drivers/I2C.h>
//#include <stdlib.h>
//#include <string.h>

/*
 * InitAdcsCmdQueue - create the queue for the commands of the ADCS
 * @return:
 * 	ERR_SUCCESS - if the queue created successfully
 * 	ERR_FAIL - if it failed to create the queue
 */
TroubleErrCode AdcsCmdQueueInit();

/*
 * GetAdcsCmdFromQueue - pull the first command from the ADCS command queue
 * @param request - a pointer for the pulled command to be placed in
 * @return:
 * 	ERR_SUCCESS - if a command was pulled successfully
 * 	ERR_FAIL - if it failed to pull a command from the queue
 */
TroubleErrCode AdcsCmdQueueGet(TC_spl *request);

/*
 * AddAdcsCmdToQueue - push a new ADCS command to the queue end
 * @param request - a pointer for the command to be pushed
 * @return:
 * 	ERR_SUCCESS - if a command was pushed successfully
 * 	ERR_FAIL - if it failed to push a command to the queue because queue is full
 */
TroubleErrCode AdcsCmdQueueAdd(TC_spl *cmd);

/*
 * IsAdcsCmdQueueEmpty - check if the ADCS command queue is empty
 * @return:
 * 	TRUE - if the queue is empty
 * 	FALSE - if there are waiting commands in the queue
 */
int AdcsCmdQueueIsEmpty();

time_unix* getAdcsQueueWaitPointer();

#endif // !ADCS_CMD_H_

/*//def
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
*/
