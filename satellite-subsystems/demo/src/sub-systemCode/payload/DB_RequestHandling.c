/*
 * DB_RequestHandling.c
 *
 *  Created on: Jun 6, 2019
 *      Author: Roy's Laptop
 */

#include <freertos/FreeRTOS.h>

#include <string.h>

#include "GeckoCameraDriver.h"

#include "DB_RequestHandling.h"

ImageDataBaseResult TakePicture(ImageDataBase database, unsigned char* data)
{
	Boolean8bit isTestPattern;

	memcpy(&isTestPattern, data, sizeof(Boolean8bit));

	TurnOnGecko();

	ImageDataBaseResult DB_result = takePicture(database, isTestPattern);

	return DB_result;
}
ImageDataBaseResult TakeSpecialPicture(ImageDataBase database, unsigned char* data)
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

	ImageDataBaseResult DB_result = takePicture_withSpecialParameters(database, frameAmount, frameRate, adcGain, pgaGain, exposure, isTestPattern);

	return DB_result;
}
ImageDataBaseResult DeletePicture(ImageDataBase database, unsigned char* data)
{
	unsigned int cameraID;
	byte GroundFileType;

	memcpy(&cameraID, data, sizeof(int));
	memcpy(&GroundFileType, data + 4, sizeof(byte));

	TurnOnGecko();

	if (GroundFileType == GeckoSD)
	{
		if (cameraID == 0)
		{
			return clearImageDataBase(database);
		}
		else
		{
			return DeleteImageFromPayload(database, cameraID);
		}
	}
	else
	{
		return DeleteImageFromOBC(database, cameraID, GroundFileType);
	}
}
ImageDataBaseResult TransferPicture(ImageDataBase database, unsigned char* data)
{
	unsigned int cameraID;

	memcpy(&cameraID, data, sizeof(int));

	TurnOnGecko();

	ImageDataBaseResult DB_result = transferImageToSD(database, cameraID);

	return DB_result;
}
byte* GetImageDataBase(ImageDataBase database, unsigned char* data)
{
	unsigned int start, end;

	memcpy(&start, data, 4);
	memcpy(&end, data + 4, 4);

	return getImageDataBaseBuffer(database, start, end);
}
ImageDataBaseResult CreateThumbnail(unsigned char* data)
{
	unsigned int cameraID;
	byte reductionLevel;
	Boolean8bit Skipping;

	memcpy(&cameraID, data, sizeof(int));
	memcpy(&reductionLevel, data + 4, sizeof(byte));
	memcpy(&Skipping, data + 5, sizeof(byte));

	ImageDataBaseResult DB_result = CreateImageThumbnail(cameraID, reductionLevel, Skipping);

	return DB_result;
}
ImageDataBaseResult CreateJPG(unsigned char* data)
{
	unsigned int cameraID;
	unsigned int quality_factor;
	fileType reductionLevel;

	memcpy(&cameraID, data, sizeof(int));
	memcpy(&quality_factor, data + 4, sizeof(int));
	memcpy(&reductionLevel, data + 8, sizeof(byte));

	ImageDataBaseResult DB_result = compressImage(cameraID, quality_factor, reductionLevel);

	return DB_result;
}
ImageDataBaseResult UpdatePhotographyValues(ImageDataBase database, unsigned char* data)
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

	setCameraPhotographyValues(database, frameRate, adcGain, pgaGain, exposure, frameAmount);

	return DataBaseSuccess;
}
