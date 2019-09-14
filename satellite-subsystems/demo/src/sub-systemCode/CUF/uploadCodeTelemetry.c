#include "uploadCodeTelemetry.h"

#include <hal/Drivers/I2C.h>
#include <hal/Storage/FRAM.h>
#include <hcc/api_fat.h>
#include <stdlib.h>
#include <string.h>

#include "..\Global\TM_managment.h"
static unsigned char uploadCodeArry[uploadCodeLength]; //Set global code array
static Boolean uploadBoolArray[uploadCodeLength/frameLength];


void initializeUpload()
{
	InitCUF(TRUE);
	int i = 0;
	for (; i < uploadCodeLength; i++)
	{
		uploadCodeArry[i] = 0;
	}
	i=0;
	for( ; i < boolArrayLength;i++)
	{
		uploadBoolArray[i] = 0;
	}
	saveBackup();
}

void removeFiles()
{
	//WARNING DELETE OPENED FILE what happend if i delete not existed file??
	F_FILE *fp;
	fp = f_open(fbackupName,"w+");
	if (fp != NULL)
		f_delete(fbackupName);
	else
		saveAck("Failed to open file on removeFiles codeArray");

	fp = f_open(fboolBackupName,"w+");
	if (fp != NULL)
		f_delete(fboolBackupName);
	else
	 	saveAck("Failed to open file on removeFiles boolArray");
	fp = f_open(fheaderName, "w+");
	if (fp != NULL)
		f_delete(fheaderName);
	else
		saveAck("Failed to open file on removeFiles headerFile");
}

void loadBackup()
{
	int i = 0;
	for (; i < uploadCodeLength; i++)
	{
		uploadCodeArry[i] = 0;
	}
	F_FILE *fp;
	fp = f_open(fbackupName,"r");
	if (fp != NULL)
		f_read(uploadCodeArry,uploadCodeLength,1,fp);//Read to array
	else
		saveAck("Failed to open file on loadBackup boolArray");
	f_close(fp);

	i = 0;
	for( ; i < boolArrayLength;i++)
	{
		uploadBoolArray[i] = 0;
	}
	fp = f_open(fboolBackupName,"r");
	if (fp != NULL)
		f_read(uploadBoolArray, boolArrayLength, 1, fp);
	else
		saveAck("Failed to open file on loadBackup boolArray");
	f_close(fp);
}

void addToArray(TC_spl decode, int framePlace)
{
	saveAck("Start to save code!!!");
	int i = 1;
	for (; i < decode.length; i++)
	{
		uploadCodeArry[framePlace*frameLength+i-1] = decode.data[i+3];//!!may not be used!!Upload code array (sizeof(int))*2)<is used to pass the frame parameters data
	}
	saveAck("Ack(wrote to array)");
//	printf("Now array is \n %s", uploadCodeArry);
	saveAck("Finished!!");
	uploadBoolArray[framePlace] = 1;
	//CUFTest(uploadCodeArry);
}

void headerHandle(TC_spl decode)
{
	initializeUpload();
	F_FILE *fp;
	fp = f_open(fheaderName, "w+");
	if (fp != NULL)
	{
		int charWrote = f_write(decode.data, 1, decode.length, fp);
		if (charWrote != decode.length)
		{
			saveAck("Not wrote enough");
		}
	}
	else
		saveAck("File wont open headerHandle header");
	f_close(fp);
}

void startCUFintegration()
{
	unsigned char settings[20];
	F_FILE *fp = f_open(fheaderName, "r");
	if (fp != NULL)
		f_read(settings, 19, 1, fp);
	else
		saveAck("File wont open launch header");
	int error = f_getlasterror();
	printf("%d", error);
	f_close(fp);
	settings[19] = 0;
	printf("Setting: %s\n", settings);
	//00-00-00-00-05-00-00-09-00-00-00-00-00-10-0a-ac-aa-aa-aa-aa-aa
	if (settings[0] <= 2 && settings[1] <= 2 && settings[2] <= 2)
	{
		printf("Name: %s\n", (char*)(settings+11));
		printf("SSH: %u\n", castCharPointerToInt(settings+7));
		printf("Length: %u\n", castCharPointerToInt(settings+3));
		printf("IsTemp: %d\n", settings[0]);
		printf("isTask: %d\n", settings[1]);
		printf("disabled: %d\n", settings[2]);
		IntegrateCUF((char*)(settings+11), castCharArrayToIntArray(uploadCodeArry, castCharPointerToInt(settings+3)*4), castCharPointerToInt(settings+3), GenerateSSH(uploadCodeArry)/*castCharPointerToInt(settings+7)*/, settings[0], settings[1], settings[2]);
	}
}

void saveBackup()
{
	printf("SAVE BACKUP STARTED"); //----------------------DEBUG
	F_FILE *fp;
	fp = f_open(fbackupName, "w+");//The name with its number
	int error = f_getlasterror();
	printf("%d\n", error);
   	if (fp != NULL)
   	{
   		int charWrote = f_write(uploadCodeArry, 1, uploadCodeLength, fp);
   		if (charWrote != uploadCodeLength)
   		{
   			saveAck("Not wrote enough");
   		}
   	}
   	else
   		saveAck("File wont open saveBackup code");
   	f_close(fp);

   	fp = f_open(fboolBackupName, "w+");
   	if (fp != NULL)
   		f_write(uploadBoolArray, 1, boolArrayLength, fp);
   	else
   		saveAck("File wont open saveBackup boolArray");
   	f_close(fp);
}

void saveAck(char* err)
{
    printf("%s", err);
}
