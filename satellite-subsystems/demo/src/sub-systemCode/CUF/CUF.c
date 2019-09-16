/*
 * CUF.c
 *
 *  Created on: 15 áðåá 2018
 *      Author: USER1
 */

///////////////////////////////////////////////////Code Upload File Requirements////////////////////////////////////////////////////////

//////////////////////includes//////////////////////

#include "CUF.h"

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

static int currentCUFData[TEMPORARYCUFSTORAGESIZE]; //the array that accumilates the temporary CUFs data
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
	F_FILE* PerminantCUFData = f_open("CUFPer", "w+");
	//write all data members of all perminant CUFs
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (PerminantCUF[i]->name != NULL)
		{
			F_FILE* CUFdata = f_open(PerminantCUF[i]->name, "r");
			f_read(currentCUFData, PerminantCUF[i]->length, 1, CUFdata);
			f_close(CUFdata);
			f_write(currentCUFData, sizeof(int) * PerminantCUF[i]->length, sizeof(void*), PerminantCUFData);
			f_write(&PerminantCUF[i]->isTemporary, sizeof(Boolean), sizeof(void*), PerminantCUFData);
			f_write(&PerminantCUF[i]->SSH, sizeof(unsigned long), sizeof(void*), PerminantCUFData);
			f_write(PerminantCUF[i]->name, CUFNAMELENGTH, sizeof(void*), PerminantCUFData);
			f_write(&PerminantCUF[i]->hasTask, sizeof(Boolean), sizeof(void*), PerminantCUFData);
			f_write(&PerminantCUF[i]->disabled, sizeof(Boolean), sizeof(void*), PerminantCUFData);
		}
	}
	f_close(PerminantCUFData);
	return 0; //retun 0 on success
}

int LoadPerminantCUF()
{
	int i = 0;
	F_FILE* PerminantCUFData = f_open("CUFPer", "r");
	//read all data members of all perminant CUFs
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		f_read(PerminantCUF[i]->data, sizeof(int) * PerminantCUF[i]->length, sizeof(void*), PerminantCUFData);
		f_read(&PerminantCUF[i]->isTemporary, sizeof(Boolean), sizeof(void*), PerminantCUFData);
		f_read(&PerminantCUF[i]->SSH, sizeof(unsigned long), sizeof(void*), PerminantCUFData);
		f_read(PerminantCUF[i]->name, CUFNAMELENGTH, sizeof(void*), PerminantCUFData);
		f_read(&PerminantCUF[i]->hasTask, sizeof(Boolean), sizeof(void*), PerminantCUFData);
		f_read(&PerminantCUF[i]->disabled, sizeof(Boolean), sizeof(void*), PerminantCUFData);
	}
	f_close(PerminantCUFData);
	return 0; //return 0 on success
}

int CUFManageRestart()
{
	InitCUF(FALSE); //initiate the CUF switch array
	int i = 0;
	LoadPerminantCUF();
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
		if (PerminantCUF[i]->name != NULL && !PerminantCUF[i]->disabled) //check if CUF slot is empty
			ExecuteCUF(PerminantCUF[i]->name); //run CUF in slot if not empty
	return 0; //return 0 on success
}

int InitCUF(Boolean isFirstRun)
{
	CUFArr[0] = (void*)vTaskDelay;
	//reset the CUF slot array if first run
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; ++i)
	{
		PerminantCUF[i] = malloc(sizeof(CUF));
		TempCUF[i] = malloc(sizeof(CUF));
	}
	if (isFirstRun == TRUE)
	{
		for (i = 0; i < PERMINANTCUFLENGTH; ++i)
		{
			PerminantCUF[i]->data = NULL;
			PerminantCUF[i]->name = NULL;
			PerminantCUF[i]->SSH = 0;
			PerminantCUF[i]->length = 0;
		}
	}
	return 0; //return 0 on success
}

void* CUFSwitch(int index)
{
	return CUFArr[index]; //return the function at index
}

unsigned long GenerateSSH(unsigned char* data)
{
    unsigned char* str = (unsigned char*)data;
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash; //return the completed hash
}

int AuthenticateCUF(CUF* code)
{
	unsigned long newSSH;
	newSSH = GenerateSSH((char*)code->data); //generate an ssh for the data
	if (newSSH == 0) //check generation
		return -1; //return -1 on fail
	if (newSSH == code->SSH) //authenticate the ssh with the pre-made one
		return 1; //return 1 if authenticated
	else
		return 0; //return 0 if not authenticated
	return -1;
}

int UpdatePerminantCUF(CUF* code)
{
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; i++) //do for every slot in CUF slot array
	{
		if (PerminantCUF[i]->name == NULL) //check if slot is empty
		{
			//add new CUF to first empty slot
			PerminantCUF[i] = code;
			SavePerminantCUF();
			return 0; //return 0 on success
		}
	}
	return -3; //return -3 if failed
}

int RemoveCUF(char* name)
{
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
				PerminantCUF[i]->name = NULL;
				PerminantCUF[i]->SSH = 0;
				PerminantCUF[i]->length = 0;
			}
		}
	}
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (strcmp(TempCUF[i]->name, code->name) == 0) //find the slot with the CUF to remove
		{
			//remove CUF from temp array
			TempCUF[i]->data = NULL;
			TempCUF[i]->name = NULL;
			TempCUF[i]->SSH = 0;
			TempCUF[i]->length = 0;
			return 0; //return 0 on success
		}
	}
	return 0; //return 0 on success
}

int CreateCUF(CUF* code)
{
	if (code->isTemporary == FALSE && f_open(code->name, "r") != 0)
		return -2; //return -2 if perminant CUF and name already taken
	f_delete(code->name); //delete previous CUF (if one exists / user forgot to remove it)
	F_FILE* codeFile = f_open(code->name, "w+"); //make a new CUF
	if(codeFile == NULL || (int)f_write(code->data, sizeof(void*), code->length, codeFile)!=(int)code->length) //write the data to the CUF
	{
		//close file if failed
		f_flush(codeFile);
		f_close(codeFile);
		return -1; //return -1 if failed
	}
	//close file
	f_flush(codeFile);
	f_close(codeFile);
	if (code->isTemporary == FALSE)
		return UpdatePerminantCUF(code); //add file to CUF slot array if perminant and return the outcome
	return 0; //return 0 on success
}

int ExecuteCUF(char* name)
{
	CUF* code = GetFromCUFTempArray(name);
	F_FILE* codeFile = f_open(code->name, "r"); //open the given CUF
	int CUFDataSize = ((int)(f_filelength(code->name))) / sizeof(void*); //get the CUF length
	unsigned char* CUFDataStorage = NULL; //for perminant CUFs
	if (code->hasTask == TRUE)
		CUFDataStorage = malloc(code->length * sizeof(void*)); //allocate space for CUF if perminant
	if (code->hasTask == TRUE && CUFDataStorage == NULL) //check fail
	{
		free(CUFDataStorage);
		return -2; //return -2 on fail
	}
	int (*CUFFunction)(void* (*CUFSwitch)(int)) = NULL; //create the function pointer to convert the CUF into
	if (code->hasTask == TRUE)
	{
		//read a perminant CUF into the CUF function
		if ((int)f_read(CUFDataStorage, sizeof(void*), CUFDataSize, codeFile) != (int)(CUFDataSize)) //read data from CUF and check fail
		{
			free(CUFDataStorage);
			f_close(codeFile);
			return -2; //return -2 on fail
		}
		f_close(codeFile);
		CUFFunction = (int (*)(void* (*CUFSwitch)(int))) CUFDataStorage; //convert the CUF data to a function
		code->data = castCharArrayToIntArray((unsigned char*)CUFDataStorage, code->length*4);
	}
	else
	{
		//read a temporary CUF into the CUF function
		if ((int)f_read(currentCUFData, sizeof(void*), CUFDataSize, codeFile) != (int)(CUFDataSize)) //read data from CUF and check fail
		{
			f_close(codeFile);
			return -2; //return -2 on fail
		}
		f_close(codeFile);
		CUFFunction = (int (*)(void* (*CUFSwitch)(int))) currentCUFData; //convert the CUF data to a function
		code->data = currentCUFData;
	}
	if (CUFFunction == NULL)
		return -2; //return -2 on fail
	int auth = AuthenticateCUF(code);
	if (auth != 1)
		return -3;
	return CUFFunction(CUFSwitch); //call the CUF function with the test function as its parameter and return its output
}

CUF* IntegrateCUF(char* name, int* data, unsigned int length, unsigned long SSH, Boolean isTemporary, Boolean hasTask, Boolean disabled)
{
	CUF* cuf = malloc(sizeof(CUF));
	cuf->name = malloc(strlen(name));
	strcpy(cuf->name, name);
	cuf->data = data;
	cuf->length = length;
	cuf->SSH = SSH;
	cuf->isTemporary = isTemporary;
	cuf->hasTask = hasTask;
	cuf->disabled = disabled;
	if (cuf->SSH == 0 || cuf->name == NULL || cuf->data == NULL || cuf->length == 0)
	{
		printf("Invalid Paramiters\n");
		return NULL; //return NULL on fail
	}
	int auth = AuthenticateCUF(cuf);
	//check SSH
	if (auth == -1)
	{
		printf("Failed To Generate Authentication SSH\n");
		return NULL; //return NULL on fail
	}
	else if (auth == 0)
	{
		printf("SSH Un-Authenticated\n");
		return NULL; //return NULL on fail
	}
	printf("SSH Authenticated\n");
	if(CreateCUF(cuf)) //create a test CUF and check fail
	{
		printf("Failed To Create CUF\n");
		return NULL; //return NULL on fail
	}
	printf("CUF Created\n");
	AddToCUFTempArray(cuf);
	return cuf;
}

void AddToCUFTempArray(CUF* temp)
{
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (TempCUF[i] == 0 || strcmp(TempCUF[i]->name, temp->name) == 0)
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
	return NULL;
}

void DisableCUF(char* name)
{
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (strcmp(PerminantCUF[i]->name, name) == 0)
		{
			PerminantCUF[i]->disabled = TRUE;
		}
	}
}

void EnableCUF(char* name)
{
	int i = 0;
	for (i = 0; i < PERMINANTCUFLENGTH; i++)
	{
		if (strcmp(PerminantCUF[i]->name, name) == 0)
		{
			PerminantCUF[i]->disabled = FALSE;
		}
	}
}

Boolean CUFTest(int* testCUFData)
{
	int i;
	if (InitCUF(TRUE) != 0) //initiate the CUF architecture and check fail
		return TRUE; //return TRUE on fail
	printf("initiated switch\n");
	int testCUFDataCheck[] = { 0xe92d4800, 0xe28db004, 0xe24dd008, 0xe50b0008, 0xe51b3008, 0xe3a00000, 0xe12fff33, 0xe1a03000, 0xe59f001c, 0xe12fff33, 0xe3a00005, 0xeb000005, 0xe1a03000, 0xe2833001, 0xe1a00003, 0xe24bd004, 0xe8bd8800, 0x00001388, 0xe52db004, 0xe28db000, 0xe24dd00c, 0xe50b0008, 0xe51b3008, 0xe2833001, 0xe1a00003, 0xe28bd000, 0xe8bd0800, 0xe12fff1e }; //data for CUF test file
	for (i = 0; i < 28; i++)
	{
		if (testCUFDataCheck[i] != testCUFData[i])
		{
			printf("%d, %d, %d\n", testCUFDataCheck[i], testCUFData[i], i);
		}
	}
	CUF* testFile = IntegrateCUF((char*)"CodeFile001", testCUFData, 28, GenerateSSH(testCUFData), TRUE, FALSE, FALSE);
	if (testFile == NULL)
	{
		printf("Failed To Integrate CUF\n");
		return TRUE; //return TRUE on fail
	}
	int CUFreturnData = ExecuteCUF(testFile->name); //execute the test CUF
	if (CUFreturnData == -2) //check fail
	{
		printf("Failed To Execute CUF\n");
		return TRUE; //return TRUE on fail
	}
	printf("CUF Executed: ");
	printf("%d\n", CUFreturnData); //print the test CUFs return data
	if (RemoveCUF(testFile->name) == -1) //remove the CUF once done and check fail
	{
		printf("Failed To Remove CUF\n");
		return TRUE; //return TRUE on fail
	}
	printf("CUF Removed\n");
	return TRUE; //return TRUE on success
}
