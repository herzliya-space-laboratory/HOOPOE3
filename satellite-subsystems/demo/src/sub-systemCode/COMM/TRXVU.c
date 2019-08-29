/*
/ * TRXVU.c
 *
 *  Created on: Oct 20, 2018
 *      Author: elain
 */
#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include "../Global/freertosExtended.h"
#include <freertos/task.h>

#include <at91/utility/exithandler.h>
#include <at91/commons.h>
#include <at91/utility/trace.h>
#include <at91/peripherals/cp15/cp15.h>

#include <hal/Utility/util.h>
#include <hal/Timing/WatchDogTimer.h>
#include <hal/Timing/Time.h>
#include <hal/Drivers/I2C.h>
#include <hal/Drivers/LED.h>
#include <hal/boolean.h>
#include <hal/errors.h>
#include <hcc/api_fat.h>
#include <hal/Storage/FRAM.h>

#include <string.h>

#include "../TRXVU.h"

#include "../Main/HouseKeeping.h"
#include "../Main/commands.h"
#include "../Global/FRAMadress.h"
#include "../Global/TLM_management.h"
#include "splTypes.h"
#include "APRS.h"
#include "../Global/GlobalParam.h"
#include "../ADCS/Stage_Table.h"
#include "DelayedCommand_list.h"

#define FIRST 0

#define CHECK_SENDING_BEACON_ABILITY	(get_system_state(Tx_param) && !get_system_state(transponder_active_param) && !get_system_state(mute_param) && !get_system_state(dump_param))
#define CHECK_STARTING_DUMP_ABILITY		(!get_system_state(mute_param)) && (!get_system_state(transponder_active_param)) && (get_system_state(Tx_param))

xSemaphoreHandle xIsTransmitting;

time_unix allow_transponder;

xTaskHandle xBeaconTask;

static byte Dump_buffer[DUMP_BUFFER_SIZE];

void update_FRAM_bitRate()
{
	byte dat;
	int error = FRAM_read(&dat, BIT_RATE_ADDR, 1);
	check_int("TRXVU_init_softWare, FRAM_read", error);
	ISIStrxvuBitrate newParam = DEFAULT_BIT_RATE;
	for (uint8_t i = 1; i < 9; i *= 2)
	{
		if (dat == i)
		{
			newParam = (ISIStrxvuBitrate)dat;
			break;
		}
	}
	vTaskDelay(2000);
	printf("new bit rate value: %d\n", newParam);
	error = IsisTrxvu_tcSetAx25Bitrate(0, newParam);
	vTaskDelay(1000);
	check_int("IsisTrxvu_tcSetAx25Bitrate, update_FRAM_bitRate", error);
}
void toggle_idle_state()
{
	for (int i = 0; i < 2; i++)
	{
		vTaskDelay(500);
		int retValInt = IsisTrxvu_tcSetIdlestate(0, trxvu_idle_state_on);
		check_int("init_trxvu, IsisTrxvu_tcSetIdlestate, on", retValInt);
		vTaskDelay(1000);
		retValInt = IsisTrxvu_tcSetIdlestate(0, trxvu_idle_state_off);
		check_int("init_trxvu, IsisTrxvu_tcSetIdlestate, off", retValInt);
		vTaskDelay(1500);
	}
}

void TRXVU_init_hardWare()
{
	int retValInt = 0;

	// Definition of I2C and TRXUV
	ISIStrxvuI2CAddress myTRXVUAddress[1];
	ISIStrxvuFrameLengths myTRXVUBuffers[1];
	ISIStrxvuBitrate myTRXVUBitrates[1];

	//I2C addresses defined
	myTRXVUAddress[0].addressVu_rc = I2C_TRXVU_RC_ADDR;
	myTRXVUAddress[0].addressVu_tc = I2C_TRXVU_TC_ADDR;

	//Buffer definition
	myTRXVUBuffers[0].maxAX25frameLengthTX = SIZE_TXFRAME;
	myTRXVUBuffers[0].maxAX25frameLengthRX = SIZE_RXFRAME;

	//Bitrate definition
	myTRXVUBitrates[0] = trxvu_bitrate_9600;
	retValInt = IsisTrxvu_initialize(myTRXVUAddress, myTRXVUBuffers, myTRXVUBitrates, 1);
	check_int("init_trxvu, IsisTrxvu_initialize", retValInt);

	if (!get_system_state(mute_param))
	{
		toggle_idle_state();
		if (get_system_state(transponder_active_param))
			change_TRXVU_state(NOMINAL_MODE);
	}
}
void TRXVU_init_softWare()
{
	get_APRS_list();
	get_delayCommand_list();

	update_FRAM_bitRate();
	vTaskDelay(100);

	//1. create binary semaphore for transmitting
	vSemaphoreCreateBinary(xIsTransmitting);
	vTaskDelay(SYSTEM_DEALY);
	xDumpQueue = xQueueCreate(1, sizeof(queueRequest));
	vTaskDelay(SYSTEM_DEALY);
	xTransponderQueue = xQueueCreate(1, sizeof(queueRequest));
	vTaskDelay(SYSTEM_DEALY);
	//2. check if the queues and the semaphore successfully created
	if (xDumpQueue == NULL || xTransponderQueue == NULL || xIsTransmitting == NULL)
	{
		//2.1. in case the semaphore and queues are damaged
		printf("abort! abort!!!\n");
	}
}

void init_trxvu(void)
{
	TRXVU_init_hardWare();
	TRXVU_init_softWare();
}

void TRXVU_task()
{
	//3. create beacon task
	portBASE_TYPE lu_error = xTaskCreate(Beacon_task, (const signed char * const)"Beacon_Task", BEACON_TASK_BUFFER, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), xBeaconTask);
	check_portBASE_TYPE("could not create Beacon Task.", lu_error);
	vTaskDelay(SYSTEM_DEALY);
	//4. checks if theres was a dump before the reset and turned him off
	if (get_system_state(dump_param))
	{
		//4.1. stop allowing transponder
		set_system_state(dump_param, SWITCH_OFF);
	}
	//6. entering infinite loop
	while(1)
	{
		trxvu_logic();
		vTaskDelay(TASK_DELAY);
	}
}


void dump_logic(command_id cmdID, const time_unix start_time, time_unix end_time, uint8_t resulotion, HK_types HK[5])
{
	char fileName[MAX_F_FILE_NAME_SIZE];
	ERR_type err = ERR_SUCCESS;
	int numberOfParameters, parameterSize;
	FileSystemResult FS_result;

	TM_spl packet;
	int length_raw_packet;
	byte raw_packet[MAX_SIZE_TM_PACKET];

	time_unix last_read = 0;
	time_unix last_send = 0;

	sendRequestToStop_transponder();
	vTaskDelay(SYSTEM_DEALY);

	int i_error;
	int numberOfPackets = 0;

	if (CHECK_STARTING_DUMP_ABILITY)
	{
		for (int i = 0; i < NUM_FILES_IN_DUMP; i++)
		{
			if (HK[i] == this_is_not_the_file_you_are_looking_for)
				continue;

			HK_find_fileName(HK[i], fileName);
			parameterSize = (HK_findElementSize(HK[i]) + TIME_SIZE);
			last_read = start_time;
			last_send = 0;
			do
			{
				numberOfParameters = 0;
				FS_result = c_fileRead(fileName, Dump_buffer, DUMP_BUFFER_SIZE, last_read, end_time,
						&numberOfParameters, &last_read);
				last_read++;

				if (FS_result != FS_SUCCSESS && FS_result != FS_BUFFER_OVERFLOW)
					break;
				else if (FS_result == FS_BUFFER_OVERFLOW)
					printf("overflow from reading data!!!!!\n");

				for (int l = 0; l < numberOfParameters; l++)
				{
					build_HK_spl_packet(HK[i], Dump_buffer + l * parameterSize, &packet);
					encode_TMpacket(raw_packet, &length_raw_packet, packet);

					if (last_send + (time_unix)resulotion <= packet.time || HK[i] == ACK_T || HK[i] == log_files_T)
					{
						last_send = packet.time;
						i_error = TRX_sendFrame(raw_packet, (uint8_t)length_raw_packet);
						check_int("TRX_sendFrame, dump_logic", i_error);
						printf("number of packets: %d\n", numberOfPackets++);

						if (i_error == 4)
						{
							err = ERR_SYSTEM_OFF;
							break;
						}
						else if (i_error == 5)
						{
							err = ERR_FAIL;
							break;
						}

						vTaskDelay(SYSTEM_DEALY);
					}

					lookForRequestToDelete_dump(cmdID);
				}
				vTaskDelay(1000);
			}
			while (FS_result == FS_BUFFER_OVERFLOW);
		}
	}
	else
	{
		err = ERR_FAIL;
	}

	save_ACK(ACK_DUMP, err, cmdID);
}

void Dump_task(void *arg)
{
	int f_error = f_managed_enterFS();
	check_int("error in Dump_task, f_managed_enterFS - data abort exeption\n", f_error);
	byte* dump_param_data = (byte*)arg;
	time_unix startTime;
	time_unix endTime;
	command_id id;
	uint8_t resulotion;
	HK_types HK_dump_type[5];

	id = BigEnE_raw_to_uInt(&dump_param_data[0]);
	for (int i = 0; i < 5; i++)
		HK_dump_type[i] = (HK_types)dump_param_data[4 + i];
	resulotion = dump_param_data[9];
	startTime = BigEnE_raw_to_uInt(&dump_param_data[10]);
	endTime = BigEnE_raw_to_uInt(&dump_param_data[14]);

	if (get_system_state(dump_param))
	{
		//	exit dump task and saves ACK
		save_ACK(ACK_DUMP, ERR_TASK_EXISTS, id);
		terminateTask();
	}
	else
	{
		set_system_state(dump_param, SWITCH_ON);
	}

	// 2. check if parameters are legal
	if (startTime > endTime)
	{
		save_ACK(ACK_DUMP, ERR_PARAMETERS, id);
		set_system_state(dump_param, SWITCH_OFF);
	}
	else
	{
		vTaskDelay(SYSTEM_DEALY);
		xQueueReset(xDumpQueue);
		dump_logic(id, startTime, endTime, resulotion, HK_dump_type);
	}

	set_system_state(dump_param, SWITCH_OFF);
	terminateTask();
}


void transponder_logic(time_unix time, command_id cmdID)
{
	time_unix time_now;
	int i_error;
	voltage_t low_shut_vol = DEFULT_COMM_VOL;
	for (;;)
	{
		//checks if time to return to nominal mode
		i_error = Time_getUnixEpoch(&time_now);
		check_int("error from Transponder Task, get_time", i_error);
		if (time < time_now)
			break;

		lookForRequestToDelete_transponder(cmdID);

		// 7. check if we are under right voltage
		i_error = FRAM_read((byte*)&low_shut_vol, TRANS_LOW_BATTERY_STATE_ADDR, 2);
		check_int("Transponder Task ,FRAM_read(TRANS_LOW_BATTERY_STATE_ADDR)", i_error);
		if (low_shut_vol > get_Vbatt())
			break;

		vTaskDelay(TASK_DELAY);
	}
}

void Transponder_task(void *arg)
{
	portBASE_TYPE lu_error = 0;
	int i_error = 0;

	byte *raw = arg;
	command_id cmdId = BigEnE_raw_to_uInt(raw);
	time_unix time = (time_unix)(raw[CMD_ID_SIZE] * 60);
	time_unix time_now;

	if (get_system_state(transponder_active_param))
	{
		i_error = f_managed_enterFS();
		check_int("f_managed_enterFS in Transponder task", i_error);
		save_ACK(ACK_TRANSPONDER, ERR_TASK_EXISTS, cmdId);
		terminateTask();
	}
	else
	{
		set_system_state(transponder_active_param, SWITCH_ON);
	}
	vTaskDelay(SYSTEM_DEALY);

	if (time > 0)
	{
		// not default time, time sent from ground
		i_error = Time_getUnixEpoch(&time_now);
		check_int("error from Transponder Task, get_time", i_error);
		time += time_now;
	}
	else
	{
		// default time
		i_error = Time_getUnixEpoch(&time_now);
		check_int("error from Transponder Task, get_time", i_error);
		time = time_now + DEFAULT_TIME_TRANSMITTER;
	}

	if (xSemaphoreTake_extended(xIsTransmitting, MAX_DELAY) == pdTRUE)
	{
		if (!get_system_state(mute_param) && get_system_state(Tx_param))
		{
			change_TRXVU_state(TRANSPONDER_MODE);
			xQueueReset(xTransponderQueue);
			transponder_logic(time, cmdId);
			i_error = f_managed_enterFS();
			check_int("f_managed_enterFS in Transponder task", i_error);
			save_ACK(ACK_TRANSPONDER, ERR_SUCCESS, cmdId);
			terminateTask();
		}
		else
		{
			i_error = f_managed_enterFS();
			check_int("f_managed_enterFS in Transponder task", i_error);
			save_ACK(ACK_TRANSPONDER, ERR_FAIL, cmdId);
			f_managed_releaseFS();
		}
	}

	change_TRXVU_state(NOMINAL_MODE);
	lu_error = xSemaphoreGive_extended(xIsTransmitting);
	check_portBASE_TYPE("error in transponder task, semaphore xIsTransmitting", lu_error);
	vTaskDelete(NULL);
}


void lookForRequestToDelete_transponder(command_id cmdID)
{
	portBASE_TYPE queueError;
	queueRequest queueParameter;
	queueError = xQueueReceive(xTransponderQueue, &queueParameter, SYSTEM_DEALY);
	if (queueError == pdTRUE)
	{
		if (queueParameter == deleteTask)
		{
			int i_error = f_managed_enterFS();
			check_int("f_managed_enterFS in Transponder task", i_error);
			save_ACK(ACK_TRANSPONDER, ERR_STOP_TASK, cmdID);
			change_TRXVU_state(NOMINAL_MODE);
			portBASE_TYPE lu_error = xSemaphoreGive_extended(xIsTransmitting);
			check_portBASE_TYPE("error in transponder task, semaphore xIsTransmitting", lu_error);
			terminateTask();
		}
	}
}

void lookForRequestToDelete_dump(command_id cmdID)
{
	portBASE_TYPE queueError;
	queueRequest queueParameter;
	queueError = xQueueReceive(xDumpQueue, &queueParameter, SYSTEM_DEALY);
	if (queueError == pdTRUE)
	{
		if (queueParameter == deleteTask)
		{
			save_ACK(ACK_DUMP, ERR_STOP_TASK, cmdID);
			set_system_state(dump_param, SWITCH_OFF);
			terminateTask();
		}
	}
}

int sendRequestToStop_dump()
{
	//1. check if the task is running
	if (!get_system_state(dump_param))
	{
		return 1;
	}
	//2. if the task is running, sends throw a queue a request to delete task
	queueRequest request = deleteTask;
	portBASE_TYPE error = xQueueSend(xDumpQueue, &request, QUEUE_DELAY);
	check_portBASE_TYPE("stop dump, xQueueSend", error);
	//3. if queue is full
	if (error == errQUEUE_FULL)
	{
		return 2;
	}
	return 0;
}

int sendRequestToStop_transponder()
{
	//1. check if the task is running
	if (!get_system_state(transponder_active_param))
	{
		return 1;
	}
	//2. if the task is running, sends throw a queue a request to delete task
	queueRequest request = deleteTask;
	portBASE_TYPE error = xQueueSend(xTransponderQueue, &request, QUEUE_DELAY);
	check_portBASE_TYPE("stop_transponder, xQueueSend", error);
	//3. if queue is full
	if (error == errQUEUE_FULL)
	{
		return 2;
	}
	return 0;
}


void Rx_logic()
{
	int i_error;
	byte dataBuffer[SIZE_RXFRAME];
	unsigned int dataBuffer_length;
	TC_spl packet;
	unsigned short RxCounter = 0;
	time_unix time_now;

	i_error = IsisTrxvu_rcGetFrameCount(I2C_BUS_ADDR, &RxCounter);
	check_int("IsisTrxvu_rcGetFrameCount, trxvu_logic", i_error);
	if (RxCounter > 0)
	{
		//	1.1. gets data from Rx buffer
		i_error = TRX_getFrameData(&dataBuffer_length, dataBuffer);
		if (dataBuffer != NULL && i_error == 0)
		{
			// 1.2. decode data to spl packet
			i_error = decode_TCpacket(dataBuffer, dataBuffer_length ,&packet);
			if (i_error == 0)
			{
				set_ground_conn(TRUE);
				// 1.3. sends receive ACK
				byte rawACK[ACK_RAW_SIZE];
				build_raw_ACK(ACK_RECEIVE_COMM, ERR_SUCCESS, packet.id, rawACK);
				printf("Send ACK\n");
				TRX_sendFrame(rawACK, ACK_RAW_SIZE);

				i_error = Time_getUnixEpoch(&time_now);
				check_int("trxvu_logic, Time_getUnixEpoch", i_error);
				// 1.4. checks if command is delayed command
				if (packet.time <= time_now)
				{
					//execute command
					add_command(packet);
				}
				else
				{
					add_delayCommand(packet);
				}
			}
#ifdef TESTING
			else
			{
				printf("Earth junk or space.\n");
			}
#endif
		}

	}
}

void pass_above_Ground()
{
	static time_unix started_time = 0;
	static Boolean groundConnectionStarted = FALSE;
	//the passing above ground started
	if (get_ground_conn() && !groundConnectionStarted)
	{
		groundConnectionStarted = TRUE;
		int i_error = Time_getUnixEpoch(&started_time);
		check_int("connection_toGround, Time_getUnixEpoch", i_error);

		return;
	}
	//the passing above ground ended
	if (get_ground_conn() && groundConnectionStarted)
	{
		time_unix currentTime;
		int i_error = Time_getUnixEpoch(&currentTime);
		check_int("connection_toGround, Time_getUnixEpoch", i_error);
		if (currentTime > started_time + GROUND_PASSING_TIME)
		{
			groundConnectionStarted = FALSE;
			set_ground_conn(FALSE);
		}

		return;
	}
	//an error have been made
	if (!get_ground_conn() && groundConnectionStarted)
	{
		groundConnectionStarted = FALSE;
		return;
	}
}

void trxvu_logic()
{
	Rx_logic();

	check_time_off_mute();

	check_delaycommand();

	pass_above_Ground();
}


void Beacon_task()
{
	int i_error;
	// 0. Creating variables for task, initialize variables
	uint8_t delayBaecon = DEFULT_BEACON_DELAY;
	portTickType last_time = xTaskGetTickCount();
	voltage_t low_v_beacon;
	while(1)
	{
		// 1. check if Tx on, transponder off mute Tx off, dunp is off
		if (CHECK_SENDING_BEACON_ABILITY)
			buildAndSend_beacon();

		i_error = FRAM_read(&delayBaecon, BEACON_TIME_ADDR, 1);
		check_int("beacon_task, FRAM_write(BEACON_TIME_ADDR)", i_error);
		// 6. check if value in range
		if (!(10 <= delayBaecon || delayBaecon <= 40))
		{
			delayBaecon = DEFULT_BEACON_DELAY;
		}

		//reading low_v_vbat from FRAM
		i_error = FRAM_read((byte*)&low_v_beacon, BEACON_LOW_BATTERY_STATE_ADDR, 2);
		check_int("beacon_task, FRAM_write(BEACON_LOW_BATTERY_STATE_ADDR)", i_error);

#ifdef TESTING
		if (7400 < low_v_beacon || low_v_beacon < 7200)
		{
			low_v_beacon = 7250;
		}
#endif
		// 7. check if low vBatt
		portTickType delay = CONVERT_SECONDS_TO_MS(delayBaecon);//normal
		if (low_v_beacon > get_Vbatt())
		{
			// low
			delay = GET_BEACON_DELAY_LOW_VOLTAGE(delay);
		}

		vTaskDelayUntil(&last_time, delay);
	}
}

void buildAndSend_beacon()
{
	// 1. Declaring variables
	TM_spl beacon;
	//2. Building basic structure of a beacon packet
	beacon.type = BEACON_T;
	beacon.subType = BEACON_ST;
	beacon.length = BEACON_LENGTH;
	Time_getUnixEpoch(&beacon.time);
	//3. getting the last update of the global parameters for the beacon
	global_param beacon_param;
	get_current_global_param(&beacon_param);
	//4. set raw parameters
	// 4.1. voltages [mV] and currents [mA] EPS part
	beacon.data[0] = beacon_param.Vbatt >> 8;
	beacon.data[1] = beacon_param.Vbatt;
	beacon.data[2] = beacon_param.curBat >> 8;
	beacon.data[3] = beacon_param.curBat;
	beacon.data[4] = beacon_param.cur3V3 >> 8;
	beacon.data[5] = beacon_param.cur3V3;
	beacon.data[6] = beacon_param.cur5V >> 8;
	beacon.data[7] = beacon_param.cur5V;
	// 4.2. temperatures [degC]
	int i,l;
	byte *raw_param = (byte*)&beacon_param.tempComm_LO;
	for(i = 0; i < 2; i++)
	{
		beacon.data[8 + i] = raw_param[1 - i];
	}
	raw_param = (byte*)&beacon_param.tempComm_PA;
	for(i = 0; i < 2; i++)
	{
		beacon.data[10 + i] = raw_param[1 - i];
	}
	for (l = 0 ; l < 4; l++)
	{
		raw_param = (byte*)&beacon_param.tempEPS[l];
		for(i = 0; i < 2; i++)
		{
			beacon.data[12 + l * 2 + i] = raw_param[1 - i];
		}
	}
	for (l = 0; l < 2; l++)
	{
		raw_param = (byte*)&beacon_param.tempBatt[l];
		for(i = 0; i < 2; i++)
		{
			beacon.data[20 + l * 2 + i] = raw_param[1 - i];
		}
	}
	// 4.3. Rx/Tx parameters
	beacon.data[24] = beacon_param.RxDoppler << 8;
	beacon.data[25] = beacon_param.RxDoppler;
	beacon.data[26] = beacon_param.RxRSSI << 8;
	beacon.data[27] = beacon_param.RxRSSI;
	beacon.data[28] = beacon_param.TxRefl << 8;
	beacon.data[29] = beacon_param.TxRefl;
	beacon.data[30] = beacon_param.TxForw << 8;
	beacon.data[31] = beacon_param.TxForw;
	// 4.4. ADCS
	byte raw_stageTable[STAGE_TABLE_SIZE];
	getTableTo(get_ST(), raw_stageTable);
	beacon.data[32] = raw_stageTable[2];
	beacon.data[33] = raw_stageTable[1];
	beacon.data[34] = raw_stageTable[0];
	beacon.data[35] = raw_stageTable[3];
	beacon.data[36] = raw_stageTable[4];
	beacon.data[37] = raw_stageTable[5];
	beacon.data[38] = raw_stageTable[8];
	beacon.data[39] = raw_stageTable[7];
	beacon.data[40] = raw_stageTable[6];
	for (i = 0; i < 3; i++)
	{
		raw_param = (byte*)&(beacon_param.Attitude[i]);
		for (l = 0; l < 2; l++)
		{
			beacon.data[41 + i * 2 + l] = raw_param[l];
		}
	}
	// 4.5. stats
	beacon.data[47] = beacon_param.numOfPics;
	beacon.data[48] = beacon_param.numOfAPRS;
	beacon.data[49] = beacon_param.numOfDelayedCommand;
	raw_param = (byte*)&beacon_param.numOfResets;
	for(i = 0; i < 4; i++)
	{
		beacon.data[50 + i] = raw_param[3 - i];
	}
	raw_param = (byte*)&beacon_param.lastReset;
	for(i = 0; i < 4; i++)
	{
		beacon.data[54 + i] = raw_param[3 - i];
	}
	// 4.6. states
	beacon.data[58] = beacon_param.state.raw;
	// 5. Encoding beacon for sending
	byte rawData[BEACON_LENGTH + SPL_TM_HEADER_SIZE];
	int size = 0;
	encode_TMpacket(rawData, &size, beacon);
	//11. sending beacon
	for (i = 0; i < 1; i++)
	{
		TRX_sendFrame(rawData, size);
	}
}


void reset_FRAM_TRXVU()
{
	int i_error = 0;

	byte data[2];

	//APRS list
	reset_APRS_list(TRUE);

	//Delay command list
	reset_delayCommand(TRUE);

	//BEACON_LOW_BATTERY_STATE_ADDR reset
	voltage_t voltage = DEFULT_COMM_VOL;
	i_error = FRAM_write((byte*)&voltage, BEACON_LOW_BATTERY_STATE_ADDR, sizeof(voltage_t));
	check_int("reset_FRAM_TRXVU, FRAM_write", i_error);

	i_error = FRAM_write((byte*)&voltage, TRANS_LOW_BATTERY_STATE_ADDR, sizeof(voltage_t));
	check_int("reset_FRAM_TRXVU, FRAM_write", i_error);

	data[0] = DEFULT_BEACON_DELAY;
	i_error = FRAM_write(data, BEACON_TIME_ADDR, 1);
	check_int("reset_FRAM_TRXVU,, FRAM_write(BEACON_TIME_ADDR)", i_error);

	data[0] = DEFAULT_BIT_RATE;
	i_error = FRAM_write(data, BIT_RATE_ADDR, 1);
	check_int("reset_FRAM_TRXVU,, FRAM_write(BIT_RATE_ADDR)", i_error);

	time_unix mute_time = 0;
	i_error = FRAM_write((byte*)&mute_time, MUTE_TIME_ADDR, TIME_SIZE);
	check_int("reset_FRAM_TRXVU, FRAM_read(MUTE_TIME_ADDR)", i_error);

	unsigned short trans_rssi = DEFAULT_TRANS_RSSI;
	i_error = FRAM_write((byte*)&trans_rssi, TRANSPONDER_RSSI_ADDR, sizeof(unsigned short));
	check_int("TRANSPONDER_RSSI_ADDR, FRAM_write", i_error);
}


int TRX_sendFrame(byte* data, uint8_t length)
{
	int i_error = 0, retVal = 0;
	portBASE_TYPE lu_error = 0;
	if (get_system_state(mute_param) == SWITCH_ON)
	{
		//mute
		return 3;
	}
	if (get_system_state(Tx_param) == SWITCH_OFF)
	{
		//tx off
		return 4;
	}
	if (get_system_state(transponder_active_param) == SWITCH_ON)
	{
		//we in transponder mode
		return 5;
	}

	if (xSemaphoreTake_extended(xIsTransmitting, MAX_DELAY))
	{
		unsigned char avalFrames = VALUE_TX_BUFFER_FULL;

		int count = 0;
		do
		{
			i_error = IsisTrxvu_tcSendAX25DefClSign(0, data, length, &avalFrames);
			check_int("TRX_sendFrame, IsisTrxvu_tcSendAX25DefClSign", i_error);
			retVal = 0;
			if (count > 0)
				vTaskDelay(200);
			if (count % 10 == 1)
			{
				printf("Tx buffer is full\n");
				retVal = -1;
			}
			count++;
		}while(avalFrames == VALUE_TX_BUFFER_FULL);

		// returns xIsTransmitting semaphore
		lu_error = xSemaphoreGive_extended(xIsTransmitting);
		check_portBASE_TYPE("TRX_sendFrame, xSemaphoreGive_extended", lu_error);
		retVal = 0;
	}
	else
	{
		// can't take semaphore, big problem...
		retVal = -2;
	}

	return retVal;
}

int TRX_getFrameData(unsigned int *length, byte* data_out)
{
	int i_error = 0;
	if (data_out == NULL)
	{
		return -2;
	}

	byte receive_frm[SIZE_RXFRAME]; //raw data from the Rx buffer
	unsigned short RxCounter = 0;
	i_error = IsisTrxvu_rcGetFrameCount(I2C_BUS_ADDR, &RxCounter);
	check_int("TRX_getFrameData, IsisTrxvu_rcGetFrameCount", i_error);
	ISIStrxvuRxFrame rxFrameCmd = { 0, 0, 0, receive_frm }; // for getting raw data from Rx, nullify values

	if (!(RxCounter > 0))
	{
		return -1;
	}

	i_error = IsisTrxvu_rcGetCommandFrame(I2C_BUS_ADDR, &rxFrameCmd);
	check_int("TRX_getFrameData, IsisTrxvu_rcGetCommandFrame", i_error);

	*length = (int)rxFrameCmd.rx_length;

	memcpy(data_out, receive_frm, rxFrameCmd.rx_length);

#ifdef APRS_ON
	if (rxFrameCmd.rx_length != 18)
		return 0;
	return check_APRS(data_out);
#else
	return 0;
#endif
}

/**
 * @brief	stop Dump, stop Transponder and switching on the mute Tx param
 */
void mute_Tx()
{
	sendRequestToStop_dump();
	sendRequestToStop_transponder();
	set_system_state(mute_param, SWITCH_ON);
}

void unmute_Tx()
{
	time_unix mute_time = 0;
	set_system_state(mute_param, SWITCH_OFF);
	int i_error = FRAM_write((byte*)&mute_time, MUTE_TIME_ADDR, TIME_SIZE);
	check_int("unmute_Tx, FRAM_read(MUTE_TIME_ADDR)", i_error);

	update_FRAM_bitRate();
}

void check_time_off_mute()
{
	time_unix mute_time, time_now;

	int i_error = Time_getUnixEpoch(&time_now);
	check_int("check_time_off_mute, Time_getUnixEpoch", i_error);

	i_error = FRAM_read((byte*)&mute_time, MUTE_TIME_ADDR, TIME_SIZE);
	check_int("check_time_off_mute, FRAM_read(MUTE_TIME_ADDR)", i_error);
	if (time_now >= mute_time && get_system_state(mute_param))
		unmute_Tx();
}

int set_mute_time(unsigned short time)
{
	time_unix time_now;
	int error = Time_getUnixEpoch(&time_now);
	check_int("set_mute_time, Time_getUnixEpoch", error);

	if (time > MAX_NUTE_TIME)
	{
		return 666;
	}

	time_now += (time_unix)(time * 60);

	error = FRAM_write((byte*)&time_now, MUTE_TIME_ADDR, TIME_SIZE);
	check_int("set_mute_time, FRAM_write", error);

	mute_Tx(TRUE);

	return 0;
}



temp_t check_Tx_temp()
{
	ISIStrxvuTxTelemetry_revC telemetry;
	int rv;

	// Telemetry values are presented as raw values
	printf("\r\nGet all Telemetry at once in raw values \r\n\r\n");
	rv = IsisTrxvu_tcGetTelemetryAll_revC(0, &telemetry);
	if(rv)
	{
		printf("Subsystem call failed. rv = %d", rv);
	}

	return ((float)telemetry.fields.locosc_temp) * -0.07669 + 195.6037;
}

//I2C functions
void change_TRXVU_state(Boolean state)
{
	byte rssiData[2];
	byte data[2];
	int i_error;
	//command id
	data[0] = 0x38;
	if (state == NOMINAL_MODE)
	{
		i_error = FRAM_read(rssiData, TRANSPONDER_RSSI_ADDR, 2);
		check_int("change_TRXVU_state, FRAM_read", i_error);
		change_trans_RSSI(rssiData);
		//nominal mode
		printf("\tTransponder is disabled\n\n");
		data[1] = 0x01;
		set_system_state(transponder_active_param, SWITCH_OFF);
		vTaskDelay(10000);
		update_FRAM_bitRate();
	}
	else
	{
		//transponder mode
		printf("\tTransponder enabled\n\n");
		data[1] = 0x02;
		set_system_state(transponder_active_param, SWITCH_ON);
	}
	//sends I2C command
	i_error = I2C_write(I2C_TRXVU_TC_ADDR, data, 2);
	check_int("change_TRXVU_state, I2C_write", i_error);
}

void change_trans_RSSI(byte *param)
{
	byte data[3];
	data[0] = 0x52;
	data[1] = param[0];
	data[2] = param[1];

	int i_error = I2C_write(I2C_TRXVU_TC_ADDR, data, 2);
	check_int("change_TRXVU_state, I2C_write", i_error);
}
