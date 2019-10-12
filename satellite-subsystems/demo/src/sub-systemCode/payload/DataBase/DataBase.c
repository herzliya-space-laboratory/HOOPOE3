/*
 * ImageDataBase.c
 *
 *  Created on: 7 ׳‘׳�׳�׳™ 2019
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

#include "../../ADCS/AdcsGetDataAndTlm.h"

#include "../../Global/GlobalParam.h"
#include "../../Global/logger.h"

#include "../Misc/Boolean_bit.h"
#include "../Misc/Macros.h"
#include "../Misc/FileSystem.h"

#include "../Compression/ImageConversion.h"
#include "DataBase.h"

typedef struct __attribute__ ((__packed__))
{
	uint32_t frameAmount;
	uint32_t frameRate;
	uint8_t adcGain;
	uint8_t pgaGain;
	uint16_t sensorOffset;
	uint32_t exposure;
} CameraPhotographyValues;

struct __attribute__ ((__packed__)) ImageDataBase_t
{
	// size = 23 bytes
	unsigned int numberOfPictures;		///< current number of pictures saved on the satellite
	imageid nextId;						///< the next id we will use for a picture, (camera id)
	CameraPhotographyValues cameraParameters;
	Boolean8bit AutoThumbnailCreation;
};
#define SIZEOF_IMAGE_DATABASE ( sizeof(struct ImageDataBase_t) )

#define DATABASE_FRAM_START ( DATABASEFRAMADDRESS + SIZEOF_IMAGE_DATABASE )	///< The ImageDataBase's starting address in the FRAM, not including the general fields at the start of the database
#define DATABASE_FRAM_END ( ( DATABASE_FRAM_START ) + ( MAX_NUMBER_OF_PICTURES * sizeof(ImageMetadata) ) )	///< The ImageDataBases ending address in the FRAM

#define DATABASE_FRAM_SIZE (DATABASE_FRAM_END - DATABASEFRAMADDRESS)

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
			sprintf(baseString, "%s/%s/i%u.raw", GENERAL_PAYLOAD_FOLDER_NAME, RAW_IMAGES_FOLDER_NAME, id);
			break;
		case jpg:
			sprintf(baseString, "%s/%s/i%u.jpg", GENERAL_PAYLOAD_FOLDER_NAME, JPEG_IMAGES_FOLDER_NAME, id);
			break;
		case bmp:
			sprintf(baseString, "%s/%s/i%u.bmp", GENERAL_PAYLOAD_FOLDER_NAME, JPEG_IMAGES_FOLDER_NAME, id);
			break;
		case t02:
			sprintf(baseString, "%s/%s/i%u.t02", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_1_IMAGES_FOLDER_NAME, id);
			break;
		case t04:
			sprintf(baseString, "%s/%s/i%u.t04", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_2_IMAGES_FOLDER_NAME, id);
			break;
		case t08:
			sprintf(baseString, "%s/%s/i%u.t08", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_3_IMAGES_FOLDER_NAME, id);
			break;
		case t16:
			sprintf(baseString, "%s/%s/i%u.t16", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_4_IMAGES_FOLDER_NAME, id);
			break;
		case t32:
			sprintf(baseString, "%s/%s/i%u.t32", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_5_IMAGES_FOLDER_NAME, id);
			break;
		case t64:
			sprintf(baseString, "%s/%s/i%u.t32", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_6_IMAGES_FOLDER_NAME, id);
			break;
		default:
			break;
	}

	strcpy(string, baseString);
}

/*
 * Will restart the database (only FRAM), deleting all of its contents
*/
ImageDataBaseResult zeroImageDataBase()
{
	uint32_t database_fram_size = DATABASE_FRAM_END - DATABASEFRAMADDRESS;

	for (uint32_t i = 0; i < database_fram_size; i++)
	{
		imageBuffer[i] = 0;
	}

	int result = FRAM_write_exte((unsigned char*)imageBuffer, DATABASEFRAMADDRESS, database_fram_size);
	CMP_AND_RETURN(result, 0, DataBaseFramFail);

	return DataBaseSuccess;
}

ImageDataBaseResult updateGeneralDataBaseParameters(ImageDataBase database)
{
	CHECK_FOR_NULL(database, DataBaseNullPointer)

	int FRAM_result = FRAM_write_exte((unsigned char*)(database), DATABASEFRAMADDRESS, SIZEOF_IMAGE_DATABASE);
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

	while (database_current_address < DATABASE_FRAM_END && database_current_address >= DATABASE_FRAM_START)
	{
		result = FRAM_read_exte((unsigned char *)image_metadata, database_current_address, sizeof(ImageMetadata));
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
				image_metadata->angle_rates[0], image_metadata->angle_rates[1], image_metadata->angle_rates[2], image_metadata->markedFor_TumbnailCreation);

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

	while (database_current_address < DATABASE_FRAM_END && database_current_address >= DATABASE_FRAM_START)
	{
		result = FRAM_read_exte((unsigned char *)image_metadata, database_current_address, sizeof(ImageMetadata));
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

ImageDataBaseResult SearchDataBase_forLatestMarkedImage(ImageMetadata* image_metadata)
{
	uint32_t image_address;
	return SearchDataBase_byMark(DATABASE_FRAM_START, image_metadata, &image_address);
}

ImageDataBaseResult SearchDataBase_byTimeRange(time_unix lower_barrier, time_unix higher_barrier, ImageMetadata* image_metadata, uint32_t* image_address, uint32_t database_current_address)
{
	int result;

	while (database_current_address < DATABASE_FRAM_END && database_current_address >= DATABASE_FRAM_START)
	{
		result = FRAM_read_exte((unsigned char *)image_metadata, database_current_address, sizeof(ImageMetadata));
		CMP_AND_RETURN(result, 0, DataBaseFramFail);

		if ( (image_metadata->timestamp >= lower_barrier && image_metadata->timestamp <= higher_barrier) && image_metadata->cameraId != 0)
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

ImageDataBaseResult checkForFileType(ImageMetadata image_metadata, fileType reductionLevel);

ImageDataBaseResult SearchDataBase_forImageFileType_byTimeRange(time_unix lower_barrier, time_unix higher_barrier, fileType file_type, ImageMetadata* image_metadata, uint32_t* image_address, uint32_t database_starting_address)
{
	uint32_t database_current_address = database_starting_address;
	ImageDataBaseResult error;
	ImageMetadata current_image_metadata;
	uint32_t current_image_address;

	while (database_current_address < DATABASE_FRAM_END && database_current_address >= DATABASE_FRAM_START)
	{
		error = SearchDataBase_byTimeRange(lower_barrier, higher_barrier, &current_image_metadata, &current_image_address, database_current_address);
		database_current_address = current_image_address + sizeof(ImageMetadata);

		if (error == DataBaseSuccess)
		{
			if (checkForFileType(current_image_metadata, file_type) == DataBaseSuccess)
			{
				memcpy(image_metadata, &current_image_metadata, sizeof(ImageMetadata));
				memcpy(image_address, &current_image_address, sizeof(uint32_t));
				return DataBaseSuccess;
			}
		}
		else
		{
			return error;
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

	FRAM_write_exte((unsigned char*)image_metadata, image_address, sizeof(ImageMetadata));
}

uint32_t GetImageFactor(fileType image_type)
{
	return pow(2, image_type);
}

//---------------------------------------------------------------

ImageDataBaseResult readImageFromBuffer(imageid id, fileType image_type)
{
	char fileName[FILE_NAME_SIZE];

	ImageDataBaseResult result = GetImageFileName(id, image_type, fileName);
	DB_RETURN_ERROR(result);

	F_FILE *file = NULL;
	int error = f_managed_open(fileName, "r", &file);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	uint32_t factor = GetImageFactor(image_type);
	byte* buffer = imageBuffer;

	error = ReadFromFile(file, buffer, IMAGE_SIZE / (factor * factor), 1);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	error = f_managed_close(&file);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	return DataBaseSuccess;
}

ImageDataBaseResult saveImageToBuffer(imageid id, fileType image_type)
{
	char fileName[FILE_NAME_SIZE];
	ImageDataBaseResult result = GetImageFileName(id, image_type, fileName);
	DB_RETURN_ERROR(result);

	F_FILE *file = NULL;
	int error = f_managed_open(fileName, "w", &file);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	uint32_t factor = GetImageFactor(image_type);
	byte* buffer = imageBuffer;

	error = WriteToFile(file, buffer, IMAGE_SIZE / (factor * factor), 1);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	error = f_managed_close(&file);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

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
	setCameraPhotographyValues(database, DEFAULT_FRAME_AMOUNT, DEFAULT_FRAME_RATE, DEFAULT_ADC_GAIN, DEFAULT_PGA_GAIN, DEFAULT_SENSOR_OFFSET, DEFAULT_EXPOSURE);

	ImageDataBaseResult result = updateGeneralDataBaseParameters(database);
	DB_RETURN_ERROR(result);

	printf("\nRe: database->numberOfPictures = %u, database->nextId = %u; CameraParameters: frameAmount = %lu,"
			" frameRate = %lu, adcGain = %u, pgaGain = %u, sensor offset = %u, exposure = %lu\n", database->numberOfPictures,
			database->nextId, database->cameraParameters.frameAmount, database->cameraParameters.frameRate,
			database->cameraParameters.adcGain, database->cameraParameters.pgaGain,
			database->cameraParameters.sensorOffset, database->cameraParameters.exposure);

	return DataBaseSuccess;
}

ImageDataBase initImageDataBase(Boolean first_activation)
{
	ImageDataBase database = malloc(SIZEOF_IMAGE_DATABASE);	// allocate the memory for the database's variables

	int FRAM_result = FRAM_read_exte((unsigned char*)(database),  DATABASEFRAMADDRESS, SIZEOF_IMAGE_DATABASE);
	if(FRAM_result != 0)	// checking if the read from theFRAM succeeded
	{
		free(database);
		return NULL;
	}

	printf("numberOfPictures = %u, nextId = %u; CameraParameters: frameAmount = %lu, frameRate = %lu, adcGain = %u, pgaGain = %u, sensor offset = %u, exposure = %lu\n", database->numberOfPictures, database->nextId, database->cameraParameters.frameAmount, database->cameraParameters.frameRate, database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.sensorOffset, database->cameraParameters.exposure);

	ImageDataBaseResult error;

	if (database->nextId == 0 || first_activation)	// The FRAM is empty and the ImageDataBase wasn't initialized beforehand
	{
		error = zeroImageDataBase();
		CMP_AND_RETURN(error, DataBaseSuccess, NULL);
		error = setDataBaseValues(database);
		CMP_AND_RETURN(error, DataBaseSuccess, NULL);
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

void setCameraPhotographyValues(ImageDataBase database, uint32_t frameAmount, uint32_t frameRate, uint8_t adcGain, uint8_t pgaGain, uint16_t sensorOffset, uint32_t exposure)
{
	database->cameraParameters.frameAmount = frameAmount;
	database->cameraParameters.frameRate = frameRate;
	database->cameraParameters.adcGain = adcGain;
	database->cameraParameters.pgaGain = pgaGain;
	database->cameraParameters.sensorOffset = sensorOffset;
	database->cameraParameters.exposure = exposure;
}

//---------------------------------------------------------------

ImageDataBaseResult transferImageToSD_withoutSearch(imageid cameraId, uint32_t image_address, ImageMetadata image_metadata)
{
	int error = checkForFileType(image_metadata, raw);
	CMP_AND_RETURN(error, DataBaseNotInSD, DataBasealreadyInSD);

	// Reading the image to the buffer:

	vTaskDelay(CAMERA_DELAY);

	error = GECKO_ReadImage((uint32_t)cameraId, (uint32_t*)imageBuffer);
	if( error )
	{
		printf("\ntransferImageToSD Error = (%d) reading image!\n\r", error);
		return (GECKO_Read_Success - error);
	}

	vTaskDelay(DELAY);

	// Creating a file for the picture at iOBC SD:

	updateFileTypes(&image_metadata, image_address, raw, TRUE);

	error = saveImageToBuffer(cameraId, raw);
	CMP_AND_RETURN(error, DataBaseSuccess, error);

	// Updating the DataBase:

	error = checkForFileType(image_metadata, raw);
	CMP_AND_RETURN(error, DataBaseSuccess, DataBaseFail);

	WritePayloadLog(PAYLOAD_TRANSFERRED_IMAGE, (uint32_t)image_metadata.cameraId);

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
	char fileName[FILE_NAME_SIZE];
    getFileName(cameraId, type, fileName);

	int result = checkForFileType(image_metadata, type);
	DB_RETURN_ERROR(result);

	int error = f_delete(fileName);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

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

	vTaskDelay(DELAY);

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
		int FRAM_result = FRAM_write_exte(&zero, image_address + i, sizeof(uint8_t));
		CMP_AND_RETURN(FRAM_result, 0, DataBaseFramFail);
	}

	database->numberOfPictures--;

	updateGeneralDataBaseParameters(database);

	WritePayloadLog(PAYLOAD_ERASED_IMAGE, (uint32_t)image_metadata.cameraId);

	return DataBaseSuccess;
}

ImageDataBaseResult clearImageDataBase(void)
{
	int result;
	ImageMetadata image_metadata;
	uint32_t image_address = DATABASE_FRAM_START;

	while(image_address < DATABASE_FRAM_END && image_address >= DATABASE_FRAM_START)
	{
		//vTaskDelay(DELAY);

		result = FRAM_read_exte((unsigned char*)&image_metadata, image_address, sizeof(ImageMetadata));
		CMP_AND_RETURN(result, 0, DataBaseFramFail);

		image_address += sizeof(ImageMetadata);

		if (image_metadata.cameraId != 0)
		{
			for (fileType i = 0; i < NumberOfFileTypes; i++)
			{
				if(checkForFileType(image_metadata, i) == DataBaseSuccess)
				{
					result = DeleteImageFromOBC_withoutSearch(image_metadata.cameraId, i, image_address, image_metadata);
					if (result == DataBaseSuccess)
					{
						updateFileTypes(&image_metadata, image_address, i, FALSE);
					}
					else if (result != DataBaseNotInSD)
					{
						return result;
					}
				}
			}
		}
	}

	return DataBaseSuccess;
}

//---------------------------------------------------------------

ImageDataBaseResult handleMarkedPictures()
{
	ImageMetadata image_metadata;
	uint32_t image_address;

	ImageDataBaseResult DB_result = SearchDataBase_byMark(DATABASE_FRAM_START, &image_metadata, &image_address);

	Boolean already_transferred_raw = FALSE;

	if ( DB_result == 0 && image_metadata.cameraId != 0 )
	{
		if (checkForFileType(image_metadata, raw) == DataBaseNotInSD)
		{
			TurnOnGecko();

			DB_result = transferImageToSD_withoutSearch(image_metadata.cameraId, image_address, image_metadata);
			if (DB_result != DataBaseSuccess && DB_result != DataBasealreadyInSD)
				return DB_result;

			TurnOffGecko();

			vTaskDelay(DELAY);
		}
		else
		{
			already_transferred_raw = TRUE;
		}

		FRAM_read_exte(&image_metadata, image_address, sizeof(ImageMetadata));

		if (checkForFileType(image_metadata, DEFAULT_REDUCTION_LEVEL) == DataBaseNotInSD)
		{
			DB_result = CreateImageThumbnail_withoutSearch(image_metadata.cameraId, DEFAULT_REDUCTION_LEVEL, TRUE, image_address, image_metadata);
			if (DB_result != DataBaseSuccess && DB_result != DataBasealreadyInSD)
				return DB_result;

			vTaskDelay(DELAY);
		}

		FRAM_read_exte(&image_metadata, image_address, sizeof(ImageMetadata));

		if (!already_transferred_raw)
		{
			DB_result = DeleteImageFromOBC_withoutSearch(image_metadata.cameraId, raw, image_address, image_metadata);
			DB_RETURN_ERROR(DB_result);
		}

		// making sure i wont lose the data written in the functions above to the FRAM:
		FRAM_read_exte( (unsigned char*)&image_metadata, image_address, (unsigned int)sizeof(ImageMetadata)); // reading the id from the ImageDescriptor file

		image_metadata.markedFor_TumbnailCreation = FALSE_8BIT;
		FRAM_write_exte( (unsigned char*)&image_metadata, image_address, (unsigned int)sizeof(ImageMetadata)); // reading the id from the ImageDescriptor file
	}

	return DataBaseSuccess;
}

//---------------------------------------------------------------

ImageDataBaseResult writeNewImageMetaDataToFRAM(ImageDataBase database, time_unix time_image_taken, uint16_t angle_rates[3], uint16_t estimated_angles[3], byte raw_css[10])
{
	ImageMetadata image_metadata;
	uint32_t image_address;

	imageid zero = 0;
	ImageDataBaseResult result = SearchDataBase_byID(zero, &image_metadata, &image_address, DATABASE_FRAM_START);	// Finding space at the database(FRAM) for the image's ImageMetadata:
	CMP_AND_RETURN(result, DataBaseSuccess, result);

	image_metadata.cameraId = database->nextId;
	image_metadata.timestamp = time_image_taken;
	memcpy(&image_metadata.angle_rates, angle_rates, sizeof(uint16_t) * 3);
	memcpy(&image_metadata.estimated_angles, estimated_angles, sizeof(uint16_t) * 3);
	memcpy(&image_metadata.raw_css, raw_css, sizeof(byte) * 10);
	image_metadata.fileTypes = 0;

	if (database->AutoThumbnailCreation)
		image_metadata.markedFor_TumbnailCreation = TRUE_8BIT;
	else
		image_metadata.markedFor_TumbnailCreation = FALSE_8BIT;

	result = FRAM_write_exte((unsigned char*)&image_metadata, image_address, sizeof(ImageMetadata));
	CMP_AND_RETURN(result, 0, DataBaseFramFail);

	database->nextId++;
	database->numberOfPictures++;

	return DataBaseSuccess;
}

ImageDataBaseResult takePicture(ImageDataBase database, Boolean8bit testPattern)
{
	int err = 0;
	
	if (database->numberOfPictures == MAX_NUMBER_OF_PICTURES)
		return DataBaseFull;

	// Erasing previous image before taking one to clear this part of the SDs:

	vTaskDelay(CAMERA_DELAY);

	for (unsigned int i = 0; i < database->cameraParameters.frameAmount; i++)
	{
		err = GECKO_EraseBlock(database->nextId + i);
		CMP_AND_RETURN(err, 0, GECKO_Erase_Success - err);
	}

	vTaskDelay(CAMERA_DELAY);

	unsigned int currentDate = 0;
	Time_getUnixEpoch(&currentDate);

	// Getting Sat Angular Rates:
	cspace_adcs_angrate_t sen_rates;

	ImageDataBaseResult CAM_ADCS_error = DataBaseSuccess;
	TroubleErrCode ADCS_error = AdcsGetMeasAngSpeed(&sen_rates);
	if (ADCS_error != TRBL_SUCCESS)
	{
		CAM_ADCS_error = DataBaseAdcsError_gettingAngleRates;

		for (int i = 0; i < 6; i++)
		{
			sen_rates.raw[i] = 0;
		}
	}

	uint16_t angle_rates[3];
	memcpy(angle_rates, sen_rates.raw, sizeof(uint16_t) * 3);

	uint16_t angles[3];
	ADCS_error = AdcsGetEstAngles(angles);
	if (ADCS_error != TRBL_SUCCESS)
	{
		if (CAM_ADCS_error == DataBaseSuccess)
			CAM_ADCS_error = DataBaseAdcsError_gettingEstimatedAngles;

		for (int i = 0; i < 3; i++)
		{
			angles[i] = 0;
		}
	}

	// Getting Course-Sun-Sensor Values:
	byte raw_css[10];
	ADCS_error = AdcsGetCssVector(raw_css);
	if (ADCS_error != TRBL_SUCCESS)
	{
		if (CAM_ADCS_error == DataBaseSuccess)
			CAM_ADCS_error = DataBaseAdcsError_gettingCssVector;

		for (int i = 0; i < 10; i++)
		{
			raw_css[i] = 0;
		}
	}

	// Taking the Image:
	err = GECKO_TakeImage( database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.sensorOffset, database->cameraParameters.exposure, database->cameraParameters.frameAmount, database->cameraParameters.frameRate, database->nextId, testPattern);
	CMP_AND_RETURN(err, 0, GECKO_Take_Success - err);

	vTaskDelay(CAMERA_DELAY);

	// ImageDataBase handling:
	ImageDataBaseResult result;
	for (uint32_t numOfFramesTaken = 0; numOfFramesTaken < database->cameraParameters.frameAmount; numOfFramesTaken++)
	{
		vTaskDelay(DELAY);

		result = writeNewImageMetaDataToFRAM(database, currentDate, angle_rates, angles, raw_css);
		DB_RETURN_ERROR(result);

		currentDate += database->cameraParameters.frameRate;
	}

	updateGeneralDataBaseParameters(database);

	WritePayloadLog(PAYLOAD_TOOK_IMAGE, (uint32_t)getLatestID(database));

	DB_RETURN_ERROR(CAM_ADCS_error);
	return DataBaseSuccess;
}

ImageDataBaseResult takePicture_withSpecialParameters(ImageDataBase database, uint32_t frameAmount, uint32_t frameRate, uint8_t adcGain, uint8_t pgaGain, uint16_t sensorOffset, uint32_t exposure, Boolean8bit testPattern)
{
	CameraPhotographyValues regularParameters;
	memcpy(&regularParameters, &database->cameraParameters, sizeof(CameraPhotographyValues));

	setCameraPhotographyValues(database, frameAmount, frameRate, adcGain, pgaGain, sensorOffset, exposure);

	ImageDataBaseResult DB_result = takePicture(database, testPattern);

	setCameraPhotographyValues(database, regularParameters.frameAmount, regularParameters.frameRate, regularParameters.adcGain, regularParameters.pgaGain, regularParameters.sensorOffset, regularParameters.exposure);
	setDataBaseValues(database);

	return DB_result;
}

//---------------------------------------------------------------

ImageDataBaseResult GetImageFileName_withSpecialParameters(ImageMetadata image_metadata, fileType fileType, char fileName[FILE_NAME_SIZE])
{
	if (fileType != bmp)
	{
		int result = checkForFileType(image_metadata, fileType);
		DB_RETURN_ERROR(result);
	}

	getFileName(image_metadata.cameraId, fileType, fileName);

	return DataBaseSuccess;
}

ImageDataBaseResult GetImageFileName(imageid id, fileType fileType, char fileName[FILE_NAME_SIZE])
{
	ImageMetadata image_metadata;
	uint32_t image_address;

	ImageDataBaseResult result = SearchDataBase_byID(id, &image_metadata, &image_address, DATABASE_FRAM_START);
	DB_RETURN_ERROR(result);

	return GetImageFileName_withSpecialParameters(image_metadata, fileType, fileName);
}

//---------------------------------------------------------------

uint32_t getDataBaseSize()
{
	return DATABASE_FRAM_SIZE;
}

ImageDataBaseResult getImageDataBaseBuffer(imageid start, imageid end, byte buffer[DATABASE_FRAM_SIZE], uint32_t* size)
{
	int result = FRAM_read_exte((unsigned char*)(buffer),  DATABASEFRAMADDRESS, DATABASE_FRAM_SIZE);
	CMP_AND_RETURN(result, 0, DataBaseFramFail);

	uint32_t database_size = SIZEOF_IMAGE_DATABASE;
	Boolean previus_zero = FALSE;

	// Write MetaData:

	ImageMetadata image_metadata;
	for (uint32_t i = 0; i < MAX_NUMBER_OF_PICTURES; i++)
	{
		vTaskDelay(10);

		memcpy(&image_metadata, buffer + SIZEOF_IMAGE_DATABASE + i * sizeof(ImageMetadata), sizeof(ImageMetadata));

		if (image_metadata.cameraId != 0 && (image_metadata.cameraId >= start && image_metadata.cameraId <= end))
		{
			database_size += sizeof(ImageMetadata);

			// printing for tests:
			printf("cameraId: %d, timestamp: %u, inOBC:", image_metadata.cameraId, image_metadata.timestamp);
			bit fileTypes[8];
			char2bits(image_metadata.fileTypes, fileTypes);
			printf(", files:");
			for (int j = 0; j < 8; j++) {
				printf(" %u", fileTypes[j].value);
			}
			printf(", angles: %u %u %u, markedFor_4thTumbnailCreation = %u\n",
					image_metadata.angle_rates[0], image_metadata.angle_rates[1], image_metadata.angle_rates[2], image_metadata.markedFor_TumbnailCreation);
		}
		else
		{
			if (previus_zero)
			{
				break;
			}
			else
			{
				previus_zero = TRUE;
			}
		}
	}

	memcpy(size, &database_size, sizeof(uint32_t));

	return DataBaseSuccess;
}

//---------------------------------------------------------------

void setAutoThumbnailCreation(ImageDataBase database, Boolean8bit new_AutoThumbnailCreation)
{
	database->AutoThumbnailCreation = new_AutoThumbnailCreation;
}

//---------------------------------------------------------------

void Gecko_TroubleShooter(ImageDataBaseResult error)
{
	Boolean should_reset_take = error > GECKO_Take_Success && error < GECKO_Read_Success;
	Boolean should_reset_read = error > GECKO_Read_Success && error < GECKO_Erase_Success && error != GECKO_Read_StoppedAsPerRequest;
	Boolean should_reset_erase = error > GECKO_Erase_Success && error <= GECKO_Erase_Error_ClearEraseDoneFlag;

	Boolean should_reset = should_reset_take || should_reset_read || should_reset_erase;

	if (should_reset)
	{
		TurnOffGecko();
	}
}
