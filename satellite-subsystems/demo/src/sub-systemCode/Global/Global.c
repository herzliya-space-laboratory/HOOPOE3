/*
 * Global.c
 *
 *  Created on: Oct 20, 2018
 *      Author: DBTn
 */
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <at91/utility/exithandler.h>
#include <at91/commons.h>
#include <at91/utility/trace.h>
#include <at91/peripherals/cp15/cp15.h>

#include <hal/Utility/util.h>
#include <hal/Timing/WatchDogTimer.h>
#include <hal/Drivers/I2C.h>
#include <hal/Drivers/LED.h>
#include <hal/boolean.h>
#include <hal/errors.h>

#include <hal/Storage/FRAM.h>

#include <string.h>

#include <satellite-subsystems/IsisTRXVU.h>
#include <satellite-subsystems/IsisAntS.h>
#include <satellite-subsystems/cspaceADCS.h>
#include <satellite-subsystems/GomEPS.h>

#include "Global.h"
#include "TLM_management.h"

int not_first_activation;

//check errors in driverse:
void check_int(char *string_output, int error)
{
	if (error != 0){
#if (1 == PRINT_IN_CHECK_FUNCTION)
		printf("%s.\nresult: %d\n", string_output, error);
#endif
	}
}

void check_portBASE_TYPE(char *string_output, long error)
{
	if (error != pdTRUE)
	{
#if (1 == PRINT_IN_CHECK_FUNCTION)
		printf("%s.\nresult: %lu\n", string_output, error);
#endif
	}
}

void print_array(unsigned char *arr, int length)
{
#if (1 == PRINT_IN_CHECK_FUNCTION)
	int i;
	for (i=0;i<length;i++)
	{
		printf("%x ",arr[i]);
	}
	printf("END\n");
#endif
}

void switch_endian(unsigned char *in, unsigned char *out, int len)
{
	int i;
	for(i = 0; i < len / 2; i++)
	{
		out[i] = in[len - i];
	}
	if (len % 2 == 1)
	{
		out[i] = in[i];
	}
}

void double_little_endian(unsigned char* d)
{
	int i=0;
	char temp;
	for(i=0;i<4;i++)
	{
		temp=d[i];
		d[i]=d[7-i];
		d[7-i]=temp;
	}
}

int soft_reset_subsystem(subSystem_indx reset_idx)
{
	int error;
	switch (reset_idx)
	{
	case EPS:
		error = GomEpsSoftReset(0);
		check_int("soft reset, EPS", error);
		if (error != 0)
			WriteErrorLog(LOG_ERR_EPS_SOFT_RESET, SYSTEM_OBC, error);
		break;
	case TRXVU:
		error = IsisTrxvu_softReset(0);
		check_int("soft reset, TRXVU", error);
		if (error != 0)
			WriteErrorLog(LOG_ERR_TRXVU_SOFT_RESET, SYSTEM_OBC, error);
		break;
	case ADCS:
		error = cspaceADCS_componentReset(0);
		check_int("soft reset, ADCS", error);
		if (error != 0)
			WriteErrorLog(LOG_ERR_ADCS_SOFT_RESET, SYSTEM_OBC, error);
		break;
	case OBC:
		gracefulReset();
		error = 4242432;
		break;
	default:
		return -444;
	break;
	}

	return error;
}

int hard_reset_subsystem(subSystem_indx reset_idx)
{
	int error = 0;
	gom_eps_channelstates_t channel;
	switch (reset_idx)
	{
	case EPS:
		error = GomEpsHardReset(0);
		break;
	case TRXVU:
		error = IsisTrxvu_hardReset(0);
		check_int("hard reset TRXVU", error);
		if (error != 0)
			WriteErrorLog(LOG_ERR_TRXVU_HARD_RESET, SYSTEM_OBC, error);
		break;
	case Ants:
		error = IsisAntS_reset(0, isisants_sideA);
		check_int("hard reset to ants_a", error);
		if (error != 0)
			WriteErrorLog(LOG_ERR_HARD_RESET_ANTS, SYSTEM_OBC, error);
		error = IsisAntS_reset(0, isisants_sideB);
		check_int("hard reset to ants_b", error);
		if (error != 0)
			WriteErrorLog(LOG_ERR_HARD_RESET_ANTS, SYSTEM_OBC, error);
		break;
	case ADCS:
		channel.raw = 0;
		error = GomEpsSetOutput(0, channel);
		if (error != 0)
			WriteErrorLog(LOG_ERR_ADCS_SOFT_RESET, SYSTEM_OBC, error);
		check_int("Hard reset ADCS, turn off EPS channels", error);
		break;
	case CAMMERA:
		//todo:
		break;
	default:
		return -444;
	break;
	}

	return error;
}

unsigned int BigEnE_raw_to_uInt(unsigned char raw[4])
{
	//get the epoctime signature from the packet sent
	unsigned int uInt = 0;
	uInt += raw[0]<<24;
	uInt += raw[1]<<16;
	uInt += raw[2]<<8;
	uInt += raw[3];
	//printf("final answer is %lu\n",tl);
	return uInt;
}

unsigned short BigEnE_raw_to_uShort(unsigned char raw[2])
{
	unsigned short vol = (unsigned short)(raw[0] << 8);
	vol += (unsigned short)raw[1];
	return vol;
}

void BigEnE_uInt_to_raw(unsigned int uInt, unsigned char raw[4])
{
	raw[0] = uInt >> 24;
	raw[1] = uInt >> 16;
	raw[2] = uInt >> 8;
	raw[3] = uInt;
}

void BigEnE_raw_value(byte *in, int length)
{
	byte temp;
	for (int i = 0; i < length / 2; i++)
	{
		temp = in[i];
		in[i] = in[length - 1 - i];
		in[length - 1 - i] = temp;
	}
}

int fram_byte_fix(unsigned int address)
{
	int i_error;
	unsigned char byte = 0;
	int error = 0;
	i_error = FRAM_read(&byte, address, sizeof(byte));
	if (i_error != 0)
		WriteErrorLog(LOG_ERR_FRAM_READ, SYSTEM_OBC, i_error);
	check_int("fram_byte_fix, FRAM_read", i_error);
	if ((byte != 255) || (byte != 0))
	{
		int count1 = 0, count0 = 0;
		int i;
		for (i = 0; i < 8; i++)
		{
			if ((byte & (1 << i)) == (1 << i))
				count1++;
			else
				count0++;
		}

		if (count0 > count1)
		{
			byte = 0;
			error = 0;
		}
		else if(count1 > count0)
		{
			byte = 255;
			error = 0;
		}
		else
		{
			byte = 0;
			error = 1;
		}
		i_error = FRAM_write(&byte, address, 1);
		if (i_error != 0)
			WriteErrorLog(LOG_ERR_FRAM_WRITE, SYSTEM_OBC, i_error);
		check_int("fram_byte_fix, FRAM_write", i_error);
	}
	return error;
}

Boolean comapre_string(unsigned char* str0, unsigned char* str1, int numberOfBytes)
{
	int i;
	for (i = 0; i < numberOfBytes; i++)
	{
		if (str0[i] != str1[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}

void reset_FRAM_MAIN()
{
	byte raw[4];
	raw[0] = 0;
	// reset the states byte in the FRAM
	int err = FRAM_write(raw, STATES_ADDR, 1);
	if (err != 0)
		WriteErrorLog(LOG_ERR_FRAM_WRITE, SYSTEM_OBC, err);
	check_int("reset_FRAM_MAIN, FRAM_write(STATES_ADDR)", err);
	// sets the FIRST_ACTIVATION_ADDR to true (now is the first activation)
	raw[0] = TRUE_8BIT;
	err = FRAM_write(raw, FIRST_ACTIVATION_ADDR, 1);
	if (err != 0)
		WriteErrorLog(LOG_ERR_FRAM_WRITE, SYSTEM_OBC, err);
	check_int("reset_FRAM_MAIN, FRAM_write(FIRST_ACTIVATION_ADDR)", err);

	int i;
	for (i = 0; i < 4; i++)
		raw[i] = 0;
	err = FRAM_write(raw, TIME_ADDR, TIME_SIZE);
	if (err != 0)
		WriteErrorLog(LOG_ERR_FRAM_WRITE, SYSTEM_OBC, err);
	check_int("reset_FRAM_MAIN, FRAM_write(TIME_ADDR)", err);

	err = FRAM_write(raw, RESTART_FLAG_ADDR, 4);
	if (err != 0)
		WriteErrorLog(LOG_ERR_FRAM_WRITE, SYSTEM_OBC, err);
	check_int("reset_FRAM_MAIN, FRAM_write(RESTART_FLAG)", err);

	Boolean8bit bool = FALSE_8BIT;
	err = FRAM_write(&bool, STOP_TELEMETRY_ADDR, 1);
	if (err != 0)
		WriteErrorLog(LOG_ERR_FRAM_WRITE, SYSTEM_OBC, err);
	check_int("reset_FRAM_MAIN, FRAM_write(STOP_TELEMETRY)", err);
}

Boolean getBitValueByIndex(byte* data, int length, int index)
{
	if (data == NULL)
		return FALSE;

	if (length * 8 < index)
		return FALSE;

	int byteIndexInArray = (int)(index / 8);
	byte retValue = data[byteIndexInArray] & (1 << (index % 8));

	if (retValue == 0)
		return FALSE;
	else
		return TRUE;
}


//FreeRTOS extension
void terminateTask()
{
	f_managed_releaseFS();
	vTaskDelete(NULL);
}
