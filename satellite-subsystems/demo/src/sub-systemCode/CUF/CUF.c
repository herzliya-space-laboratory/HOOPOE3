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

//////////////////////defines///////////////////////

#define VTASKDELAY_CUF(xTicksToDelay)((void(*)(portTickType))(CUFSwitch(0)))(xTicksToDelay)

////////////////////////END/////////////////////////

static void* CUFArr[CUFARRSTORAGE]; //the array that accumilates the CUF functions
static CUF* PerminantCUF[PERMINANTCUFLENGTH]; //the array that accumilates perminant CUFs (CUF slot array) (CUFs to run on restart)
static CUF* TempCUF[PERMINANTCUFLENGTH]; //the array that accumilates temporary CUFs

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

int InitCUF()
{
	CUFArr[0] = (void*)vTaskDelay;
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
	return CUFArr[index]; //return the function at index
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
