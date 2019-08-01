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

#include "../Global/GlobalParam.h"

#include "GeckoCameraDriver.h"
#include "Boolean_bit.h"
#include "FRAM_DataStructure.h"
#include "Macros.h"

#include "ImageConversion.h"
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

// The ImageDataBase's starting address in the FRAM, not including the general fields at the start of the database
#define DATABASE_FRAM_START DATABASEFRAMADDRESS +  SIZEOF_IMAGE_DATABASE

// The ImageDataBases ending address in the FRAM
#define DATABASE_FRAM_END DATABASE_FRAM_START + MAX_NUMBER_OF_PICTURES * sizeof(ImageMetadata)

//---------------------------------------------------------------

/*
 * Will put in the string the supposed filename of an picture based on its id
 * @param soon2Bstring the number you want to convert into string form,
 * string will contain the number in string form
*/
static void getFileName(imageid id, fileType type, char string[FILE_NAME_SIZE])
{
	imageid num = id;
	char digit[10] = "0123456789";

	char baseString[FILE_NAME_SIZE] = "i0000000.raw";
	strcpy(string, baseString);

	int i;
	for (i = FILE_NAME_SIZE - 6; i > 2; i--)
	{
		string[i] = digit[num % 10];
		num /= 10;
	}

	switch (type)
	{
		case raw:
			strcpy(string + (FILE_NAME_SIZE - 5), ".raw");
			break;
		case jpg:
			strcpy(string + (FILE_NAME_SIZE - 5), ".jpg");
			break;
		case bmp:
			strcpy(string + (FILE_NAME_SIZE - 5), ".bmp");
			break;
		case t02:
			strcpy(string + (FILE_NAME_SIZE - 5), ".t02");
			break;
		case t04:
			strcpy(string + (FILE_NAME_SIZE - 5), ".t04");
			break;
		case t08:
			strcpy(string + (FILE_NAME_SIZE - 5), ".t08");
			break;
		case t16:
			strcpy(string + (FILE_NAME_SIZE - 5), ".t16");
			break;
		case t32:
			strcpy(string + (FILE_NAME_SIZE - 5), ".t32");
			break;
		case t64:
			strcpy(string + (FILE_NAME_SIZE - 5), ".t64");
			break;
		default:
			break;
	}

	string[FILE_NAME_SIZE - 1] = '\0';

	printf("file name = %s\n", string);
}

/*
 * Will restart the database (only FRAM), deleting all of its contents
*/
ImageDataBaseResult zeroImageDataBase()
{
	uint8_t zero = 0;
	int result;

	for (unsigned int i = DATABASEFRAMADDRESS; i < DATABASE_FRAM_END; i += sizeof(uint8_t))
	{
		result = FRAM_write((unsigned char*)&zero, i, sizeof(uint8_t));
		if (result != 0)
		{
			return DataBaseFramFail;
		}
	}

	return DataBaseSuccess;
}

ImageDataBaseResult updateGeneralDataBaseParameters(ImageDataBase database)
{
	if(database == NULL)
	{
		free(database);
		return DataBaseNullPointer;
	}

	int FRAM_result = FRAM_write((unsigned char*)(database), DATABASEFRAMADDRESS, SIZEOF_IMAGE_DATABASE);
	if(FRAM_result != 0)	// checking if the read from theFRAM succeeded
	{
		return DataBaseFramFail;
	}

	return DataBaseSuccess;
}

//---------------------------------------------------------------

Boolean checkForID(ImageMetadata* image_metadata, imageid* id)
{
	if (memcmp(&image_metadata->cameraId, id, sizeof(imageid)) == 0)
		return TRUE;
	else
		return FALSE;
}

ImageDataBaseResult SearchDataBase_byID(imageid id, ImageMetadata* image_metadata, uint32_t* image_address)
{
	uint32_t database_start_address = DATABASE_FRAM_START;
	uint32_t database_end_address = DATABASE_FRAM_END;

	int result = FRAM_DataStructure_search(database_start_address, database_end_address,
			sizeof(ImageMetadata), image_metadata, image_address,
			(Boolean (*)(void*, void*))&checkForID, &id);
	if (result != 0)
		return DataBaseIdNotFound;

	// Checking for more images with the same id:

	if (id != 0)
	{
		database_start_address = *(image_address);

		ImageMetadata check_image_metadata;
		uint32_t check_image_address;

		result = FRAM_DataStructure_search(database_start_address, database_end_address,
				sizeof(ImageMetadata), &check_image_metadata, &check_image_address,
				(Boolean (*)(void*, void*))&checkForID, &id);
		if (result != -1)
			return DataBaseIllegalId;
	}

	return DataBaseSuccess;
}

//---------------------------------------------------------------

ImageDataBaseResult checkForFileType(ImageMetadata image_metadata, fileType reductionLevel)
{
	bit fileTypes[8];
	char2bits(image_metadata.fileTypes, fileTypes);

	if (!fileTypes[reductionLevel].value)
		return DataBaseNotInSD;
	else
		return DataBaseSuccess;
}

void updateFileTypes(ImageMetadata image_metadata, uint32_t image_address, fileType reductionLevel, Boolean value)
{
	bit fileTypes[8];
	char2bits(image_metadata.fileTypes, fileTypes);

	if (value)
		fileTypes[reductionLevel].value = TRUE_bit;
	else
		fileTypes[reductionLevel].value = FALSE_bit;

	image_metadata.fileTypes = bits2char(fileTypes);

	FRAM_write((unsigned char*)&image_metadata, image_address, sizeof(ImageMetadata));
}

uint32_t GetImageFactor(fileType image_type)
{
	uint32_t factor = 1;

	switch(image_type)
	{
		case raw: factor = 1; break;
		case t02: factor = 2; break;
		case t04: factor = 4; break;
		case t08: factor = 8; break;
		case t16: factor = 16; break;
		case t32: factor = 32; break;
		case t64: factor = 64; break;
		default: factor = 0;
	}

	return factor;
}

//---------------------------------------------------------------

ImageDataBaseResult readImageToBuffer(imageid id, fileType image_type)
{
	char fileName[FILE_NAME_SIZE];

	ImageDataBaseResult result = GetImageFileName(id, image_type, fileName);
	DB_RETURN_ERROR(result);

	F_FILE *file;
	OPEN_FILE(file, fileName, "r", DataBaseFileSystemError);	// open file for writing in safe mode

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

	uint32_t factor = GetImageFactor(image_type);
	WRITE_TO_FILE(file, imageBuffer, sizeof(imageBuffer) / (factor * factor), 1, DataBaseFileSystemError);
	f_flush( file );	// only after flushing can data be considered safe

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

//---------------------------------------------------------------

Boolean checkForMark(ImageMetadata* image_metadata, void* placeholder)
{
	if (image_metadata->cameraId != 0 && image_metadata->markedFor_TumbnailCreation)
		return TRUE;
	else
		return FALSE;
}

ImageDataBaseResult handleMarkedPictures(ImageDataBase database, uint32_t nuberOfPicturesToBeHandled)
{
	ImageMetadata image_metadata;
	uint32_t image_address;

	uint32_t database_start_address = DATABASE_FRAM_START;
	uint32_t database_end_address = DATABASE_FRAM_END;

	int result;
	ImageDataBaseResult DB_result;

	for (uint32_t i = 0; i < nuberOfPicturesToBeHandled; i++)
	{
		result = FRAM_DataStructure_search(database_start_address, database_end_address,
				sizeof(ImageMetadata), &image_metadata, &image_address,
				(Boolean (*)(void*, void*))&checkForMark, NULL);
		database_start_address = image_address;

		if ( result == 0 )
		{
			TurnOnGecko();

			DB_result = transferImageToSD(database, image_metadata.cameraId);
			if (DB_result != DataBaseSuccess && DB_result != DataBasealreadyInSD)
				return DB_result;

			TurnOffGecko();

			vTaskDelay(1000);

			DB_result = CreateImageThumbnail(image_metadata.cameraId, 4, TRUE);
			if (DB_result != DataBaseSuccess && DB_result != DataBasealreadyInSD)
				return DB_result;

			vTaskDelay(1000);

			DB_result = DeleteImageFromOBC(database, image_metadata.cameraId, raw);
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

ImageDataBase initImageDataBase()
{
	ImageDataBase database = malloc(SIZEOF_IMAGE_DATABASE);	// allocate the memory for the database's variables
	if (database == NULL)	// ToDo: use check for NULL!
	{
		free(database);
		return NULL;
	}

	int FRAM_result = FRAM_read((unsigned char*)(database),  DATABASEFRAMADDRESS, SIZEOF_IMAGE_DATABASE);
	if(FRAM_result != 0)	// checking if the read from theFRAM succeeded
	{
		free(database);
		return NULL;
	}

	printf("numberOfPictures = %u, nextId = %u; CameraParameters: frameAmount = %lu, frameRate = %lu, adcGain = %u, pgaGain = %u, exposure = %lu\n", database->numberOfPictures, database->nextId, database->cameraParameters.frameAmount, database->cameraParameters.frameRate, database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.exposure);

	if (database->nextId == 0)	// The FRAM is empty and the ImageDataBase wasn't initialized beforehand
	{
		setDataBaseValues(database);
	}

	return database;
}

ImageDataBaseResult resetImageDataBase(ImageDataBase database)
{
	CHECK_FOR_NULL(database, DataBaseNullPointer);
/*
	ImageDataBaseResult result = clearImageDataBase(database);
	if (result != DataBaseSuccess)
		return NULL;
*/

	ImageDataBaseResult result = setDataBaseValues(database);
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

	updateGeneralDataBaseParameters(database);
}

//---------------------------------------------------------------

ImageDataBaseResult transferImageToSD(ImageDataBase database, imageid cameraId)
{
	CHECK_FOR_NULL(database, DataBaseNullPointer);

	int err = GomEpsResetWDT(0);
	printf("err DB eps: %d\n", err);

	err = GECKO_ReadImage((uint32_t)cameraId, (uint32_t*)imageBuffer);
	if( err )
	{
		printf("\ntransferImageToSD Error = (%d) reading image!\n\r",err);
		return (GECKO_Read_Success - err);
	}

	vTaskDelay(500);

	// Creating a file for the picture at iOBC sd:
	char fileName[FILE_NAME_SIZE];
	getFileName(cameraId, raw, fileName);

	F_FILE *PictureFile;
	OPEN_FILE(PictureFile, fileName, "w+", DataBaseFileSystemError);

	err = GomEpsResetWDT(0);
	printf("err DB eps: %d\n", err);

	WRITE_TO_FILE(PictureFile, imageBuffer, sizeof(uint8_t), IMAGE_SIZE, DataBaseFileSystemError);

	f_flush(PictureFile );	// only after flushing can data be considered safe

	CLOSE_FILE(PictureFile, DataBaseFileSystemError);

	// searching for the image on the database:

	ImageMetadata image_metadata;
	uint32_t image_address;

	int result;

	result = SearchDataBase_byID(cameraId, &image_metadata, &image_address);
	DB_RETURN_ERROR(result);

	result = checkForFileType(image_metadata, raw);
	DB_RETURN_ERROR(result);

	updateFileTypes(image_metadata, image_address, raw, TRUE);

	updateGeneralDataBaseParameters(database);

	return DataBaseSuccess;
}

//---------------------------------------------------------------

ImageDataBaseResult DeleteImageFromOBC(ImageDataBase database, imageid cameraId, fileType type)
{
	CHECK_FOR_NULL(database, DataBaseNullPointer);

    char fileName[FILE_NAME_SIZE];
    getFileName(cameraId, type, fileName);

	ImageMetadata image_metadata;
	uint32_t image_address;

	ImageDataBaseResult result = SearchDataBase_byID(cameraId, &image_metadata, &image_address);
	DB_RETURN_ERROR(result);

	result = checkForFileType(image_metadata, type);
	DB_RETURN_ERROR(result);

	DELETE_FILE(fileName, DataBaseFileSystemError);

	updateFileTypes(image_metadata, image_address, type, FALSE);

	updateGeneralDataBaseParameters(database);

    return DataBaseSuccess;
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

	ImageDataBaseResult result = SearchDataBase_byID(id, &image_metadata, &image_address);
	DB_RETURN_ERROR(result);

	for (fileType i = 0; i < NumberOfFileTypes; i++)
	{
		if(checkForFileType(image_metadata, i) == DataBaseSuccess)
		{
			updateFileTypes(image_metadata, image_address, i, FALSE);
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

ImageDataBaseResult clearImageDataBase(ImageDataBase database)
{
	printf("\n----------resetImageDataBase----------\n");

	ImageDataBaseResult DB_result = DataBaseSuccess;

	// Deleting all pictures saved on iOBC SD so we wont have doubles later
	for (int i = 0; i < NumberOfFileTypes; i++) {
		DB_result = DeleteImageFromOBC(database, 0, i);
		if(DB_result != DataBaseSuccess)
			return DB_result;
	}

	zeroImageDataBase(database);

	return DataBaseSuccess;
}

//---------------------------------------------------------------

ImageDataBaseResult writeNewImageMetaDataToFRAM(ImageDataBase database, ImageMetadata image_metadata, uint32_t image_address, uint32_t time_image_taken, global_param globalParameters)
{
	ImageDataBaseResult result = SearchDataBase_byID(0, &image_metadata, &image_address);	// Finding space at the database(FRAM) for the image's ImageMetadata:
	CMP_AND_RETURN(result, DataBaseSuccess, DataBaseFull);

	image_metadata.cameraId = database->nextId;
	image_metadata.timestamp = time_image_taken;
	memcpy(&image_metadata.angles, &globalParameters.Attitude, sizeof(short) * 3);

	if (database->AutoThumbnailCreation)
		image_metadata.markedFor_TumbnailCreation = TRUE_8BIT;
	else
		image_metadata.markedFor_TumbnailCreation = FALSE_8BIT;

	for (fileType i = 0; i < NumberOfFileTypes; i++) {
		updateFileTypes(image_metadata, image_address, i, FALSE_bit);
	}

	database->nextId++;
	database->numberOfPictures++;

	return DataBaseSuccess;
}

ImageDataBaseResult takePicture(ImageDataBase database, Boolean8bit testPattern)
{
	global_param globalParameters;
	get_current_global_param(&globalParameters);

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

	ImageMetadata image_metadata;
	uint32_t image_address = 0;

	for (uint32_t numOfFramesTaken = 0; numOfFramesTaken < database->cameraParameters.frameAmount; numOfFramesTaken++)
	{
		vTaskDelay(100);

		result = writeNewImageMetaDataToFRAM(database, image_metadata, image_address, currentDate, globalParameters);
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

	ImageDataBaseResult result = SearchDataBase_byID(id, &image_metadata, &image_address);
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

Boolean checkIfInRange(ImageMetadata* image_metadata, time_unix* time_range)
{
	time_unix start, end;

	memcpy(&start, time_range, sizeof(time_unix));
	memcpy(&end, time_range + sizeof(time_unix), sizeof(time_unix));

	if (image_metadata->cameraId != 0 && (image_metadata->timestamp >= start && image_metadata->timestamp <= end)) // check if the current id is the requested id
		return TRUE;
	else
		return FALSE;
}

byte* getImageDataBaseBuffer(ImageDataBase database, time_unix start, time_unix end)
{
	// will be the first value in the array and will indicate its length (including itself!)
	uint32_t size = sizeof(int) + SIZEOF_IMAGE_DATABASE;

	byte* buffer = malloc(size);
	if (buffer == NULL)
		return NULL;

	memcpy(buffer + sizeof(int), database, SIZEOF_IMAGE_DATABASE);

	// printing for tests:
	printf("numberOfPictures = %u, nextId = %u, cameraParameters: frameAmount = %lu, frameRate = %lu, adcGain = %lu, pgaGain = %lu, exposure = %lu\n",
			 database->numberOfPictures, database->nextId,
			 database->cameraParameters.frameAmount, database->cameraParameters.frameRate,
			 (uint32_t)database->cameraParameters.adcGain, (uint32_t)database->cameraParameters.pgaGain, database->cameraParameters.exposure);

	// Write MetaData:

	ImageMetadata image_metadata;
	uint32_t image_address;

	uint32_t database_start_address = DATABASE_FRAM_START;
	uint32_t database_end_address = DATABASE_FRAM_END;

	time_unix* time_range = malloc(sizeof(time_unix) * 2);
	memcpy(time_range, &start, sizeof(time_unix));
	memcpy(time_range + sizeof(time_unix), &end, sizeof(time_unix));

	int result = 0;

	while(result == 0)
	{
		result = FRAM_DataStructure_search(database_start_address, database_end_address,
				sizeof(ImageMetadata), &image_metadata, &image_address,
				(Boolean (*)(void*, void*))&checkIfInRange, time_range);
		database_start_address = image_address + sizeof(ImageMetadata);

		if (result == 0)
		{
			memcpy(buffer + size, &image_metadata, sizeof(ImageMetadata));

			size += sizeof(ImageMetadata);
			realloc(buffer, size);

			// printing for tests:
			printf("cameraId: %d, timestamp: %u, inOBC:", image_metadata.cameraId, image_metadata.timestamp);
			bit fileTypes[8];
			char2bits(image_metadata.fileTypes, fileTypes);
			printf(", files:");
			for (int j = 0; j < 8; j++) {
				printf(" %u", fileTypes[j].value);
			}
			printf(", ");
			printf(", angles: %u %u %u, markedFor_TumbnailCreation = %u\n",
					image_metadata.angles[0], image_metadata.angles[1], image_metadata.angles[2], image_metadata.markedFor_TumbnailCreation);
		}
	}

	memcpy(buffer, &size, sizeof(size));

	free(time_range);
	return buffer;
}

imageid* get_ID_list_withDefaltThumbnail(time_unix startingTime, time_unix endTime)
{
	unsigned int numberOfIDs = 0;
	imageid* ID_list = malloc((numberOfIDs + 1) * sizeof(imageid));

	unsigned int currentPosition = DATABASE_FRAM_START;
	ImageMetadata currentImageMetadata; // will contain our current ID while going through the ImageDescriptor

	while(currentPosition <  DATABASE_FRAM_END) // as long as we are not in the end of the file ,we will continue
	{
		FRAM_read( (unsigned char*)&currentImageMetadata, (unsigned int)currentPosition, (unsigned int)sizeof(ImageMetadata)); // reading the ID from the ImageDescriptor file
		currentPosition += sizeof(ImageMetadata);

		if (checkForFileType(currentImageMetadata, DEFALT_REDUCTION_LEVEL) == DataBaseSuccess && (currentImageMetadata.timestamp >=startingTime && currentImageMetadata.timestamp <=endTime)) // check if the current ID is the requested ID
		{
			numberOfIDs += 1;
			realloc(ID_list, (numberOfIDs + 1) * sizeof(imageid));
			memcpy(ID_list + numberOfIDs*sizeof(imageid), &currentImageMetadata.cameraId, sizeof(imageid));
		}
	}

	memcpy(ID_list, &numberOfIDs, sizeof(int));

	return ID_list;
}
