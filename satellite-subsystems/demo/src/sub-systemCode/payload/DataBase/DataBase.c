/*
 * ImageDataBase.c
 *
 *  Created on: 7 במאי 2019
 *      Author: I7COMPUTER
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>		//TODO: remove before flight. including printfs
#include <stdint.h>
#include <math.h>

#include <hcc/api_fat.h>
#include <hal/Boolean.h>
#include <hal/Timing/Time.h>
#include <hal/Utility/util.h>
#include <hal/Storage/FRAM.h>

#include <satellite-subsystems/GomEPS.h>

#include "../../Global/GlobalParam.h"

#include "../Misc/Boolean_bit.h"
#include "../Misc/Macros.h"

#include "../Compression//ImageConversion.h"
#include "DataBase.h"

typedef struct __attribute__ ((__packed__))
{
	uint32_t frameAmount;
	uint32_t frameRate;
	uint8_t adcGain;
	uint8_t pgaGain;
	uint32_t exposure;
} CameraPhotographyValues;

struct __attribute__ ((__packed__)) ImageDataBase_t
{
	// size = 22 bytes (padded to 24)
	unsigned int numberOfPictures;		///< current number of pictures saved on the satellite
	imageid nextId;						///< the next id we will use for a picture, (camera id)
	CameraPhotographyValues cameraParameters;
	Boolean8bit AutoThumbnailCreation;
};
#define SIZEOF_IMAGE_DATABASE sizeof(struct ImageDataBase_t)

#define DATABASE_FRAM_START DATABASEFRAMADDRESS +  SIZEOF_IMAGE_DATABASE	///< The ImageDataBase's starting address in the FRAM, not including the general fields at the start of the database
#define DATABASE_FRAM_END DATABASE_FRAM_START + MAX_NUMBER_OF_PICTURES * sizeof(ImageMetadata)	///< The ImageDataBases ending address in the FRAM

//---------------------------------------------------------------

/*
 * Will put in the string the supposed filename of an picture based on its id
 * @param soon2Bstring the number you want to convert into string form,
 * string will contain the number in string form
*/
static void getFileName(imageid id, fileType type, char string[FILE_NAME_SIZE])
{
	char baseString[FILE_NAME_SIZE];

	switch (type)
	{
		case raw:
			sprintf(baseString, "i%u.raw", id);
			break;
		case jpg:
			sprintf(baseString, "i%u.jpg", id);
			break;
		case bmp:
			sprintf(baseString, "i%u.bmp", id);
			break;
		case t02:
			sprintf(baseString, "i%u.t02", id);
			break;
		case t04:
			sprintf(baseString, "i%u.t04", id);
			break;
		case t08:
			sprintf(baseString, "i%u.t08", id);
			break;
		case t16:
			sprintf(baseString, "i%u.t16", id);
			break;
		case t32:
			sprintf(baseString, "i%u.t32", id);
			break;
		case t64:
			sprintf(baseString, "i%u.t64", id);
			break;
		default:
			break;
	}

	strcpy(string, baseString);

	printf("file name = %s\n", string);
}

/*
 * Will restart the database (only FRAM), deleting all of its contents
*/
ImageDataBaseResult zeroImageDataBase()
{
	// ToDo: do gud
	uint8_t zero = 0;
	int result;

	for (unsigned int i = DATABASEFRAMADDRESS; i < DATABASE_FRAM_END; i += sizeof(uint8_t))
	{
		result = FRAM_write((unsigned char*)&zero, i, sizeof(uint8_t));
		if (result != 0)
		{
			return DataBaseFramFail;
		}

		vTaskDelay(10);
	}

	return DataBaseSuccess;
}

ImageDataBaseResult updateGeneralDataBaseParameters(ImageDataBase database)
{
	CHECK_FOR_NULL(database, DataBaseNullPointer)

	int FRAM_result = FRAM_write((unsigned char*)(database), DATABASEFRAMADDRESS, SIZEOF_IMAGE_DATABASE);
	if(FRAM_result != 0)	// checking if the read from theFRAM succeeded
	{
		return DataBaseFramFail;
	}

	return DataBaseSuccess;
}

//---------------------------------------------------------------

ImageDataBaseResult SearchDataBase_byID(imageid id, ImageMetadata* image_metadata, uint32_t* image_address, uint32_t database_current_address)
{
	int result;

	while (database_current_address < DATABASE_FRAM_END)
	{
		result = FRAM_read((unsigned char *)image_metadata, database_current_address, sizeof(ImageMetadata));
		CMP_AND_RETURN(result, 0, DataBaseFramFail);

		// printing for tests:
		printf("cameraId: %d, timestamp: %u, ", image_metadata->cameraId, image_metadata->timestamp);
		bit fileTypes[8];
		char2bits(image_metadata->fileTypes, fileTypes);
		printf("files types:");
		for (int j = 0; j < 8; j++) {
			printf(" %u", fileTypes[j].value);
		}
		printf(", angles: %u %u %u, marked = %u\n",
				image_metadata->angles[0], image_metadata->angles[1], image_metadata->angles[2], image_metadata->markedFor_TumbnailCreation);

		if (image_metadata->cameraId == id)
		{
			memcpy(image_address, &database_current_address, sizeof(uint32_t));
			return DataBaseSuccess;
		}
		else
		{
			database_current_address += sizeof(ImageMetadata);
		}
	}

	if (id == 0)
		return DataBaseFull;
	else
		return DataBaseIdNotFound;
}

ImageDataBaseResult SearchDataBase_byMark(uint32_t database_current_address, ImageMetadata* image_metadata, uint32_t* image_address)
{
	int result;

	while (database_current_address < DATABASE_FRAM_END)
	{
		result = FRAM_read((unsigned char *)image_metadata, database_current_address, sizeof(ImageMetadata));
		CMP_AND_RETURN(result, 0, DataBaseFramFail);

		if (image_metadata->markedFor_TumbnailCreation)
		{
			memcpy(image_address, &database_current_address, sizeof(uint32_t));
			return DataBaseSuccess;
		}
		else
		{
			database_current_address += sizeof(ImageMetadata);
		}
	}

	return DataBaseIdNotFound;
}

//---------------------------------------------------------------

ImageDataBaseResult checkForFileType(ImageMetadata image_metadata, fileType reductionLevel)
{
	bit fileTypes[8];
	char2bits(image_metadata.fileTypes, fileTypes);

	if (fileTypes[reductionLevel].value)
		return DataBaseSuccess;
	else
		return DataBaseNotInSD;
}

void updateFileTypes(ImageMetadata* image_metadata, uint32_t image_address, fileType reductionLevel, Boolean value)
{
	bit fileTypes[8];
	char2bits(image_metadata->fileTypes, fileTypes);

	if (value)
		fileTypes[reductionLevel].value = TRUE_bit;
	else
		fileTypes[reductionLevel].value = FALSE_bit;

	image_metadata->fileTypes = bits2char(fileTypes);

	FRAM_write((unsigned char*)image_metadata, image_address, sizeof(ImageMetadata));
}

uint32_t GetImageFactor(fileType image_type)
{
	return pow(2, image_type);
}

//---------------------------------------------------------------

ImageDataBaseResult readImageToBuffer(imageid id, fileType image_type)
{
	char fileName[FILE_NAME_SIZE];

	ImageDataBaseResult result = GetImageFileName(id, image_type, fileName);
	DB_RETURN_ERROR(result);

	F_FILE *file;
	OPEN_FILE(file, fileName, "r", DataBaseFileSystemError);	// open file for writing in safe mode

	printf("\n-F- file system error (%d)\n\n", f_getlasterror());
	f_flush(file);
	printf("\n-F- file system error (%d)\n\n", f_getlasterror());

	uint32_t factor = GetImageFactor(image_type);
	READ_FROM_FILE(file, imageBuffer, sizeof(imageBuffer) / (factor * factor), 1, DataBaseFileSystemError);

	CLOSE_FILE(file, DataBaseFileSystemError);

	return DataBaseSuccess;
}

ImageDataBaseResult saveImageFromBuffer(imageid id, fileType image_type)
{
	char fileName[FILE_NAME_SIZE];
	ImageDataBaseResult result = GetImageFileName(id, image_type, fileName);
	DB_RETURN_ERROR(result);

	F_FILE *file;
	OPEN_FILE(file, fileName, "w+", DataBaseFileSystemError);	// open file for writing in safe mode

	printf("\n-F- file system error (%d)\n\n", f_getlasterror());
	f_flush(file);
	printf("\n-F- file system error (%d)\n\n", f_getlasterror());

	uint32_t factor = GetImageFactor(image_type);
	WRITE_TO_FILE(file, imageBuffer, sizeof(imageBuffer) / (factor * factor), 1, DataBaseFileSystemError);

	FLUSH_FILE(file, DataBaseFileSystemError);	// only after flushing can data be considered safe

	CLOSE_FILE(file, DataBaseFileSystemError);	// data is also considered safe when file is closed

	return DataBaseSuccess;
}

//---------------------------------------------------------------

imageid getLatestID(ImageDataBase database)
{
	return database->nextId - 1;
}
unsigned int getNumberOfFrames(ImageDataBase database)
{
	return database->cameraParameters.frameAmount;
}

uint32_t getDataBaseStart()
{
	return DATABASE_FRAM_START;
}
uint32_t getDataBaseEnd()
{
	return DATABASE_FRAM_END;
}

//---------------------------------------------------------------

ImageDataBaseResult setDataBaseValues(ImageDataBase database)
{
	CHECK_FOR_NULL(database, DataBaseNullPointer);

	database->numberOfPictures = 0;
	database->nextId = 1;
	database->AutoThumbnailCreation = TRUE_8BIT;
	setCameraPhotographyValues(database, DEFALT_FRAME_RATE, DEFALT_ADC_GAIN, DEFALT_PGA_GAIN, DEFALT_EXPOSURE, DEFALT_FRAME_AMOUNT);

	ImageDataBaseResult result = updateGeneralDataBaseParameters(database);
	DB_RETURN_ERROR(result);

	printf("\nRe: database->numberOfPictures = %u, database->nextId = %u; CameraParameters: frameAmount = %lu,"
			" frameRate = %lu, adcGain = %u, pgaGain = %u, exposure = %lu\n", database->numberOfPictures,
			database->nextId, database->cameraParameters.frameAmount, database->cameraParameters.frameRate,
			database->cameraParameters.adcGain, database->cameraParameters.pgaGain,
			database->cameraParameters.exposure);

	return DataBaseSuccess;
}

ImageDataBase initImageDataBase(Boolean first_activation)
{
	ImageDataBase database = malloc(SIZEOF_IMAGE_DATABASE);	// allocate the memory for the database's variables

	int FRAM_result = FRAM_read((unsigned char*)(database),  DATABASEFRAMADDRESS, SIZEOF_IMAGE_DATABASE);
	if(FRAM_result != 0)	// checking if the read from theFRAM succeeded
	{
		free(database);
		return NULL;
	}

	printf("numberOfPictures = %u, nextId = %u; CameraParameters: frameAmount = %lu, frameRate = %lu, adcGain = %u, pgaGain = %u, exposure = %lu\n", database->numberOfPictures, database->nextId, database->cameraParameters.frameAmount, database->cameraParameters.frameRate, database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.exposure);

	if (database->nextId == 0 || first_activation)	// The FRAM is empty and the ImageDataBase wasn't initialized beforehand
	{
		zeroImageDataBase();
		setDataBaseValues(database);
	}

	return database;
}

ImageDataBaseResult resetImageDataBase(ImageDataBase database)
{
	CHECK_FOR_NULL(database, DataBaseNullPointer);

	ImageDataBaseResult result = clearImageDataBase();
	CMP_AND_RETURN(result, DataBaseSuccess, result);

	result = zeroImageDataBase();
	CMP_AND_RETURN(result, DataBaseSuccess, result);

	result = setDataBaseValues(database);
	DB_RETURN_ERROR(result);

	return DataBaseSuccess;
}

//---------------------------------------------------------------

void setCameraPhotographyValues(ImageDataBase database, uint32_t frameRate, uint8_t adcGain, uint8_t pgaGain, uint32_t exposure, uint32_t frameAmount)
{
	database->cameraParameters.frameAmount = frameAmount;
	database->cameraParameters.frameRate = frameRate;
	database->cameraParameters.adcGain = adcGain;
	database->cameraParameters.pgaGain = pgaGain;
	database->cameraParameters.exposure = exposure;
}

//---------------------------------------------------------------

ImageDataBaseResult transferImageToSD_withoutSearch(imageid cameraId, uint32_t image_address, ImageMetadata image_metadata)
{
	FRAM_read((unsigned char*)&image_metadata, image_address, sizeof(ImageMetadata));

	int result = checkForFileType(image_metadata, raw);
	CMP_AND_RETURN(result, DataBaseNotInSD, DataBasealreadyInSD);

	// Reading the image to the buffer:
/*
	GomEpsResetWDT(0);

	err = GECKO_ReadImage((uint32_t)cameraId, (uint32_t*)imageBuffer);
	if( err )
	{
		printf("\ntransferImageToSD Error = (%d) reading image!\n\r",err);
		return (GECKO_Read_Success - err);
	}

	vTaskDelay(500);
*/
	// Creating a file for the picture at iOBC SD:

	char fileName[FILE_NAME_SIZE];
	getFileName(cameraId, raw, fileName);

	F_FILE *PictureFile;
	OPEN_FILE(PictureFile, fileName, "w+", DataBaseFileSystemError);

	GomEpsResetWDT(0);

	WRITE_TO_FILE(PictureFile, imageBuffer, sizeof(uint8_t), IMAGE_SIZE, DataBaseFileSystemError);

	FLUSH_FILE(PictureFile, DataBaseFileSystemError);	// only after flushing can data be considered safe

	CLOSE_FILE(PictureFile, DataBaseFileSystemError);

	// Updating the DataBase:

	updateFileTypes(&image_metadata, image_address, raw, TRUE);

	result = checkForFileType(image_metadata, raw);
	CMP_AND_RETURN(result, DataBaseSuccess, DataBaseFail);

	return DataBaseSuccess;
}

ImageDataBaseResult transferImageToSD(ImageDataBase database, imageid cameraId)
{
	CHECK_FOR_NULL(database, DataBaseNullPointer);

	// Searching for the image on the database:

	ImageMetadata image_metadata;
	uint32_t image_address;

	int result;

	result = SearchDataBase_byID(cameraId, &image_metadata, &image_address, DATABASE_FRAM_START);
	DB_RETURN_ERROR(result);

	return transferImageToSD_withoutSearch(cameraId, image_address, image_metadata);
}

//---------------------------------------------------------------

ImageDataBaseResult DeleteImageFromOBC_withoutSearch(imageid cameraId, fileType type, uint32_t image_address, ImageMetadata image_metadata)
{
	FRAM_read((unsigned char*)&image_metadata, image_address, sizeof(ImageMetadata));

	char fileName[FILE_NAME_SIZE];
    getFileName(cameraId, type, fileName);

	int result = checkForFileType(image_metadata, type);
	DB_RETURN_ERROR(result);

	DELETE_FILE(fileName, DataBaseFileSystemError);

	updateFileTypes(&image_metadata, image_address, type, FALSE);

	return DataBaseSuccess;
}

ImageDataBaseResult DeleteImageFromOBC(ImageDataBase database, imageid cameraId, fileType type)
{
	CHECK_FOR_NULL(database, DataBaseNullPointer);

	ImageMetadata image_metadata;
	uint32_t image_address;

	ImageDataBaseResult result = SearchDataBase_byID(cameraId, &image_metadata, &image_address, DATABASE_FRAM_START);
	DB_RETURN_ERROR(result);

    return DeleteImageFromOBC_withoutSearch(cameraId, type, image_address, image_metadata);
}

ImageDataBaseResult DeleteImageFromPayload(ImageDataBase database, imageid id)
{
	CHECK_FOR_NULL(database, DataBaseNullPointer);

	GomEpsResetWDT(0);

	int err = GECKO_EraseBlock(id);
	if( err )
	{
		printf("Error (%d) erasing image!\n\r",err);
		return (GECKO_Erase_Success - err);
	}

	vTaskDelay(500);

	ImageMetadata image_metadata;
	uint32_t image_address;

	ImageDataBaseResult result = SearchDataBase_byID(id, &image_metadata, &image_address, DATABASE_FRAM_START);
	DB_RETURN_ERROR(result);

	for (fileType i = 0; i < NumberOfFileTypes; i++)
	{
		if(checkForFileType(image_metadata, i) == DataBaseSuccess)
		{
			DeleteImageFromOBC_withoutSearch(id, i, image_address, image_metadata);
			updateFileTypes(&image_metadata, image_address, i, FALSE);
		}
	}

	uint8_t zero = 0;
	for(unsigned int i = 0; i < sizeof(ImageMetadata); i += sizeof(uint8_t))
	{
		int FRAM_result = FRAM_write(&zero, image_address + i, sizeof(uint8_t));
		CMP_AND_RETURN(FRAM_result, 0, DataBaseFramFail);
	}

	database->numberOfPictures--;

	updateGeneralDataBaseParameters(database);

	return DataBaseSuccess;
}

ImageDataBaseResult clearImageDataBase(void)
{
	int result;
	ImageMetadata image_metadata;
	uint32_t image_address = DATABASE_FRAM_START;

	while(image_address < DATABASE_FRAM_END)
	{
		vTaskDelay(100);

		result = FRAM_read((unsigned char*)&image_metadata, image_address, sizeof(ImageMetadata));
		CMP_AND_RETURN(result, 0, DataBaseFramFail);

		image_address += sizeof(ImageMetadata);

		if (image_metadata.cameraId != 0)
		{
			for (fileType i = 0; i < NumberOfFileTypes; i++)
			{
				if(checkForFileType(image_metadata, i) == DataBaseSuccess)
				{
					result = DeleteImageFromOBC_withoutSearch(image_metadata.cameraId, i, image_address, image_metadata);
					DB_RETURN_ERROR(result);
					updateFileTypes(&image_metadata, image_address, i, FALSE);
				}
			}
		}
	}

	return DataBaseSuccess;
}

//---------------------------------------------------------------

ImageDataBaseResult handleMarkedPictures(uint32_t nuberOfPicturesToBeHandled)
{
	ImageMetadata image_metadata;
	uint32_t image_address;

	uint32_t database_current_address = DATABASE_FRAM_START;

	ImageDataBaseResult DB_result;

	for (uint32_t i = 0; i < nuberOfPicturesToBeHandled; i++)
	{
		DB_result = SearchDataBase_byMark(database_current_address, &image_metadata, &image_address);
		database_current_address = image_address + sizeof(ImageMetadata);

		if ( DB_result == 0 )
		{
			TurnOnGecko();

			DB_result = transferImageToSD_withoutSearch(image_metadata.cameraId, image_address, image_metadata);
			if (DB_result != DataBaseSuccess && DB_result != DataBasealreadyInSD)
				return DB_result;

			printf("\n-F- file system error (%d)\n\n", f_getlasterror());

			TurnOffGecko();

			vTaskDelay(1000);

			DB_result = CreateImageThumbnail_withoutSearch(image_metadata.cameraId, 4, TRUE, image_address, image_metadata);
			if (DB_result != DataBaseSuccess && DB_result != DataBasealreadyInSD)
				return DB_result;

			vTaskDelay(1000);

			DB_result = DeleteImageFromOBC_withoutSearch(image_metadata.cameraId, raw, image_address, image_metadata);
			DB_RETURN_ERROR(DB_result);

			// making sure i wont lose the data written in the functions above to the FRAM:
			FRAM_read( (unsigned char*)&image_metadata, image_address, (unsigned int)sizeof(ImageMetadata)); // reading the id from the ImageDescriptor file

			image_metadata.markedFor_TumbnailCreation = FALSE_8BIT;
			FRAM_write( (unsigned char*)&image_metadata, image_address, (unsigned int)sizeof(ImageMetadata)); // reading the id from the ImageDescriptor file
		}
	}

	return DataBaseSuccess;
}

//---------------------------------------------------------------

ImageDataBaseResult writeNewImageMetaDataToFRAM(ImageDataBase database, time_unix time_image_taken, short Attitude[3])
{
	ImageMetadata image_metadata;
	uint32_t image_address;

	imageid zero = 0;
	ImageDataBaseResult result = SearchDataBase_byID(zero, &image_metadata, &image_address, DATABASE_FRAM_START);	// Finding space at the database(FRAM) for the image's ImageMetadata:
	CMP_AND_RETURN(result, DataBaseSuccess, DataBaseFull);

	image_metadata.cameraId = database->nextId;
	image_metadata.timestamp = time_image_taken;
	memcpy(&image_metadata.angles, Attitude, sizeof(short) * 3);

	if (database->AutoThumbnailCreation)
		image_metadata.markedFor_TumbnailCreation = TRUE_8BIT;
	else
		image_metadata.markedFor_TumbnailCreation = FALSE_8BIT;

	for (fileType i = 0; i < NumberOfFileTypes; i++) {
		updateFileTypes(&image_metadata, image_address, i, FALSE_bit);
	}

	result = FRAM_write((unsigned char*)&image_metadata, image_address, sizeof(ImageMetadata));
	CMP_AND_RETURN(result, 0, DataBaseFramFail);

	database->nextId++;
	database->numberOfPictures++;

	return DataBaseSuccess;
}

ImageDataBaseResult takePicture(ImageDataBase database, Boolean8bit testPattern)
{
	short Attitude[3];
	for (int i = 0; i < 3; i++) {
		Attitude[i] = get_Attitude(i);
	}

	int err = 0;

	// Erasing previous image before taking one to clear this part of the SDs:

	for (unsigned int i = 0; i < database->cameraParameters.frameAmount; i++)
	{
		err = GECKO_EraseBlock(database->nextId + i);
		CMP_AND_RETURN(err, 0, GECKO_Erase_Success - err);

		vTaskDelay(500);
	}

	unsigned int currentDate = 0;
	Time_getUnixEpoch(&currentDate);

	err = GECKO_TakeImage( database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.exposure, database->cameraParameters.frameAmount, database->cameraParameters.frameRate, database->nextId, testPattern);
	CMP_AND_RETURN(err, 0, GECKO_Take_Success - err);

	vTaskDelay(500);

	// ImageDataBase handling:

	ImageDataBaseResult result;

	for (uint32_t numOfFramesTaken = 0; numOfFramesTaken < database->cameraParameters.frameAmount; numOfFramesTaken++)
	{
		vTaskDelay(100);

		result = writeNewImageMetaDataToFRAM(database, currentDate, Attitude);
		DB_RETURN_ERROR(result);

		currentDate += database->cameraParameters.frameRate;
	}

	updateGeneralDataBaseParameters(database);

	return DataBaseSuccess;
}

ImageDataBaseResult takePicture_withSpecialParameters(ImageDataBase database, uint32_t frameRate, uint8_t adcGain, uint8_t pgaGain, uint32_t exposure, uint32_t frameAmount, Boolean8bit testPattern)
{
	CameraPhotographyValues regularParameters;
	memcpy(&regularParameters, &database->cameraParameters, sizeof(CameraPhotographyValues));

	setCameraPhotographyValues(database, frameRate, adcGain, pgaGain, exposure, frameAmount);

	ImageDataBaseResult DB_result = takePicture(database, testPattern);

	setCameraPhotographyValues(database, regularParameters.frameRate, regularParameters.adcGain, regularParameters.pgaGain, regularParameters.exposure, regularParameters.frameAmount);

	return DB_result;
}

//---------------------------------------------------------------

ImageDataBaseResult GetImageFileName(imageid id, fileType fileType, char fileName[FILE_NAME_SIZE])
{
	ImageMetadata image_metadata;
	uint32_t image_address;

	ImageDataBaseResult result = SearchDataBase_byID(id, &image_metadata, &image_address, DATABASE_FRAM_START);
	DB_RETURN_ERROR(result);

	if (fileType != bmp)
	{
		result = checkForFileType(image_metadata, fileType);
		DB_RETURN_ERROR(result);
	}

	getFileName(id, fileType, fileName);

	return DataBaseSuccess;
}

//---------------------------------------------------------------

byte* getImageDataBaseBuffer(imageid start, imageid end)
{
	byte* buffer = malloc(sizeof(int));
	CHECK_FOR_NULL(buffer, NULL);

	unsigned int size = 0; // will be the first value in the array and will indicate its length (including itself!)

	memcpy(buffer + size, &size, sizeof(size));
	size += sizeof(size);

	ImageDataBase database = malloc(SIZEOF_IMAGE_DATABASE);
	int FRAM_result = FRAM_read((unsigned char*)(database),  DATABASEFRAMADDRESS, SIZEOF_IMAGE_DATABASE);
	if (FRAM_result != 0)
	{
		free(database);
		return NULL;
	}

	memcpy(buffer + size, database, SIZEOF_IMAGE_DATABASE);
	size += SIZEOF_IMAGE_DATABASE;

	free(database);

	printf("numberOfPictures = %u, nextId = %u, auto thumbnail creation = %u",
			database->numberOfPictures, database->nextId, database->AutoThumbnailCreation);
	printf("cameraParameters: frameAmount = %lu, frameRate = %lu, adcGain = %lu, pgaGain = %lu, exposure = %lu\n",
			database->cameraParameters.frameAmount, database->cameraParameters.frameRate,
			(uint32_t)database->cameraParameters.adcGain, (uint32_t)database->cameraParameters.pgaGain,
			database->cameraParameters.exposure);

	// Write MetaData:

	int result;
	ImageMetadata image_metadata;
	uint32_t image_address = DATABASE_FRAM_START;

	while(image_address < DATABASE_FRAM_END)
	{
		vTaskDelay(100);

		result = FRAM_read((unsigned char*)&image_metadata, image_address, sizeof(ImageMetadata));
		CMP_AND_RETURN(result, 0, NULL);

		image_address += sizeof(ImageMetadata);

		if (image_metadata.cameraId != 0 && (image_metadata.cameraId >= start && image_metadata.cameraId <= end)) // check if the current id is the requested id
		{
			realloc(buffer, size + sizeof(ImageMetadata));

			memcpy(buffer + size, &image_metadata, sizeof(ImageMetadata));
			size += sizeof(ImageMetadata);

			// printing for tests:
			printf("cameraId: %d, timestamp: %u, inOBC:", image_metadata.cameraId, image_metadata.timestamp);
			bit fileTypes[8];
			char2bits(image_metadata.fileTypes, fileTypes);
			printf(", files:");
			for (int j = 0; j < 8; j++) {
				printf(" %u", fileTypes[j].value);
			}
			printf(", angles: %u %u %u, markedFor_4thTumbnailCreation = %u\n",
					image_metadata.angles[0], image_metadata.angles[1], image_metadata.angles[2], image_metadata.markedFor_TumbnailCreation);
		}
	}

	memcpy(buffer + 0, &size, sizeof(size));

	return buffer;
}

imageid* get_ID_list_withDefaltThumbnail(imageid start, imageid end)
{
	imageid* buffer = malloc(sizeof(imageid));
	CHECK_FOR_NULL(buffer, NULL);

	imageid size = 0; // will be the first value in the array and will indicate its length (including itself!)

	memcpy(buffer + size, &size, sizeof(size));
	size += sizeof(size);

	// Write MetaData:

	int result;
	ImageMetadata image_metadata;
	uint32_t image_address = DATABASE_FRAM_START;

	while(image_address < DATABASE_FRAM_END)
	{
		vTaskDelay(100);

		result = FRAM_read((unsigned char*)&image_metadata, image_address, sizeof(ImageMetadata));
		CMP_AND_RETURN(result, 0, NULL);

		image_address += sizeof(ImageMetadata);

		if (image_metadata.cameraId != 0 && (image_metadata.cameraId >= start && image_metadata.cameraId <= end)) // check if the current id is the requested id
		{
			realloc(buffer, size + sizeof(imageid));

			memcpy(buffer + size, &image_metadata.cameraId, sizeof(imageid));
			size += sizeof(imageid);
		}
	}

	memcpy(buffer + 0, &size, sizeof(size));

	return buffer;
}
