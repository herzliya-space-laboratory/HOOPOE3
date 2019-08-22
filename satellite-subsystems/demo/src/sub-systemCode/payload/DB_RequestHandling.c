/*
 * DB_RequestHandling.c
 *
 *  Created on: Jun 6, 2019
 *      Author: Roy's Laptop
 */

#include <hcc/api_fat.h>

#include "Camera.h"

#include "DB_RequestHandling.h"

DataBaseResult TakePicture(DataBase database, unsigned char data[])
{
	Boolean8bit isAuto = data[0];
	Boolean8bit isTestPattern = data[1];
	unsigned int frameAmount = *((unsigned int*)data + 2);

	TurnOnGecko();

	DataBaseResult DB_result = takePicture(database, frameAmount, isTestPattern, isAuto);

	TurnOffGecko();

	return DB_result;
}
DataBaseResult TakeSpecialPicture(DataBase database, unsigned char data[])
{
	Boolean8bit isAuto = data[0];
	Boolean8bit isTestPattern = data[1];
	unsigned int frameAmount = *((unsigned int*)data + 2);
	unsigned int frameRate = *((unsigned int*)data + 6);
	unsigned int adcGain = *((unsigned int*)data + 10);
	unsigned int pgaGain = *((unsigned int*)data + 14);
	unsigned int exposure = *((unsigned int*)data + 18);

	TurnOnGecko();

	DataBaseResult DB_result = takePicture_withSpecialParameters(database, frameRate, adcGain, pgaGain, exposure, frameAmount, isTestPattern, isAuto);

	TurnOffGecko();

	return DB_result;
}
DataBaseResult DeletePicture(DataBase database, unsigned char data[])
{
	Boolean8bit isAuto = data[0];
	unsigned int cameraID = *((unsigned int*)data + 1);
	fileType fileType = *((unsigned int*)data + 5);

	TurnOnGecko();

	DataBaseResult DB_result;
	if (fileType == bmp) // Gecko.... just look at "δχρν TLM&TLC" and u wil understand how stupid it is abd why I did it this way.
	{
		DB_result = DeleteImageFromPayload(database, cameraID, isAuto);
	}
	else
	{
		DB_result = DeleteImageFromOBC(database, cameraID, fileType, isAuto);
	}

	TurnOffGecko();

	return DB_result;
}
DataBaseResult TransferPicture(DataBase database, unsigned char data[])
{
	Boolean8bit isAuto = data[0];
	unsigned int cameraID = *((unsigned int*)data + 1);

	TurnOnGecko();

	DataBaseResult DB_result = transferImageToSD(database, cameraID, TRUE, isAuto);

	TurnOffGecko();

	return DB_result;
}
F_FILE* GetDataBaseFile(DataBase database, unsigned char data[])
{
	unsigned int start = *((unsigned int*)data);
	unsigned int end = *((unsigned int*)data + 4);

	return getDataBaseFile(database, start, end);
}
DataBaseResult CreateThumbnail(DataBase database, unsigned char data[])
{
	unsigned int cameraID = *((unsigned int*)data);
	unsigned int reductionLevel = *((unsigned int*)data + 4);

	DataBaseResult DB_result = BinImage(database, cameraID, reductionLevel);

	return DB_result;
}
DataBaseResult CreateJPG(DataBase database, unsigned char data[])
{
	unsigned int cameraID = *((unsigned int*)data);
	unsigned int imageFactor = *((unsigned int*)data + 4);

	DataBaseResult DB_result = compressImage(database, cameraID, imageFactor);

	return DB_result;
}
DataBaseResult UpdatePhotographyValues(DataBase database, unsigned char data[])
{
	unsigned int isAuto = data[0];
	unsigned int frameRate = *((unsigned int*)data + 1);
	unsigned int adcGain = *((unsigned int*)data + 5);
	unsigned int pgaGain = *((unsigned int*)data + 9);
	unsigned int exposure = *((unsigned int*)data + 13);
	unsigned int frameAmount = *((unsigned int*)data + 17);

	updateCameraParameters(database, frameRate, adcGain, pgaGain, exposure, frameAmount, isAuto);

	return DataBaseSuccess;
}
DataBaseResult UpdateMaxNumberOfPicturesInDB(DataBase database, unsigned char data[])
{
	unsigned int isAuto = data[0];
	unsigned int maxNumberOfPictures = *((unsigned int*)data + 1);

	if (isAuto)
		updateMaxNumberOfAutoPictures(database, maxNumberOfPictures);
	else
		updateMaxNumberOfGroundPictures(database, maxNumberOfPictures);

	return DataBaseSuccess;
}
char* GetPictureFile(DataBase database, unsigned char data[])
{
	unsigned int cameraID = *((unsigned int*)data);
	unsigned int fileType = *((unsigned int*)data + 4);

	char* fileName = GetImageFileName(database, cameraID, fileType);

	return fileName;
}
