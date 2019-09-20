/*
 * GSC.h
 *
 *  Created on: Oct 20, 2018
 *      Author: Hoopoe3n
 */

#ifndef GSC_H_
#define GSC_H_

#include "../Global/Global.h"
#include "splTypes.h"

#define MAX_NAMBER_OF_APRS_PACKETS 20//the max number of APRS packets in the FRAM list
#define NO_AVAILABLE_SLOTS -1

typedef enum ERR_type
{
	ERR_SUCCESS,
	ERR_FAIL,
	ERR_PARAMETERS,
	ERR_LENGTH,
	ERR_WRITE_FAIL,
	ERR_READ_FAIL,
	ERR_NOT_RUNNING,
	ERR_FULL,
	ERR_EMPTY,
	ERR_DRIVER,
	ERR_NOT_INITIALIZED,
	ERR_TIME_OUT,
	ERR_NOT_EXIST,
	ERR_ALLREADY_EXIST,
	ERR_SPL,
	ERR_MUTE,
	ERR_OFF,
	ERR_TRANSPONDER,
	ERR_SEMAPHORE,
	ERR_STOP_REQUEST,
}ERR_type;

//ACK enum
typedef enum ACK
{
	ACK_RECEIVE_COMM = 0,					//ACK when receive any packet
	ACK_NOTHING = 1,
	ACK_CMD_FAIL,
	ACK_FILE_SYSTEM ,
	ACK_FRAM ,
	ACK_QUEUE,
	ACK_SYSTEM,
	ACK_EPS,
	ACK_TRXVU,
	ACK_ANTS,
	ACK_CAMERA,
	ACK_I2C,
	ACK_LIST,
	ACK_FREERTOS,
	ACK_TASK,
}Ack_type;

typedef unsigned int command_id;

typedef struct inklajn_spl_TM
{
	uint8_t type;//service type
	uint8_t subType;//service sub type
	unsigned short length;//Length of data array
	time_unix time;//Unix time
	byte data[SIZE_TXFRAME - SPL_TM_HEADER_SIZE];//the data in the packet
}TM_spl;

typedef struct inklajn_spl_TC
{
	command_id id;
	uint8_t type;//service type
	uint8_t subType;//service sub type
	unsigned short length;//Length of data array
	time_unix time;//Unix time
	byte data[SIZE_OF_COMMAND - SPL_TC_HEADER_SIZE];//the data in the packet
}TC_spl;

/**
 *	@brief			decode raw data to a TM spl packet
 *	@param[in]		data to decode
 *	@param[in]		length of data to decode
 *	@param[in][out]	decoded TM spl packet
 *	@return			0 no problems in decoding,
 *					1 ,2 ,3 a problem with length
 *					-1 a NULL pointer
 */
int decode_TMpacket(byte* data, TM_spl* packet);//this function use calloc for the data

/**
 *	@brief			encode TM spl packet to raw data
 *	@param[out] 	data encoded data
 *	@param[out]		size length of *data
 *	@param[in]		packet TM spl packet to encode
 *	@return			0 no problems in decoding,
 *					1 a problem with length
 *					-1 a NULL pointer
 */
int encode_TMpacket(byte* data, int* size, TM_spl packet);//this function use calloc for the data

/**
 *	@brief			decode raw data to a TC spl packet
 *	@param[in]		data to decode
 *	@param[in]		length of data to decode
 *	@note			if length is -1 that means that the command is delayed command
 *	@param[in][out]	decoded TC spl packet
 *	@return			0 no problems in decoding,
 *					1 a problem with length
 *					-1 a NULL pointer
 */
int decode_TCpacket(byte* data, int length, TC_spl* packet);//this function use calloc for the data

/**
 *	@brief			encode TC spl packet to raw data
 *	@param[out] 	data encoded data
 *	@param[out]		size length of *data
 *	@param[in]		packet TC spl packet to encode
 *	@return			0 no problems in decoding,
 *					1 a problem with length
 *					-1 a NULL pointer
 */
int encode_TCpacket(byte* data, int* size, TC_spl packet);//this function use calloc for the data

/**
 * @brief 		build Ack inside spl.
 * @param[in] 	type Ack type according to Ack_type (typedef enum).
 * @param[in]	err Errors to be send.
 * return		ACK packet as inklajn_spl
 */
int build_raw_ACK(Ack_type type, ERR_type err, command_id ACKcommandId, byte* raw_ACK);

/**
 * @brief 		build raw ack
 * @param[in] 	type Ack type according to Ack_type (typedef enum).
 * @param[in]	err Errors to be send.
 * @param[in]	the array to fill the the raw ack
 * return		0
 * 				-1 a NULL pointer
 */
int build_data_field_ACK(Ack_type type, ERR_type err, command_id ACKcommandId, byte* data_feild);

void print_TM_spl_packet(TM_spl packet);
#endif /* GSC_H_ */
