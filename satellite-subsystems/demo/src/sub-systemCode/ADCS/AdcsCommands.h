#ifndef ADCS_COMMANDS_H_
#define ADCS_COMMANDS_H_

#include "AdcsTroubleShooting.h"
#include "sub-systemCode/COMM/GSC.h"

#define ADCS_I2C_TELEMETRY_ACK_REQUEST 240 // this i2c command request an ack with data from the ADCS


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


/*!
 * @brief allows the user to send a read request directly to the I2C.
 * @param[out] rv return value from the ADCS I2C ACK request
 * @return Errors according to "<hal/Drivers/I2C.h>"
 */
int AdcsReadI2cAck(int *rv);

/*!
 * @brief allows the user to send a command directly to the I2C bus to the ADCS.
 * @param[in] data data to send to the ADCS on the I2C bus.
 * @param[in] length length of the data
 * @param[in] ack acknowledge from the ADCS after command was sent
 * @return Errors according to "<hal/Drivers/I2C.h>"
 */
int AdcsGenericI2cCmd(unsigned char *data, unsigned int length, int *ack);


/*!
 * @brief Executes the command sent to the ADCS
 * @param[in] cmd the command to be executed
 * @return Errors according to 'TroubleErrCode' enum
 * @note Advised to be used in the 'Act Upon Command' logic
 */
TroubleErrCode AdcsExecuteCommand(TC_spl *cmd);


#endif /* ADCS_COMMANDS_H_ */
