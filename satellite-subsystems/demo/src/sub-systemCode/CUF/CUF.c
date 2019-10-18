/*
 * CUF.c
 *
 *  Created on: 15 áðåá 2018
 *      Author: USER1
 */

///////////////////////////////////////////////////Code Upload File Requirements////////////////////////////////////////////////////////

//////////////////////includes//////////////////////

#include "CUF.h"
#include "../Global/Global.h"
#include "../Global/logger.h"
#include "../EPS.h"

#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <hal/drivers/I2C.h>
#include <hal/storage/FRAM.h>
#include <hcc/api_fat.h>

#include <stdlib.h>
#include <string.h>
#include "../../hal/include/hal/boolean.h"
//////////////////////defines///////////////////////

#define VTASKDELAY_CUF(xTicksToDelay)((void(*)(portTickType))(CUFSwitch(0)))(xTicksToDelay)

////////////////////////END/////////////////////////


static CUF* PerminantCUF[PERMINANTCUFLENGTH]; //the array that accumilates perminant CUFs (CUF slot array) (CUFs to run on restart)
static CUF* TempCUF[PERMINANTCUFLENGTH]; //the array that accumilates temporary CUFs
static void* CUFARR[3000]; //the array that accumilates the CUF functions
int /*compile*/CUFTestFunction(void* (*CUFSwitch)(int))
{
	VTASKDELAY_CUF(5000);
	return CUFTestFunction2(5) + 1;
}

int /*compile*/CUFTestFunction2(int index)
{
	return index + 1;
}

unsigned int castCharPointerToInt(unsigned char* pointer)
{
	unsigned int ret = *pointer;
	ret += pointer[1]<<8;
	ret += pointer[2]<<16;
	ret += pointer[3]<<24;
	return ret;
}

unsigned int* castCharArrayToIntArray(unsigned char* array, int length)
{
	int i;
	unsigned int* ret = malloc(length);
	for (i = 0; i < length / 4; i++)
		ret[i] = castCharPointerToInt(array + i*4);
	return ret;
}

int SavePerminantCUF()
{
	int i = 0;
	f_delete(PERCUFSAVEFILENAME);
	F_FILE* PerminantCUFData = NULL;
	if (f_managed_open(PERCUFSAVEFILENAME, "w+", &PerminantCUFData) != 0)
	{
		WriteErrorLog(CUF_SAVE_PERMINANTE_FAIL, SYSTEM_CUF, 0);
		return -1; //return -1 on fail
	}
	//write all data members of all perminant CUFs
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (PerminantCUF[i]->length == 0)
			break;
		f_write(&PerminantCUF[i]->length, sizeof(unsigned int), 1, PerminantCUFData);
		f_write(PerminantCUF[i]->data, sizeof(int), PerminantCUF[i]->length, PerminantCUFData);
		f_write(&PerminantCUF[i]->isTemporary, sizeof(Boolean), 1, PerminantCUFData);
		f_write(&PerminantCUF[i]->SSH, sizeof(unsigned long), 1, PerminantCUFData);
		f_write(PerminantCUF[i]->name, sizeof(char), CUFNAMELENGTH, PerminantCUFData);
		f_write(&PerminantCUF[i]->disabled, sizeof(Boolean), 1, PerminantCUFData);
	}
	f_managed_close(&PerminantCUFData);
	return 0; //retun 0 on success
}

int LoadPerminantCUF()
{
	int i = 0;
	F_FILE* PerminantCUFData = NULL;
	if (f_managed_open(PERCUFSAVEFILENAME, "r", &PerminantCUFData) != 0)
	{
		WriteErrorLog(CUF_LOAD_PERMINANTE_FAIL, SYSTEM_CUF, 0);
		return -1; //return -1 on fail
	}
	//read all data members of all perminant CUFs
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (f_read(&PerminantCUF[i]->length, sizeof(unsigned int), 1, PerminantCUFData) == 0)
		{
			PerminantCUF[i]->length = 0;
			break;
		}
		f_read(PerminantCUF[i]->data, sizeof(int), PerminantCUF[i]->length, PerminantCUFData);
		f_read(&PerminantCUF[i]->isTemporary, sizeof(Boolean), 1, PerminantCUFData);
		f_read(&PerminantCUF[i]->SSH, sizeof(unsigned long), 1, PerminantCUFData);
		f_read(PerminantCUF[i]->name, sizeof(char), CUFNAMELENGTH, PerminantCUFData);
		f_read(&PerminantCUF[i]->disabled, sizeof(Boolean), 1, PerminantCUFData);
	}
	f_close(PerminantCUFData);
	return 0; //return 0 on success
}

Boolean CheckResetThreashold(Boolean isFirst)
{
	gom_eps_hk_wdt_t resets;
	int gret = GomEpsGetHkData_wdt(EPS_I2C_ADDR, &resets);
	if (isFirst)
	{
		FRAM_write_exte((unsigned char*)&resets, CUFRAMADRESS, sizeof(gom_eps_hk_wdt_t));
		return TRUE;
	}
	gom_eps_hk_wdt_t savedresets;
	int fret = FRAM_read_exte((unsigned char*)&savedresets, CUFRAMADRESS, sizeof(gom_eps_hk_wdt_t));
	if (fret != 0 || gret != 0)
		return TRUE;
	if ((resets.fields.counter_wdt_i2c - savedresets.fields.counter_wdt_i2c) % RESETSTHRESHOLD == 0 || (resets.fields.counter_wdt_gnd - savedresets.fields.counter_wdt_gnd) % RESETSTHRESHOLD == 0 || (resets.fields.counter_wdt_csp[0] - savedresets.fields.counter_wdt_csp[0]) % RESETSTHRESHOLD == 0 || (resets.fields.counter_wdt_csp[1] - savedresets.fields.counter_wdt_csp[1]) % RESETSTHRESHOLD == 0)
		return FALSE;
	return TRUE;
}

int CUFManageRestart(Boolean isFirst)
{
	WriteResetLog(SYSTEM_CUF, 0);
	if (isFirst)
	{
		f_delete(PERCUFSAVEFILENAME);
		F_FIND find;
		if (!f_findfirst("A:/*.*",&find))
		{
			do
			{
				if (strstr(find.filename, "cuf") != NULL || strstr(find.filename, "CUF") != NULL)
					f_delete(find.filename);
				vTaskDelay(1);

			} while (!f_findnext(&find));
		}
	}
	InitCUF(); //initiate the CUF switch array
	LoadPerminantCUF();
	int i = 0;
	if (!CheckResetThreashold(isFirst)) //check for reset threshold
	{
		WriteCUFLog(CUF_RESET_THRESHOLD_MET, 0);
		for (i = 0; i < PERMINANTCUFLENGTH; i++)
			if (PerminantCUF[i]->length != 0)
				PerminantCUF[i]->disabled = TRUE;
		return -1; //return -1 on reset threshold met
	}
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
		if (PerminantCUF[i]->length != 0 && !PerminantCUF[i]->disabled) //check if CUF slot is empty
			ExecuteCUF(PerminantCUF[i]->name); //run CUF in slot if not empty
	WriteCUFLog(CUF_RESET_SUCCESSFULL, 0);
	return 0; //return 0 on success
}
void funcsPointerToArr();

int InitCUF()
{
	funcsPointerToArr();
	//reset the CUF slot array if first run
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; ++i)
	{
		TempCUF[i] = malloc(sizeof(CUF));
		TempCUF[i]->data = NULL;
		TempCUF[i]->name[0] = 0;
		TempCUF[i]->SSH = 0;
		TempCUF[i]->length = 0;
		PerminantCUF[i] = malloc(sizeof(CUF));
		PerminantCUF[i]->data = NULL;
		PerminantCUF[i]->name[0] = 0;
		PerminantCUF[i]->SSH = 0;
		PerminantCUF[i]->length = 0;
	}
	return 0; //return 0 on success
}

void* CUFSwitch(int index)
{
	return CUFARR[index]; //return the function at index
}

unsigned int GenerateSSH(char* data, int len)
{
	int hash = 0, i = 0;
	for (hash = len, i = 0; i < len; ++i)
		hash = (hash << 4) ^ (hash >> 28) ^ data[i];
	if (hash == 0 || i == 0)
		WriteErrorLog(CUF_GENERATE_SSH_FAIL, SYSTEM_CUF, 0);
	return hash; //return the hash
}

char* getExtendedName(char* name)
{
	char* extended = malloc(13);
	for (int i = 0; i < 8; i++)
		extended[i] = name[i];
	extended[8] = '.';
	extended[9] = 'c';
	extended[10] = 'u';
	extended[11] = 'f';
	extended[12] = 0;
	return extended;
}

int AuthenticateCUF(CUF* code)
{
	unsigned int newSSH = GenerateSSH((char*)code->data, code->length*4); //generate an ssh for the data
	if (newSSH == 0) //check generation
	{
		WriteErrorLog(CUF_AUTHENTICATE_FAIL, SYSTEM_CUF, 0);
		return -1; //return -1 on fail
	}
	if (newSSH == code->SSH) //authenticate the ssh with the pre-made one
		return 1; //return 1 if authenticated
	else
		return 0; //return 0 if not authenticated
	WriteErrorLog(CUF_AUTHENTICATE_FAIL, SYSTEM_CUF, 0);
	return -1; //return -1 on fail
}

int UpdatePerminantCUF(CUF* code)
{
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; i++) //do for every slot in CUF slot array
	{
		if (PerminantCUF[i]->length == 0) //check if slot is empty
		{
			//add new CUF to first empty slot
			PerminantCUF[i] = code;
			SavePerminantCUF();
			return 0; //return 0 on success
		}
	}
	WriteErrorLog(CUF_UPDATE_PERMINANTE_FAIL, SYSTEM_CUF, 0);
	return -3; //return -3 if failed
}

int RemoveCUF(char* name)
{
	name = getExtendedName(name);
	int i = 0;
	CUF* code = GetFromCUFTempArray(name);
	f_delete(code->name); //delete the CUF
	if (code->isTemporary == FALSE)
	{
		//remove it from the CUF slot array if not temporary
		for (i = 0; i < PERMINANTCUFLENGTH; i++)
		{
			if (strcmp(PerminantCUF[i]->name, code->name) == 0) //find the slot with the CUF to remove
			{
				//remove CUF
				PerminantCUF[i]->data = NULL;
				PerminantCUF[i]->name[0] = 0;
				PerminantCUF[i]->SSH = 0;
				PerminantCUF[i]->length = 0;
				SavePerminantCUF();
				break;
			}
		}
	}
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (strcmp(TempCUF[i]->name, code->name) == 0) //find the slot with the CUF to remove
		{
			//remove CUF from temp array
			TempCUF[i]->data = NULL;
			TempCUF[i]->name[0] = 0;
			TempCUF[i]->SSH = 0;
			TempCUF[i]->length = 0;
			return 0; //return 0 on success
		}
	}
	WriteCUFLog(CUF_REMOVED, 0);
	return 0; //return 0 on success
}

int ExecuteCUF(char* name)
{
	name = getExtendedName(name);
	CUF* code = GetFromCUFTempArray(name);
	if (code == NULL)
		return -200;
	unsigned char* CUFDataStorage = (unsigned char*) code->data;
	int (*CUFFunction)(void* (*CUFSwitch)(int)) = (int (*)(void* (*CUFSwitch)(int))) CUFDataStorage; //convert the CUF data to a function
	if (CUFFunction == NULL)
	{
		WriteErrorLog(CUF_EXECUTE_FAIL, SYSTEM_CUF, 0);
		return -200; //return -200 on fail
	}
	int auth = AuthenticateCUF(code);
	if (auth != 1)
	{
		WriteCUFLog(CUF_EXECUTE_unauthenticated, 0);
		return -300; //return -300 on un-authenticated
	}
	int ret = CUFFunction(CUFSwitch); //call the CUF function with the test function as its parameter and return its output
	WriteCUFLog(CUF_EXECUTED, ret);
	return ret;
}

int IntegrateCUF(char* name, int* data, unsigned int length, unsigned long SSH, Boolean isTemporary, Boolean disabled)
{
	CUF* cuf = malloc(sizeof(CUF));
	strcpy(cuf->name, getExtendedName(name));
	cuf->data = data;
	cuf->length = length;
	cuf->SSH = SSH;
	cuf->isTemporary = isTemporary;
	cuf->disabled = disabled;
	if (cuf->SSH == 0 || cuf->length == 0 || cuf->data == NULL || cuf->length == 0)
	{
		WriteErrorLog(CUF_INTEGRATED_FAIL, SYSTEM_CUF, 0);
		return -1; //return -1 on fail
	}
	int auth = AuthenticateCUF(cuf);
	//check SSH
	if (auth == -1)
	{
		WriteErrorLog(CUF_INTEGRATED_FAIL, SYSTEM_CUF, 0);
		return -1; //return -1 on fail
	}
	else if (auth == 0)
	{
		WriteCUFLog(CUF_INTEGRATED_unauthenticated, 0);
		return -2; //return -2 on un-authenticated
	}
	AddToCUFTempArray(cuf);
	if (cuf->isTemporary == FALSE)
		return UpdatePerminantCUF(cuf);
	WriteCUFLog(CUF_INTEGRATED, 0);
	return 0;
}

void AddToCUFTempArray(CUF* temp)
{
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (TempCUF[i]->length == 0 || strcmp(TempCUF[i]->name, temp->name) == 0)
		{
			TempCUF[i] = temp;
			return;
		}
	}
}

CUF* GetFromCUFTempArray(char* name)
{
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (strcmp(TempCUF[i]->name, name) == 0)
		{
			return TempCUF[i];
		}
	}
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (strcmp(PerminantCUF[i]->name, name) == 0)
		{
			return PerminantCUF[i];
		}
	}
	return NULL;
}

void DisableCUF(char* name)
{
	name = getExtendedName(name);
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (strcmp(PerminantCUF[i]->name, name) == 0)
		{
			PerminantCUF[i]->disabled = TRUE;
			SavePerminantCUF();
			WriteCUFLog(CUF_DISABLED, 0);
			return;
		}
	}
	WriteErrorLog(CUF_DISABLE_FAIL, SYSTEM_CUF, 0);
}

void EnableCUF(char* name)
{
	name = getExtendedName(name);
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (strcmp(PerminantCUF[i]->name, name) == 0)
		{
			PerminantCUF[i]->disabled = FALSE;
			SavePerminantCUF();
			WriteCUFLog(CUF_ENABLED, 0);
			return;
		}
	}
	WriteErrorLog(CUF_ENABLE_FAIL, SYSTEM_CUF, 0);
}

Boolean CUFTest(int* testCUFData)
{
	int i;
	if (InitCUF(TRUE) != 0) //initiate the CUF architecture and check fail
		return TRUE; //return TRUE on fail
	int testCUFDataCheck[] = { 0xe92d4800, 0xe28db004, 0xe24dd008, 0xe50b0008, 0xe51b3008, 0xe3a00000, 0xe12fff33, 0xe1a03000, 0xe59f001c, 0xe12fff33, 0xe3a00005, 0xeb000005, 0xe1a03000, 0xe2833001, 0xe1a00003, 0xe24bd004, 0xe8bd8800, 0x00001388, 0xe52db004, 0xe28db000, 0xe24dd00c, 0xe50b0008, 0xe51b3008, 0xe2833001, 0xe1a00003, 0xe28bd000, 0xe8bd0800, 0xe12fff1e }; //data for CUF test file
	for (i = 0; i < 28; i++)
	{
		if (testCUFDataCheck[i] != testCUFData[i])
		{
			//printf("%d, %d, %d\n", testCUFDataCheck[i], testCUFData[i], i);
		}
	}
	int intret = IntegrateCUF("CUF001.cuf", testCUFData, 28, GenerateSSH((char*)testCUFData, 112), TRUE, FALSE);
	CUF* testFile = GetFromCUFTempArray("CUF001.cuf");
	if (intret != 0 || testFile == NULL)
	{
		//printf("Failed To Integrate CUF\n");
		return TRUE; //return TRUE on fail
	}
	int CUFreturnData = ExecuteCUF(testFile->name); //execute the test CUF
	if (CUFreturnData == -2) //check fail
	{
		//printf("Failed To Execute CUF\n");
		return TRUE; //return TRUE on fail
	}
	//printf("CUF Executed: ");
	//printf("%d\n", CUFreturnData); //print the test CUFs return data
	if (RemoveCUF(testFile->name) == -1) //remove the CUF once done and check fail
	{
		//printf("Failed To Remove CUF\n");
		return TRUE; //return TRUE on fail
	}
	//printf("CUF Removed\n");
	return TRUE; //return TRUE on success
}


#include "../../../hal/at91/include/at91/commons.h"
#include "../../../hal/at91/include/at91/boards/ISIS_OBC_G20/board.h"
#include "../../../hal/at91/include/at91/boards/ISIS_OBC_G20/board_lowlevel.h"
#include "../../../hal/at91/include/at91/boards/ISIS_OBC_G20/board_memories.h"
#include "../../../hal/at91/include/at91/boards/ISIS_OBC_G20/at91sam9g20/AT91SAM9G20.h"
#include "../../../hal/at91/include/at91/memories/norflash/NorFlashAmd.h"
#include "../../../hal/at91/include/at91/memories/norflash/NorFlashApi.h"
#include "../../../hal/at91/include/at91/memories/norflash/NorFlashCFI.h"
#include "../../../hal/at91/include/at91/memories/norflash/NorFlashCommon.h"
#include "../../../hal/at91/include/at91/memories/norflash/NorFlashIntel.h"
#include "../../../hal/at91/include/at91/memories/sdmmc/sdmmc_mci.h"
#include "../../../hal/at91/include/at91/peripherals/adc/adc_at91.h"
#include "../../../hal/at91/include/at91/peripherals/aic/aic.h"
#include "../../../hal/at91/include/at91/peripherals/cp15/cp15.h"
#include "../../../hal/at91/include/at91/peripherals/dbgu/dbgu.h"
//#include "../../../hal/at91/include/at91/peripherals/isi/isi.h"
#include "../../../hal/at91/include/at91/peripherals/mci/mci.h"
#include "../../../hal/at91/include/at91/peripherals/pio/pio.h"
#include "../../../hal/at91/include/at91/peripherals/pio/pio_it.h"
#include "../../../hal/at91/include/at91/peripherals/pit/pit.h"
#include "../../../hal/at91/include/at91/peripherals/pmc/pmc.h"
#include "../../../hal/at91/include/at91/peripherals/rstc/rstc.h"
#include "../../../hal/at91/include/at91/peripherals/spi/spi_at91.h"
#include "../../../hal/at91/include/at91/peripherals/tc/tc.h"
#include "../../../hal/at91/include/at91/peripherals/twi/twi_at91.h"
#include "../../../hal/at91/include/at91/peripherals/usart/usart_at91.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBConfigurationDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBDeviceDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBDeviceQualifierDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBEndpointDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBFeatureRequest.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBGenericDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBGenericRequest.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBGetDescriptorRequest.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBInterfaceDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBInterfaceRequest.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBSetAddressRequest.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBSetConfigurationRequest.h"
#include "../../../hal/at91/include/at91/usb/common/core/USBStringDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDButton.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDDeviceDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDGenericDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDGenericDesktop.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDGenericRequest.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDIdleRequest.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDInterfaceDescriptor.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDLeds.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDReport.h"
#include "../../../hal/at91/include/at91/usb/common/hid/HIDReportRequest.h"
#include "../../../hal/at91/include/at91/usb/device/core/USBD.h"
#include "../../../hal/at91/include/at91/usb/device/core/USBDCallbacks.h"
#include "../../../hal/at91/include/at91/usb/device/core/USBDDriver.h"
#include "../../../hal/at91/include/at91/usb/device/core/USBDDriverCallbacks.h"
#include "../../../hal/at91/include/at91/usb/device/core/USBDDriverDescriptors.h"
#include "../../../hal/at91/include/at91/usb/device/hid-mouse/HIDDMouseDriver.h"
#include "../../../hal/at91/include/at91/usb/device/hid-mouse/HIDDMouseDriverDescriptors.h"
#include "../../../hal/at91/include/at91/usb/device/hid-mouse/HIDDMouseInputReport.h"
#include "../../../hal/at91/include/at91/utility/assert.h"
#include "../../../hal/at91/include/at91/utility/bmp.h"
#include "../../../hal/at91/include/at91/utility/exithandler.h"
#include "../../../hal/at91/include/at91/utility/hamming.h"
#include "../../../hal/at91/include/at91/utility/math.h"
#include "../../../hal/at91/include/at91/utility/rand.h"
#include "../../../hal/at91/include/at91/utility/trace.h"
#include "../../../hal/at91/include/at91/utility/video.h"
#include "../../../hal/demo/src/Demo/demo_sd.h"
#include "../../../hal/demo/src/Demo/demo_uart.h"
#include "../../../hal/demo/src/Tests/ADCtest.h"
#include "../../../hal/demo/src/Tests/boardTest.h"
#include "../../../hal/demo/src/Tests/checksumTest.h"
#include "../../../hal/demo/src/Tests/FloatingPointTest.h"
#include "../../../hal/demo/src/Tests/I2CslaveTest.h"
#include "../../../hal/demo/src/Tests/I2Ctest.h"
#include "../../../hal/demo/src/Tests/LEDtest.h"
#include "../../../hal/demo/src/Tests/PinTest.h"
#include "../../../hal/demo/src/Tests/PWMtest.h"
#include "../../../hal/demo/src/Tests/SDCardTest.h"
#include "../../../hal/demo/src/Tests/SPI_FRAM_RTCtest.h"
#include "../../../hal/demo/src/Tests/SupervisorTest.h"
#include "../../../hal/demo/src/Tests/ThermalTest.h"
#include "../../../hal/demo/src/Tests/TimeTest.h"
#include "../../../hal/demo/src/Tests/UARTtest.h"
#include "../../../hal/demo/src/Tests/USBdeviceTest.h"
#include "../../../hal/freertos/include/freertos/croutine.h"
#include "../../../hal/freertos/include/freertos/FreeRTOS.h"
#include "../../../hal/freertos/include/freertos/FreeRTOSConfig.h"
#include "../../../hal/freertos/include/freertos/list.h"
#include "../../../hal/freertos/include/freertos/mpu_wrappers.h"
#include "../../../hal/freertos/include/freertos/portable.h"
#include "../../../hal/freertos/include/freertos/projdefs.h"
#include "../../../hal/freertos/include/freertos/queue.h"
#include "../../../hal/freertos/include/freertos/semphr.h"
#include "../../../hal/freertos/include/freertos/StackMacros.h"
#include "../../../hal/freertos/include/freertos/task.h"
#include "../../../hal/freertos/include/freertos/timers.h"
#include "../../../hal/freertos/include/freertos/portable/GCC/ARM9_AT91SAM9G20/portmacro.h"
#include "../../../hal/freertos/include/freertos/portable/MemMang/standardMemMang.h"
#include "../../../hal/freertos/src/StackMacros.h"
#include "../../../hal/hal/include/hal/boolean.h"
#include "../../../hal/hal/include/hal/checksum.h"
#include "../../../hal/hal/include/hal/errors.h"
#include "../../../hal/hal/include/hal/interruptPriorities.h"
#include "../../../hal/hal/include/hal/supervisor.h"
#include "../../../hal/hal/include/hal/Drivers/ADC.h"
#include "../../../hal/hal/include/hal/Drivers/I2C.h"
#include "../../../hal/hal/include/hal/Drivers/I2Cslave.h"
#include "../../../hal/hal/include/hal/Drivers/LED.h"
#include "../../../hal/hal/include/hal/Drivers/PWM.h"
#include "../../../hal/hal/include/hal/Drivers/SPI.h"
#include "../../../hal/hal/include/hal/Drivers/UART.h"
#include "../../../hal/hal/include/hal/Storage/FRAM.h"
#include "../../../hal/hal/include/hal/Storage/NORflash.h"
#include "../../../hal/hal/include/hal/Timing/RTC.h"
#include "../../../hal/hal/include/hal/Timing/RTT.h"
#include "../../../hal/hal/include/hal/Timing/Time.h"
#include "../../../hal/hal/include/hal/Timing/WatchDogTimer.h"
#include "../../../hal/hal/include/hal/Utility/dbgu_int.h"
#include "../../../hal/hal/include/hal/Utility/fprintf.h"
#include "../../../hal/hal/include/hal/Utility/util.h"
#include "../../../hal/hal/include/hal/version/version.h"
#include "../../../hal/hcc/include/config/config_fat.h"
#include "../../../hal/hcc/include/hcc/api_fat.h"
#include "../../../hal/hcc/include/hcc/api_fat_test.h"
#include "../../../hal/hcc/include/hcc/api_fs_err.h"
#include "../../../hal/hcc/include/hcc/api_hcc_mem.h"
#include "../../../hal/hcc/include/hcc/api_mdriver.h"
#include "../../../hal/hcc/include/hcc/api_mdriver_atmel_mcipdc.h"
#include "../../../hal/hcc/include/psp/include/psp_types.h"
#include "../../../hal/hcc/include/version/ver_fat.h"
#include "../../../hal/hcc/include/version/ver_fat_test.h"
#include "../../../hal/hcc/include/version/ver_hcc_mem.h"
#include "../../../hal/hcc/include/version/ver_mdriver.h"
#include "../../../hal/hcc/include/version/ver_mdriver_atmel_mcipdc.h"
#include "../../../hal/hcc/include/version/ver_oal.h"
#include "../../../hal/hcc/include/version/ver_psp_types.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Ants.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/EPS.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/HoopoeTestingConfigurations.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/TRXVU.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/ADCS/AdcsCommands.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/ADCS/AdcsGetDataAndTlm.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/ADCS/AdcsMain.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/ADCS/AdcsTroubleShooting.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/ADCS/StateMachine.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/COMM/APRS.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/COMM/DelayedCommand_list.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/COMM/GSC.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/COMM/splTypes.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/CUF/CUF.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/CUF/uploadCodeTelemetry.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Global/FRAMadress.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Global/freertosExtended.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Global/GenericTaskSave.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Global/Global.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Global/GlobalParam.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Global/logger.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Global/OnlineTM.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Global/sizes.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Global/TLM_management.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Main/commands.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Main/HouseKeeping.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Main/Main_Init.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Main/CMD/ADCS_CMD.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Main/CMD/COMM_CMD.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Main/CMD/EPS_CMD.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Main/CMD/General_CMD.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Main/CMD/payload_CMD.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/Main/CMD/SW_CMD.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/ImageConversion.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/BooleanJpeg.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/color_conv.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/DCT.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/ImgCompressor.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/jpeg.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/jpeg_encoder.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/jpeg_memory_alloc.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/jpeg_params.h"
//#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/jpeg_tables.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/output_stream.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Compression/jpeg/bmp/rawToBMP.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/DataBase/DataBase.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Drivers/GeckoCameraDriver.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Misc/Boolean_bit.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Misc/FileSystem.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Misc/Macros.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Request Management/AutomaticImageHandler.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Request Management/CameraManeger.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Request Management/DB_RequestHandling.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Request Management/Dump/Butchering.h"
#include "../../../satellite-subsystems/demo/src/sub-systemCode/payload/Request Management/Dump/imageDump.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/cspaceADCS.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/cspaceADCS_types.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/GomEPS.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/IsisAntS.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/IsisEPS.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/IsisMTQv1.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/IsisMTQv2.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/IsisSolarPanel.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/IsisSolarPanelv2.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/IsisTRXUV.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/IsisTRXVU.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/IsisTxS.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/SCS_Gecko/gecko_driver.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/SCS_Gecko/gecko_use_cases.h"
#include "../../../satellite-subsystems/satellite-subsystems/include/satellite-subsystems/version/version.h"
#include "../../freertos/include/freertos/task.h"


void funcsPointerToArr()
{

	CUFARR[1] = (void *)LowLevelInit;
	CUFARR[2] = (void *)BOARD_RemapRom;
	CUFARR[3] = (void *)BOARD_RemapRam;
	CUFARR[4] = (void *)BOARD_ConfigureSdram;
	CUFARR[5] = (void *)BOARD_ConfigureNorFlash;

	CUFARR[6] = (void *)AMD_Reset;
	CUFARR[7] = (void *)AMD_ReadManufactoryId;
	CUFARR[8] = (void *)AMD_ReadDeviceID;
	//CUFARR[9] = (void *)AMD_EraseSector;


	CUFARR[32] = (void *)WriteCommand;
	CUFARR[33] = (void *)ReadRawData;
	CUFARR[34] = (void *)WriteRawData;

	CUFARR[41] = (void *)SD_Init;
	CUFARR[42] = (void *)SD_ReadBlock;
	CUFARR[43] = (void *)SD_WriteBlock;
	CUFARR[44] = (void *)SD_Stop;
	CUFARR[45] = (void *)ADC_Initialize;
	CUFARR[46] = (void *)ADC_GetModeReg;
	CUFARR[47] = (void *)ADC_EnableChannel;
	CUFARR[48] = (void *)ADC_DisableChannel;
	CUFARR[49] = (void *)ADC_GetChannelStatus;
	CUFARR[50] = (void *)ADC_StartConversion;
	CUFARR[51] = (void *)ADC_SoftReset;
	CUFARR[52] = (void *)ADC_GetLastConvertedData;
	CUFARR[53] = (void *)ADC_GetConvertedData;
	CUFARR[54] = (void *)ADC_EnableIt;
	CUFARR[55] = (void *)ADC_EnableDataReadyIt;
	CUFARR[56] = (void *)ADC_DisableIt;
	CUFARR[57] = (void *)ADC_GetStatus;
	CUFARR[58] = (void *)ADC_GetInterruptMaskStatus;
	CUFARR[59] = (void *)ADC_IsInterruptMasked;
	CUFARR[60] = (void *)ADC_IsStatusSet;
	CUFARR[61] = (void *)ADC_IsChannelInterruptStatusSet;
	CUFARR[62] = (void *)AIC_ConfigureIT;
	CUFARR[63] = (void *)AIC_EnableIT;
	CUFARR[64] = (void *)AIC_DisableIT;
	CUFARR[65] = (void *)CP15_Enable_I_Cache;
	CUFARR[66] = (void *)CP15_Is_I_CacheEnabled;
	CUFARR[67] = (void *)CP15_Enable_I_Cache;
	CUFARR[68] = (void *)CP15_Disable_I_Cache;
	CUFARR[69] = (void *)CP15_Is_MMUEnabled;
	CUFARR[70] = (void *)CP15_EnableMMU;

	CUFARR[71] = (void *)CP15_DisableMMU;
	CUFARR[72] = (void *)CP15_Is_DCacheEnabled;
	CUFARR[73] = (void *)CP15_Enable_D_Cache;
	CUFARR[74] = (void *)CP15_Disable_D_Cache;
	CUFARR[75] = (void *)_readControlRegister;
	CUFARR[76] = (void *)_writeControlRegister;
	CUFARR[77] = (void *)_waitForInterrupt;
	CUFARR[78] = (void *)_writeTTB;
	CUFARR[79] = (void *)_writeDomain;
	CUFARR[80] = (void *)_writeITLBLockdown;
	CUFARR[81] = (void *)_prefetchICacheLine;
	CUFARR[82] = (void *)_flushDCache;
	CUFARR[83] = (void *)_flushWriteBuffer;
	CUFARR[84] = (void *)_invalidateIDTLBs;
	CUFARR[85] = (void *)DBGU_Configure;
	CUFARR[86] = (void *)DBGU_GetChar;
	CUFARR[87] = (void *)DBGU_PutChar;
	CUFARR[88] = (void *)DBGU_IsRxReady;

	//CUFARR[89] = (void *)ISI_Enable;
	//CUFARR[90] = (void *)ISI_Disable;
	//CUFARR[91] = (void *)ISI_EnableInterrupt;
	//CUFARR[92] = (void *)ISI_DisableInterrupt;
	//CUFARR[93] = (void *)ISI_CodecPathFull;
	//CUFARR[94] = (void *)ISI_SetFrame;
	//CUFARR[95] = (void *)ISI_BytesForOnePixel;
	//CUFARR[96] = (void *)ISI_Reset;
	//CUFARR[97] = (void *)ISI_Init;
	//CUFARR[98] = (void *)ISI_StatusRegister;
	CUFARR[99] = (void *)MCI_Init;
	CUFARR[100] = (void *)MCI_SetSpeed;

	CUFARR[101] = (void *)MCI_SendCommand;
	CUFARR[102] = (void *)MCI_Handler;
	CUFARR[103] = (void *)MCI_IsTxComplete;
	CUFARR[104] = (void *)MCI_CheckBusy;
	CUFARR[105] = (void *)MCI_Close;
	CUFARR[106] = (void *)MCI_SetBusWidth;
	CUFARR[107] = (void *)PIO_Configure;
	CUFARR[108] = (void *)PIO_Set;
	CUFARR[109] = (void *)PIO_Clear;
	CUFARR[110] = (void *)PIO_Get;
	CUFARR[111] = (void *)PIO_GetISR;
	CUFARR[112] = (void *)PIO_GetOutputDataStatus;
	CUFARR[113] = (void *)PIO_InitializeInterrupts;
	CUFARR[114] = (void *)PIO_ConfigureIt;
	CUFARR[115] = (void *)PIO_EnableIt;
	CUFARR[116] = (void *)PIO_DisableIt;
	CUFARR[117] = (void *)PIT_Init;
	CUFARR[118] = (void *)PIT_SetPIV;
	CUFARR[119] = (void *)PIT_Enable;
	CUFARR[120] = (void *)PIT_EnableIT;
	CUFARR[121] = (void *)PIT_DisableIT;
	CUFARR[122] = (void *)PIT_GetMode;
	CUFARR[123] = (void *)PIT_GetStatus;
	CUFARR[124] = (void *)PIT_GetPIIR;
	CUFARR[125] = (void *)PIT_GetPIVR;
	//CUFARR[126] = (void *)PMC_SetFastWakeUpInputs;
	//CUFARR[127] = (void *)PMC_DisableMainOscillator;
	//CUFARR[128] = (void *)PMC_DisableMainOscillatorForWaitMode;
	CUFARR[129] = (void *)PMC_DisableProcessorClock;
	CUFARR[130] = (void *)PMC_EnablePeripheral;
	CUFARR[131] = (void *)PMC_DisablePeripheral;
	CUFARR[132] = (void *)PMC_CPUInIdleMode;
	CUFARR[133] = (void *)PMC_EnableAllPeripherals;
	CUFARR[134] = (void *)PMC_DisableAllPeripherals;
	CUFARR[135] = (void *)PMC_IsAllPeriphEnabled;
	CUFARR[136] = (void *)PMC_IsPeriphEnabled;
	CUFARR[137] = (void *)RSTC_ConfigureMode;
	CUFARR[138] = (void *)RSTC_SetUserResetEnable;
	CUFARR[139] = (void *)RSTC_SetUserResetInterruptEnable;
	CUFARR[140] = (void *)RSTC_SetExtResetLength;
	CUFARR[141] = (void *)RSTC_ProcessorReset;
	CUFARR[142] = (void *)RSTC_PeripheralReset;
	CUFARR[143] = (void *)RSTC_ExtReset;
	CUFARR[144] = (void *)RSTC_GetNrstLevel;
	CUFARR[145] = (void *)RSTC_IsUserResetDetected;
	CUFARR[146] = (void *)RSTC_IsBusy;
	CUFARR[147] = (void *)SPI_Enable;
	CUFARR[148] = (void *)SPI_Disable;
	CUFARR[149] = (void *)SPI_Configure;
	CUFARR[150] = (void *)SPI_ConfigureNPCS;
	CUFARR[151] = (void *)SPI_Write;
	CUFARR[152] = (void *)SPI_WriteBuffer;
	CUFARR[153] = (void *)SPI_IsFinished;
	CUFARR[154] = (void *)SPI_Read;
	CUFARR[155] = (void *)SPI_ReadBuffer;
	CUFARR[156] = (void *)TC_Configure;
	CUFARR[157] = (void *)TC_Start;
	CUFARR[158] = (void *)TC_Stop;
	CUFARR[159] = (void *)TC_FindMckDivisor;
	CUFARR[160] = (void *)TWI_ConfigureMaster;
	CUFARR[161] = (void *)TWI_ConfigureSlave;
	CUFARR[162] = (void *)TWI_Stop;
	CUFARR[163] = (void *)TWI_StartRead;
	CUFARR[164] = (void *)TWI_ReadByte;
	CUFARR[165] = (void *)TWI_WriteByte;
	CUFARR[166] = (void *)TWI_StartWrite;
	CUFARR[167] = (void *)TWI_ByteReceived;
	CUFARR[168] = (void *)TWI_ByteSent;
	CUFARR[169] = (void *)TWI_TransferComplete;
	CUFARR[170] = (void *)TWI_EnableIt;
	CUFARR[171] = (void *)TWI_DisableIt;
	CUFARR[172] = (void *)TWI_GetStatus;
	CUFARR[173] = (void *)TWI_GetMaskedStatus;
	CUFARR[174] = (void *)TWI_SendSTOPCondition;
	CUFARR[175] = (void *)USART_Configure;
	CUFARR[176] = (void *)USART_SetTransmitterEnabled;
	CUFARR[177] = (void *)USART_SetReceiverEnabled;
	CUFARR[178] = (void *)USART_Write;
	CUFARR[179] = (void *)USART_WriteBuffer;
	CUFARR[180] = (void *)USART_Read;
	CUFARR[181] = (void *)USART_ReadBuffer;
	CUFARR[182] = (void *)USART_IsDataAvailable;
	CUFARR[183] = (void *)USART_SetIrdaFilter;
	CUFARR[184] = (void *)USBConfigurationDescriptor_GetTotalLength;
	CUFARR[185] = (void *)USBConfigurationDescriptor_GetNumInterfaces;
	CUFARR[186] = (void *)USBConfigurationDescriptor_IsSelfPowered;
	CUFARR[187] = (void *)USBConfigurationDescriptor_Parse;
	CUFARR[188] = (void *)USBEndpointDescriptor_GetNumber;
	CUFARR[189] = (void *)USBEndpointDescriptor_GetDirection;
	CUFARR[190] = (void *)USBEndpointDescriptor_GetType;
	CUFARR[191] = (void *)USBEndpointDescriptor_GetMaxPacketSize;
	CUFARR[192] = (void *)USBFeatureRequest_GetFeatureSelector;
	CUFARR[193] = (void *)USBFeatureRequest_GetTestSelector;
	CUFARR[194] = (void *)USBGenericDescriptor_GetLength;
	CUFARR[195] = (void *)USBGenericDescriptor_GetType;
	CUFARR[196] = (void *)USBGenericDescriptor_GetNextDescriptor;
	CUFARR[197] = (void *)USBGenericRequest_GetType;
	CUFARR[198] = (void *)USBGenericRequest_GetRequest;
	CUFARR[199] = (void *)USBGenericRequest_GetValue;
	CUFARR[200] = (void *)USBGenericRequest_GetIndex;
	CUFARR[201] = (void *)USBGenericRequest_GetLength;
	CUFARR[202] = (void *)USBGenericRequest_GetEndpointNumber;
	CUFARR[203] = (void *)USBGenericRequest_GetRecipient;
	CUFARR[204] = (void *)USBGenericRequest_GetDirection;
	CUFARR[205] = (void *)USBGetDescriptorRequest_GetDescriptorType;
	CUFARR[206] = (void *)USBGetDescriptorRequest_GetDescriptorIndex;
	CUFARR[207] = (void *)USBInterfaceRequest_GetInterface;
	CUFARR[208] = (void *)USBInterfaceRequest_GetAlternateSetting;
	CUFARR[209] = (void *)USBSetAddressRequest_GetAddress;
	CUFARR[210] = (void *)USBSetConfigurationRequest_GetConfiguration;
	CUFARR[211] = (void *)HIDIdleRequest_GetIdleRate;
	CUFARR[212] = (void *)HIDReportRequest_GetReportType;
	CUFARR[213] = (void *)HIDReportRequest_GetReportId;
	//CUFARR[214] = (void *)USBD_InterruptHandler;
	//CUFARR[215] = (void *)USBD_Init;
	//CUFARR[216] = (void *)USBD_Connect;
	//CUFARR[217] = (void *)USBD_Disconnect;
	//CUFARR[218] = (void *)USBD_Write;
	//CUFARR[219] = (void *)USBD_Read;
	//CUFARR[220] = (void *)USBD_Stall;
	//CUFARR[221] = (void *)USBD_Halt;
	//CUFARR[222] = (void *)USBD_Unhalt;
	//CUFARR[223] = (void *)USBD_ConfigureEndpoint;
	//CUFARR[224] = (void *)USBD_IsHalted;
	//CUFARR[225] = (void *)USBD_RemoteWakeUp;
	//CUFARR[226] = (void *)USBD_SetAddress;
	//CUFARR[227] = (void *)USBD_SetConfiguration;
	//CUFARR[228] = (void *)USBD_GetState;
	//CUFARR[229] = (void *)USBD_IsHighSpeed;
	//CUFARR[230] = (void *)USBD_Test;
	//CUFARR[231] = (void *)USBDCallbacks_Initialized;
	//CUFARR[232] = (void *)USBDCallbacks_Reset;
	//CUFARR[233] = (void *)USBDCallbacks_Suspended;
	//CUFARR[234] = (void *)USBDCallbacks_Resumed;
	//CUFARR[235] = (void *)USBDCallbacks_RequestReceived;
	//CUFARR[236] = (void *)USBDDriver_Initialize;
	//CUFARR[237] = (void *)USBDDriver_RequestHandler;
	//CUFARR[238] = (void *)USBDDriver_IsRemoteWakeUpEnabled;
	//CUFARR[239] = (void *)USBDDriverCallbacks_ConfigurationChanged;
	//CUFARR[240] = (void *)USBDDriverCallbacks_InterfaceSettingChanged;
	//CUFARR[241] = (void *)HIDDMouseDriver_Initialize;
	//CUFARR[242] = (void *)HIDDMouseDriver_RequestHandler;
	//CUFARR[243] = (void *)HIDDMouseDriver_ChangePoints;
	//CUFARR[244] = (void *)HIDDMouseDriver_RemoteWakeUp;
	//CUFARR[245] = (void *)HIDDMouseInputReport_Initialize;
	//CUFARR[246] = (void *)BMP_IsValid;
	//CUFARR[247] = (void *)BMP_GetFileSize;
	//CUFARR[248] = (void *)BMP_Decode;
	//CUFARR[249] = (void *)WriteBMPheader;
	//CUFARR[250] = (void *)BMP_displayHeader;
	//CUFARR[251] = (void *)RGB565toBGR555;
	CUFARR[252] = (void *)restart;
	CUFARR[253] = (void *)restartPrefetchAbort;
	CUFARR[254] = (void *)restartDataAbort;
	CUFARR[255] = (void *)gracefulReset;
	CUFARR[256] = (void *)Hamming_Compute256x;
	CUFARR[257] = (void *)Hamming_Verify256x;
	CUFARR[258] = (void *)min;
	CUFARR[259] = (void *)absv;
	//CUFARR[260] = (void *)pow;
	CUFARR[261] = (void *)power;
	CUFARR[262] = (void *)srand;
	CUFARR[263] = (void *)rand;
	CUFARR[264] = (void *)VIDEO_Ycc2Rgb;
	//CUFARR[265] = (void *)DEMO_SD_Basic;
	//CUFARR[266] = (void *)DEMO_SD_FileTasks;
	//CUFARR[267] = (void *)DEMO_SD_StringFunc;
	//CUFARR[268] = (void *)DEMO_UART_SetupVariablePacketTask;
	//CUFARR[269] = (void *)DEMO_UART_SetupUnknownPacketTask;
	//CUFARR[270] = (void *)ADCtest;
	//CUFARR[271] = (void *)ADCtestSingleShot;
	//CUFARR[272] = (void *)ADCtest_initalize;
	//CUFARR[273] = (void *)ADCtest_printReadout;
	//CUFARR[274] = (void *)boardTest;
	//CUFARR[275] = (void *)checksumTest;
	//CUFARR[276] = (void *)floatingPointTest;
	//CUFARR[277] = (void *)I2CslaveTest;
	//CUFARR[278] = (void *)I2Ctest;
	//CUFARR[279] = (void *)LEDtest;
	//CUFARR[280] = (void *)PinTest;
	//CUFARR[281] = (void *)PWMtest;
	//CUFARR[282] = (void *)SDCardTest;
	//CUFARR[283] = (void *)SPI_FRAM_RTCtest;
	//CUFARR[284] = (void *)SupervisorControllerTest;
	//CUFARR[285] = (void *)SupervisorTest;
	//CUFARR[286] = (void *)thermalTest;
	//CUFARR[287] = (void *)TimeTest;
	//CUFARR[288] = (void *)UARTtest;
	//CUFARR[289] = (void *)USBdeviceTest;
	CUFARR[290] = (void *)xCoRoutineCreate;
	CUFARR[291] = (void *)vCoRoutineSchedule;
	CUFARR[292] = (void *)vCoRoutineAddToDelayedList;
	CUFARR[293] = (void *)xCoRoutineRemoveFromEventList;
	CUFARR[294] = (void *)vListInitialise;
	CUFARR[295] = (void *)vListInitialiseItem;
	CUFARR[296] = (void *)vListInsert;
	CUFARR[297] = (void *)vListInsertEnd;
	CUFARR[298] = (void *)uxListRemove;
	CUFARR[299] = (void *)pxPortInitialiseStack;
	CUFARR[300] = (void *)pxPortInitialiseStack;
	CUFARR[301] = (void *)pvPortMalloc;
	CUFARR[302] = (void *)vPortFree;
	CUFARR[303] = (void *)vPortInitialiseBlocks;
	CUFARR[304] = (void *)xPortGetFreeHeapSize;
	CUFARR[305] = (void *)xPortStartScheduler;
	CUFARR[306] = (void *)vPortEndScheduler;
	//CUFARR[307] = (void *)vPortStoreTaskMPUSettings;
	CUFARR[308] = (void *)xQueueGenericSend;
	CUFARR[309] = (void *)xQueuePeekFromISR;
	//CUFARR[310] = (void *)xQueueGenericReceive;
	CUFARR[311] = (void *)uxQueueMessagesWaiting;
	CUFARR[312] = (void *)uxQueueSpacesAvailable;
	CUFARR[313] = (void *)vQueueDelete;
	CUFARR[314] = (void *)xQueueGenericSendFromISR;
	CUFARR[315] = (void *)xQueueReceiveFromISR;
	CUFARR[316] = (void *)xQueueIsQueueEmptyFromISR;
	CUFARR[317] = (void *)xQueueIsQueueFullFromISR;
	CUFARR[318] = (void *)uxQueueMessagesWaitingFromISR;
	//CUFARR[319] = (void *)xQueueAltGenericSend;
	//CUFARR[320] = (void *)xQueueAltGenericReceive;
	//CUFARR[321] = (void *)xQueueCRSendFromISR;
	//CUFARR[322] = (void *)xQueueCRReceiveFromISR;
	//CUFARR[323] = (void *)xQueueCRSend;
	//CUFARR[324] = (void *)xQueueCRReceive;
	CUFARR[325] = (void *)xQueueCreateMutex;
	CUFARR[326] = (void *)xQueueCreateCountingSemaphore;
	//CUFARR[327] = (void *)xQueueGetMutexHolder;
	CUFARR[328] = (void *)xQueueTakeMutexRecursive;
	CUFARR[329] = (void *)xQueueGiveMutexRecursive;
	CUFARR[330] = (void *)vQueueAddToRegistry;
	CUFARR[331] = (void *)vQueueUnregisterQueue;
	CUFARR[332] = (void *)xQueueGenericCreate;
	//CUFARR[333] = (void *)xQueueCreateSet;
	//CUFARR[334] = (void *)xQueueAddToSet;
	//CUFARR[335] = (void *)xQueueRemoveFromSet;
	//CUFARR[336] = (void *)xQueueSelectFromSet;
	//CUFARR[337] = (void *)xQueueSelectFromSetFromISR;
	//CUFARR[338] = (void *)vQueueWaitForMessageRestricted;
	CUFARR[339] = (void *)xQueueGenericReset;
	//CUFARR[340] = (void *)vQueueSetQueueNumber;
	//CUFARR[341] = (void *)ucQueueGetQueueNumber;
	//CUFARR[342] = (void *)ucQueueGetQueueType;
	//CUFARR[343] = (void *)vTaskAllocateMPURegions;
	CUFARR[344] = (void *)vTaskDelete;
	CUFARR[345] = (void *)vTaskDelay;
	CUFARR[346] = (void *)vTaskDelayUntil;
	CUFARR[347] = (void *)uxTaskPriorityGet;
	CUFARR[348] = (void *)eTaskGetState;
	CUFARR[349] = (void *)vTaskPrioritySet;
	CUFARR[350] = (void *)vTaskSuspend;
	CUFARR[351] = (void *)vTaskResume;
	CUFARR[352] = (void *)xTaskResumeFromISR;
	CUFARR[353] = (void *)vTaskStartScheduler;
	CUFARR[354] = (void *)vTaskEndScheduler;
	CUFARR[355] = (void *)vTaskSuspendAll;
	CUFARR[356] = (void *)xTaskResumeAll;
	CUFARR[357] = (void *)xTaskIsTaskSuspended;
	CUFARR[358] = (void *)xTaskGetTickCount;
	CUFARR[359] = (void *)xTaskGetTickCountFromISR;
	CUFARR[360] = (void *)uxTaskGetNumberOfTasks;
	CUFARR[361] = (void *)pcTaskGetTaskName;
	//CUFARR[362] = (void *)uxTaskGetStackHighWaterMark;
	//CUFARR[363] = (void *)vTaskSetApplicationTaskTag;
	//CUFARR[364] = (void *)xTaskGetApplicationTaskTag;
	//CUFARR[365] = (void *)xTaskCallApplicationTaskHook;
	//CUFARR[366] = (void *)xTaskGetIdleTaskHandle;
	//CUFARR[367] = (void *)uxTaskGetSystemState;
	//CUFARR[368] = (void *)vTaskList;
	//CUFARR[369] = (void *)vTaskGetRunTimeStats;
	CUFARR[370] = (void *)xTaskIncrementTick;
	//CUFARR[371] = (void *)vTaskPlaceOnEventList;
	//CUFARR[372] = (void *)vTaskPlaceOnEventListRestricted;
	CUFARR[373] = (void *)xTaskRemoveFromEventList;
	CUFARR[374] = (void *)vTaskSwitchContext;
	CUFARR[375] = (void *)xTaskGetCurrentTaskHandle;
	CUFARR[376] = (void *)vTaskSetTimeOutState;
	CUFARR[377] = (void *)xTaskCheckForTimeOut;
	CUFARR[378] = (void *)vTaskMissedYield;
	CUFARR[379] = (void *)xTaskGetSchedulerState;
	CUFARR[380] = (void *)vTaskPriorityInherit;
	CUFARR[381] = (void *)vTaskPriorityDisinherit;
	CUFARR[382] = (void *)xTaskGenericCreate;
	//CUFARR[383] = (void *)uxTaskGetTaskNumber;
	//CUFARR[384] = (void *)vTaskSetTaskNumber;
	//CUFARR[385] = (void *)vTaskStepTick;
	//CUFARR[386] = (void *)eTaskConfirmSleepModeStatus;
	//CUFARR[387] = (void *)xTimerCreate;
	//CUFARR[388] = (void *)pvTimerGetTimerID;
	//CUFARR[389] = (void *)xTimerIsTimerActive;
	//CUFARR[390] = (void *)xTimerGetTimerDaemonTaskHandle;
	//CUFARR[391] = (void *)xTimerCreateTimerTask;
	//CUFARR[392] = (void *)xTimerGenericCommand;
	//CUFARR[393] = (void *)vPortDisableInterruptsFromThumb;
	//CUFARR[394] = (void *)vPortEnableInterruptsFromThumb;
	CUFARR[395] = (void *)vPortEnterCritical;
	CUFARR[396] = (void *)vPortExitCritical;
	CUFARR[397] = (void *)pvPortMalloc;
	CUFARR[398] = (void *)vPortFree;
	CUFARR[399] = (void *)vPortInitialiseBlocks;
	CUFARR[400] = (void *)xPortGetFreeHeapSize;
	CUFARR[401] = (void *)checksum_prepareLUTCRC32;
	CUFARR[402] = (void *)checksum_calculateCRC32LUT;
	CUFARR[403] = (void *)checksum_calculateCRC32;
	CUFARR[404] = (void *)checksum_verifyCRC32;
	CUFARR[405] = (void *)checksum_prepareLUTCRC16;
	CUFARR[406] = (void *)checksum_calculateCRC16LUT;
	CUFARR[407] = (void *)checksum_calculateCRC16;
	CUFARR[408] = (void *)checksum_verifyCRC16;
	CUFARR[409] = (void *)checksum_prepareLUTCRC8;
	CUFARR[410] = (void *)checksum_calculateCRC8LUT;
	CUFARR[411] = (void *)checksum_calculateCRC8;
	CUFARR[412] = (void *)checksum_verifyCRC8;
	CUFARR[413] = (void *)Supervisor_start;
	CUFARR[414] = (void *)Supervisor_emergencyReset;
	CUFARR[415] = (void *)Supervisor_reset;
	CUFARR[416] = (void *)Supervisor_writeOutput;
	CUFARR[417] = (void *)Supervisor_powerCycleIobc;
	CUFARR[418] = (void *)Supervisor_getVersion;
	CUFARR[419] = (void *)Supervisor_getHousekeeping;
	CUFARR[420] = (void *)Supervisor_calculateAdcValues;
	CUFARR[421] = (void *)ADC_ConvertRaw10bitToMillivolt;
	CUFARR[422] = (void *)ADC_ConvertRaw8bitToMillivolt;
	CUFARR[423] = (void *)ADC_SingleShot;
	CUFARR[424] = (void *)ADC_start;
	CUFARR[425] = (void *)ADC_stop;
	CUFARR[426] = (void *)I2C_start;
	CUFARR[427] = (void *)I2C_setTransferTimeout;
	CUFARR[428] = (void *)I2C_stop;
	CUFARR[429] = (void *)I2C_blockBus;
	CUFARR[430] = (void *)I2C_releaseBus;
	CUFARR[431] = (void *)I2C_write;
	CUFARR[432] = (void *)I2C_read;
	CUFARR[433] = (void *)I2C_writeRead;
	CUFARR[434] = (void *)I2C_queueTransfer;
	CUFARR[435] = (void *)I2C_getDriverState;
	CUFARR[436] = (void *)I2C_getCurrentTransferStatus;
	CUFARR[437] = (void *)I2Cslave_start;
	CUFARR[438] = (void *)I2Cslave_stop;
	CUFARR[439] = (void *)I2Cslave_write;
	CUFARR[440] = (void *)I2Cslave_read;
	CUFARR[441] = (void *)I2Cslave_getDriverState;
	CUFARR[442] = (void *)I2Cslave_mute;
	CUFARR[443] = (void *)I2Cslave_unMute;
	CUFARR[444] = (void *)LED_start;
	CUFARR[445] = (void *)LED_glow;
	CUFARR[446] = (void *)LED_dark;
	CUFARR[447] = (void *)LED_toggle;
	CUFARR[448] = (void *)LED_wave;
	CUFARR[449] = (void *)LED_waveReverse;
	CUFARR[450] = (void *)PWM_start;
	CUFARR[451] = (void *)PWM_startAuto;
	CUFARR[452] = (void *)PWM_setDutyCycles;
	CUFARR[453] = (void *)PWM_setRawDutyCycles;
	CUFARR[454] = (void *)PWM_setAllDutyCyclesZero;
	CUFARR[455] = (void *)PWM_setAllDutyCyclesFull;
	CUFARR[456] = (void *)PWM_stop;
	CUFARR[457] = (void *)SPI_start;
	CUFARR[458] = (void *)SPI_stop;
	CUFARR[459] = (void *)SPI_queueTransfer;
	CUFARR[460] = (void *)SPI_writeRead;
	CUFARR[461] = (void *)SPI_getDriverState;
	CUFARR[462] = (void *)UART_start;
	CUFARR[463] = (void *)UART_stop;
	CUFARR[464] = (void *)UART_setRxEnabled;
	CUFARR[465] = (void *)UART_isRxEnabled;
	CUFARR[466] = (void *)UART_queueTransfer;
	CUFARR[467] = (void *)UART_write;
	CUFARR[468] = (void *)UART_read;
	CUFARR[469] = (void *)UART_setPostTransferDelay;
	CUFARR[470] = (void *)UART_getPrevBytesRead;
	CUFARR[471] = (void *)UART_getDriverState;
	CUFARR[472] = (void *)FRAM_start;
	CUFARR[473] = (void *)FRAM_stop;
	CUFARR[474] = (void *)FRAM_write;
	CUFARR[475] = (void *)FRAM_read;
	CUFARR[476] = (void *)FRAM_writeAndVerify;
	CUFARR[477] = (void *)FRAM_protectBlocks;
	CUFARR[478] = (void *)FRAM_getProtectedBlocks;
	CUFARR[479] = (void *)FRAM_getDeviceID;
	//CUFARR[480] = (void *)NORflash_start;
	//CUFARR[481] = (void *)NORflash_test;
	CUFARR[482] = (void *)RTC_start;
	CUFARR[483] = (void *)RTC_stop;
	CUFARR[484] = (void *)RTC_setTime;
	CUFARR[485] = (void *)RTC_getTime;
	CUFARR[486] = (void *)RTC_testGetSet;
	CUFARR[487] = (void *)RTC_testSeconds;
	CUFARR[488] = (void *)RTC_printSeconds;
	CUFARR[489] = (void *)RTC_checkTimeValid;
	CUFARR[490] = (void *)RTC_getTemperature;

	CUFARR[491] = (void *)RTT_start;
	CUFARR[492] = (void *)RTT_GetTime;
	CUFARR[493] = (void *)RTT_GetStatus;
	CUFARR[494] = (void *)RTT_test;
	CUFARR[495] = (void *)Time_start;
	CUFARR[496] = (void *)Time_set;
	CUFARR[497] = (void *)Time_setUnixEpoch;
	CUFARR[498] = (void *)Time_get;
	CUFARR[499] = (void *)Time_getUptimeSeconds;
	CUFARR[500] = (void *)Time_getUnixEpoch;
	CUFARR[501] = (void *)Time_sync;
	CUFARR[502] = (void *)Time_syncIfNeeded;
	CUFARR[503] = (void *)Time_setSyncInterval;
	CUFARR[504] = (void *)Time_isLeapYear;
	CUFARR[505] = (void *)Time_diff;
	CUFARR[506] = (void *)Time_isRTCworking;
	CUFARR[507] = (void *)WDT_start;
	CUFARR[508] = (void *)WDT_kickEveryNcalls;
	CUFARR[509] = (void *)WDT_forceKick;
	CUFARR[510] = (void *)WDT_forceKickEveryNms;
	CUFARR[511] = (void *)WDT_startWatchdogKickTask;
	CUFARR[512] = (void *)WDT_stopWatchdogKickTask;
	CUFARR[513] = (void *)DBGU_Init;
	CUFARR[514] = (void *)DBGU_IntIsRxReady;
	CUFARR[515] = (void *)DBGU_IntGetChar;
	CUFARR[516] = (void *)f_printf;
	CUFARR[517] = (void *)UTIL_DbguDumpMemory;
	CUFARR[518] = (void *)UTIL_DbguDumpArrayBytes;
	CUFARR[519] = (void *)UTIL_DbguGetInteger;
	CUFARR[520] = (void *)UTIL_DbguGetIntegerMinMax;
	CUFARR[521] = (void *)UTIL_DbguGetHexa32;
	CUFARR[522] = (void *)UTIL_DbguGetString;
	CUFARR[523] = (void *)fn_init;
	CUFARR[524] = (void *)fn_start;
	CUFARR[525] = (void *)fn_stop;
	CUFARR[526] = (void *)fsn_delete;
	CUFARR[527] = (void *)fm_initvolume;
	CUFARR[528] = (void *)fm_initvolumepartition;
	CUFARR[529] = (void *)fm_initvolume_nonsafe;
	CUFARR[530] = (void *)fm_initvolumepartition_nonsafe;
	CUFARR[531] = (void *)fm_createpartition;
	CUFARR[532] = (void *)fm_createdriver;
	CUFARR[533] = (void *)fm_releasedriver;
	CUFARR[534] = (void *)fm_getpartition;
	CUFARR[535] = (void *)fm_delvolume;
	CUFARR[536] = (void *)fm_checkvolume;
	CUFARR[537] = (void *)fm_get_volume_count;
	CUFARR[538] = (void *)fm_get_volume_list;
	//CUFARR[539] = (void *)fm_setvolname;
	//CUFARR[540] = (void *)fm_getvolname;
	CUFARR[541] = (void *)fm_format;
	CUFARR[542] = (void *)fm_getcwd;
	CUFARR[543] = (void *)fm_getdcwd;
	CUFARR[544] = (void *)fm_chdrive;
	CUFARR[545] = (void *)fm_getdrive;
	CUFARR[546] = (void *)fm_getfreespace;
	CUFARR[547] = (void *)fm_getlasterror;
	CUFARR[548] = (void *)fm_chdir;
	CUFARR[549] = (void *)fm_mkdir;
	CUFARR[550] = (void *)fm_rmdir;
	CUFARR[551] = (void *)fm_findfirst;
	CUFARR[552] = (void *)fm_findnext;
	CUFARR[553] = (void *)fm_move;
	CUFARR[554] = (void *)fm_rename;
	CUFARR[555] = (void *)fm_filelength;
	CUFARR[556] = (void *)fm_abortclose;
	CUFARR[557] = (void *)fm_close;
	CUFARR[558] = (void *)fm_flush_filebuffer;
	CUFARR[559] = (void *)fm_flush;
	CUFARR[560] = (void *)fm_open;
	CUFARR[561] = (void *)fm_open_nonsafe;
	CUFARR[562] = (void *)fm_truncate;
	CUFARR[563] = (void *)fm_ftruncate;
	CUFARR[564] = (void *)fm_read;
	CUFARR[565] = (void *)fm_write;
	CUFARR[566] = (void *)fm_seek;
	CUFARR[567] = (void *)fm_tell;
	CUFARR[568] = (void *)fm_getc;
	CUFARR[569] = (void *)fm_putc;
	CUFARR[570] = (void *)fm_rewind;
	CUFARR[571] = (void *)fm_eof;
	CUFARR[572] = (void *)fm_seteof;
	CUFARR[573] = (void *)fm_stat;
	CUFARR[574] = (void *)fm_fstat;
	CUFARR[575] = (void *)fm_gettimedate;
	CUFARR[576] = (void *)fm_settimedate;
	CUFARR[577] = (void *)fm_delete;
	//CUFARR[578] = (void *)fm_deletecontent;
	CUFARR[579] = (void *)fm_getattr;
	CUFARR[580] = (void *)fm_setattr;
	CUFARR[581] = (void *)fm_getlabel;
	CUFARR[582] = (void *)fm_setlabel;
	CUFARR[583] = (void *)fm_get_oem;
	CUFARR[584] = (void *)f_enterFS;
	CUFARR[585] = (void *)f_releaseFS;
	//CUFARR[586] = (void *)fm_wgetcwd;
	//CUFARR[587] = (void *)fm_wgetdcwd;
	//CUFARR[588] = (void *)fm_wchdir;
	//CUFARR[589] = (void *)fm_wmkdir;
	//CUFARR[590] = (void *)fm_wrmdir;
	//CUFARR[591] = (void *)fm_wfindfirst;
	//CUFARR[592] = (void *)fm_wfindnext;
	//CUFARR[593] = (void *)fm_wmove;
	//CUFARR[594] = (void *)fm_wrename;
	//CUFARR[595] = (void *)fm_wfilelength;
	//CUFARR[596] = (void *)fm_wopen;
	//CUFARR[597] = (void *)fm_wopen_nonsafe;
	//CUFARR[598] = (void *)fm_wtruncate;
	//CUFARR[599] = (void *)fm_wstat;
	//CUFARR[600] = (void *)fm_wgettimedate;
	//CUFARR[601] = (void *)fm_wsettimedate;
	//CUFARR[602] = (void *)fm_wdelete;
	//CUFARR[603] = (void *)fm_wdeletecontent;
	//CUFARR[604] = (void *)fm_wgetattr;
	//CUFARR[605] = (void *)fm_wsetattr;

	CUFARR[606] = (void *)f_dotest;
	CUFARR[607] = (void *)hcc_mem_init;
	CUFARR[608] = (void *)hcc_mem_start;
	CUFARR[609] = (void *)hcc_mem_stop;
	CUFARR[610] = (void *)hcc_mem_delete;
	CUFARR[611] = (void *)atmel_mcipdc_initfunc;
	CUFARR[612] = (void *)ARM_ants;
	CUFARR[613] = (void *)DISARM_ants;
	CUFARR[614] = (void *)deploye_ants;
	CUFARR[615] = (void *)init_Ants;
	CUFARR[616] = (void *)reset_FRAM_ants;
	CUFARR[617] = (void *)DeployIfNeeded;
	CUFARR[618] = (void *)update_stopDeploy_FRAM;
	CUFARR[619] = (void *)EPS_Init;
	CUFARR[620] = (void *)reset_FRAM_EPS;
	CUFARR[621] = (void *)reset_EPS_voltages;
	CUFARR[622] = (void *)get_EPS_mode_t;
	CUFARR[623] = (void *)EPS_Conditioning;
	CUFARR[624] = (void *)get_shut_CAM;
	CUFARR[625] = (void *)shut_CAM;
	CUFARR[626] = (void *)get_shut_ADCS;
	CUFARR[627] = (void *)shut_ADCS;
	CUFARR[628] = (void *)EnterFullMode;
	CUFARR[629] = (void *)EnterCruiseMode;
	CUFARR[630] = (void *)EnterSafeMode;
	CUFARR[631] = (void *)EnterCriticalMode;
	CUFARR[632] = (void *)WriteCurrentTelemetry;
	CUFARR[633] = (void *)convert_raw_voltage;
	CUFARR[634] = (void *)check_EPSTableCorrection;
	CUFARR[635] = (void *)Rx_logic;
	CUFARR[636] = (void *)pass_above_Ground;
	CUFARR[637] = (void *)trxvu_logic;
	CUFARR[638] = (void *)TRXVU_task;
	CUFARR[639] = (void *)Dump_task;
	CUFARR[640] = (void *)Transponder_task;
	CUFARR[641] = (void *)buildAndSend_beacon;
	CUFARR[642] = (void *)Beacon_task;
	CUFARR[643] = (void *)sendRequestToStop_dump;
	CUFARR[644] = (void *)sendRequestToStop_transponder;
	CUFARR[645] = (void *)lookForRequestToDelete_dump;
	CUFARR[646] = (void *)lookForRequestToDelete_transponder;
	CUFARR[647] = (void *)TRX_sendFrame;
	CUFARR[648] = (void *)TRX_getFrameData;
	CUFARR[649] = (void *)init_trxvu;
	CUFARR[650] = (void *)reset_FRAM_TRXVU;
	CUFARR[651] = (void *)unmute_Tx;
	CUFARR[652] = (void *)set_mute_time;
	CUFARR[653] = (void *)check_time_off_mute;
	CUFARR[654] = (void *)change_TRXVU_state;
	CUFARR[655] = (void *)change_trans_RSSI;
	CUFARR[656] = (void *)check_Tx_temp;
	CUFARR[657] = (void *)AdcsReadI2cAck;
	CUFARR[658] = (void *)AdcsGenericI2cCmd;
	CUFARR[659] = (void *)AdcsExecuteCommand;
	CUFARR[660] = (void *)InitTlmElements;
	CUFARR[661] = (void *)CreateTlmElementFiles;
	CUFARR[662] = (void *)AdcsGetMeasAngSpeed;
	CUFARR[663] = (void *)AdcsGetCssVector;
	CUFARR[664] = (void *)AdcsGetEstAngles;
	CUFARR[665] = (void *)UpdateTlmToSaveVector;
	CUFARR[666] = (void *)GetTlmToSaveVector;
	CUFARR[667] = (void *)UpdateTlmPeriodVector;
	CUFARR[668] = (void *)GetTlmPeriodVector;
	CUFARR[669] = (void *)UpdateTlmElementAtIndex;
	CUFARR[670] = (void *)GetOperationgFlags;
	CUFARR[671] = (void *)GetLastSaveTimes;
	CUFARR[672] = (void *)AdcsGetTlmOverrideFlag;
	CUFARR[673] = (void *)AdcsSetTlmOverrideFlag;
	CUFARR[674] = (void *)RestoreDefaultTlmElement;
	CUFARR[675] = (void *)GetTlmElementAtIndex;
	CUFARR[676] = (void *)GatherTlmAndData;
	CUFARR[677] = (void *)UpdateAdcsFramParameters;
	CUFARR[678] = (void *)AdcsInit;
	CUFARR[679] = (void *)AdcsTask;
	CUFARR[680] = (void *)AdcsTroubleShooting;
	CUFARR[681] = (void *)InitStateMachine;
	CUFARR[682] = (void *)UpdateAdcsStateMachine;
	CUFARR[683] = (void *)reset_APRS_list;
	CUFARR[684] = (void *)send_APRS_Dump;
	CUFARR[685] = (void *)check_APRS;
	CUFARR[686] = (void *)get_APRS_list;
	CUFARR[687] = (void *)check_delaycommand;
	CUFARR[688] = (void *)reset_delayCommand;
	CUFARR[689] = (void *)add_delayCommand;
	CUFARR[690] = (void *)get_delayCommand_list;
	CUFARR[691] = (void *)EmptyOldestCommand;
	CUFARR[692] = (void *)decode_TMpacket;
	CUFARR[693] = (void *)encode_TMpacket;
	CUFARR[694] = (void *)decode_TCpacket;
	CUFARR[695] = (void *)encode_TCpacket;
	CUFARR[696] = (void *)build_raw_ACK;
	CUFARR[697] = (void *)build_data_field_ACK;
	CUFARR[698] = (void *)print_TM_spl_packet;
	CUFARR[699] = (void *)InitCUF;
	CUFARR[700] = (void *)CUFSwitch;
	CUFARR[701] = (void *)GenerateSSH;
	CUFARR[702] = (void *)AuthenticateCUF;
	CUFARR[703] = (void *)SavePerminantCUF;
	CUFARR[704] = (void *)LoadPerminantCUF;
	CUFARR[705] = (void *)CUFManageRestart;
	CUFARR[706] = (void *)UpdatePerminantCUF;
	CUFARR[707] = (void *)RemoveCUF;
	CUFARR[708] = (void *)CUFTestFunction;
	CUFARR[709] = (void *)CUFTestFunction2;
	CUFARR[710] = (void *)ExecuteCUF;
	CUFARR[711] = (void *)IntegrateCUF;
	CUFARR[712] = (void *)AddToCUFTempArray;
	CUFARR[713] = (void *)GetFromCUFTempArray;
	CUFARR[714] = (void *)CUFTest;
	CUFARR[715] = (void *)DisableCUF;
	CUFARR[716] = (void *)EnableCUF;
	CUFARR[717] = (void *)CheckResetThreashold;
	CUFARR[718] = (void *)castCharPointerToInt;
	CUFARR[719] = (void *)castCharArrayToIntArray;
	CUFARR[720] = (void *)getExtendedName;
	CUFARR[721] = (void *)addToArray;
	CUFARR[722] = (void *)loadBackup;
	CUFARR[723] = (void *)saveBackup;
	CUFARR[724] = (void *)saveAck;
	CUFARR[725] = (void *)initializeUpload;
	CUFARR[726] = (void *)startCUFintegration;
	CUFARR[727] = (void *)headerHandle;
	CUFARR[728] = (void *)removeFiles;
	CUFARR[729] = (void *)xSemaphoreGive_extended;
	CUFARR[730] = (void *)xSemaphoreTake_extended;
	CUFARR[731] = (void *)init_GenericSaveQueue;
	CUFARR[732] = (void *)add_GenericElement_queue;
	CUFARR[733] = (void *)GenericSave_Task;
	CUFARR[734] = (void *)check_int;
	CUFARR[735] = (void *)check_portBASE_TYPE;
	CUFARR[736] = (void *)BigEnE_raw_to_uInt;
	CUFARR[737] = (void *)BigEnE_uInt_to_raw;
	CUFARR[738] = (void *)BigEnE_raw_to_uShort;
	CUFARR[739] = (void *)BigEnE_raw_value;
	CUFARR[740] = (void *)print_array;
	CUFARR[741] = (void *)switch_endian;
	CUFARR[742] = (void *)double_little_endian;
	CUFARR[743] = (void *)soft_reset_subsystem;
	CUFARR[744] = (void *)hard_reset_subsystem;
	CUFARR[745] = (void *)fram_byte_fix;
	CUFARR[746] = (void *)comapre_string;
	CUFARR[747] = (void *)getBitValueByIndex;
	CUFARR[748] = (void *)reset_FRAM_MAIN;
	CUFARR[749] = (void *)FRAM_write_exte;
	CUFARR[750] = (void *)FRAM_read_exte;
	CUFARR[751] = (void *)FRAM_writeAndVerify_exte;
	CUFARR[752] = (void *)init_GP;
	CUFARR[753] = (void *)get_system_state;
	CUFARR[754] = (void *)set_system_state;
	CUFARR[755] = (void *)get_systems_state_param;
	CUFARR[756] = (void *)get_current_global_param;
	CUFARR[757] = (void *)get_Vbatt;
	CUFARR[758] = (void *)set_Vbatt;
	CUFARR[759] = (void *)set_curBat;
	CUFARR[760] = (void *)get_curBat;
	CUFARR[761] = (void *)get_cur3V3;
	CUFARR[762] = (void *)set_cur3V3;
	CUFARR[763] = (void *)get_cur5V;
	CUFARR[764] = (void *)set_cur5V;
	CUFARR[765] = (void *)get_tempComm_LO;
	CUFARR[766] = (void *)set_tempComm_LO;
	CUFARR[767] = (void *)get_tempComm_PA;
	CUFARR[768] = (void *)set_tempComm_PA;
	CUFARR[769] = (void *)get_tempEPS;
	CUFARR[770] = (void *)set_tempEPS;
	CUFARR[771] = (void *)get_tempBatt;
	CUFARR[772] = (void *)set_tempBatt;
	CUFARR[773] = (void *)get_RxDoppler;
	CUFARR[774] = (void *)set_RxDoppler;
	CUFARR[775] = (void *)get_RxRSSI;
	CUFARR[776] = (void *)set_RxDoppler;
	CUFARR[777] = (void *)get_RxRSSI;
	CUFARR[778] = (void *)set_RxRSSI;
	CUFARR[779] = (void *)get_TxRefl;
	CUFARR[780] = (void *)set_TxRefl;
	CUFARR[781] = (void *)get_TxForw;
	CUFARR[782] = (void *)set_TxForw;
	CUFARR[783] = (void *)get_Attitude;
	CUFARR[784] = (void *)set_Attitude;
	CUFARR[785] = (void *)get_FS_failFlag;
	CUFARR[786] = (void *)set_FS_failFlag;
	CUFARR[787] = (void *)get_EPSState;
	CUFARR[788] = (void *)set_EPSState;
	CUFARR[789] = (void *)get_numOfDelayedCommand;
	CUFARR[790] = (void *)set_numOfDelayedCommand;
	CUFARR[791] = (void *)set_numOfResets;
	CUFARR[792] = (void *)get_numOfResets;
	CUFARR[793] = (void *)set_lastReset;
	CUFARR[794] = (void *)get_lastReset;
	CUFARR[795] = (void *)set_ground_conn;
	CUFARR[796] = (void *)get_ground_conn;
	CUFARR[797] = (void *)WriteErrorLog;
	CUFARR[798] = (void *)WriteResetLog;
	CUFARR[799] = (void *)WritePayloadLog;
	CUFARR[800] = (void *)WriteEpsLog;
	CUFARR[801] = (void *)WriteTransponderLog;
	CUFARR[802] = (void *)WriteAdcsLog;
	CUFARR[803] = (void *)WriteCUFLog;
	CUFARR[804] = (void *)get_item_by_index;
	CUFARR[805] = (void *)init_onlineParam;
	CUFARR[806] = (void *)reset_offline_TM_list;
	CUFARR[807] = (void *)get_online_packet;
	CUFARR[808] = (void *)get_offlineSettingPacket;
	CUFARR[809] = (void *)delete_onlineTM_param_from_offline;
	CUFARR[810] = (void *)add_onlineTM_param_to_save_list;
	CUFARR[811] = (void *)save_onlineTM_task;
	CUFARR[812] = (void *)f_managed_enterFS;
	CUFARR[813] = (void *)f_managed_releaseFS;
	CUFARR[814] = (void *)f_managed_open;
	CUFARR[815] = (void *)f_managed_close;
	CUFARR[816] = (void *)reset_FRAM_FS;
	CUFARR[817] = (void *)sd_format;
	CUFARR[818] = (void *)deleteDir;
	CUFARR[819] = (void *)InitializeFS;
	CUFARR[820] = (void *)DeInitializeFS;
	CUFARR[821] = (void *)c_fileCreate;
	CUFARR[822] = (void *)c_fileWrite;
	CUFARR[823] = (void *)c_fileDeleteElements;
	//CUFARR[824] = (void *)c_fileGetNumOfElements;
	CUFARR[825] = (void *)c_fileRead;
	CUFARR[826] = (void *)print_file;
	CUFARR[827] = (void *)c_fileReset;
	//CUFARR[828] = (void *)FS_test;
	//CUFARR[829] = (void *)test_i;
	CUFARR[830] = (void *)init_command;
	CUFARR[831] = (void *)add_command;
	CUFARR[832] = (void *)get_command;
	CUFARR[833] = (void *)act_upon_command;
	CUFARR[834] = (void *)AUC_COMM;
	CUFARR[835] = (void *)AUC_general;
	CUFARR[836] = (void *)AUC_payload;
	CUFARR[837] = (void *)AUC_EPS;
	//CUFARR[838] = (void *)AUC_ADCS;
	CUFARR[839] = (void *)AUC_SW;
	CUFARR[840] = (void *)AUC_onlineTM;
	CUFARR[841] = (void *)AUC_CUF;
	CUFARR[842] = (void *)save_ACK;
	CUFARR[843] = (void *)SP_HK_collect;
	CUFARR[844] = (void *)EPS_HK_collect;
	CUFARR[845] = (void *)CAM_HK_collect;
	CUFARR[846] = (void *)COMM_HK_collect;
	CUFARR[847] = (void *)FS_HK_collect;
	CUFARR[848] = (void *)HK_find_fileName;
	CUFARR[849] = (void *)HK_findElementSize;
	CUFARR[850] = (void *)build_HK_spl_packet;
	CUFARR[851] = (void *)InitSubsystems;
	CUFARR[852] = (void *)SubSystemTaskStart;
	CUFARR[853] = (void *)AdcsCmdQueueInit;
	CUFARR[854] = (void *)AdcsCmdQueueGet;
	CUFARR[855] = (void *)AdcsCmdQueueAdd;
	CUFARR[856] = (void *)AdcsCmdQueueIsEmpty;
	CUFARR[857] = (void *)getAdcsQueueWaitPointer;
	CUFARR[858] = (void *)cmd_mute;
	CUFARR[859] = (void *)cmd_unmute;
	CUFARR[860] = (void *)cmd_active_trans;
	CUFARR[861] = (void *)cmd_shut_trans;
	CUFARR[862] = (void *)cmd_change_trans_rssi;
	CUFARR[863] = (void *)cmd_aprs_dump;
	CUFARR[864] = (void *)cmd_stop_dump;
	CUFARR[865] = (void *)cmd_time_frequency;
	CUFARR[866] = (void *)cmd_change_def_bit_rate;
	CUFARR[867] = (void *)cmd_upload_volt_logic;
	CUFARR[868] = (void *)cmd_heater_temp;
	CUFARR[869] = (void *)cmd_upload_volt_COMM;
	CUFARR[870] = (void *)cmd_SHUT_ADCS;
	CUFARR[871] = (void *)cmd_SHUT_CAM;
	CUFARR[872] = (void *)cmd_allow_ADCS;
	CUFARR[873] = (void *)cmd_allow_CAM;
	CUFARR[874] = (void *)cmd_update_alpha;
	CUFARR[875] = (void *)cmd_resetGWT;
	CUFARR[876] = (void *)cmd_delete_TM;
	CUFARR[877] = (void *)cmd_reset_file;
	CUFARR[878] = (void *)cmd_format_SD;
	CUFARR[879] = (void *)cmd_dummy_func;
	CUFARR[880] = (void *)cmd_generic_I2C;
	CUFARR[881] = (void *)cmd_dump;
	CUFARR[882] = (void *)cmd_resume_TM;
	CUFARR[883] = (void *)cmd_stop_TM;
	CUFARR[884] = (void *)cmd_soft_reset_cmponent;
	CUFARR[885] = (void *)cmd_reset_satellite;
	CUFARR[886] = (void *)cmd_gracefull_reset_satellite;
	CUFARR[887] = (void *)cmd_hard_reset_cmponent;
	CUFARR[888] = (void *)cmd_reset_TLM_SD;
	CUFARR[889] = (void *)cmd_upload_time;
	CUFARR[890] = (void *)cmd_ARM_DIARM;
	CUFARR[891] = (void *)cmd_deploy_ants;
	CUFARR[892] = (void *)cmd_get_onlineTM;
	CUFARR[893] = (void *)cmd_reset_off_line;
	CUFARR[894] = (void *)cmd_add_item_off_line;
	CUFARR[895] = (void *)cmd_delete_item_off_line;
	CUFARR[896] = (void *)cmd_get_off_line_setting;
	CUFARR[897] = (void *)cmd_reset_APRS_list;
	CUFARR[898] = (void *)cmd_reset_delayed_command_list;
	CUFARR[899] = (void *)cmd_reset_FRAM;
	CUFARR[900] = (void *)cmd_FRAM_read;
	CUFARR[901] = (void *)cmd_FRAM_write;
	CUFARR[902] = (void *)CreateImageThumbnail_withoutSearch;
	CUFARR[903] = (void *)CreateImageThumbnail;
	CUFARR[904] = (void *)compressImage;
	CUFARR[905] = (void *)RGB_to_YCC;
	CUFARR[906] = (void *)RGB_to_Y;
	CUFARR[907] = (void *)Y_to_YCC;
	CUFARR[908] = (void *)dct;
	CUFARR[909] = (void *)JPEG_compressor;
	CUFARR[910] = (void *)Create_BMP_File;
	CUFARR[911] = (void *)CompressImage;
	CUFARR[912] = (void *)compress_image_to_jpeg_file;
	CUFARR[913] = (void *)compress_image_to_jpeg_file_in_memory;
	CUFARR[914] = (void *)clear;
	CUFARR[915] = (void *)init;
	CUFARR[916] = (void *)deinit;
	CUFARR[917] = (void *)process_scanline;
	CUFARR[918] = (void *)jpge_malloc;
	CUFARR[919] = (void *)jpge_cmalloc;
	CUFARR[920] = (void *)jpge_free;
	CUFARR[921] = (void *)check;
	CUFARR[922] = (void *)open_jpeg_file;
	CUFARR[923] = (void *)close_jpeg_file;
	CUFARR[924] = (void *)init_jpeg_mem;
	CUFARR[925] = (void *)close_jpeg_mem;
	CUFARR[926] = (void *)put_buf;
	CUFARR[927] = (void *)get_size;
	CUFARR[928] = (void *)TransformRawToBMP;
	CUFARR[929] = (void *)getDataBaseStart;
	CUFARR[930] = (void *)getDataBaseEnd;
	CUFARR[931] = (void *)updateGeneralDataBaseParameters;
	CUFARR[932] = (void *)initImageDataBase;
	CUFARR[933] = (void *)resetImageDataBase;
	CUFARR[934] = (void *)getLatestID;
	CUFARR[935] = (void *)getNumberOfFrames;
	CUFARR[936] = (void *)handleMarkedPictures;
	CUFARR[937] = (void *)takePicture;
	CUFARR[938] = (void *)takePicture_withSpecialParameters;
	CUFARR[939] = (void *)setCameraPhotographyValues;
	CUFARR[940] = (void *)transferImageToSD;
	CUFARR[941] = (void *)DeleteImageFromOBC;
	CUFARR[942] = (void *)DeleteImageFromPayload;
	CUFARR[943] = (void *)clearImageDataBase;
	CUFARR[944] = (void *)GetImageFileName;
	CUFARR[945] = (void *)getImageDataBaseBuffer;
	CUFARR[946] = (void *)readImageFromBuffer;
	CUFARR[947] = (void *)saveImageToBuffer;
	CUFARR[948] = (void *)SearchDataBase_byID;
	CUFARR[949] = (void *)SearchDataBase_forLatestMarkedImage;
	CUFARR[950] = (void *)SearchDataBase_forImageFileType_byTimeRange;
	CUFARR[951] = (void *)updateFileTypes;
	CUFARR[952] = (void *)GetImageFactor;
	CUFARR[953] = (void *)getDataBaseSize;
	CUFARR[954] = (void *)setAutoThumbnailCreation;
	CUFARR[955] = (void *)Gecko_TroubleShooter;
	CUFARR[956] = (void *)SearchDataBase_forImageFileType_byTimeRange;
	CUFARR[957] = (void *)initGecko;
	CUFARR[958] = (void *)GECKO_Reset;
	CUFARR[959] = (void *)create_xGeckoStateSemaphore;
	CUFARR[960] = (void *)Initialized_GPIO;
	CUFARR[961] = (void *)De_Initialized_GPIO;
	CUFARR[962] = (void *)TurnOnGecko;
	CUFARR[963] = (void *)TurnOffGecko;
	CUFARR[964] = (void *)getPIOs;
	CUFARR[965] = (void *)gecko_get_current_3v3;
	CUFARR[966] = (void *)gecko_get_voltage_3v3;
	CUFARR[967] = (void *)gecko_get_current_5v;
	CUFARR[968] = (void *)gecko_get_voltage_5v;
	CUFARR[969] = (void *)GECKO_TakeImage;
	//CUFARR[970] = (void *)readStopFlag;
	//CUFARR[971] = (void *)setStopFlag;
	CUFARR[972] = (void *)GECKO_ReadImage;
	CUFARR[973] = (void *)GECKO_EraseBlock;
	CUFARR[974] = (void *)GECKO_GetRegister;
	CUFARR[975] = (void *)GECKO_SetRegister;
	CUFARR[976] = (void *)convert2char;
	CUFARR[977] = (void *)convert2byte;
	CUFARR[978] = (void *)bits2byte;
	CUFARR[979] = (void *)byte2bits;
	CUFARR[980] = (void *)char2bits;
	CUFARR[981] = (void *)bits2char;
	CUFARR[982] = (void *)ReadFromFile;
	CUFARR[983] = (void *)WriteToFile;
	CUFARR[984] = (void *)AutomaticImageHandlerTaskMain;
	CUFARR[985] = (void *)KickStartAutomaticImageHandlerTask;
	CUFARR[986] = (void *)initAutomaticImageHandlerTask;
	CUFARR[987] = (void *)get_automatic_image_handling_task_suspension_flag;
	CUFARR[988] = (void *)set_automatic_image_handling_task_suspension_flag;
	CUFARR[989] = (void *)get_automatic_image_handling_ready_for_long_term_stop;
	CUFARR[990] = (void *)set_automatic_image_handling_ready_for_long_term_stop;
	CUFARR[991] = (void *)stopAction;
	CUFARR[992] = (void *)resumeAction;
	CUFARR[993] = (void *)checkIfInAutomaticImageHandlingTask;
	CUFARR[994] = (void *)CameraManagerTaskMain;
	//CUFARR[995] = (void *)handleDump;
	CUFARR[996] = (void *)initCamera;
	CUFARR[997] = (void *)KickStartCamera;
	CUFARR[998] = (void *)addRequestToQueue;
	CUFARR[999] = (void *)resetQueue;
	CUFARR[1000] = (void *)send_request_to_reset_database;
	CUFARR[1001] = (void *)TakePicture;
	CUFARR[1002] = (void *)TakeSpecialPicture;
	CUFARR[1003] = (void *)DeletePictureFile;
	CUFARR[1004] = (void *)DeletePicture;
	CUFARR[1005] = (void *)TransferPicture;
	//CUFARR[1006] = (void *)GetImageDataBase;
	CUFARR[1007] = (void *)CreateThumbnail;
	CUFARR[1008] = (void *)CreateJPG;
	CUFARR[1009] = (void *)UpdatePhotographyValues;
	CUFARR[1010] = (void *)setChunkDimensions;
	CUFARR[1011] = (void *)GeckoSetRegister;
	CUFARR[1012] = (void *)GetChunkFromImage;
	CUFARR[1013] = (void *)SimpleButcher;
	CUFARR[1014] = (void *)setChunkDimensions_inFRAM;
	CUFARR[1015] = (void *)imageDump_task;
	CUFARR[1016] = (void *)KickStartImageDumpTask;
	CUFARR[1017] = (void *)SendGeckoRegisters;
	CUFARR[1018] = (void *)cspaceADCS_initialize;
	CUFARR[1019] = (void *)cspaceADCS_componentReset;
	CUFARR[1020] = (void *)cspaceADCS_setCurrentTime;
	CUFARR[1021] = (void *)cspaceADCS_cacheStateADCS;
	CUFARR[1022] = (void *)cspaceADCS_setRunMode;
	CUFARR[1023] = (void *)cspaceADCS_setPwrCtrlDevice;
	CUFARR[1024] = (void *)cspaceADCS_clearErrors;
	CUFARR[1025] = (void *)cspaceADCS_setMagOutput;
	CUFARR[1026] = (void *)cspaceADCS_setWheelSpeed;
	CUFARR[1027] = (void *)cspaceADCS_deployMagBoomADCS;
	CUFARR[1028] = (void *)cspaceADCS_setAttCtrlMode;
	CUFARR[1029] = (void *)cspaceADCS_setAttEstMode;
	CUFARR[1030] = (void *)cspaceADCS_setCam1SensorConfig;
	CUFARR[1031] = (void *)cspaceADCS_setCam2SensorConfig;
	CUFARR[1032] = (void *)cspaceADCS_setMagMountConfig;
	CUFARR[1033] = (void *)cspaceADCS_setMagOffsetScalingConfig;
	CUFARR[1034] = (void *)cspaceADCS_setMagSensConfig;
	CUFARR[1035] = (void *)cspaceADCS_setRateSensorConfig;
	CUFARR[1036] = (void *)cspaceADCS_setStarTrackerConfig;
	CUFARR[1037] = (void *)cspaceADCS_setEstimationParam1;
	CUFARR[1038] = (void *)cspaceADCS_setEstimationParam2;
	CUFARR[1039] = (void *)cspaceADCS_setMagnetometerMode;
	CUFARR[1040] = (void *)cspaceADCS_setSGP4OrbitParameters;
	CUFARR[1041] = (void *)cspaceADCS_saveConfig;
	CUFARR[1042] = (void *)cspaceADCS_saveOrbitParam;
	CUFARR[1043] = (void *)cspaceADCS_saveImage;
	CUFARR[1044] = (void *)cspaceADCS_BLSetBootIndex;
	CUFARR[1045] = (void *)cspaceADCS_BLRunSelectedProgram;
	CUFARR[1046] = (void *)cspaceADCS_getGeneralInfo;
	CUFARR[1047] = (void *)cspaceADCS_getBootProgramInfo;
	CUFARR[1048] = (void *)cspaceADCS_getSRAMLatchupCounters;
	CUFARR[1049] = (void *)cspaceADCS_getEDACCounters;
	CUFARR[1050] = (void *)cspaceADCS_getCurrentTime;
	CUFARR[1051] = (void *)cspaceADCS_getCommStatus;
	CUFARR[1052] = (void *)cspaceADCS_getCurrentState;
	CUFARR[1053] = (void *)cspaceADCS_getMagneticFieldVec;
	CUFARR[1054] = (void *)cspaceADCS_getCoarseSunVec;
	CUFARR[1055] = (void *)cspaceADCS_getFineSunVec;
	CUFARR[1056] = (void *)cspaceADCS_getNadirVector;
	CUFARR[1057] = (void *)cspaceADCS_getSensorRates;
	CUFARR[1058] = (void *)cspaceADCS_getWheelSpeed;
	CUFARR[1059] = (void *)cspaceADCS_getMagnetorquerCmd;
	CUFARR[1060] = (void *)cspaceADCS_getWheelSpeedCmd;
	CUFARR[1061] = (void *)cspaceADCS_getRawCam2Sensor;
	CUFARR[1062] = (void *)cspaceADCS_getRawCam1Sensor;
	CUFARR[1063] = (void *)cspaceADCS_getRawCss1_6Measurements;
	CUFARR[1064] = (void *)cspaceADCS_getRawCss7_10Measurements;
	CUFARR[1065] = (void *)cspaceADCS_getRawMagnetometerMeas;
	CUFARR[1066] = (void *)cspaceADCS_getCSenseCurrentMeasurements;
	CUFARR[1067] = (void *)cspaceADCS_getCControlCurrentMeasurements;
	CUFARR[1068] = (void *)cspaceADCS_getWheelCurrentsTlm;
	CUFARR[1069] = (void *)cspaceADCS_getADCSTemperatureTlm;
	CUFARR[1070] = (void *)cspaceADCS_getRateSensorTempTlm;
	CUFARR[1071] = (void *)cspaceADCS_getRawGpsStatus;
	CUFARR[1072] = (void *)cspaceADCS_getRawGpsTime;
	CUFARR[1073] = (void *)cspaceADCS_getRawGpsX;
	CUFARR[1074] = (void *)cspaceADCS_getRawGpsY;
	CUFARR[1075] = (void *)cspaceADCS_getRawGpsZ;
	CUFARR[1076] = (void *)cspaceADCS_getStateTlm;
	CUFARR[1077] = (void *)cspaceADCS_getADCSMeasurements;
	CUFARR[1078] = (void *)cspaceADCS_getActuatorsCmds;
	CUFARR[1079] = (void *)cspaceADCS_getEstimationMetadata;
	CUFARR[1080] = (void *)cspaceADCS_getRawSensorMeasurements;
	CUFARR[1081] = (void *)cspaceADCS_getPowTempMeasTLM;
	CUFARR[1082] = (void *)cspaceADCS_getADCSExcTimes;
	CUFARR[1083] = (void *)cspaceADCS_getPwrCtrlDevice;
	CUFARR[1084] = (void *)cspaceADCS_getMiscCurrentMeas;
	CUFARR[1085] = (void *)cspaceADCS_getCommandedAttitudeAngles;
	CUFARR[1086] = (void *)cspaceADCS_getADCSRawGPSMeas;
	CUFARR[1087] = (void *)cspaceADCS_getStartrackerTLM;
	CUFARR[1088] = (void *)cspaceADCS_getADCRawRedMag;
	CUFARR[1089] = (void *)cspaceADCS_getACPExecutionState;
	CUFARR[1090] = (void *)cspaceADCS_getADCSConfiguration;
	CUFARR[1091] = (void *)cspaceADCS_getSGP4OrbitParameters;
	CUFARR[1092] = (void *)GomEpsInitialize;
	CUFARR[1093] = (void *)GomEpsPing;
	CUFARR[1094] = (void *)GomEpsSoftReset;
	CUFARR[1095] = (void *)GomEpsHardReset;
	CUFARR[1096] = (void *)GomEpsGetHkData_param;
	CUFARR[1097] = (void *)GomEpsGetHkData_general;
	CUFARR[1098] = (void *)GomEpsGetHkData_vi;
	CUFARR[1099] = (void *)GomEpsGetHkData_out;
	CUFARR[1100] = (void *)GomEpsGetHkData_wdt;
	CUFARR[1101] = (void *)GomEpsGetHkData_basic;
	CUFARR[1102] = (void *)GomEpsSetOutput;
	CUFARR[1103] = (void *)GomEpsSetSingleOutput;
	CUFARR[1104] = (void *)GomEpsSetPhotovoltaicInputs;
	CUFARR[1105] = (void *)GomEpsSetPptMode;
	CUFARR[1106] = (void *)GomEpsSetHeaterAutoMode;
	CUFARR[1107] = (void *)GomEpsResetCounters;
	CUFARR[1108] = (void *)GomEpsResetWDT;
	CUFARR[1109] = (void *)GomEpsConfigCMD;
	CUFARR[1110] = (void *)GomEpsConfigGet;
	CUFARR[1111] = (void *)GomEpsConfigSet;
	CUFARR[1112] = (void *)GomEpsConfig2CMD;
	CUFARR[1113] = (void *)GomEpsConfig2Get;
	CUFARR[1114] = (void *)GomEpsConfig2Set;
	CUFARR[1115] = (void *)IsisAntS_initialize;
	CUFARR[1116] = (void *)IsisAntS_setArmStatus;
	CUFARR[1117] = (void *)IsisAntS_attemptDeployment;
	CUFARR[1118] = (void *)IsisAntS_getStatusData;
	CUFARR[1119] = (void *)IsisAntS_getTemperature;
	CUFARR[1120] = (void *)IsisAntS_getUptime;
	CUFARR[1121] = (void *)IsisAntS_getAlltelemetry;
	CUFARR[1122] = (void *)IsisAntS_getActivationCount;
	CUFARR[1123] = (void *)IsisAntS_getActivationTime;
	CUFARR[1124] = (void *)IsisAntS_reset;
	CUFARR[1125] = (void *)IsisAntS_cancelDeployment;
	CUFARR[1126] = (void *)IsisAntS_autoDeployment;
	CUFARR[1127] = (void *)IsisEPS_initialize;
	CUFARR[1128] = (void *)IsisEPS_hardReset;
	CUFARR[1129] = (void *)IsisEPS_noOperation;
	CUFARR[1130] = (void *)IsisEPS_cancelOperation;
	CUFARR[1131] = (void *)IsisEPS_resetWDT;
	CUFARR[1132] = (void *)IsisEPS_outputBusGroupOn;
	CUFARR[1133] = (void *)IsisEPS_outputBusGroupOff;
	CUFARR[1134] = (void *)IsisEPS_outputBusGroupState;
	CUFARR[1135] = (void *)IsisEPS_outputBus3v3On;
	CUFARR[1136] = (void *)IsisEPS_outputBus3v3Off;
	CUFARR[1137] = (void *)IsisEPS_outputBus5vOn;
	CUFARR[1138] = (void *)IsisEPS_outputBus5vOff;
	CUFARR[1139] = (void *)IsisEPS_outputBusHvOn;
	CUFARR[1140] = (void *)IsisEPS_outputBusHvOff;
	CUFARR[1141] = (void *)IsisEPS_getSystemState;
	CUFARR[1142] = (void *)IsisEPS_getRawHKDataMB;
	CUFARR[1143] = (void *)IsisEPS_getEngHKDataMB;
	CUFARR[1144] = (void *)IsisEPS_getRAEngHKDataMB;
	CUFARR[1145] = (void *)IsisEPS_getRawHKDataCDB;
	CUFARR[1146] = (void *)IsisEPS_getEngHKDataCDB;
	CUFARR[1147] = (void *)IsisEPS_getRAEngHKDataCDB;
	CUFARR[1148] = (void *)IsisEPS_getLOState3V3;
	CUFARR[1149] = (void *)IsisEPS_getLOState5V;
	CUFARR[1150] = (void *)IsisEPS_getLOStateBV;
	CUFARR[1151] = (void *)IsisEPS_getLOStateHV;
	CUFARR[1152] = (void *)IsisEPS_getParSize;
	CUFARR[1153] = (void *)IsisEPS_getParameter;
	CUFARR[1154] = (void *)IsisEPS_setParameter;
	CUFARR[1155] = (void *)IsisEPS_resetParameter;
	CUFARR[1156] = (void *)IsisEPS_resetConfig;
	CUFARR[1157] = (void *)IsisEPS_loadConfig;
	CUFARR[1158] = (void *)IsisEPS_saveConfig;
	CUFARR[1159] = (void *)isisMTQv1_initialize;
	CUFARR[1160] = (void *)IsisMTQv1SetResolution;
	CUFARR[1161] = (void *)IsisMTQv1GetTemperature;
	CUFARR[1162] = (void *)IsisMTQv2_initialize;
	CUFARR[1163] = (void *)IsisMTQv2_softReset;
	CUFARR[1164] = (void *)IsisMTQv2_noOperation;
	CUFARR[1165] = (void *)IsisMTQv2_cancelOperation;
	CUFARR[1166] = (void *)IsisMTQv2_startMTMMeasurement;
	CUFARR[1167] = (void *)IsisMTQv2_startMTQActuationCurrent;
	CUFARR[1168] = (void *)IsisMTQv2_startMTQActuationDipole;
	CUFARR[1169] = (void *)IsisMTQv2_startMTQActuationPWM;
	CUFARR[1170] = (void *)IsisMTQv2_startSelfTest;
	CUFARR[1171] = (void *)IsisMTQv2_startDetumble;
	CUFARR[1172] = (void *)IsisMTQv2_getSystemState;
	CUFARR[1173] = (void *)IsisMTQv2_getRawMTMData;
	CUFARR[1174] = (void *)IsisMTQv2_getCalMTMData;
	CUFARR[1175] = (void *)IsisMTQv2_getCoilCurrent;

}


