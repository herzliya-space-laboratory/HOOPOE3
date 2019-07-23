/*
/ * TRXVU.c
 *
 *  Created on: Oct 20, 2018
 *      Author: elain
 */
#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
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
#include "../Global/TM_managment.h"
#include "splTypes.h"
#include "../Global/GlobalParam.h"
#include "../ADCS/Stage_Table.h"
#include  "../payload/DataBase.h"
#include "DelayedCommand_list.h"

#define FIRST 0

#define CHECK_SENDING_BEACON_ABILITY	(get_system_state(Tx_param) && !get_system_state(transponder_active_param) && !get_system_state(mute_param) && !get_system_state(dump_param))

xSemaphoreHandle xIsTransmitting;

time_unix allow_transponder;

xTaskHandle xBeaconTask;

static byte APRS_list[APRS_SIZE_WITH_TIME * MAX_NAMBER_OF_APRS_PACKETS];
static byte Dump_buffer[DUMP_BUFFER_SIZE];

void TRXVU_task()
{
	portBASE_TYPE lu_error = 0;
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
		//todo: return error
		return;
	}
	//3. create beacon task
	lu_error = xTaskCreate(Beacon_task, (const signed char * const)"Beacon_Task", BEACON_TASK_BUFFER, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), xBeaconTask);
	check_portBASE_TYPE("could not create Beacon Task.", lu_error);
	vTaskDelay(SYSTEM_DEALY);
	//4. checks if theres was a dump before the reset and turned him off
	if (get_system_state(dump_param))
	{
		//4.1. stop allowing transponder
		set_system_state(dump_param, SWITCH_OFF);
	}
	//5. gets FRAM lists to ram
	get_APRS_list();
	get_delayCommand_list();
	//6. entering infinite loop
	while(1)
	{
		trxvu_logic();
		vTaskDelay(TASK_DELAY);
	}
}

void dump_logic(command_id cmdID, time_unix start_time, time_unix end_time, HK_types HK[5])
{
	char fileName[MAX_F_FILE_NAME_SIZE];
	ERR_type err = ERR_SUCCESS;
	int numberOfParameters, parameterSize;
	FileSystemResult result;
	Boolean continueDump = TRUE;

	// 1. exit transponder mode to nominal mode
	stop_transponder();
	vTaskDelay(SYSTEM_DEALY);

	// 2. send packets, check if the satellite in mute or the the transponder active
	if ((!get_system_state(mute_param)) && (!get_system_state(transponder_active_param)) && (get_system_state(Tx_param)))
	{
		TM_spl packet;
		byte raw_packet[SIZE_TXFRAME];
		int length_raw;
		// 2.1. each dump file
		for (int l = 0; l < 5; l++)
		{
			if (HK[l] != this_is_not_the_file_you_are_looking_for && continueDump)
			{
				// 2.2 gets elements of HK file
				parameterSize = size_of_element(HK[l]);
				find_fileName(HK[l], fileName);
				result = fileRead(fileName, Dump_buffer, DUMP_BUFFER_SIZE, start_time, end_time, &numberOfParameters, parameterSize);
				//result = c_fileRead(fileName, Dump_buffer, DUMP_BUFFER_SIZE, startTime, endTime, &numberOfParameters);
				if (result != FS_SUCCSESS)
				{
					err = ERR_FAIL;
				}
				else
				{
					// 2.3 starts dump
					for(int i = 0; i < numberOfParameters; i++)
					{
						build_HK_spl_packet(HK[l], &Dump_buffer[i * (parameterSize + TIME_SIZE)], &packet);
						encode_TMpacket(raw_packet, &length_raw, packet);
						// 2.4. send data
#ifdef TESTING
						for (int j = 0; j < 1; j++)
						{
							vTaskDelay(SYSTEM_DEALY);
							TRX_sendFrame(raw_packet, length_raw, trxvu_bitrate_9600);
							printf("send current packet: %d\n", i);
							if (i % 200 == 0)
							{
								fflush(0);
								vutc_getTxTelemTest();
								if (check_Tx_temp() > 40)
								{
									continueDump = FALSE;
									break;
								}
							}
						}
#else
						TRX_sendFrame(raw_packet, length_raw, trxvu_bitrate_9600);
						if (i % 200 == 0)
						{
							fflush(0);
							if (check_Tx_temp() > 40)
							{
								continueDump = FALSE;
								break;
							}
						}
#endif
						// 2.5. check in xDumpQueue if there was a request to stop dump
						if (check_dump_delete())
						{
							continueDump = TRUE;
							err = ERR_TURNED_OFF;
							break;
						}
					}
				}
			}
		}
	}
	// 3. after all dumps saves ACK
	save_ACK(ACK_DUMP, err, cmdID);
}

void Dump_task(void *arg) //DUMp
{
	byte* dump_param_data = (byte*)arg;
	time_unix startTime;
	time_unix endTime;
	command_id id;

	id = BigEnE_raw_to_uInt(&dump_param_data[0]);

	HK_types HK_dump_type[5];

	for (int i = 0; i < 5; i++)
	{
		HK_dump_type[i] = (HK_types)dump_param_data[4 + i];
	}

	startTime = BigEnE_raw_to_uInt(&dump_param_data[9]);
	endTime = BigEnE_raw_to_uInt(&dump_param_data[13]);
	// 1. dump already exist
	if (get_system_state(dump_param))
	{
		//	exit dump task and saves ACK
		save_ACK(ACK_DUMP, ERR_TASK_EXISTS, id);
		vTaskDelete(NULL);
	}
	else
	{
		set_system_state(dump_param, SWITCH_ON);
	}

	// 2. check parameters
	if (startTime > endTime)
	{
		save_ACK(ACK_DUMP, ERR_PARAMETERS, id);
		set_system_state(dump_param, SWITCH_OFF);
	}
	else
	{
		int ret = f_enterFS(); /* Register this task with filesystem */
		check_int("Dump task enter FileSystem", ret);
		vTaskDelay(SYSTEM_DEALY);
		dump_logic(id, startTime, endTime, HK_dump_type);
	}
	// 9. deleting task from scheduler
	set_system_state(dump_param, SWITCH_OFF);
	vTaskDelete(NULL);
	vTaskDelay(TASK_DELAY);
}

void Dump_image_task(void *arg)
{
	//Variables
	int chunkIndex[2];
	int image_id;
	fileType compress;
	command_id idCMD;
	char fileName[MAX_F_FILE_NAME_SIZE];

	// 1. gets variables from arg
	int *var;
	var = (int*)arg;
	idCMD = *var;

	chunkIndex[0] = (int)BigEnE_raw_to_uInt(arg + 4);
	chunkIndex[1] = (int)BigEnE_raw_to_uInt(arg + 8);
	// image bin image
	image_id = BigEnE_raw_to_uInt(arg + 12);
	// compress type
	var = (int*)(arg + 16);
	compress = (char)*var;
	strcpy(fileName, arg + 17);

	vTaskDelay(SYSTEM_DEALY);
	// 2. check if dump already exist
	if (get_system_state(dump_param))
	{
		save_ACK(ACK_IMAGE_DUMP, ERR_TASK_EXISTS, idCMD);
		vTaskDelete(NULL);
	}
	else
	{
		set_system_state(dump_param, SWITCH_ON);
	}
	//3. check which chunk is smaller
	int start_chunk;
	int end_chunk;
	if (chunkIndex[0] > chunkIndex[1])
	{
		end_chunk = chunkIndex[0];
		start_chunk = chunkIndex[1];
	}
	// 4. get image to a buffer from SD
	// todo: get_image_from_SD(dump_buffer, image_id);

	chunk_t chunkToSend;
	TM_spl packet;
	int error;
	if (!get_system_state(mute_param) || !get_system_state(transponder_active_param) || !get_system_state(Tx_param))
	{
		F_FILE* file = f_open(fileName, "r");
		int file_length = f_filelength(file);
		f_read(Dump_buffer, file_length, 1, file);
		chunk_t toSend[CHUNK_SIZE];
		for (unsigned int i = (unsigned int)start_chunk; i < (unsigned int)end_chunk; i++)
		{
			if (compress == jpg)
			{

			}
			else
			{
				// 5. get slice of the image
				GetChunkFromImage(chunkToSend, i, Dump_buffer, compress);
				// 6. build SPL packet to send
				packet = spl_image(i, compress, image_id, chunkToSend);
				send_TM_spl(packet, trxvu_bitrate_9600);
				// 7. check if dump need to be deleted
				vTaskDelay(SYSTEM_DEALY);
				if (check_dump_delete())
					break;
			}
		}
		f_close(file);
	}
	// 8. saves ACK
	save_ACK(ACK_IMAGE_DUMP, ERR_SUCCESS, idCMD);
	// 9 .delete task
	set_system_state(dump_param, SWITCH_OFF);
	vTaskDelete(NULL);
}

Boolean check_dump_delete()
{
	portBASE_TYPE queueError;
	queueRequest queueParameter;
	queueError = xQueueReceive(xDumpQueue, &queueParameter, SYSTEM_DEALY);
	if (queueError)
	{
		if (queueParameter == deleteTask)
			return TRUE;
	}

	return FALSE;
}

void transponder_logic(time_unix time)
{
	time_unix time_now;
	int i_error;
	queueRequest queueError;
	Boolean queueParameter;
	voltage_t low_shut_vol = DEFULT_COMM_VOL;
	for (;;)
	{
		// 5. checks if time to return to nominal mode
		i_error = Time_getUnixEpoch(&time_now);
		check_int("error from Transponder Task, get_time", i_error);
		if (time < time_now)
			break;

		// 6. check in xDumpQueue if there was a request to stop dump
		queueError = xQueueReceive(xTransponderQueue, &queueParameter, QUEUE_DELAY);
		if (queueError == deleteTask)
			break;

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

	command_id cmdId;

	byte *raw = arg;
	//	1. extracting data to variables
	cmdId = BigEnE_raw_to_uInt(raw);
	time_unix time = (time_unix)(raw[CMD_ID_SIZE] * 60);
	time_unix time_now;

	if (get_system_state(transponder_active_param))
	{
		save_ACK(ACK_TRANSPONDER, ERR_TASK_EXISTS, cmdId);
		vTaskDelete(NULL);
	}
	else
	{
		set_system_state(transponder_active_param, SWITCH_ON);
	}
	vTaskDelay(SYSTEM_DEALY);

	// 2. check if the time the transponder will be on is set to default
	if (time > 0)
	{
		// 2.1. not default time, time sent from ground
		i_error = Time_getUnixEpoch(&time_now);
		check_int("error from Transponder Task, get_time", i_error);
		time += time_now;
	}
	else
	{
		// 2.2. default time
		i_error = Time_getUnixEpoch(&time_now);
		check_int("error from Transponder Task, get_time", i_error);
		time = time_now + DEFAULT_TIME_TRANSMITTER;
	}

	if (xSemaphoreTake(xIsTransmitting, MAX_DELAY) == pdTRUE)
	{
		if (!get_system_state(mute_param) && get_system_state(Tx_param))
		{
			// 3. build ACK of transponder mode
			save_ACK(ACK_TRANSPONDER, ERR_ACTIVE, cmdId);
			// 4. changing the TRXVU mode to transponder mode
			change_TRXVU_state(TRANSPONDER_MODE);
			// 5. enter loop of transponder
			transponder_logic(time);
		}
	}
	// 6. delete task
	change_TRXVU_state(NOMINAL_MODE);
	lu_error = xSemaphoreGive(xIsTransmitting);
	check_portBASE_TYPE("error in transponder task, semaphore xIsTransmitting", lu_error);
	vTaskDelete(NULL);
}

void Beacon_task(void *arg)
{
	int i_error;
	// 0. Creating variables for task, initialize variables
	uint8_t delayBaecon = DEFULT_BEACON_DELAY;
	portTickType last_time = xTaskGetTickCount();
	int beacon_count = 0;
	ISIStrxvuBitrate bitrate = trxvu_bitrate_9600;
	voltage_t low_v_beacon;
	while(1)
	{
		// 1. check if Tx on, transponder off mute Tx off, dunp is off
		if (CHECK_SENDING_BEACON_ABILITY)
		{
			// 2. check if this is the third beacon in a row
			if (beacon_count % 3 == 0)
				bitrate = trxvu_bitrate_1200;
			else
				bitrate = trxvu_bitrate_9600;

			// 3. build and send beacon
			Beacon(bitrate);
			// 4. adding last beacon to count
			beacon_count++;
		}
		// 5. reading from FRAM the low vBatt in wich the beacon is once in a minute
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

int stop_dump()
{
	//1. check if the task is running
	if (eTaskGetState(xDumpHandle) == eDeleted)
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

int stop_transponder()
{
	//1. check if the task is running
	if (eTaskGetState(xTransponderHandle) == eDeleted)
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

void trxvu_logic()
{
	// 1. check if theres data from GS and what to do with it
	data_from_ground_logic();
	// 2. check if mute is on and its time to stop it
	check_time_off_mute();
	// 3. check if time execute delayed command
	check_delaycommand();
}

void data_from_ground_logic()
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
				// 1.3. sends receive ACK
				byte rawACK[ACK_RAW_SIZE];
				build_raw_ACK(ACK_RECEIVE_COMM, ERR_SUCCESS, packet.id, rawACK);
				printf("Send ACK\n");
				TRX_sendFrame(rawACK, ACK_RAW_SIZE, trxvu_bitrate_9600);

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
			else
			{
				printf("Earth junk, not command\n");
			}
		}

	}
}

void Beacon(ISIStrxvuBitrate bitRate)
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
		TRX_sendFrame(rawData, size, bitRate);
	}
}

void init_trxvu(void)
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


	retValInt = IsisTrxvu_tcSetIdlestate(0, trxvu_idle_state_on);
	check_int("init_trxvu, IsisTrxvu_tcSetIdlestate, on", retValInt);
	vTaskDelay(1000);
	retValInt = IsisTrxvu_tcSetIdlestate(0, trxvu_idle_state_off);
	check_int("init_trxvu, IsisTrxvu_tcSetIdlestate, off", retValInt);
	vTaskDelay(1000);

	change_TRXVU_state(NOMINAL_MODE);

	vTaskDelay(1000);
	retValInt = IsisTrxvu_tcSetIdlestate(0, trxvu_idle_state_on);
	check_int("init_trxvu, IsisTrxvu_tcSetIdlestate, on", retValInt);
	vTaskDelay(1000);
	retValInt = IsisTrxvu_tcSetIdlestate(0, trxvu_idle_state_off);
	check_int("init_trxvu, IsisTrxvu_tcSetIdlestate, off", retValInt);

#ifndef TESTING
	retValInt = IsisTrxvu_tcSetDefFromClSign(0, TRXVU_FROM_CALSIGN);
	check_int("init_trxvu, frommm call sign", retValInt);

	retValInt = IsisTrxvu_tcSetDefToClSign(0, TRXVU_TO_CALSIGN);
	check_int("init_trxvu, tooo call sign", retValInt);
#endif
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

}

int TRX_sendFrame(byte* data, uint8_t length, ISIStrxvuBitrate bitRate)
{
	int i_error = 0, retVal = 0;
	portBASE_TYPE lu_error = 0;
	if (get_system_state(mute_param) == SWITCH_ON)
	{
		//mute
		return -3;
	}
	if (get_system_state(Tx_param) == SWITCH_OFF)
	{
		//tx off
		return -4;
	}
	if (get_system_state(transponder_active_param) == SWITCH_ON)
	{
		//we in transponder mode
		return -5;
	}

	if (xSemaphoreTake(xIsTransmitting, MAX_DELAY))
	{
		if (bitRate != trxvu_bitrate_9600)
		{
			i_error = IsisTrxvu_tcSetAx25Bitrate(0, bitRate);
			check_int("TRX_sendFrame, IsisTrxvu_tcSetAx25Bitrate", i_error);
		}
		unsigned char avalFrames = 0;

		i_error = IsisTrxvu_tcSendAX25DefClSign(0, data, length, &avalFrames);
		check_int("TRX_sendFrame, IsisTrxvu_tcSendAX25DefClSign", i_error);
		vTaskDelay(SYSTEM_DEALY);

		int count = 0;
		if (avalFrames == 0)
		{
			printf("Tx buffer is full\n");
			retVal = -1;
		}
		while (avalFrames == 0)
		{
			count++;
			i_error = IsisTrxvu_tcSendAX25DefClSign(0, data, length, &avalFrames);
			check_int("TRX_sendFrame, IsisTrxvu_tcSendAX25DefClSign", i_error);
			vTaskDelay(SYSTEM_DEALY);
			if (count > 2000)
			{
				break;
			}
		}



		availableFrames = avalFrames;

		//delay so Tx buffer will never be full
		if (bitRate == trxvu_bitrate_9600 && retVal == 0)
		{
			vTaskDelay(TRANSMMIT_DELAY_9600(length));
		}
		else if (retVal == 0)
		{
			vTaskDelay(TRANSMMIT_DELAY_1200(length));
		}

		//returns bit rate to normal (9600)
		i_error = IsisTrxvu_tcSetAx25Bitrate(0, trxvu_bitrate_9600);
		check_int("TRX_sendFrame, IsisTrxvu_tcSetAx25Bitrate", i_error);

		// returns xIsTransmitting semaphore
		lu_error = xSemaphoreGive(xIsTransmitting);
		check_portBASE_TYPE("TRX_sendFrame, xSemaphoreGive", lu_error);
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

	// 1. check for frames
	byte receive_frm[SIZE_RXFRAME]; //raw data from the Rx buffer
	unsigned short RxCounter = 0;
	i_error = IsisTrxvu_rcGetFrameCount(I2C_BUS_ADDR, &RxCounter);
	check_int("TRX_getFrameData, IsisTrxvu_rcGetFrameCount", i_error);
	ISIStrxvuRxFrame rxFrameCmd = { 0, 0, 0, receive_frm }; // for getting raw data from Rx, nullify values

	// 2. no available frames
	if (!(RxCounter > 0))
	{
		return -1;
	}

	// 3. check rssi
	ISIStrxvuRxTelemetry telemetry;
	i_error = IsisTrxvu_rcGetTelemetryAll(0, &telemetry);
	check_int("TRX_getFrameData, IsisTrxvu_rcGetTelemetryAll", i_error);
	printf("Receiver RSSI = %f dBm\r\n", ((float)telemetry.fields.rx_rssi) * 0.03 - 152);

	// 4. get frame
	i_error = IsisTrxvu_rcGetCommandFrame(I2C_BUS_ADDR, &rxFrameCmd);
	check_int("TRX_getFrameData, IsisTrxvu_rcGetCommandFrame", i_error);

	// 5. setting size of data
	*length = (int)rxFrameCmd.rx_length;

	// 6. copy frame to *buffer
	memcpy(data_out, receive_frm, rxFrameCmd.rx_length);

	// 7. checks if received data is an APRS packet
	return checks_APRS(data_out);
}

int send_TM_spl(TM_spl packet, ISIStrxvuBitrate bitRate)
{
	byte raw[SIZE_TXFRAME];
	int size;
	int err = encode_TMpacket(raw, &size, packet);
	check_int("send_TM_spl, encode_TMpacket", err);
	return TRX_sendFrame(raw, (uint8_t)size, bitRate);
}

TM_spl spl_image(unsigned int index, fileType compressType, unsigned int pic_id, chunk_t chunk)
{
	TM_spl packet;

	packet.type = IMAGE_DUMP_T;
	packet.subType = IMAGE_DUMP_ST;
	//todo: get the time when the image was taken
	packet.time = 3232;
	packet.length = CHUNK_SIZE + sizeof(int) * 2 + 1;

	BigEnE_uInt_to_raw(pic_id, packet.data);
	BigEnE_uInt_to_raw(index, packet.data + 4);
	packet.data[8] = (byte)compressType;
	memcpy((packet.data + sizeof(int) * 2 + 1), chunk, (int)packet.length);

	return packet;
}

void reset_APRS_list(Boolean firstActivation)
{
	int i_error = 0;
	uint8_t numberOfPackets = 0;	//get number of packets in the FRAM

	memset(APRS_list, 0, APRS_SIZE_WITH_TIME * MAX_NAMBER_OF_APRS_PACKETS);	//sets all slots in the array to zero for later write to the FRAM

	// write 0 to all the FRAM list
	i_error = FRAM_write(APRS_list, APRS_PACKETS_ADDR, (APRS_SIZE_WITH_TIME * MAX_NAMBER_OF_APRS_PACKETS));//reset the memory of the delay command list in the FRAM
	check_int("reset_APRS_list, FRAM_write", i_error);

	// write 0 in number of APRS
	i_error = FRAM_write(&numberOfPackets, NUMBER_PACKET_APRS_ADDR, 1);//reset the number of commands in the FRAM to 0
	check_int("reset_APRS_list, FRAM_write", i_error);

	//  if its not init, update the number of APRS commands
	if (!firstActivation)
	{
		set_numOfAPRS(numberOfPackets);
	}
}

int send_APRS_Dump()
{
	int i_error = 0;
	byte numberOfAPRS = 0;	// number of packets in APRS_packets

	// 1. Get the list of APRS packets and number of packets from the FRAM
	i_error = FRAM_read(&numberOfAPRS, NUMBER_PACKET_APRS_ADDR, sizeof(numberOfAPRS));
	check_int("send_APRS_Dump, FRAM_read", i_error);
	i_error =FRAM_read(APRS_list, APRS_PACKETS_ADDR, (numberOfAPRS * APRS_SIZE_WITH_TIME));
	check_int("send_APRS_Dump, FRAM_read", i_error);

	if (numberOfAPRS == 0)
	{
		return 1;
	}

	time_unix time_now;
	byte rawData[APRS_SIZE_WITH_TIME + 8];
	int rawDataLength = 0;

	TM_spl packet;
	packet.type = APRS;
	packet.subType = APRS_PACKET_FRAM;
	packet.length = APRS_SIZE_WITH_TIME;

	// 2. Going throw every packet on the list
	int i, j;
	for (i = 0; i < numberOfAPRS; i++)
	{
		// 3. Insert data to packet
		memcpy(packet.data, APRS_list, APRS_SIZE_WITH_TIME);

		i_error = Time_getUnixEpoch(&time_now);	//get time
		check_int("send_APRS_Dump, Time_getUnixEpoch", i_error);
		packet.time = time_now;

		encode_TMpacket(rawData, &rawDataLength, packet);
		// 4. Sends packet twice
		for (j = 0; j < 2; j++)
		{
			TRX_sendFrame(rawData, (unsigned char)rawDataLength, trxvu_bitrate_9600);
		}
	}

	// 5. Reseting the APRS list in the FRAM
	reset_APRS_list(FALSE);

	return 0;
}

int checks_APRS(unsigned char* data)
{
	//1. checks if the packet is APRS packet
	char PrefixAPRS[] = { '!' };
	if (!memcmp(PrefixAPRS, data, 1))
	{
		//2. Add Time stamp
		time_unix time_now;
		Time_getUnixEpoch(&time_now);
		BigEnE_uInt_to_raw(time_now, &data[APRS_SIZE_WITHOUT_TIME]);

		//3. save APRS Packet in the FRAM list
		uint8_t number_of_APRS_save = 0;
		FRAM_read(&number_of_APRS_save, NUMBER_PACKET_APRS_ADDR, sizeof(number_of_APRS_save));

		if (number_of_APRS_save == 0)	//in case there's no list in the FRAM
		{
			FRAM_write(data, APRS_PACKETS_ADDR, APRS_SIZE_WITH_TIME);//write the list back to the FRAM
			number_of_APRS_save++;
			FRAM_write(&number_of_APRS_save, NUMBER_PACKET_APRS_ADDR, sizeof(number_of_APRS_save));//write the number of APRS packets in the FRAM back to the FRAM

			return 1;	//returns that the packet was an APRS packet
		}

		FRAM_read(APRS_list, APRS_PACKETS_ADDR, (MAX_NAMBER_OF_APRS_PACKETS * APRS_SIZE_WITH_TIME));//read exiting list in the FRAM
		int count = 0;
		while (APRS_list[count] == '!')//find if empty place for the new APRS packet
		{
			count += APRS_SIZE_WITH_TIME;
		}

		//saves the APRS packet in the exiting list
		for (int i = 0; i < APRS_SIZE_WITH_TIME; i++)
		{
			APRS_list[i + count] = data[i];
		}

		FRAM_write(APRS_list, APRS_PACKETS_ADDR, APRS_SIZE_WITH_TIME);//write the list back to the FRAM
		number_of_APRS_save++;
		FRAM_write(&number_of_APRS_save, NUMBER_PACKET_APRS_ADDR, sizeof(number_of_APRS_save));//write the number of APRS packets in the FRAM back to the FRAM
		set_numOfAPRS(number_of_APRS_save);

		return 1;	//returns that the packet was an APRS packet
	}

	return 0;
}

void get_APRS_list()
{
	int error;

	memset(APRS_list, 0, APRS_SIZE_WITH_TIME * MAX_NAMBER_OF_APRS_PACKETS);	//sets all slots in the array to zero for later write to the FRAM
	error = FRAM_read(APRS_list, APRS_PACKETS_ADDR, APRS_SIZE_WITH_TIME * MAX_NAMBER_OF_APRS_PACKETS);
	check_int("get_APRS_list, FRAM_read(APRS_PACKETS_ADDR)", error);

	uint8_t num_of_APRS_save;
	error = FRAM_read(&num_of_APRS_save, NUMBER_PACKET_APRS_ADDR, 1);
	check_int("get_APRS_list, FRAM_read(NUMBER_PACKET_APRS_ADDR)", error);

	set_numOfAPRS(num_of_APRS_save);
}

void change_TRXVU_state(Boolean state)
{
	byte data[2];
	//command id
	data[0] = 0x38;
	if (state == NOMINAL_MODE)
	{
		//nominal mode
		printf("\tTransponder is disabled\n\n");
		data[1] = 0x01;
		set_system_state(transponder_active_param, SWITCH_OFF);
	}
	else
	{
		//transponder mode
		printf("\tTransponder enabled\n\n");
		data[1] = 0x02;
		set_system_state(transponder_active_param, SWITCH_ON);
	}
	//sends I2C command
	int i_error = I2C_write(I2C_TRXVU_TC_ADDR, data, 2);
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

void check_time_off_mute()
{
	time_unix mute_time, time_now;

	int i_error = Time_getUnixEpoch(&time_now);
	check_int("trxvu_logic, Time_getUnixEpoch", i_error);

	i_error = FRAM_read((byte*)&mute_time, MUTE_TIME_ADDR, TIME_SIZE);
	check_int("trxvu_logic, FRAM_read(MUTE_TIME_ADDR)", i_error);
	if (time_now >= mute_time)
	{
		// stop mute
		mute_Tx(FALSE);

		mute_time = 0;
		i_error = FRAM_write((byte*)&mute_time, MUTE_TIME_ADDR, TIME_SIZE);
		check_int("trxvu_logic, FRAM_write(MUTE_TIME_ADDR)", i_error);
	}
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

void mute_Tx(Boolean state)
{
	if (state == TRUE)
	{
		//1. Shut down transponder if active
		stop_dump();
		//2. Stop dump if Dump task active
		stop_transponder();
		//3. update state
		set_system_state(mute_param, SWITCH_ON);
	}
	else
	{
		//1.1. check if the satellite in mute state
		Boolean state = get_system_state(mute_param);
		if (state)
		{
			//1.2. update state
			set_system_state(mute_param, SWITCH_OFF);
		}
	}
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

void vutc_getTxTelemTest(void)
{
	unsigned short telemetryValue;
	float eng_value = 0.0;
	ISIStrxvuTxTelemetry_revC telemetry;
	int rv;

	// Telemetry values are presented as raw values
	printf("\r\nGet all Telemetry at once in raw values \r\n\r\n");
	rv = IsisTrxvu_tcGetTelemetryAll_revC(0, &telemetry);
	if(rv)
	{
		printf("Subsystem call failed. rv = %d", rv);
	}

	telemetryValue = telemetry.fields.bus_volt;
	eng_value = ((float)telemetryValue) * 0.00488;
	printf("Bus voltage = %f V\r\n", eng_value);

	telemetryValue = telemetry.fields.total_current;
	eng_value = ((float)telemetryValue) * 0.16643964;
	printf("Total current = %f mA\r\n", eng_value);

	telemetryValue = telemetry.fields.pa_temp;
	eng_value = ((float)telemetryValue) * -0.07669 + 195.6037;
	printf("PA temperature = %f degC\r\n", eng_value);

	telemetryValue = telemetry.fields.tx_reflpwr;
	eng_value = ((float)(telemetryValue * telemetryValue)) * 5.887E-5;
	printf("RF reflected power = %f mW\r\n", eng_value);

	telemetryValue = telemetry.fields.tx_fwrdpwr;
	eng_value = ((float)(telemetryValue * telemetryValue)) * 5.887E-5;
	printf("RF forward power = %f mW\r\n", eng_value);

	telemetryValue = telemetry.fields.locosc_temp;
	eng_value = ((float)telemetryValue) * -0.07669 + 195.6037;
	printf("Local oscillator temperature = %f degC\r\n", eng_value);
}

void vurc_getRxTelemTest(void)
{
	unsigned short telemetryValue;
	float eng_value = 0.0;
	ISIStrxvuRxTelemetry_revC telemetry;
	int rv;

	// Telemetry values are presented as raw values
	printf("\r\nGet all Telemetry at once in raw values \r\n\r\n");
	rv = IsisTrxvu_rcGetTelemetryAll_revC(0, &telemetry);
	if(rv)
	{
		printf("Subsystem call failed. rv = %d", rv);
	}

	telemetryValue = telemetry.fields.bus_volt;
	eng_value = ((float)telemetryValue) * 0.00488;
	printf("Bus voltage = %f V\r\n", eng_value);

	telemetryValue = telemetry.fields.total_current;
	eng_value = ((float)telemetryValue) * 0.16643964;
	printf("Total current = %f mA\r\n", eng_value);

	telemetryValue = telemetry.fields.pa_temp;
	eng_value = ((float)telemetryValue) * -0.07669 + 195.6037;
	printf("PA temperature = %f degC\r\n", eng_value);

	telemetryValue = telemetry.fields.locosc_temp;
	eng_value = ((float)telemetryValue) * -0.07669 + 195.6037;
	printf("Local oscillator temperature = %f degC\r\n", eng_value);

	telemetryValue = telemetry.fields.rx_doppler;
	eng_value = ((float)telemetryValue) * 13.352 - 22300;
	printf("Receiver doppler = %f Hz\r\n", eng_value);

	telemetryValue = telemetry.fields.rx_rssi;
	eng_value = ((float)telemetryValue) * 0.03 - 152;
	printf("Receiver RSSI = %f dBm\r\n", eng_value);
}
