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

#include "../../Global/FRAMadress.h"

#include "../../Global/GlobalParam.h"
#include "../../Global/logger.h"

#include "GeckoCameraDriver.h"

#define PIN_GPIO06_INPUT	{1 << 22, AT91C_BASE_PIOB, AT91C_ID_PIOB, PIO_INPUT, PIO_DEFAULT}
#define PIN_GPIO07_INPUT	{1 << 23, AT91C_BASE_PIOB, AT91C_ID_PIOB, PIO_INPUT, PIO_DEFAULT}

#define VOLTAGE_3V3_CONVERSION(ANALOG_SIGNAL)	(ANALOG_SIGNAL * 2)
#define CURRENT_3V3_CONVERSION(ANALOG_SIGNAL)	(ANALOG_SIGNAL / 2)
#define VOLTAGE_5V_CONVERSION(ANALOG_SIGNAL)	(ANALOG_SIGNAL * 3)
#define CURRENT_5V_CONVERSION(ANALOG_SIGNAL)	(ANALOG_SIGNAL / 2)

#define Result(value, errorType)	if(value != 0) { return errorType; }

#define _SPI_GECKO_BUS_SPEED MHZ(5)

#define	totalPageCount 136
#define totalFlashCount 4096

#define READ_DELAY_INDEXES 10000

#define PIN_GPIO06_INPUT	{1 << 22, AT91C_BASE_PIOB, AT91C_ID_PIOB, PIO_INPUT, PIO_DEFAULT}
#define PIN_GPIO07_INPUT	{1 << 23, AT91C_BASE_PIOB, AT91C_ID_PIOB, PIO_INPUT, PIO_DEFAULT}

void Initialized_GPIO()
{
	Pin gpio12 = PIN_GPIO12;
	PIO_Configure(&gpio12, 1);/*PIO_LISTSIZE(&gpio12));*/
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

int gecko_power_mux_init()
{
	Pin gpio9 = PIN_GPIO09;
	Pin gpio10 = PIN_GPIO10;
	Pin gpio11 = PIN_GPIO11;

	PIO_Configure(&gpio9, 1);/*PIO_LISTSIZE(&gpio9));*/
	vTaskDelay(10);
	PIO_Configure(&gpio10, 1);/*PIO_LISTSIZE(&gpio10));*/
	vTaskDelay(10);
	PIO_Configure(&gpio11, 1);/*PIO_LISTSIZE(&gpio11));*/
	vTaskDelay(10);

	PIO_Set(&gpio9);
	vTaskDelay(10);

	return 0;
}
int gecko_power_mux_deinit()
{
	Pin gpio9 = PIN_GPIO09;
	Pin gpio10 = PIN_GPIO10;
	Pin gpio11 = PIN_GPIO11;

	PIO_Clear(&gpio9, 1);/*PIO_LISTSIZE(&gpio9));*/
	vTaskDelay(10);
	PIO_Clear(&gpio10, 1);/*PIO_LISTSIZE(&gpio10));*/
	vTaskDelay(10);
	PIO_Clear(&gpio11, 1);/*PIO_LISTSIZE(&gpio11));*/
	vTaskDelay(10);

	return 0;
}

int gecko_power_mux_get(Boolean mux0, Boolean mux1)
{
	Pin gpio10 = PIN_GPIO10;
	Pin gpio11 = PIN_GPIO11;

	if (mux0)
		PIO_Set(&gpio10);
	else
		PIO_Clear(&gpio10);
	vTaskDelay(10);

	if (mux1)
		PIO_Set(&gpio11);
	else
		PIO_Clear(&gpio11);
	vTaskDelay(10);

	unsigned short adcSamples[8];
	int error = ADC_SingleShot(adcSamples);
	if (error != 0)
	{
		return -1;
	}

	printf("ADC Samples: ");
	for (int i = 0; i < 8; i++)
	{
		adcSamples[i] = ADC_ConvertRaw10bitToMillivolt(adcSamples[i]);
		printf("%u, ", adcSamples[i]);
	}
	printf("\n\n");

	return adcSamples[6];
}

int gecko_get_current_3v3()
{
	gecko_power_mux_init();
	int ret = CURRENT_3V3_CONVERSION (gecko_power_mux_get(FALSE, FALSE) );
	gecko_power_mux_deinit();
	return ret;
}
int gecko_get_voltage_3v3()
{
	gecko_power_mux_init();
	int ret = VOLTAGE_3V3_CONVERSION( gecko_power_mux_get(FALSE, TRUE) );
	gecko_power_mux_deinit();
	return ret;
}
int gecko_get_current_5v()
{
	gecko_power_mux_init();
	int ret = CURRENT_5V_CONVERSION( gecko_power_mux_get(TRUE, FALSE) );
	gecko_power_mux_deinit();
	return ret;
}
int gecko_get_voltage_5v()
{
	gecko_power_mux_init();
	int ret = VOLTAGE_5V_CONVERSION (gecko_power_mux_get(TRUE, TRUE) );
	gecko_power_mux_deinit();
	return ret;
}

Boolean TurnOnGecko()
{
	printf("turning camera on\n");
	Pin gpio4 = PIN_GPIO04;
	Pin gpio5 = PIN_GPIO05;
	Pin gpio6 = PIN_GPIO06_INPUT;
	Pin gpio7 = PIN_GPIO07_INPUT;

	PIO_Configure(&gpio4, 1);/*PIO_LISTSIZE(&gpio4));*/
	vTaskDelay(10);
	PIO_Configure(&gpio5, 1);/*PIO_LISTSIZE(&gpio5));*/
	vTaskDelay(10);
	PIO_Configure(&gpio6, 1);/*PIO_LISTSIZE(&gpio6));*/
	vTaskDelay(10);
	PIO_Configure(&gpio7, 1);/*PIO_LISTSIZE(&gpio7));*/
	vTaskDelay(10);

	PIO_Set(&gpio4);
	vTaskDelay(10);
	PIO_Set(&gpio5);
	vTaskDelay(10);
	int gpio6_get = PIO_Get(&gpio6);
	vTaskDelay(10);
	int gpio7_get = PIO_Get(&gpio7);
	vTaskDelay(10);

	if (gpio6_get != 1 || gpio7_get != 1)
	{
		return FALSE;
	}

	Initialized_GPIO();

	printf("\n\ncurrent_3v3 = (%d), voltage_3v3 = (%d), current_5v = (%d), voltage_5v = (%d), \n\n",
			gecko_get_current_3v3(), gecko_get_voltage_3v3(), gecko_get_current_5v(), gecko_get_voltage_5v());

	return TRUE;
}
Boolean TurnOffGecko()
{
	printf("turning camera off\n");
	Pin gpio4 = PIN_GPIO05;
	Pin gpio5 = PIN_GPIO07;
	Pin gpio6 = PIN_GPIO06_INPUT;
	Pin gpio7 = PIN_GPIO07_INPUT;

	PIO_Clear(&gpio4);
	vTaskDelay(10);
	PIO_Clear(&gpio5);
	vTaskDelay(10);
	PIO_Clear(&gpio6);
	vTaskDelay(10);
	PIO_Clear(&gpio7);
	vTaskDelay(10);

	De_Initialized_GPIO();

	return TRUE;
}

Boolean getPIOs()
{
	Pin gpio4 = PIN_GPIO05;
	Pin gpio5 = PIN_GPIO07;
	Pin gpio6 = PIN_GPIO06_INPUT;
	Pin gpio7 = PIN_GPIO07_INPUT;
	Pin gpio12 = PIN_GPIO12;

	char output;

	output = PIO_Get(&gpio4);
	printf("gpio get 4 (%d)\n", output);
	output = PIO_Get(&gpio5);
	printf("gpio get 5 (%d)\n", output);
	output = PIO_Get(&gpio6);
	printf("gpio get 6 (%d)\n", output);
	output = PIO_Get(&gpio7);
	printf("gpio get 7 (%d)\n", output);
	output = PIO_Get(&gpio12);
	printf("gpio get 12 (%d)\n", output);

	return TRUE;
}

int initGecko()
{
	return GECKO_Init( (SPIslaveParameters){ bus1_spi, mode0_spi, slave1_spi, 100, 1, _SPI_GECKO_BUS_SPEED, 0 } );
}

int GECKO_TakeImage( uint8_t adcGain, uint8_t pgaGain, uint16_t sensorOffset, uint32_t exposure, uint32_t frameAmount, uint32_t frameRate, uint32_t imageID, Boolean testPattern)
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
	result = GECKO_SetOffset(sensorOffset);
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

int readStopFlag(Boolean8bit* stop_flag)
{
	int result = FRAM_read_exte(stop_flag, GECKO_STOP_TRANSFER_FLAG_ADDR, GECKO_STOP_TRANSFER_FLAG_SIZE);
	Result(result, -9);
	return 0;
}
int setStopFlag(Boolean8bit stop_flag)
{
	int result = FRAM_write_exte(&stop_flag, GECKO_STOP_TRANSFER_FLAG_ADDR, GECKO_STOP_TRANSFER_FLAG_SIZE);
	Result(result, -1);
	return 0;
}

int GECKO_ReadImage(uint32_t imageID, uint32_t *buffer)
{
	Boolean8bit stop_flag = FALSE_8BIT;

	int result, i = 0;
	result = GomEpsResetWDT(0);

	// Init Flash:
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

		// Printing a value one every 40000 pixels:
		if(i % READ_DELAY_INDEXES == 0)
		{
			printf("%u, %u\n", i, (uint8_t)*(buffer + i));

			vTaskDelay(SYSTEM_DEALY);

			result = readStopFlag(&stop_flag);
			if (stop_flag)
			{
				setStopFlag(FALSE_8BIT);
				return -10;
			}
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
