/*
 * Camera.c
 *
 *  Created on: 7 ׳‘׳�׳�׳™ 2019
 *      Author: I7COMPUTER
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <hal/Storage/FRAM.h>
#include <hal/Timing/Time.h>

#include <at91/peripherals/pio/pio.h>

#include <satellite-subsystems/GomEPS.h>
#include <satellite-subsystems/SCS_Gecko/gecko_driver.h>

#include "../../Global/GlobalParam.h"
#include "../../Global/logger.h"

#include "GeckoCameraDriver.h"

#define Result(value, errorType)	if(value != 0) { return errorType; }

#define _SPI_GECKO_BUS_SPEED MHZ(5)

#define	totalPageCount 136
#define totalFlashCount 4096

#define MAGIC_NUM 10000

void Initialized_GPIO()
{
	Pin gpio12 = PIN_GPIO12;
	PIO_Configure(&gpio12, PIO_LISTSIZE(&gpio12));
	vTaskDelay(10);
	PIO_Set(&gpio12);
	vTaskDelay(10);
}
void De_Initialized_GPIO()
{
	Pin Pin12 = PIN_GPIO12;
	PIO_Clear(&Pin12);
	vTaskDelay(10);
}

Boolean TurnOnGecko()
{
	printf("turning camera on\n");
	Pin gpio4=PIN_GPIO04;
	Pin gpio5=PIN_GPIO05;
	Pin gpio6=PIN_GPIO06;
	Pin gpio7=PIN_GPIO07;
	PIO_Configure(&gpio4, PIO_LISTSIZE(&gpio4));
	vTaskDelay(10);
	PIO_Configure(&gpio5, PIO_LISTSIZE(&gpio5));
	vTaskDelay(10);
	PIO_Configure(&gpio6, PIO_LISTSIZE(&gpio6));
	vTaskDelay(10);
	PIO_Configure(&gpio7, PIO_LISTSIZE(&gpio7));
	vTaskDelay(10);

	PIO_Set(&gpio4);
	vTaskDelay(10);
	PIO_Set(&gpio5);
	vTaskDelay(10);
	PIO_Set(&gpio6);
	vTaskDelay(10);
	PIO_Set(&gpio7);
	vTaskDelay(10);

	//Initialized_GPIO();

	set_system_state(cam_param, SWITCH_ON);
	WritePayloadLog(PAYLOAD_TURNED_GECKO_ON, (int)0);

	return TRUE;
}
Boolean TurnOffGecko()
{
	printf("turning camera off\n");
	Pin gpio4=PIN_GPIO05;
	Pin gpio5=PIN_GPIO07;
	Pin gpio6=PIN_GPIO05;
	Pin gpio7=PIN_GPIO07;

	PIO_Clear(&gpio4);
	vTaskDelay(10);
	PIO_Clear(&gpio5);
	vTaskDelay(10);
	PIO_Clear(&gpio6);
	vTaskDelay(10);
	PIO_Clear(&gpio7);
	vTaskDelay(10);

	//De_Initialized_GPIO();

	set_system_state(cam_param, SWITCH_OFF);
	WritePayloadLog(PAYLOAD_TURNED_GECKO_OFF, (int)0);

	return TRUE;
}

int initGecko()
{
	return GECKO_Init( (SPIslaveParameters){ bus1_spi, mode0_spi, slave1_spi, 100, 1, _SPI_GECKO_BUS_SPEED, 0 } );
}

int GECKO_TakeImage( uint8_t adcGain, uint8_t pgaGain, uint32_t exposure, uint32_t frameAmount, uint32_t frameRate, uint32_t imageID, Boolean testPattern)
{
	unsigned char somebyte = 0;
	GomEpsPing(0, 0, &somebyte);

	printf("GomEpsResetWDT = %d\n", GomEpsResetWDT(0));

	// Setting PGA Gain:
	int result = GECKO_SetPgaGain(pgaGain);
	Result(result, -3);

	// Setting ADC Gain:
	result = GECKO_SetAdcGain(adcGain);
	Result(result, -2);

	// Setting sensor offset:
	/*
	 * All info taken from datasheet v1.4
	 * Contained in register 0x0D in bits 16 to 31,
	 * during tests with ISIS's function for taking pictures we checked the registers' values, 0x0D value was 0x3FC30335
	 * hence the number 0x3FC3
	 */
	result = GECKO_SetOffset(0x3FC3);
	Result( result, -18);

	// Setting exposure time:
	result = GECKO_SetExposure(exposure);
	Result( result, -4);

	// Setting Frame amount:
	result=GECKO_SetFrameAmount(frameAmount);
	Result( result, -5);

	// Setting Frame rate:
	result = GECKO_SetFrameRate(frameRate);
	Result( result, -6);

	// Turning sensor ON:
	result = GECKO_SensorOn();
	Result( result, -7);

	// Training:
	int i = 0;
	do
	{
		result = GECKO_GetTrainingDone();

		if (i == 120)	// timeout at 2 minutes
			return -9;
		vTaskDelay(500);
		i++;
	} while(result == 0);

	// Checking for training error:
	result = GECKO_GetTrainingError();
	Result( result, -10);

	// Init Flash:
	i = 0;
	do
	{
		result = GECKO_GetFlashInitDone();

		if (i == 120)	// timeout at 2 minutes
			return -11;
		vTaskDelay(500);
		i++;
	} while(result == 0);

	// Setting image ID:
	result = GECKO_SetImageID(imageID);
	Result( result, -12);

	if (testPattern)	// Enabling test pattern:
	{
		result = GECKO_EnableTestPattern();
		Result( result, -14);
	}
	else	// Disabling test pattern:
	{
		result = GECKO_DisableTestPattern();
		Result( result, -13);
	}

	// Start sampling:
	result = GECKO_StartSample();
	Result( result, -15);

	// Waiting until sample done:
	i = 0;
	do
	{
		result = GECKO_GetSampleDone();

		if (i == 120)	// timeout at 2 minutes
			return -16;
		vTaskDelay(500);
		i++;
	} while(result == 0);

	// Clearing the register:
	int result_clearSampleDone = GECKO_ClearSampleDone();
	Result( result_clearSampleDone, -17);

	// Turning sensor OFF:
	result = GECKO_SensorOff();
	Result( result, -1);

	return 0;
}

int GECKO_ReadImage( uint32_t imageID, uint32_t *buffer)
{
	unsigned char somebyte = 0;
	GomEpsPing(0, 0, &somebyte);

	printf("GomEpsResetWDT = %d\n", GomEpsResetWDT(0));
	printf("starting read\n");
	// Init Flash:
	int result, i = 0;
	do
	{
		result = GECKO_GetFlashInitDone();

		if (i == 120)	// timeout at 2 minutes
			return -1;
		vTaskDelay(500);
		i++;
	} while(result == 0);

	// Setting image ID:
	result = GECKO_SetImageID(imageID);
	Result( result, -2);

	// Starting Readout:
	result = GECKO_StartReadout();
	Result( result, -3);

	i = 0;
	// Checking if the data is ready to be read:
	do
	{
		result = GECKO_GetReadReady();

		printf("not finish in: GECKO_GetReadReady = %d\n" , result);

		if (i == 120)	// timeout at 2 minutes
			return -1;
		vTaskDelay(500);
		i++;
	} while(result == 0);

	vTaskDelay(1000);

	for (unsigned int i = 0; i < IMAGE_SIZE/sizeof(uint32_t); i++)
	{
		buffer[i] = GECKO_GetImgData();

		// Printimg a value one every 40000 pixels:
		if(i % MAGIC_NUM == 0)
		{
			printf("%u, %u\n", i, (uint8_t)*(buffer + i));
			vTaskDelay(SYSTEM_DEALY);
		}
	}

	printf("ImageSize = %d\n", IMAGE_SIZE);

	result = GECKO_GetReadDone();
	if (result == 0)
		return -7;

	result = GECKO_ClearReadDone();
	Result(result, -8);

	return 0;
}

int GECKO_EraseBlock( uint32_t imageID )
{
	unsigned char somebyte = 0;
	GomEpsPing(0, 0, &somebyte);

	printf("GomEpsResetWDT = %d\n", GomEpsResetWDT(0));

	// Setting image ID:
	int result_setImageId = GECKO_SetImageID(imageID);
	Result( result_setImageId, -1);

	// Starting erase:
	int result_startErase = GECKO_StartErase();
	Result(result_startErase, -2);

	int i = 0;
	do {
		if (i == 120)	// timeout at 2 minutes
			return -3;
		vTaskDelay(500);
		i++;

	} while (GECKO_GetEraseDone() == 0);

	int result_clearEraseDone = GECKO_ClearEraseDone();
	Result(result_clearEraseDone, -4);

	return 0;
}
