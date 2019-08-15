#ifndef ADCS_COMMANDS_H_
#define ADCS_COMMANDS_H_

#include "AdcsTroubleShooting.h"
#include "sub-systemCode/COMM/GSC.h"

#define ADCS_I2C_TELEMETRY_ACK_REQUEST 240 // this i2c command request an ack with data from the ADCS

#define RESET_BOOT_REGISTER_CMD_ID			6
#define RESET_BOOT_REGISTER_LENGTH 			3

#define MTQ_CONFIG_CMD_ID 					21
#define MTQ_CONFIG_CMD_DATA_LENGTH 			3

#define SET_WHEEL_CONFIG_CMD_ID 			22
#define SET_WHEEL_CONFIG_DATA_LENGTH 		4

#define SET_GYRO_CONFIG_CMD_ID 				23
#define SET_GYRO_CONFIG_DATA_LENGTH 		3

#define SET_CSS_CONFIG_CMD_ID				24
#define SET_CSS_CONFIG_DATA_LENGTH			10

#define SET_CSS_SCALE_CMD_ID				25
#define SET_CSS_SCALE_DATA_LENGTH			11

#define SET_RATE_SENSOR_CONFIG_CMD_ID		36
#define SET_RATE_SENSOR_CONFIG_DATA_LENGTH	7

#define DETUMB_CTRL_PARAM_CMD_ID 			38
#define DETUMB_CTRL_PARAM_DATA_LENGTH 		14

#define SET_YWHEEL_CTRL_PARAM_CMD_ID		39
#define SET_YWHEEL_CTRL_PARAM_DATA_LENGTH	20

#define SET_RWHEEL_CTRL_PARAM_CMD_ID		40
#define SET_RWHEEL_CTRL_PARAM_DATA_LENGTH	8

#define SET_MOMENT_INERTIA_CMD_ID 			41
#define SET_MOMENT_INERTIA_DATA_LENGTH 		12

#define SET_PROD_INERTIA_CMD_ID 			42
#define SET_PROD_INERTIA_DATA_LENGTH 		12

#define SET_SGP4_ORBIT_INC_CMD_ID		 	46
#define SET_SGP4_ORBIT_ECC_CMD_ID 			47
#define SET_SGP4_ORBIT_RAAN_CMD_ID 			48
#define SET_SGP4_ORBIT_ARG_OF_PER_CMD_ID 	49
#define SET_SGP4_ORBIT_BSTAR_DRAG_CMD_ID 	50
#define SET_SGP4_ORBIT_MEAN_MOT_CMD_ID		51
#define SET_SGP4_ORBIT_MEAN_ANOM_CMD_ID 	52
#define SET_SGP4_ORBIT_EPOCH_CMD_ID 		53
#define SET_SGP4_ORBIT_DATA_LENGTH	 		8
#define GET_ADCS_FULL_CONFIG_CMD_ID 		206
#define GET_ADCS_FULL_CONFIG_DATA_LENGTH 	272


#define ADCS_FULL_CONFIG_DATA_LENGTH 		272
#define ADCS_SGP4_ORBIT_PARAMS_DATA_LENGTH 	68

#define ADCS_CMD_MAX_DATA_LENGTH			300

typedef enum __attribute__ ((__packed__)){
	ADCS_TC__NO_ERR 			= 0,
	ADCS_TC_INVALID_TC 			= 1,
	ADCS_TC_INCORRECT_LENGTH 	= 2,
	ADCS_TC_INCORRECT_PARAM 	= 3
}AdcsTcErrorReason;

typedef struct _adcs_i2c_cmd
{
	unsigned short id;
	unsigned short length;
	byte data[ADCS_CMD_MAX_DATA_LENGTH];
	AdcsTcErrorReason ack;
}adcs_i2c_cmd;


/*!
 * @brief allows the user to send a read request directly to the I2C.
 * @param[out] rv return value from the ADCS I2C ACK request
 * @return Errors according to "<hal/Drivers/I2C.h>"
 */
int AdcsReadI2cAck(AdcsTcErrorReason *rv);

/*!
 * @brief allows the user to send a command directly to the I2C bus to the ADCS.
 * @param[in][out] i2c_cmd executes data according to 'i2c_cmd' data. Saves ADCS ack into the struct.
 * @return Errors according to "<hal/Drivers/I2C.h>"
 */
int AdcsGenericI2cCmd(adcs_i2c_cmd *i2c_cmd);


/*!
 * @brief Executes the command sent to the ADCS
 * @param[in] cmd the command to be executed
 * @return Errors according to 'TroubleErrCode' enum
 * @note Advised to be used in the 'Act Upon Command' logic
 */
TroubleErrCode AdcsExecuteCommand(TC_spl *cmd);


#endif /* ADCS_COMMANDS_H_ */
