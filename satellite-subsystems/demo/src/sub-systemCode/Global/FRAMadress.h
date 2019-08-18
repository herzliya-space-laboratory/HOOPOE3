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
#define DEPLOY_ANTS_ATTEMPTS_ADDR	0x10F// << 3 byte >>, array of 3 variables - 3 attempts


//ANTS
#define ARM_DEPLOY_ADDR 0x1100// << 1 byte >> , can be 0 or 255
#define ANTS_FRAM_ADDR 0x1101


//EPS
#define EPS_VOLTAGES_ADDR 0x2100 // << EPS_VOLTAGES_SIZE_RAW bytes >>
#define EPS_STATES_ADDR 0x211C // << 1 byte >> Address of consumption state


//TRX
#define BEACON_LOW_BATTERY_STATE_ADDR 0x311D // << 2 bytes >>
#define TRANS_LOW_BATTERY_STATE_ADDR 0x311F
#define NUMBER_COMMAND_FRAM_ADDR  0x3121 // << 1 byte >> The number of delayed command stored in the FRAM
#define DELAY_COMMAD_FRAM_ADDR	0x3122 //<<100 * SIZE_OF_COMMAND = 30080 bytes >> All delayed command will be stored in this address ass one big array of bytes
#define NUMBER_PACKET_APRS_ADDR 0x8CEE // << 1 byte >> number of APRS packets in the FRAM
#define APRS_PACKETS_ADDR 0x8CEF// << 20 * 18 = 360 >>
#define BEACON_TIME_ADDR 0x8E58// << 1 byte >>
#define MUTE_TIME_ADDR		0x8E59//<<4 bytes>>
#define BIT_RATE_ADDR		0x8E5D //<<1 bytes>>
#define TRANSPONDER_RSSI_ADDR	0x8E5E// << 2 bytes>


//ADCS
#define STAGE_TABLE_ADDR 0x9044


//CAMERA
#define IMAGE_CHUNK_WIDTH_ADDR 0xD000
#define IMAGE_CHUNK_WIDTH_SIZE 2

#define IMAGE_CHUNK_HEIGHT_ADDR 0xD002
#define IMAGE_CHUNK_HEIGHT_SIZE 2


#define DATABASEFRAMADDRESS 0x10000	// The database's address at the FRAM (currently 200 bytes long, alto its dynamic meaning it might change...)
#endif /* FRAMADRESS_H_ */
