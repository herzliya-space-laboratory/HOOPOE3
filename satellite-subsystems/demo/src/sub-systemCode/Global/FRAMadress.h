/*
 * FRAMadress.h
 *
 *  Created on: Dec 5, 2018
 *      Author: DBTn
 */

#ifndef FRAMADRESS_H_
#define FRAMADRESS_H_

// FRAM addresses
//MAIN
#define STATES_ADDR    0x100	// << 1 byte >>
#define FIRST_ACTIVATION_ADDR 0x101 // << 4 bytes >>
#define TIME_ADDR 0x105// << 4 bytes  >>
#define RESTART_FLAG_ADDR 0x109 // << 4 byte >>
#define STOP_TELEMETRY_ADDR 0x10C// << 1 byte >>
#define SHUT_ADCS_ADDR		0x10D// << 1 byte >>
#define SHUT_CAM_ADDR		0x10E// << 1 byte >>
#define OFFLINE_LIST_SETTINGS_ADDR	0x200// << OFFLINE_FRAM_STRUCT_SIZE * MAX_ITEMS_OFFLINE_LIST = 9 * 10 = 90 >>
//ANTS

#define ARM_DEPLOY_ADDR 0x1100// << 1 byte >> , can be 0 or 255
#define ANTS_AUTO_DEPLOY_FINISH_ADDR 0x1101
#define DEPLOY_ANTS_ATTEMPTS_ADDR	0x110F// <<3 * 4 = 12 byte >>, array of 3 variables - 3 attempts
#define STOP_DEPLOY_ATTEMPTS_ADDR	0x111B// << 1 byte >>


//EPS
#define EPS_VOLTAGES_ADDR 0x2100 // << EPS_VOLTAGES_SIZE_RAW bytes >>
#define EPS_STATES_ADDR 0x211C // << 1 byte >> Address of consumption state
#define EPS_ALPHA_ADDR		0x211D	//<< 4 byte >>


//TRX
#define BEACON_LOW_BATTERY_STATE_ADDR 	0x311D // << 2 bytes >>
#define TRANS_LOW_BATTERY_STATE_ADDR 	0x311F
#define NUMBER_COMMAND_FRAM_ADDR  		0x3121 // << 1 byte >> The number of delayed command stored in the FRAM
#define DELAY_COMMAD_FRAM_ADDR			0x3122 //<<MAX_NUMBER_OF_DELAY_COMMAND * SIZE_OF_COMMAND = 51000 bytes >> All delayed command will be stored in this address ass one big array of bytes
//#define NUMBER_PACKET_APRS_ADDR 0x8CEE // << 1 byte >> number of APRS packets in the FRAM
//#define APRS_PACKETS_ADDR 0x8CEF// << 20 * 18 = 360 >>
#define BEACON_TIME_ADDR 				0xF85A// << 1 byte >>
#define MUTE_TIME_ADDR					0xF85B//<<4 bytes>>
#define BIT_RATE_ADDR					0xF85F //<<1 bytes>>
#define TRANSPONDER_RSSI_ADDR			0xF860// << 2 bytes>


//ADCS
#define ADCS_LOOP_DELAY_ADDR 	(0X500)			// main loop delay
#define ADCS_LOOP_DELAY_SIZE 	(4)

#define ADCS_SYS_OFF_DELAY_ADDR 	(0x510)			// delay in case the system is off- wait to wakeup
#define ADCS_SYS_OFF_DELAY_SIZE		(4)

#define ADCS_QUEUE_WAIT_TIME_ADDR 	(0x520)		// max wait time for the adcs cmd queue
#define ADCS_QUEUE_WAIT_TIME_SIZE 	(4)

#define ADCS_TLM_SAVE_VECTOR_ADDR 	(0x530)		//<! FRAM start address

#define ADCS_TLM_PERIOD_VECTOR_ADDR (ADCS_TLM_SAVE_VECTOR_ADDR + NUM_OF_ADCS_TLM + 1)	//<! FRAM start address

#define ADCS_OVERRIDE_SAVE_TLM_ADDR	(0X580)
#define ADCS_OVERRIDE_SAVE_TLM_SIZE	(4)

#define ADCS_SUCCESSFUL_INIT_FLAG_ADDR	(0x590)
#define ADCS_SUCCESSFUL_INIT_FLAG_SIZE	(4)

//CAMERA
#define IMAGE_CHUNK_WIDTH_ADDR (0xD000)
#define IMAGE_CHUNK_WIDTH_SIZE (2)

#define IMAGE_CHUNK_HEIGHT_ADDR (0xD002)
#define IMAGE_CHUNK_HEIGHT_SIZE (2)

#define DATABASEFRAMADDRESS (0x20000)	// The database's address at the FRAM (currently 200 bytes long, alto its dynamic meaning it might change...)
#endif /* FRAMADRESS_H_ */
