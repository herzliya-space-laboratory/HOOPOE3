/*
 * DB_RequestHandling.c
 *
 *  Created on: Jun 6, 2019
 *      Author: Roy's Laptop
 */

#include <freertos/FreeRTOS.h>

#include <string.h>

#include "Camera.h"

#include "DB_RequestHandling.h"

DataBaseResult TakePicture(DataBase database, unsigned char* data)
{
	Boolean8bit isTestPattern;

	memcpy(&isTestPattern, data, sizeof(Boolean8bit));

	TurnOnGecko();

	DataBaseResult DB_result = takePicture(database, isTestPattern);

	return DB_result;
}
DataBaseResult TakeSpecialPicture(DataBase database, unsigned char* data)
{
	Boolean8bit isTestPattern;
	unsigned int frameAmount;
	unsigned int frameRate;
	unsigned char adcGain;
	unsigned char pgaGain;
	unsigned int exposure;

	memcpy(&isTestPattern, data, sizeof(Boolean8bit));
	memcpy(&frameAmount, data + 1, sizeof(int));
	memcpy(&frameRate, data + 5, sizeof(int));
	memcpy(&adcGain, data + 9, sizeof(char));
	memcpy(&pgaGain, data + 10, sizeof(char));
	memcpy(&exposure, data + 11, sizeof(int));

	TurnOnGecko();

	DataBaseResult DB_result = takePicture_withSpecialParameters(database, frameRate, adcGain, pgaGain, exposure, frameAmount, isTestPattern);

	return DB_result;
}
DataBaseResult DeletePicture(DataBase database, unsigned char* data)
{
	unsigned int cameraID;
	byte GroundFileType;

	memcpy(&cameraID, data, sizeof(int));
	memcpy(&GroundFileType, data + 4, sizeof(byte));

	TurnOnGecko();

	if (GroundFileType == GeckoSD)
	{
		return DeleteImageFromPayload(database, cameraID);
	}
	else
	{
		return DeleteImageFromOBC(database, cameraID, GroundFileType);
	}
}
DataBaseResult TransferPicture(DataBase database, unsigned char* data)
{
	unsigned int cameraID;

	memcpy(&cameraID, data, sizeof(int));

	TurnOnGecko();

	DataBaseResult DB_result = transferImageToSD(database, cameraID);

	return DB_result;
}
byte* GetDataBase(DataBase database, unsigned char* data)
{
	unsigned int start, end;

	memcpy(&start, data, 4);
	memcpy(&end, data + 4, 4);

	// ToDo: fix
	//return getDataBaseFile(database, start, end);

	return NULL;
}
DataBaseResult CreateThumbnail(DataBase database, unsigned char* data)
{
	unsigned int cameraID;
	byte reductionLevel;

	memcpy(&cameraID, data, sizeof(int));
	memcpy(&reductionLevel, data + 4, sizeof(byte));

	DataBaseResult DB_result = BinImage(database, cameraID, reductionLevel);

	return DB_result;
}
DataBaseResult CreateJPG(DataBase database, unsigned char* data)
{
	unsigned int cameraID;

	memcpy(&cameraID, data, sizeof(int));

	DataBaseResult DB_result = compressImage(database, cameraID);

	return DB_result;
}
DataBaseResult UpdatePhotographyValues(DataBase database, unsigned char* data)
{
	unsigned int frameAmount;
	unsigned int frameRate;
	unsigned char adcGain;
	unsigned char pgaGain;
	unsigned int exposure;

	memcpy(&frameAmount, data, sizeof(int));
	memcpy(&frameRate, data + 4, sizeof(int));
	memcpy(&adcGain, data + 8, sizeof(char));
	memcpy(&pgaGain, data + 9, sizeof(char));
	memcpy(&exposure, data + 10, sizeof(int));

	updateCameraParameters(database, frameRate, adcGain, pgaGain, exposure, frameAmount);

	return DataBaseSuccess;
}
DataBaseResult UpdateMaxNumberOfPicturesInDB(DataBase database, unsigned char* data)
{
	unsigned int maxNumberOfPictures;

	memcpy(&maxNumberOfPictures, data, sizeof(int));

	updateMaxNumberOfPictures(database, maxNumberOfPictures);

	return DataBaseSuccess;
}
char* GetPictureFile(DataBase database, unsigned char* data)
{
	unsigned int cameraID;
	byte reductionLevel;

	memcpy(&cameraID, data, sizeof(int));
	memcpy(&reductionLevel, data + 4, sizeof(byte));


	char* fileName = GetImageFileName(database, cameraID, reductionLevel);

	return fileName;
}
