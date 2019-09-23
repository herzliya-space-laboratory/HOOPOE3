#include "uploadCodeTelemetry.h"

#include <hal/Drivers/I2C.h>
#include <hal/Storage/FRAM.h>
#include <hcc/api_fat.h>
#include <stdlib.h>
#include <string.h>

#include "../Global/TLM_management.h"
#include "../TRXVU.h"
static unsigned char uploadCodeArry[uploadCodeLength]; //Set global code array
static Boolean uploadBoolArray[uploadCodeLength/frameLength];


void initializeUpload()
{
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
	F_FILE *fp = NULL;
	f_managed_open(fbackupName,"w+",&fp);
	if (fp != NULL)
		f_delete(fbackupName);
	else
		saveAck("Failed to open file on removeFiles codeArray");

	f_managed_open(fboolBackupName,"w+",&fp);
	if (fp != NULL)
		f_delete(fboolBackupName);
	else
	 	saveAck("Failed to open file on removeFiles boolArray");
	f_managed_open(fheaderName, "w+", &fp);
	if (fp != NULL)
		f_delete(fheaderName);
	else
		saveAck("Failed to open file on removeFiles headerFile");
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

void loadBackup()
{
	int i = 0;
	for (; i < uploadCodeLength; i++)
	{
		uploadCodeArry[i] = 0;
	}
	F_FILE *fp = NULL;
	f_managed_open(fbackupName,"r",&fp);
	if (fp != NULL)
		f_read(uploadCodeArry,uploadCodeLength,1,fp);//Read to array
	else
		saveAck("Failed to open file on loadBackup boolArray");
	f_managed_close(&fp);

	i = 0;
	for( ; i < boolArrayLength;i++)
	{
		uploadBoolArray[i] = 0;
	}
	f_managed_open(fboolBackupName,"r",&fp);
	if (fp != NULL)
		f_read(uploadBoolArray, boolArrayLength, 1, fp);
	else
		saveAck("Failed to open file on loadBackup boolArray");
	f_managed_close(&fp);
}

void addToArray(TC_spl decode, int framePlace)
{
	saveAck("Start to save code!!!");
	int i = 1;
	for (; i < decode.length - 3; i++)
	{
		uploadCodeArry[framePlace*frameLength+i-1] = decode.data[i+3];//!!may not be used!!Upload code array (sizeof(int))*2)<is used to pass the frame parameters data
	}
	saveAck("Ack(wrote to array)");
	saveAck("Finished!!");
	uploadBoolArray[framePlace] = 1;
	TRX_sendFrame((byte*)uploadBoolArray, (uint8_t)boolArrayLength);
}

void headerHandle(TC_spl decode)
{
	initializeUpload();
	F_FILE *fp = NULL;
	f_managed_open(fheaderName, "w+", &fp);
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
	f_managed_close(&fp);
}

void startCUFintegration()
{
	unsigned char settings[19];
	F_FILE *fp = NULL;
	f_managed_open(fheaderName, "r", &fp);
	if (fp != NULL)
		f_read(settings, 18, 1, fp);
	else
		saveAck("File wont open launch header");
	int error = f_getlasterror();
	printf("%d", error);
	f_managed_close(&fp);
	settings[18] = 0;
	printf("Setting: %s\n", settings);
	//00-00-00-00-05-00-00-09-00-00-00-00-00-10-0a-ac-aa-aa-aa-aa-aa
	if (settings[0] <= 2 && settings[1] <= 2)
	{
		printf("Name: %s\n", (char*)(settings+10));
		printf("SSH: %u\n", castCharPointerToInt(settings+6));
		printf("Length: %u\n", castCharPointerToInt(settings+2));
		printf("IsTemp: %d\n", settings[0]);
		printf("disabled: %d\n", settings[1]);
		IntegrateCUF((char*)(settings+10), (int*)castCharArrayToIntArray(uploadCodeArry, castCharPointerToInt(settings+2)*4), castCharPointerToInt(settings+2), castCharPointerToInt(settings+6), settings[0], settings[1]);
	}
}

void saveBackup()
{
	printf("SAVE BACKUP STARTED"); //----------------------DEBUG
	F_FILE *fp = NULL;
	f_managed_open(fbackupName, "w+", &fp);//The name with its number
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
   	f_managed_close(&fp);

   	f_managed_open(fboolBackupName, "w+", &fp);
   	if (fp != NULL)
   		f_write(uploadBoolArray, 1, boolArrayLength, fp);
   	else
   		saveAck("File wont open saveBackup boolArray");
   	f_managed_close(&fp);
}

void saveAck(char* err)
{
    printf("%s", err);
}
