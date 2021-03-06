/*
 * DelayedCommand_list.c
 *
 *  Created on: Apr 6, 2019
 *      Author: DBTn
 */
#include <hal/Timing/Time.h>
#include <hal/Storage/FRAM.h>
#include "string.h"

#include "../Global/GlobalParam.h"
#include "../Global/Global.h"
#include "../Global/logger.h"

#include "DelayedCommand_list.h"
#include "../Main/commands.h"

#define DELETE_CMD_FROM_LIST(starting_point) memset((starting_point + DelayCommand_list) , 0, SIZE_OF_DELAYED_COMMAND)
#define EXTRACT_TIME_FROM_DELAYED_COMMAND(startOfCommand) BigEnE_raw_to_uInt(DelayCommand_list + startOfCommand + SPL_TC_HEADER_SIZE - TIME_SIZE)

static byte DelayCommand_list[MAX_NUMBER_OF_DELAY_COMMAND * SIZE_OF_DELAYED_COMMAND];

/*
 * @brief		Find a delayed command in the list
 * @param[in]	Starting index in the buffer
 * @return		The index were the next command in the list starts
 */
int find_CMD_in_list(int startingPoint)
{
	int count = startingPoint + CMD_ID_SIZE;
	while (count < SIZE_OF_DELAYED_COMMAND * MAX_NUMBER_OF_DELAY_COMMAND)
	{
		if ((byte)NOT_DELAYED_COMMAND != DelayCommand_list[count])
		{
			return count - CMD_ID_SIZE;
		}
		else
		{
			count += SIZE_OF_DELAYED_COMMAND;
		}
	}
	return -1;
}

/*
 * @brief	Find the next empty space in the list from the start
 * @return	The startimg index of the empty space
 */
int find_empty_in_list()
{
	int count = CMD_ID_SIZE;
	while (count < SIZE_OF_DELAYED_COMMAND * MAX_NUMBER_OF_DELAY_COMMAND)
	{
		if ((byte)NOT_DELAYED_COMMAND == DelayCommand_list[count])
		{
			return count - CMD_ID_SIZE;
		}
		else
		{
			count += SIZE_OF_DELAYED_COMMAND;
		}
	}
	return -1;
}

/*
 * @brief		Check if the time indecent a command ready to execute,
 * 				delete if the command expired or if the command is not ready yet
 * @return		-1 expired command, 1 command ready to execute 0 not ready
 */
int check_delayCommandTime(time_unix cmd_time)
{
	time_unix time_now;
	int error = Time_getUnixEpoch(&time_now);
	if (error != 0)
		WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_GET_TIME, SYSTEM_TRXVU, error);
	check_int("check_delayCommandTime, Time_getUnixEpoch", error);

	int ret = 0;
	if (cmd_time + EXPIRED_TIME_DC <= time_now)
		ret = -1;//expired command
	else if (cmd_time <= time_now)
		ret = 1;//command to execute
	else
		ret = 0;//not the time

	//printf("time now: %u \ncmd time: %u \nret value: %d\n\n", time_now, cmd_time, ret);
	return ret;
}

void execute_delayedCommand(int startingPoint)
{
	TC_spl decode;	//decode command
	decode_TCpacket(DelayCommand_list + startingPoint, -1, &decode);	//docoded packet to execute
	add_command(decode);	//executing command
	//printf("execute command: %u\n", decode.id);
	//printf("Time stamp: %u\n", decode.time);
}

void check_delaycommand()
{
	Boolean list_changed = FALSE;
	int err;
	unsigned char numberOfCommands = 0;	//stored the number of commands from the FRAM
	err = FRAM_read_exte(&numberOfCommands, NUMBER_COMMAND_FRAM_ADDR, 1);
	if (err != 0)
	{
		WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_FRAM_READ, SYSTEM_TRXVU, err);
		return;
	}
	check_int("check_delaycommand, FRAM_read_exte(NUMBER_COMMAND_FRAM_ADDR)", err);
	set_numOfDelayedCommand(numberOfCommands);
	//printf("number of commands in FRAM %d\n", numberOfCommands);
	//2. Loading delayed command list from FRAM

	time_unix time_;
	int count = find_CMD_in_list(0);	//count the number of byte of the list, starts from 4 (the first command type)
	while (count != -1)
	{
		time_ = EXTRACT_TIME_FROM_DELAYED_COMMAND(count);
		int ret = check_delayCommandTime(time_);
		if (ret == -1)
		{
			DELETE_CMD_FROM_LIST(count);
			list_changed = TRUE;
			numberOfCommands--;
		}
		else if (ret == 1)
		{
			execute_delayedCommand(count);
			DELETE_CMD_FROM_LIST(count);
			list_changed = TRUE;
			numberOfCommands--;
		}

		count = find_CMD_in_list(count + SIZE_OF_DELAYED_COMMAND);
	}

	// 11. check if there's the need to return list to FRAM
	if (list_changed == TRUE)
	{
		// 12. return the list to the FRAM
		err = FRAM_write_exte(DelayCommand_list, DELAY_COMMAD_FRAM_ADDR, (MAX_NUMBER_OF_DELAY_COMMAND * SIZE_OF_DELAYED_COMMAND));	//write back the new list
		if (err != 0)
		{
			WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_FRAM_WRITE, SYSTEM_TRXVU, err);
			return;
		}
		err = FRAM_write_exte(&numberOfCommands, NUMBER_COMMAND_FRAM_ADDR, sizeof(numberOfCommands));	//write back the nuber of commands that stored in the FRAM
		if (err != 0)
			WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_FRAM_WRITE, SYSTEM_TRXVU, err);
	}

}

void reset_delayCommand()
{
	byte numberOfCommands = 0;				//get number of command in the FRAM

	memset(DelayCommand_list, 0, MAX_NUMBER_OF_DELAY_COMMAND * SIZE_OF_DELAYED_COMMAND);

	int err = FRAM_write_exte(DelayCommand_list, DELAY_COMMAD_FRAM_ADDR, (MAX_NUMBER_OF_DELAY_COMMAND * SIZE_OF_DELAYED_COMMAND));	//reset the memory of the delay command list in the FRAM
	if (err != 0)
		WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_FRAM_WRITE, SYSTEM_TRXVU, err);
	check_int("reset_delayCommand, FRAM_write_exte(DELAY_COMMAD_FRAM_ADDR)", err);

	err = FRAM_write_exte(&numberOfCommands, NUMBER_COMMAND_FRAM_ADDR, sizeof(numberOfCommands));	//reset the number of commands in the FRAM to 0
	if (err != 0)
		WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_FRAM_WRITE, SYSTEM_TRXVU, err);
	check_int("reset_delayCommand, FRAM_write_exte(NUMBER_COMMAND_FRAM_ADDR)", err);
}

int add_delayCommand(TC_spl decode)
{
	int i_error = 0;
	// 1. check if the length of the command in range of delay command
	if (decode.length + SPL_TC_HEADER_SIZE > SIZE_OF_DELAYED_COMMAND)
	{
		return -1;
	}
	unsigned char numberOfCommands = 0;				//get number of command in the FRAM

	int size = SIZE_OF_DELAYED_COMMAND;				//size of array data
	byte command[SIZE_OF_DELAYED_COMMAND];				//encoded packet
	memset(command, 0, SIZE_OF_DELAYED_COMMAND);

	// 2. encode TC command
	encode_TCpacket(command, &size, decode);

	// 3. read number of delay commands in delay command list
	i_error = FRAM_read_exte(&numberOfCommands, NUMBER_COMMAND_FRAM_ADDR, 1);
	if (i_error != 0)
	{
		WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_FRAM_READ, SYSTEM_TRXVU, i_error);
		return i_error;
	}
	check_int("add_delayCommand, FRAM_read", i_error);

	// 4. check if the list is full
	if (numberOfCommands >= MAX_NUMBER_OF_DELAY_COMMAND)
	{
		EmptyOldestCommand();
		numberOfCommands--;
		//printf("list full delete one element\n");
	}

	int count = find_empty_in_list();
	if (count != -1)//if there's an empty space in the list
	{
		memcpy(DelayCommand_list + count, command, decode.length + SPL_TC_HEADER_SIZE);
		//printf("add command: %u\n", decode.id);
		//printf("Time stamp: %u\n", decode.time);
	}

	// 7.1. returns the list to the FRAM
	i_error = FRAM_write_exte(DelayCommand_list, DELAY_COMMAD_FRAM_ADDR, (SIZE_OF_DELAYED_COMMAND * MAX_NUMBER_OF_DELAY_COMMAND));
	if (i_error != 0)
	{
		WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_FRAM_WRITE, SYSTEM_TRXVU, i_error);
		DELETE_CMD_FROM_LIST(count);
		return i_error;
	}
	check_int("add_delayCommand, FRAM_write", i_error);
	numberOfCommands++;
	i_error = FRAM_write_exte(&numberOfCommands, NUMBER_COMMAND_FRAM_ADDR, 1);
	if (i_error != 0)
	{
		WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_FRAM_WRITE, SYSTEM_TRXVU, i_error);
		DELETE_CMD_FROM_LIST(count);
		numberOfCommands--;
		return i_error;
	}
	check_int("add_delayCommand, FRAM_write", i_error);

	return 0;
}

int get_delayCommand_list()
{
	int error;
	memset(DelayCommand_list, 0, MAX_NUMBER_OF_DELAY_COMMAND * SIZE_OF_DELAYED_COMMAND);	//sets all slots in the array to zero for later write to the FRAM
	error = FRAM_read_exte(DelayCommand_list, DELAY_COMMAD_FRAM_ADDR, MAX_NUMBER_OF_DELAY_COMMAND * SIZE_OF_DELAYED_COMMAND);
	if (error != 0)
	{
		WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_FRAM_READ, SYSTEM_TRXVU, error);
		return error;
	}
	check_int("get_delayCommand_list, FRAM_read_exte(DELAY_COMMAD_FRAM_ADDR)", error);
	unsigned char num;
	error = FRAM_read_exte(&num, NUMBER_COMMAND_FRAM_ADDR, 1);
	if (error != 0)
	{
		WriteErrorLog((log_errors)LOG_ERR_COMM_DELAYED_COMMAND_FRAM_READ, SYSTEM_TRXVU, error);
		return error;
	}
	check_int("get_delayCommand_list, FRAM_read_exte(NUMBER_COMMAND_FRAM_ADDR)", error);
	return 0;
}

void EmptyOldestCommand()
{
	time_unix OldTime = 0, cmdTime = 0;
	for (int i = 0; i < MAX_NUMBER_OF_DELAY_COMMAND * SIZE_OF_DELAYED_COMMAND; i+= SIZE_OF_COMMAND)
	{
		cmdTime = EXTRACT_TIME_FROM_DELAYED_COMMAND(i);
		if (OldTime < cmdTime)
		{
			OldTime = cmdTime;
		}
	}
	for (int i = 0; i < MAX_NUMBER_OF_DELAY_COMMAND * SIZE_OF_DELAYED_COMMAND; i += SIZE_OF_COMMAND)
	{
		cmdTime = EXTRACT_TIME_FROM_DELAYED_COMMAND(i);
		if (OldTime == cmdTime)
		{
			DELETE_CMD_FROM_LIST(i);
			//printf("The list is full, delete the command with the oldest time stamp\n");
			break;
		}
	}
}

