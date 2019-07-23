/*
 * TRXVU.h
 *
 *  Created on: Oct 20, 2018
 *      Author: elain
 */

#ifndef TRXVU_H_
#define TRXVU_H_

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <satellite-subsystems/IsisTRXVU.h>

#include "Global/Global.h"
#include "COMM/GSC.h"
#include "payload/DataBase.h"
#include "payload/Butchering.h"

#define TRXVU_TO_CALSIGN "GS1"
#define TRXVU_FROM_CALSIGN "4x4hsc1"

#define NOMINAL_MODE TRUE
#define TRANSPONDER_MODE FALSE

#define MAX_NUTE_TIME 3

#define DEFAULT_TIME_TRANSMITTER (60 * 15)// in seconds

#define GET_BEACON_DELAY_LOW_VOLTAGE(ms) (ms * 3)

#ifndef TESTING
#define DEFULT_BEACON_DELAY 20// in seconds todo
#else
#define DEFULT_BEACON_DELAY 20// in seconds
#endif

#define MIN_TIME_DELAY_BEACON	10
#define MAX_TIME_DELAY_BEACON 	40

#define TRANSMMIT_DELAY_9600(length) (portTickType)((length + 30) * (5 / 6) + 30)
#define TRANSMMIT_DELAY_1200(length) (portTickType)(TRANSMMIT_DELAY_9600(length) * 100)

//todo: find real values
#define DEFULT_COMM_VOL		7250

#define MIN_TRANS_RSSI 	0
#define MAX_TRANS_RSSI 	4095

typedef enum
{
	nothing,
	deleteTask
}queueRequest;

typedef struct isisTXtlm
{
    float tx_reflpwr; 														///< Tx Telemetry reflected power.
    float pa_temp; ///< Tx Telemetry power amplifier temperature.
    float tx_fwrdpwr; ///< Tx Telemetry forward power.
    float tx_current; ///< Tx Telemetry transmitter current.
} isisTXtlm;

typedef struct isisRXtlm {
	float tx_current; ///< Rx Telemetry transmitter current.
	float rx_doppler; ///< Rx Telemetry receiver doppler.
	float rx_current; ///< Rx Telemetry receiver current.
	float bus_volt; ///< Rx Telemetry bus voltage.
	float board_temp; ///< Rx Telemetry board temperature.
	float pa_temp; ///< Rx Telemetry power amplifier temperature.
	float rx_rssi; ///< Rx Telemetry rssi measurement.
} isisRXtlm;

xQueueHandle xDumpQueue;
xQueueHandle xTransponderQueue;//
xTaskHandle xDumpHandle;//task handle for dump task
xTaskHandle xTransponderHandle;//task handle for transponder task

extern time_unix allow_transponder;

int availableFrames;//avail Number of the available slots in the transmission buffer of the VU_TC after the frame has been added

/**
 * 	@brief 		task function for TRXVU
 */
void TRXVU_task();

/**
 * 	@brief 		task function for dump
 * 	@param[in] 	need to be an unsigned char* (size 9 bytes), data of dump command (packet.data)
 */
void Dump_task(void *arg);

//todo:
void Dump_image_task(void *arg);

/**
 * 	@brief		task function for transponde mode
 * 	@param[in]	need to be unsigned char* (size 4 bytes), data of command (packet.data)
 */
void Transponder_task(void *arg);

//todo:
void Beacon_task(void *arg);
/**
 * 	@brief		send request to Dump_task to delete it self
 * 	@return		0 request send, 1 task does not exists, 2 queue is full
 */
int stop_dump();

/**
 * 	@brief		send request to Transponder_task to delete it self
 * 	@return		0 request send, 1 task does not exists, 2 queue is full
 */
int stop_transponder();

//todo
Boolean check_dump_delete();

/**
 * @brief		sends data as an AX.25 frame
 * @param[in]	data to send, can't be over
 * @param[in]	length of data to send as an AX.25 frame
 */
int TRX_sendFrame(byte* data, uint8_t length, ISIStrxvuBitrate bitRate);

/**
 * @brief		gets data from Rx buffer
 * @param[in]	length - length of dataBuffer
 * @param[out]	data_out - data from Rx buffer
 * @note		if define SEQUENCE_FLAG the code can handle data across number of frames
 * @return
 */
int TRX_getFrameData(unsigned int *length, byte* data_out);

//todo
int send_TM_spl(TM_spl packet, ISIStrxvuBitrate bitRate);
//todo
TM_spl spl_image(unsigned int index, fileType compressType, unsigned int pic_id, chunk_t chunk);

/**
 * 	@brief		initialize the TRXVU
 */
void init_trxvu(void);

/**
 * TODO:
 */
void reset_FRAM_TRXVU();

/**
 * 	@brief		one run of the TRXVU logic, according to ...
 */
void trxvu_logic();

void data_from_ground_logic();

/**
 * @brief		mute/unmute the Tx
 */
void mute_Tx(Boolean state);

int set_mute_time(unsigned short time);

void check_time_off_mute();

/**
 * 	@brief		build and send beacon packet
 */
void Beacon(ISIStrxvuBitrate bitRate);

/**
 * 	@brief 		reset the APRS packet list in the FRAM, deleting all packets
 */
void reset_APRS_list(Boolean firstActivation);

/**
 * 	@brief		send all APRS packets from the FRAM list and reseting it
 */
int send_APRS_Dump();

/**
 *  @brief		checks if the data we got from ground is an APRS packet or an ordinary data
 *  @param[in]	bytes array, the data we got from the ground
 *  @param[in]	the length of unsigned char* data
 *  @return		0 if ordinary data, 1 if APRS data
 */
int checks_APRS(byte* data);

/*
 * todo:
 */
void get_APRS_list();

/**
 * @brife 					change the Tx to nominal mode or transponder mode
 * @param[in] state			0 for nominal mode, 1 for transponder
 */
void change_TRXVU_state(Boolean state);

void change_trans_RSSI(byte* param);

temp_t check_Tx_temp();

//Tests
void vutc_getTxTelemTest(void);
void vurc_getRxTelemTest(void);
#endif /* TRXVU_H_ */
