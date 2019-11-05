/*
 * main.c
 *      Author: Akhil
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <satellite-subsystems/version/version.h>

#include <at91/utility/exithandler.h>
#include <at91/commons.h>
#include <at91/utility/trace.h>
#include <at91/peripherals/cp15/cp15.h>


#include <hal/Timing/WatchDogTimer.h>
#include <hal/Timing/Time.h>
#include <hal/Drivers/LED.h>
#include <hal/Drivers/I2C.h>
#include <hal/Drivers/SPI.h>
#include <hal/boolean.h>
#include <hal/version/version.h>

#include <hal/Storage/FRAM.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sub-systemCode/COMM/GSC.h"
#include "sub-systemCode/Main/commands.h"
#include "sub-systemCode/Main/Main_Init.h"
#include "sub-systemCode/Global/Global.h"
#include "sub-systemCode/EPS.h"

#include "sub-systemCode/Main/CMD/ADCS_CMD.h"
#include "sub-systemCode/ADCS/AdcsMain.h"
#include "sub-systemCode/ADCS/AdcsCommands.h"
#include "sub-systemCode/Global/OnlineTM.h"
#include "sub-systemCode/Ants.h"

#include "sub-systemCode/payload/Drivers/GeckoCameraDriver.h"

#define DEBUGMODE

#ifndef DEBUGMODE
	#define DEBUGMODE
#endif

void save_time()
{
	int err;
	// get current time from time module
	time_unix current_time;
	err = Time_getUnixEpoch(&current_time);
	check_int("set time, Time_getUnixEpoch", err);
	byte raw_time[TIME_SIZE];
	// converting the time_unix to raw (small endien)
	raw_time[0] = (byte)(current_time);
	raw_time[1] = (byte)(current_time >> 8);
	raw_time[2] = (byte)(current_time >> 16);
	raw_time[3] = (byte)(current_time >> 24);
	// writing to FRAM the current time
	err = FRAM_write_exte(raw_time, TIME_ADDR, TIME_SIZE);
	check_int("set time, FRAM_write", err);
}

void Command_logic()
{
	TC_spl command;
	int error = 0;
	do
	{
		error = get_command(&command);
		if (error == 0)
			act_upon_command(command);
	}
	while (error == 0);
}


static void EPS_TelemetryHKGeneral(void)
{
	gom_eps_hk_t myEpsTelemetry_hk;

	printf("\r\nEPS Telemetry HK General \r\n\n");
	printf("driver error:%d\n", GomEpsGetHkData_general(0, &myEpsTelemetry_hk));

	printf("Voltage of boost converters PV1 = %d mV\r\n", myEpsTelemetry_hk.fields.vboost[0]);
	printf("Voltage of boost converters PV2 = %d mV\r\n", myEpsTelemetry_hk.fields.vboost[1]);
	printf("Voltage of boost converters PV3 = %d mV\r\n", myEpsTelemetry_hk.fields.vboost[2]);

	printf("Voltage of the battery = %d mV\r\n", myEpsTelemetry_hk.fields.vbatt);

	printf("Current inputs current 1 = %d mA\r\n", myEpsTelemetry_hk.fields.curin[0]);
	printf("Current inputs current 2 = %d mA\r\n", myEpsTelemetry_hk.fields.curin[1]);
	printf("Current inputs current 3 = %d mA\r\n", myEpsTelemetry_hk.fields.curin[2]);

	printf("Current from boost converters = %d mA\r\n", myEpsTelemetry_hk.fields.cursun);
	printf("Current out of the battery = %d mA\r\n", myEpsTelemetry_hk.fields.cursys);

	printf("Current outputs current 1 = %d mA\r\n", myEpsTelemetry_hk.fields.curout[0]);
	printf("Current outputs current 2 = %d mA\r\n", myEpsTelemetry_hk.fields.curout[1]);
	printf("Current outputs current 3 = %d mA\r\n", myEpsTelemetry_hk.fields.curout[2]);
	printf("Current outputs current 4 = %d mA\r\n", myEpsTelemetry_hk.fields.curout[3]);
	printf("Current outputs current 5 = %d mA\r\n", myEpsTelemetry_hk.fields.curout[4]);
	printf("Current outputs current 6 = %d mA\r\n", myEpsTelemetry_hk.fields.curout[5]);

	printf("Output Status 1 = %d \r\n", myEpsTelemetry_hk.fields.output[0]);
	printf("Output Status 2 = %d \r\n", myEpsTelemetry_hk.fields.output[1]);
	printf("Output Status 3 = %d \r\n", myEpsTelemetry_hk.fields.output[2]);
	printf("Output Status 4 = %d \r\n", myEpsTelemetry_hk.fields.output[3]);
	printf("Output Status 5 = %d \r\n", myEpsTelemetry_hk.fields.output[4]);
	printf("Output Status 6 = %d \r\n", myEpsTelemetry_hk.fields.output[5]);
	printf("Output Status 7 = %d \r\n", myEpsTelemetry_hk.fields.output[6]);
	printf("Output Status 8 = %d \r\n", myEpsTelemetry_hk.fields.output[7]);

	printf("Output Time until Power is on 1 = %d \r\n", myEpsTelemetry_hk.fields.output_on_delta[0]);
	printf("Output Time until Power is on 2 = %d \r\n", myEpsTelemetry_hk.fields.output_on_delta[1]);
	printf("Output Time until Power is on 3 = %d \r\n", myEpsTelemetry_hk.fields.output_on_delta[2]);
	printf("Output Time until Power is on 4 = %d \r\n", myEpsTelemetry_hk.fields.output_on_delta[3]);
	printf("Output Time until Power is on 5 = %d \r\n", myEpsTelemetry_hk.fields.output_on_delta[4]);
	printf("Output Time until Power is on 6 = %d \r\n", myEpsTelemetry_hk.fields.output_on_delta[5]);
	printf("Output Time until Power is on 7 = %d \r\n", myEpsTelemetry_hk.fields.output_on_delta[6]);
	printf("Output Time until Power is on 8 = %d \r\n", myEpsTelemetry_hk.fields.output_on_delta[7]);

	printf("Output Time until Power is off 1 = %d \r\n", myEpsTelemetry_hk.fields.output_off_delta[0]);
	printf("Output Time until Power is off 2 = %d \r\n", myEpsTelemetry_hk.fields.output_off_delta[1]);
	printf("Output Time until Power is off 3 = %d \r\n", myEpsTelemetry_hk.fields.output_off_delta[2]);
	printf("Output Time until Power is off 4 = %d \r\n", myEpsTelemetry_hk.fields.output_off_delta[3]);
	printf("Output Time until Power is off 5 = %d \r\n", myEpsTelemetry_hk.fields.output_off_delta[4]);
	printf("Output Time until Power is off 6 = %d \r\n", myEpsTelemetry_hk.fields.output_off_delta[5]);
	printf("Output Time until Power is off 7 = %d \r\n", myEpsTelemetry_hk.fields.output_off_delta[6]);
	printf("Output Time until Power is off 8 = %d \r\n", myEpsTelemetry_hk.fields.output_off_delta[7]);

	printf("Number of latch-ups 1 = %d \r\n", myEpsTelemetry_hk.fields.latchup[0]);
	printf("Number of latch-ups 2 = %d \r\n", myEpsTelemetry_hk.fields.latchup[1]);
	printf("Number of latch-ups 3 = %d \r\n", myEpsTelemetry_hk.fields.latchup[2]);
	printf("Number of latch-ups 4 = %d \r\n", myEpsTelemetry_hk.fields.latchup[3]);
	printf("Number of latch-ups 5 = %d \r\n", myEpsTelemetry_hk.fields.latchup[4]);
	printf("Number of latch-ups 6 = %d \r\n", myEpsTelemetry_hk.fields.latchup[5]);

	printf("Time left on I2C WDT = %d \r\n", myEpsTelemetry_hk.fields.wdt_i2c_time_left);
	printf("Time left on WDT GND = %d \r\n", myEpsTelemetry_hk.fields. wdt_gnd_time_left);
	printf("Time left on I2C WDT CSP ping 1 = %d \r\n", myEpsTelemetry_hk.fields.wdt_csp_pings_left[0]);
	printf("Time left on I2C WDT CSP ping 2 = %d \r\n", myEpsTelemetry_hk.fields.wdt_csp_pings_left[1]);

	printf("Number of I2C WD reboots = %d\r\n", myEpsTelemetry_hk.fields.counter_wdt_i2c);
	printf("Number of WDT GND reboots = %d\r\n", myEpsTelemetry_hk.fields.counter_wdt_gnd);
	printf("Number of WDT CSP ping 1 reboots = %d\r\n", myEpsTelemetry_hk.fields.counter_wdt_csp[0]);
	printf("Number of WDT CSP ping 2 reboots = %d\r\n", myEpsTelemetry_hk.fields.counter_wdt_csp[0]);
	printf("Number of EPS reboots = %d\r\n", myEpsTelemetry_hk.fields.counter_boot);

	printf("Temperature sensors. TEMP1 = %d \r\n", myEpsTelemetry_hk.fields.temp[0]);
	printf("Temperature sensors. TEMP2 = %d \r\n", myEpsTelemetry_hk.fields.temp[1]);
	printf("Temperature sensors. TEMP3 = %d \r\n", myEpsTelemetry_hk.fields.temp[2]);
	printf("Temperature sensors. TEMP4 = %d \r\n", myEpsTelemetry_hk.fields.temp[3]);
	printf("Temperature sensors. BATT0 = %d \r\n", myEpsTelemetry_hk.fields.temp[4]);
	printf("Temperature sensors. BATT1 = %d \r\n", myEpsTelemetry_hk.fields.temp[5]);

	printf("Cause of last EPS reset = %d\r\n", myEpsTelemetry_hk.fields.bootcause);
	printf("Battery Mode = %d\r\n", myEpsTelemetry_hk.fields.battmode);
	printf("PPT tracker Mode = %d\r\n", myEpsTelemetry_hk.fields.pptmode);
	printf(" \r\n");
}

static void EPS_SetOutputOff(void)
{
	gom_eps_channelstates_t var;
	var.raw = 0;
	int i_error = GomEpsSetOutput(0, var);
	printf("error value: %d\n", i_error);
}

static void EPS_SetOutputOn(void)
{
	gom_eps_channelstates_t var;
	var.raw = 0;
	var.fields.channel3V3_1 = 1;
	var.fields.channel5V_1 = 1;
	int i_error = GomEpsSetOutput(0, var);
	printf("error value: %d\n", i_error);
}


static void sendDefClSignTest(void)
{
	//Buffers and variables definition
	unsigned char testBuffer1[10]  = {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x40};
	unsigned char txCounter = 0;
	unsigned char avalFrames = 0;
	unsigned int timeoutCounter = 0;

	while(txCounter < 5 && timeoutCounter < 5)
	{
		printf("\r\n Transmission of single buffers with default callsign. AX25 Format. \r\n");
		printf("driver error:%d\n", IsisTrxvu_tcSendAX25DefClSign(0, testBuffer1, 10, &avalFrames));

		if ((avalFrames != 0)&&(avalFrames != 255))
		{
			printf("\r\n Number of frames in the buffer: %d  \r\n", avalFrames);
			txCounter++;
		}
		else
		{
			vTaskDelay(100 / portTICK_RATE_MS);
			timeoutCounter++;
		}
	}
}

static void vutc_getTxTelemTest_revC(void)
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

static void vurc_getRxTelemTest_revC(void)
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


static void eslADC_ADCSGenInfo(void)
{
	cspace_adcs_geninfo_t info_data;

	cspaceADCS_getGeneralInfo(0, &info_data);

	printf("Note type: %d \r\n", info_data.fields.node_type);
	printf("Version Interface: %d \r\n", info_data.fields.version_interface);
	printf("Version major: %d \r\n", info_data.fields.version_major);
	printf("Version minor: %d \r\n", info_data.fields.version_minor);
	printf("Uptime secs: %d \r\n", info_data.fields.uptime_secs);
	printf("Uptime millisecs: %d \r\n", info_data.fields.uptime_millisecs);
}


static void SD_space(void)
{
	FS_HK hk;
	int error = FS_HK_collect(&hk, 0);
	printf("error from driver: %d", error);
	printf("bad space: %u\n", hk.fields.bad);
	printf("free space: %u\n", hk.fields.free);
	printf("total space: %u\n", hk.fields.total);
	printf("used space: %u\n", hk.fields.used);
}

void taskMain()
{
	WDT_startWatchdogKickTask(10 / portTICK_RATE_MS, FALSE);

	InitSubsystems();

	vTaskDelay(100);

	portTickType xLastWakeTime = xTaskGetTickCount();
	const portTickType xFrequency = 1000;
	
	unsigned int selection = 0;
	while(1)
	{
		printf("\n\n(0) EPS - print TM 2\n\r");
		printf("(1) EPS - turn on ADCS channels\n\r");
		printf("(2) EPS - turn off ADCS channels\n\r");
		printf("(3) TRXVU - transmmit 5 AX.25 packet\n\r");
		printf("(4) TRXVU - print TLM\n\r");
		printf("(5) ADCS - print UNIX time\n\r");
		printf("(6) GECKO - turn on camera\n\r");
		printf("(7) GECKO - turn off camera\n\r");
		printf("(8) GECKO - print 5v & 3v3 values (daughterboard - mux)\n\r");
		printf("(9) SD - print space\n\r\n\n");

		while(UTIL_DbguGetIntegerMinMax(&selection, 0, 9) == 0);

		switch (selection)
		{
		case 0:
			EPS_TelemetryHKGeneral();
			break;
		case 1:
			EPS_SetOutputOn();
			break;
		case 2:
			EPS_SetOutputOff();
			break;
		case 3:
			sendDefClSignTest();
			break;
		case 4:
			vutc_getTxTelemTest_revC();
			vurc_getRxTelemTest_revC();
			break;
		case 5:
			eslADC_ADCSGenInfo();
			break;
		case 6:
			TurnOnGecko();
			break;
		case 7:
			TurnOffGecko();
			break;
		case 8:
			printf("Gecko Daughterboard: 3v3 current = %u, 3v3 voltage = %u, 5v current = %u, 5v voltage = %u. (Note: 5v might be 0)",
					gecko_get_current_3v3(), gecko_get_voltage_3v3(), gecko_get_current_5v(), gecko_get_voltage_5v());
			break;
		case 9:
			SD_space();
			break;
		}
	}
}

int main()
{
	TRACE_CONFIGURE_ISP(DBGU_STANDARD, 2000000, BOARD_MCK);
	// Enable the Instruction cache of the ARM9 core. Keep the MMU and Data Cache disabled.
	CP15_Enable_I_Cache();

	// The actual watchdog is already started, this only initializes the watchdog-kick interface.
	WDT_start();

	xTaskGenericCreate(taskMain, (const signed char *)("taskMain"), 8196, NULL, configMAX_PRIORITIES - 2, NULL, NULL, NULL);
	vTaskStartScheduler();
	while(1){
		printf("should not be here\n");
		vTaskDelay(2000);
	}

	return 0;
}
