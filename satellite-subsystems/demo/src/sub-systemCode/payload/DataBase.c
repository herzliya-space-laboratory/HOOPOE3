/*
 * DataBase.c
 *
 *  Created on: 7 במאי 2019
 *      Author: I7COMPUTER
 */

#include <freertos/FreeRTOS.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>		//TODO: remove before flight. including printfs
#include <stdint.h>

#include <freertos/task.h>

#include <hcc/api_fat.h>
#include <hal/Boolean.h>
#include <hal/Timing/Time.h>
#include <hal/Utility/util.h>
#include <hal/Storage/FRAM.h>

#include <satellite-subsystems/GomEPS.h>

// #include <satellite-subsystems/SCS_Gecko/gecko_use_cases.h>
#include "Camera.h"

#include "ImgCompressor/ImgCompressor.h"
#include "FRAM_Extended.h"
#include "DataBase.h"

typedef struct
{
	// size = 66 bytes
	imageid cameraId;
	imageid secondaryId;

	unsigned int firstPictureInSequance;
	unsigned int numberOfPictureInSequance;

	unsigned int autoIterationId;

	unsigned int date;

	Boolean fileTypes[NumberOfFileTypes];

	unsigned char angles[6];
} ImageDetails;

typedef struct
{
	unsigned int frameRate;
	uint8_t adcGain;
	uint8_t pgaGain;
	unsigned int exposure;
} CameraParameters;

struct DataBase_t
{
	// size = 56 bytes
	unsigned int maxNumberOfGroundPictures;
	unsigned int maxNumberOfAutoPictures;

	unsigned int numberOfPictures;					// current number of pictures saved on the satellite

	unsigned int nextAutoIterationId;

	unsigned int autoFrequency;	// ToDo: Change this name!

	unsigned int autoFrameAmount;

	unsigned int nextId;						// the next id we will use for a picture, camera id
	unsigned int nextGroundId;					// ground id
	unsigned int nextAutoId;

	CameraParameters cameraParameters;
	CameraParameters autoCameraParameters;
};

static uint8_t imageBuffer[IMAGE_HEIGHT][IMAGE_WIDTH];

// ----------INTERNAL FUNCTIONS----------

/*
 * Will put in the string the supposed filename of an picture based on its id
 * @param soon2Bstring the number you want to convert into string form,
 * string will contain the number in string form
*/
static void getFileName(unsigned int id, fileType type, char string[FILENAMESIZE])
{
	unsigned int num = id;
	char digit[10] = "0123456789";

	char baseString[FILENAMESIZE] = "img0000000.raw";
	strcpy(string, baseString);

	int i;
	for (i = FILENAMESIZE - 6; i > 2; i--)
	{
		string[i] = digit[num % 10];
		num /= 10;
	}

	switch (type)
	{
		case raw:
			strcpy(string + (FILENAMESIZE - 5), ".raw");
			break;
		case jpg:
			strcpy(string + (FILENAMESIZE - 5), ".jpg");
			break;
		case bmp:
			strcpy(string + (FILENAMESIZE - 5), ".bmp");
			break;
		case t02:
			strcpy(string + (FILENAMESIZE - 5), ".t02");
			break;
		case t04:
			strcpy(string + (FILENAMESIZE - 5), ".t04");
			break;
		case t08:
			strcpy(string + (FILENAMESIZE - 5), ".t08");
			break;
		case t16:
			strcpy(string + (FILENAMESIZE - 5), ".t16");
			break;
		case t32:
			strcpy(string + (FILENAMESIZE - 5), ".t32");
			break;
		case t64:
			strcpy(string + (FILENAMESIZE - 5), ".t64");
			break;
		default:
			break;
	}

	string[FILENAMESIZE - 1] = '\0';

	printf("file name = %s\n", string);
}

/*
 * Will return the DataBases starting address in the FRAM
 * meaning not including the general fields at the start of the database
*/
static unsigned int getDatabaseStart(DataBase database)
{
	return DATABASEFRAMADDRESS +  sizeof(*database);
}

static unsigned int getDatabaseAutoSegmentStart(DataBase database)
{
	return getDatabaseStart(database) + (database->maxNumberOfGroundPictures * sizeof(ImageDetails));
}

/*
 * Will return the DataBases ending address in the FRAM
*/
static unsigned int getDatabaseEnd(DataBase database)
{
	return getDatabaseStart(database) + ((database->maxNumberOfGroundPictures + database->maxNumberOfAutoPictures) * sizeof(ImageDetails));
}

/*
 * Will restart the database (only FRAM), deleting all of its contents
*/
static DataBaseResult zeroDataBase(DataBase database)
{
	printf("\n----------zeroDataBase----------\n");

	uint8_t data = 0;
	int result;
	for (unsigned int i = DATABASEFRAMADDRESS; i < getDatabaseEnd(database); i += sizeof(uint8_t))
	{
		result = FRAM_write((unsigned char*)&data, i, sizeof(uint8_t));
		if (result != 0)
		{
			return DataBaseFramFail;
		}

		//Test:
		result = FRAM_read((unsigned char*)&data, i, sizeof(uint8_t));
		if (result != 0)
		{
			return DataBaseFramFail;
		}
		printf("%u, ", data);
	}

	return DataBaseSuccess;
}

static DataBaseResult updateGeneralParameters(DataBase database)
{
	if(database == NULL)
	{
		free(database);
		return DataBaseNullPointer;
	}

	unsigned int currentPosition = DATABASEFRAMADDRESS;

	int FRAM_result1 = FRAM_writeAndProgress((unsigned char*)(&database->maxNumberOfGroundPictures),  &currentPosition, sizeof(unsigned int));
	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->maxNumberOfAutoPictures),  &currentPosition, sizeof(unsigned int));

	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->numberOfPictures),  &currentPosition, sizeof(unsigned int));

	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->nextAutoIterationId),  &currentPosition, sizeof(unsigned int));

	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->autoFrequency),  &currentPosition, sizeof(unsigned int));
	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->autoFrameAmount),  &currentPosition, sizeof(unsigned int));

	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->nextId),  &currentPosition, sizeof(int));
	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->nextGroundId),  &currentPosition, sizeof(int));
	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->nextAutoId),  &currentPosition, sizeof(int));

	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->cameraParameters),  &currentPosition, sizeof(CameraParameters));
	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->autoCameraParameters),  &currentPosition, sizeof(CameraParameters));

	if(FRAM_result1 != 0)									// checking if the read from theFRAM succeeded
	{
		free(database);
		return DataBaseFramFail;
	}
	return DataBaseSuccess;
}

/*
static DataBaseResult searchDataBaseByTime(DataBase database, ImageDetails images[], unsigned int start, unsigned int end)
{
	ImageDetails currentImageDetails;

	unsigned int currentPosition = getDatabaseStart(database);
	unsigned int endPosition = getDatabaseEnd(database);

	unsigned int i = 0;
	while(currentPosition < endPosition) // as long as we are not in the end of the file ,we will continue
	{
		FRAM_readAndProgress((unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails));

		if (currentImageDetails.cameraId != 0 && (currentImageDetails.date >= start && currentImageDetails.date <= end))
		{
			images[i] = currentImageDetails;
			i++;
		}
	}

	return DataBaseSuccess;
}
*/
// ----------BASIC FUNCTIONS----------

imageid getCameraId_bySecondaryId(DataBase database, imageid secondaryId, Boolean Auto)
{
	ImageDetails currentImageDetails;

	unsigned int currentPosition;
	unsigned int endPosition;
	if (Auto)
	{
		currentPosition = getDatabaseAutoSegmentStart(database);
		endPosition = getDatabaseEnd(database);
	}
	else
	{
		currentPosition = getDatabaseStart(database);
		endPosition = getDatabaseAutoSegmentStart(database);
	}

	while(currentPosition < endPosition) // as long as we are not in the end of the file ,we will continue
	{
		FRAM_readAndProgress((unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails));

		if (currentImageDetails.secondaryId == secondaryId)
		{
			return currentImageDetails.cameraId;
		}
	}

	return 0;
}

DataBase initDataBase(Boolean reset)
{
	DataBase database = malloc(sizeof(*database));	// allocate the memory for the database's variables

	if(database == NULL)
	{
		printf("\n *ERROR* - NULL POINTER\n");
		free(database);
		return NULL;
	}

	unsigned int currentPosition = DATABASEFRAMADDRESS;
	int FRAM_result1 = FRAM_readAndProgress((unsigned char*)(&database->maxNumberOfGroundPictures),  &currentPosition, sizeof(unsigned int));
	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->maxNumberOfAutoPictures),  &currentPosition, sizeof(unsigned int));

	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->numberOfPictures),  &currentPosition, sizeof(unsigned int));

	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->nextAutoIterationId),  &currentPosition, sizeof(unsigned int));

	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->autoFrequency),  &currentPosition, sizeof(unsigned int));
	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->autoFrameAmount),  &currentPosition, sizeof(unsigned int));

	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->nextId),  &currentPosition, sizeof(int));
	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->nextGroundId),  &currentPosition, sizeof(int));
	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->nextAutoId),  &currentPosition, sizeof(int));

	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->cameraParameters),  &currentPosition, sizeof(CameraParameters));
	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->autoCameraParameters),  &currentPosition, sizeof(CameraParameters));

	if(FRAM_result1 != 0)				// checking if the read from theFRAM succeeded
	{
		free(database);
		return NULL;
	}

	printf("maxNumberOfGroundPictures = %u, maxNumberOfAutoPictures = %u, numberOfPictures = %u, nextAutoIterationId = %u, autoFrequency = %u, autoFrameAmount = %u, nextId = %u,  nextGroundId = %u, nextAutoId = %u; CameraParameters: frameRate = %u, adcGain = %u, pgaGain = %u, exposure = %u; AutoCameraParameters: frameRate = %u, adcGain = %u, pgaGain = %u, exposure = %u\n", database->maxNumberOfGroundPictures, database->maxNumberOfAutoPictures, database->numberOfPictures, database->nextAutoIterationId, database->autoFrequency, database->autoFrameAmount, database->nextId, database->nextGroundId, database->nextAutoId, database->cameraParameters.frameRate, database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.exposure, database->autoCameraParameters.frameRate, database->autoCameraParameters.adcGain, database->autoCameraParameters.pgaGain, database->autoCameraParameters.exposure);

	if (database->nextId == 0 || reset)	// The FRAM is empty and the DataBase wasn't initialized beforehand
	{
		// Resetting the DataBase itself (on the FRAM)
		DataBaseResult DB_result = zeroDataBase(database);
		if (DB_result != DataBaseSuccess)
		{
			printf("Init DB - *ERROR* zero database (%u)", DB_result);
			return NULL;
		}

		// Writing default values: (the number of pictures currently on the satellite is 0 so the next one's id will be 1.)

		database->maxNumberOfGroundPictures = NUMBEROFGROUNDPICTURES;
		database->maxNumberOfAutoPictures = NUMBEROFAUTOPICTURES;

		database->numberOfPictures = 0;

		database->nextAutoIterationId = 1;

		database->autoFrequency = defaltAutoFrequancy;

		database->nextId = 1;
		database->nextGroundId = 1;
		database->nextAutoId = 1;

		updateCameraParameters(database, defaltFrameRate, defaltAdcGain, defaltPgaGain, defaltExposure, 0, FALSE);
		updateCameraParameters(database, defaltFrameRate, defaltAdcGain, defaltPgaGain, defaltExposure, defaltAutoFrameAmount, TRUE);

		updateGeneralParameters(database);

		printf("\nRe: database->numberOfPictures = %u, database->nextId = %u\n", database->numberOfPictures, database->nextId);
	}

	return database;
}

DataBaseResult resetDataBase(DataBase database)
{
	printf("\n----------resetDataBase----------\n");

	DataBaseResult DB_result = DataBaseSuccess;

	// Deleting all pictures saved on iOBC SD so we wont have doubles later
	for (int i = 0; i < NumberOfFileTypes; i++) {
		DB_result = DeleteImageFromOBC(database, 0, i, FALSE);
		DB_result = DeleteImageFromOBC(database, 0, i, TRUE);
		if(DB_result != DataBaseSuccess)
			return DB_result;
	}

	// Reinitializing the database:
	free(database);
	database = initDataBase(TRUE);

	return DataBaseSuccess;
}

void updateCameraParameters(DataBase database, unsigned int frameRate, unsigned char adcGain, unsigned char pgaGain, unsigned int exposure, unsigned int frameAmount, Boolean Auto)
{
	if (Auto)
	{
		database->autoCameraParameters.frameRate = frameRate;
		database->autoCameraParameters.adcGain = adcGain;
		database->autoCameraParameters.pgaGain = pgaGain;
		database->autoCameraParameters.exposure = exposure;

		database->autoFrameAmount = frameAmount;
	}
	else
	{
		database->cameraParameters.frameRate = frameRate;
		database->cameraParameters.adcGain = adcGain;
		database->cameraParameters.pgaGain = pgaGain;
		database->cameraParameters.exposure = exposure;
	}

	updateGeneralParameters(database);
}

DataBaseResult transferImageToSD(DataBase database, imageid cameraId, Boolean mode, Boolean Auto)
{
	printf("\n----------transferImageToSD----------\n");

	if(database == NULL)	// Checking for NULL pointer
	{
		return DataBaseNullPointer;
	}

	int err = GomEpsResetWDT(0);
	printf("err DB eps: %d\n", err);

	err = GECKO_ReadImage((uint32_t)cameraId, (uint32_t*)imageBuffer, (Boolean)mode);
	if( err )
	{
		printf("transferImageToSD Error = (%d) reading image!\n\r",err);
		return (GECKO_Read_Success - err);
	}

	vTaskDelay(500);

	// Creating a file for the picture at iOBC sd:
	char fileName[FILENAMESIZE];
	getFileName(cameraId, raw, fileName);

	F_FILE *PictureFile = f_open(fileName, "w+" );	// open a new file for writing in safe mode
	if(PictureFile == NULL)
	{
		printf("transferImageToSD - f_open");	// if file pointer is NULL, get an error
		return DataBaseFail;
	}

	err = GomEpsResetWDT(0);
	printf("err DB eps: %d\n", err);


	unsigned int elementsWritten = f_write(imageBuffer, sizeof(uint8_t), IMAGE_SIZE, PictureFile);

	if (elementsWritten != IMAGE_SIZE)
	{
		// if bytes to write doesn't equal bytes written, get the error
		printf("transferImageToSD - f_write");
		return DataBaseFail;
	}

	f_flush(PictureFile );	// only after flushing can data be considered safe

	err = f_close(PictureFile);	// data is also considered safe when file is closed
	if( err )
	{
		printf("transferImageToSD - f_close");
		return DataBaseFail;
	}


	// Updating the file_name of this specific picture at the database:
	unsigned int currentPosition;
	unsigned int endPosition;
	if (Auto)
	{
		currentPosition = getDatabaseAutoSegmentStart(database);
		endPosition = getDatabaseEnd(database);
	}
	else
	{
		currentPosition = getDatabaseStart(database);
		endPosition = getDatabaseAutoSegmentStart(database);
	}


	ImageDetails currentImageDetails;	// will contain our current ID while going through the ImageDescriptors at the database
	int FRAM_result;

	while(currentPosition < endPosition)	// as long as we are not in the end of the file ,we will continue
	{
		FRAM_result = FRAM_readAndProgress((unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails));	// reading the id from the ImageDescriptor file
		if (FRAM_result != 0)
		{
			printf("DB ERROR - FRAM fail\n");
			return DataBaseFramFail;
		}


		printf("TransferImageToOBC - current id = %u, current position = %u\n", currentImageDetails.cameraId, (unsigned int)(currentPosition - sizeof(imageid)));

		if (currentImageDetails.cameraId == cameraId)	// check if the current id is the requested id
		{
			currentImageDetails.fileTypes[raw] = TRUE;

			currentPosition -= sizeof(ImageDetails);

			FRAM_result = FRAM_writeAndProgress((unsigned char*)&currentImageDetails, &currentPosition, (unsigned int)sizeof(ImageDetails));	// reading the name of the file from the ImageDescriptor file

			if (FRAM_result != 0)
			{
				printf("DB ERROR - FRAM fail\n");
				return DataBaseFramFail;
			}

			updateGeneralParameters(database);

			return DataBaseSuccess;
		}
	}
	printf("DB ERROR - Illegal ID\n");
	return DataBaseIllegalId;	// Does not exist
}

DataBaseResult DeleteImageFromOBC(DataBase database, imageid cameraId, fileType type, Boolean Auto)
{
	printf("\n----------DeleteImageFromOBC----------\n");

	if (database == NULL) // making sure that database isn't a NULL pointer
	{
		printf("null");
		return DataBaseNullPointer;
	}

    ImageDetails currentImageDetails;	// the struct that will contain the information about each image

    char fileName[FILENAMESIZE];
    getFileName(cameraId, type, fileName);

	unsigned int currentPosition;
	unsigned int endPosition;
	if (Auto)
	{
		currentPosition = getDatabaseAutoSegmentStart(database);
		endPosition = getDatabaseEnd(database);
	}
	else
	{
		currentPosition = getDatabaseStart(database);
		endPosition = getDatabaseAutoSegmentStart(database);
	}


    while (currentPosition < endPosition)	// as long as we are not in the end of the file ,we will continue
    {
    	FRAM_readAndProgress((unsigned char*)&currentImageDetails, &currentPosition, sizeof(ImageDetails)); // coping data from image descriptor to ImageDetails
    	printf("id = %u, file type = %u, file_name = %u\n", currentImageDetails.cameraId, type, currentImageDetails.fileTypes[type]);

    	if (currentImageDetails.cameraId != 0)
    	{
    		if (cameraId == 0)
    		{
    			if (currentImageDetails.fileTypes[type])
    			{
    				int file_result = f_delete(fileName); // will delete the file
    				if(file_result != 0) // remove returns -1 if the deletion process failed
   					{
    					printf("DeleteImageFromOBC Fail - file deletion fail\n");
    					return DataBaseFail;
    				}
    			}
    		}
    		else if (currentImageDetails.cameraId == cameraId)	// if we dont skip on that id
    		{
    			if(!currentImageDetails.fileTypes[type])
   				{
   					printf("Not in SD\n");
    				return DataBaseNotInSD;
    			}
    			int file_result = f_delete(fileName); // will delete the file
    			if(file_result != 0) // remove returns -1 if the deletion process failed
    			{
    				printf("DeleteImageFromOBC Fail - file deletion fail\n");
    				return DataBaseFail;
    			}
    		}

    		if (cameraId == 0 || currentImageDetails.cameraId == cameraId)
    		{
    			currentImageDetails.fileTypes[type] = FALSE;

    			currentPosition -= sizeof(ImageDetails); // we jump back in the ImageDescriptor to the start of the filename (tight after id)
    			FRAM_writeAndProgress((unsigned char*)&currentImageDetails, &currentPosition, sizeof(ImageDetails)); // writing zeroes over the file name of the current ImageDescriptor
    		}

    		if (currentImageDetails.cameraId == cameraId)
    		{
    			printf("c id = %u, id = %u\n", currentImageDetails.cameraId, cameraId);
    			printf("id = %u, file type = %u, file_name = %u\ncurrentPosition = %u, DB end = %u\n", currentImageDetails.cameraId, type, currentImageDetails.fileTypes[type], currentPosition, getDatabaseEnd(database));
    			break;
    		}
    	}
    }

	updateGeneralParameters(database);

    return DataBaseSuccess;
}

DataBaseResult DeleteImageFromPayload(DataBase database, imageid id, Boolean Auto)
{
	printf("\n----------DeleteImageFromPayload----------\n");

	if (database == NULL) // making sure that database isn't a NULL pointer
	{
		printf("\nNULL pointer\n");
		return DataBaseNullPointer;
	}

	int WDerr = GomEpsResetWDT(0);
	printf("err DB eps: %d\n", WDerr);

	int err = GECKO_EraseBlock(id);
	if( err )
	{
		printf("Error (%d) erasing image!\n\r",err);
		return (GECKO_Erase_Success - err);
	}

	vTaskDelay(500);

	uint8_t temp = 0; // a  variable that we will use to delete stuff from the ImageDescriptor later in the function

	unsigned int currentPosition;
	unsigned int endPosition;
	if (Auto)
	{
		currentPosition = getDatabaseAutoSegmentStart(database);
		endPosition = getDatabaseEnd(database);
	}
	else
	{
		currentPosition = getDatabaseStart(database);
		endPosition = getDatabaseAutoSegmentStart(database);
	}

	ImageDetails currentImageDetails; // will contain our current ID while going through the ImageDescriptor

	Boolean Found = FALSE;

	while(currentPosition < endPosition)  // as long as we are not in the end of the file ,we will continue
	{
		FRAM_readAndProgress( (unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

		if (currentImageDetails.cameraId == id) // check if the current id is the requested id
		{
			printf("current id = %u\n", currentImageDetails.cameraId);

			// Checking if the one we found is a double:
			if (Found == TRUE)
			{
				printf("\nDouble\n");
				return DataBaseIllegalId;
			}

			Found = TRUE;

			for (unsigned int i = 0; i < NumberOfFileTypes; i++) {
				if(currentImageDetails.fileTypes[i])
				{
					DataBaseResult DB_result = DeleteImageFromOBC(database, id, i, Auto);
					if (DB_result != DataBaseSuccess)
					{
						return DB_result;
					}
					break;
				}
			}

			// Clearing the portion of the database:
			currentPosition -= sizeof(ImageDetails);

			for(unsigned int i = 0; i < sizeof(ImageDetails); i += sizeof(uint8_t))
			{
				int FRAM_result = FRAM_writeAndProgress( &temp, &currentPosition, sizeof(uint8_t)); // writing zeroes over the ImageDescriptor

				if (FRAM_result != 0)
				{
					printf("\nFRAM\n");
					return DataBaseFramFail;
				}
			}

			database->numberOfPictures--;

			updateGeneralParameters(database);
		}
	}

	// Checking if id was found at all: (meaning did it even exists?)
	if (Found == FALSE)
		return DataBaseIllegalId;

	updateGeneralParameters(database);

	return DataBaseSuccess;
}

DataBaseResult takePicture(DataBase database, unsigned int frameAmount, Boolean testPattern, Boolean Auto)
{
	printf("\n----------TAKE PICTURE----------\n");

	database = initDataBase(FALSE);

	unsigned int currentDate = 0;
	Time_getUnixEpoch(&currentDate);
	printf("currentDate = %u", currentDate);

	unsigned char angles[6];

	F_FILE* anglesFile = f_open(ESTIMATED_ANGLES_FILE, "r");

	f_seek(anglesFile, 0, SEEK_END);
	unsigned int end = f_tell(anglesFile);
	f_seek(anglesFile, end - sizeof(angles), SEEK_SET);

	f_read(angles, sizeof(angles), 1, anglesFile);

/*
	if (Auto)
	{
		ImageDetails images[database->maxNumberOfAutoPictures + database->maxNumberOfGroundPictures];
		for (unsigned int i = 0; i < database->maxNumberOfGroundPictures + database->maxNumberOfAutoPictures; i++) {
			for (int j = 0; j < NumberOfFileTypes; j++) {
				images[i].fileTypes[j] = FALSE;
			}
			for (unsigned int j = 0; j < sizeof(images[i].angles); j++) {
				*(images[i].angles + j) = 0;
			}
			images[i].autoIterationId = 0;
			images[i].cameraId = 0;
			images[i].date = 0;
			images[i].firstPictureInSequance = 0;
			images[i].numberOfPictureInSequance = 0;
			images[i].secondaryId = 0;
		}

		searchDataBaseByTime(database, images, currentDate - database->autoFrequency, currentDate);
		if (images[0].cameraId != 0)
			return DataBaseShotPicNotLongAgo;
	}*/

	int err = 0;
	for (unsigned int i = 0; i < frameAmount; i++)
	{
		err = GomEpsResetWDT(0);
		printf("err DB eps: %d\n", err);
		// Erasing previous image before taking one to clear this part of the SDs
		err = GECKO_EraseBlock(database->nextId + i);
		if (err)
		{
			printf("Error (%d) erasing image!\n",err);
		}
		vTaskDelay(500);
	}


	int WDerr = GomEpsResetWDT(0);
	printf("err DB eps: %d\n", WDerr);

	// Taking images:
	if (Auto)
		err = GECKO_TakeImage( database->autoCameraParameters.adcGain, database->autoCameraParameters.pgaGain, database->autoCameraParameters.exposure, database->autoFrameAmount, database->autoCameraParameters.frameRate, database->nextId - (frameAmount - 1), testPattern);
	else
		err = GECKO_TakeImage( database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.exposure, frameAmount, database->cameraParameters.frameRate, database->nextId - (frameAmount - 1), testPattern);

	vTaskDelay(500);
	if(err)
	{
		printf("Error (%d) taking image!\n",err);
		return (GECKO_Take_Success - err);
	}



	// DataBase handling:

	unsigned int starting_nextAutoId = database->nextAutoId;
	err = 0;

	unsigned int numOfFramesTaken;
	for (numOfFramesTaken = 0; numOfFramesTaken < frameAmount; numOfFramesTaken++)
	{
		printf("database->nextId = %u, database->nextAutoId = %u, database->numberOfPictures = %u\n", database->nextId, database->nextAutoId, database->numberOfPictures);

		ImageDetails currentImageDetails; //id, filename, score, type, date

		// Writing current time to the DataBase(in Unix):
		if (Auto)
			currentImageDetails.date = currentDate + database->autoCameraParameters.frameRate*numOfFramesTaken;
		else
			currentImageDetails.date = currentDate + database->cameraParameters.frameRate*numOfFramesTaken;


		//ImageDetails.date += numOfFramesTaken*frameRate;	// handling the time stamp of multiple frames
		printf("ImageDetails.date = %d\n", currentImageDetails.date);

		// Finding space at the database(FRAM) for the image's ImageDetails:
		printf("\nFinding space at the database(FRAM) for the image's ImageDetails:\n");

		unsigned int currentPosition;
		unsigned int endPosition;
		if (Auto)
		{
			currentPosition = getDatabaseAutoSegmentStart(database);
			endPosition = getDatabaseEnd(database);
		}
		else
		{
			currentPosition = getDatabaseStart(database);
			endPosition = getDatabaseAutoSegmentStart(database);
		}


		Boolean IsEmpty = FALSE;

		while(currentPosition < endPosition) // as long as we are not in the end of the file ,we will continue
		{
			int FRAM_result = FRAM_readAndProgress((unsigned char*)&currentImageDetails.cameraId, (unsigned int*)&currentPosition, (unsigned int)sizeof(imageid));
			FRAM_result += FRAM_readAndProgress((unsigned char*)&currentImageDetails.secondaryId, (unsigned int*)&currentPosition, (unsigned int)sizeof(imageid));

			printf("\n currentPosition = %d, current id = %d\n", (unsigned int)(currentPosition - sizeof(imageid)), currentImageDetails.cameraId);

			if (FRAM_result != 0)
			{
				return DataBaseFramFail;
			}
			if (currentImageDetails.cameraId == 0)
			{
				IsEmpty = TRUE;
				break;
			}
			else if (Auto && currentImageDetails.secondaryId == database->nextAutoId)
			{
				IsEmpty = TRUE;
				break;
			}

			else
			{
				currentPosition += sizeof(ImageDetails) - 2*sizeof(imageid);
			}
		}

		if (IsEmpty == TRUE)
		{
			currentPosition -= 2*sizeof(imageid);

			if(currentImageDetails.cameraId != 0)
			{
				for (unsigned int i = 0; i < NumberOfFileTypes; i++) {
					if(currentImageDetails.fileTypes[i])
					{
						DataBaseResult DB_result = DeleteImageFromOBC(database, currentImageDetails.cameraId, i, Auto);
						if (DB_result != DataBaseSuccess && DB_result != DataBaseNotInSD)
						{
							return DB_result;
						}
						break;
					}
				}
			}

			currentImageDetails.cameraId = database->nextId;

			// saving the angles to ImageDetails
			for (int h = 0; h < 6; h++) {
				*(currentImageDetails.angles + h) = *(angles + h);
			}

			if(Auto)
			{
				currentImageDetails.secondaryId = database->nextAutoId;
				database->nextAutoId++;

				currentImageDetails.autoIterationId = database->nextAutoIterationId;

				if (database->nextAutoId == NUMBEROFAUTOPICTURES + 1)
					database->nextAutoId = 1;
			}
			else
			{
				currentImageDetails.secondaryId = database->nextGroundId;
				database->nextGroundId++;

				currentImageDetails.autoIterationId = 0;
			}

			if (numOfFramesTaken == 0)
				currentImageDetails.firstPictureInSequance = currentImageDetails.cameraId;
			currentImageDetails.numberOfPictureInSequance = numOfFramesTaken + 1;

			database->nextId++;
			database->numberOfPictures++;

			printf("\nid: %d, first Picture In Sequance = %u, number Of Picture In Sequance = %u, inOBC:", currentImageDetails.cameraId, currentImageDetails.firstPictureInSequance, currentImageDetails.numberOfPictureInSequance);

			for (int i = 0; i < NumberOfFileTypes; i++) {
				currentImageDetails.fileTypes[i] = FALSE;
				printf(" %u", currentImageDetails.fileTypes[i]);
			}

			FRAM_writeAndProgress((unsigned char*)&currentImageDetails, &currentPosition, sizeof(ImageDetails)); // writing data from image descriptor to ImageDetails

			printf(", date: %u, angles = %s\n", currentImageDetails.date, currentImageDetails.angles);
		}
		else	// If we got here and didn't find space at the database it is probably full:
		{
			database->nextId -= numOfFramesTaken;
			database->numberOfPictures -= numOfFramesTaken;

			if(Auto)
			{
				database->nextAutoId = starting_nextAutoId;
			}
			else
			{
				database->nextGroundId -= numOfFramesTaken;
			}

			return DataBaseFull;
		}
	}

	updateGeneralParameters(database);

	printf("\nSo guys we did it! we reached a quarter of a million subscribers!\n");

	updateGeneralParameters(database);

	return DataBaseSuccess;
}

DataBaseResult takePicture_withSpecialParameters(DataBase database, unsigned int frameRate, unsigned char adcGain, unsigned char pgaGain, unsigned int exposure, unsigned int frameAmount, Boolean testPattern, Boolean Auto)
{
	CameraParameters regularParameters;
	regularParameters.adcGain = database->cameraParameters.adcGain;
	regularParameters.pgaGain = database->cameraParameters.pgaGain;
	regularParameters.exposure = database->cameraParameters.exposure;
	regularParameters.frameRate = database->cameraParameters.frameRate;

	DataBaseResult DB_result = takePicture(database, frameAmount, testPattern, Auto);

	updateCameraParameters(database, regularParameters.frameRate, regularParameters.adcGain, regularParameters.pgaGain, regularParameters.exposure, frameAmount, Auto);

	return DB_result;
}

char* GetImageFileName(DataBase database, imageid cameraId, fileType fileType)
{
	printf("\n----------GetImageFile----------\n");

	unsigned int currentPosition = getDatabaseStart(database);

	ImageDetails currentImageDetails; // will contain our current ID while going through the ImageDescriptor

	while(currentPosition < getDatabaseEnd(database)) // as long as we are not in the end of the file ,we will continue
	{
		FRAM_readAndProgress( (unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

		if (currentImageDetails.cameraId == cameraId) // check if the current id is the requested id
		{
			if (currentImageDetails.fileTypes[fileType])
			{
				char* fileName = malloc(FILENAMESIZE);
				getFileName(cameraId, fileType, fileName);
				return fileName;
			}
			else
				return NULL;
		}
	}
	return NULL;
}

F_FILE* getDataBaseFile(DataBase database, unsigned int start, unsigned int end)
{
	printf("\n----------getDataBaseFile----------\n");

	F_FILE* DataBaseFile = f_open("DataBaseFile", "w+"); // opening file
	int File_result = 0;

	database = initDataBase(FALSE);

	f_seek(DataBaseFile, 0, SEEK_SET);		// Making sure we are at the start of the file

	//writing general information on database:
	File_result += f_write(&database->maxNumberOfGroundPictures, sizeof(unsigned int), 1, DataBaseFile);
	File_result += f_write(&database->maxNumberOfAutoPictures, sizeof(unsigned int), 1, DataBaseFile);

	File_result += f_write(&database->numberOfPictures, sizeof(unsigned int), 1, DataBaseFile);

	File_result += f_write(&database->nextAutoIterationId, sizeof(unsigned int), 1, DataBaseFile);
	File_result += f_write(&database->autoFrequency, sizeof(unsigned int), 1, DataBaseFile);
	File_result += f_write(&database->autoFrameAmount, sizeof(unsigned int), 1, DataBaseFile);

	File_result += f_write(&database->nextId, sizeof(unsigned int), 1, DataBaseFile);
	File_result += f_write(&database->nextGroundId, sizeof(unsigned int), 1, DataBaseFile);
	File_result += f_write(&database->nextAutoId, sizeof(unsigned int), 1, DataBaseFile);

	File_result += f_write(&database->cameraParameters.frameRate, sizeof(unsigned int), 1, DataBaseFile);
	File_result += f_write(&database->cameraParameters.adcGain, sizeof(uint8_t), 1, DataBaseFile);
	File_result += f_write(&database->cameraParameters.pgaGain, sizeof(uint8_t), 1, DataBaseFile);
	File_result += f_write(&database->cameraParameters.exposure, sizeof(unsigned int), 1, DataBaseFile);

	File_result += f_write(&database->autoCameraParameters.frameRate, sizeof(unsigned int), 1, DataBaseFile);
	File_result += f_write(&database->autoCameraParameters.adcGain, sizeof(uint8_t), 1, DataBaseFile);
	File_result += f_write(&database->autoCameraParameters.pgaGain, sizeof(uint8_t), 1, DataBaseFile);
	File_result += f_write(&database->autoCameraParameters.exposure, sizeof(unsigned int), 1, DataBaseFile);

	if (File_result != 17) {
		printf("getDatabase - general (only %u)\n", File_result);
		return NULL;
	}

	// printf("maxNumberOfGroundPictures = %u, maxNumberOfAutoPictures = %u, numberOfPictures = %u, nextAutoIterationId = %u, autoFrequency = %u, autoFrameAmount = %u, nextId = %u,  nextGroundId = %u, nextAutoId = %u; CameraParameters: frameRate = %u, adcGain = %u, pgaGain = %u, exposure = %u; AutoCameraParameters: frameRate = %u, adcGain = %u, pgaGain = %u, exposure = %u\n", database->maxNumberOfGroundPictures, database->maxNumberOfAutoPictures, database->numberOfPictures, database->nextAutoIterationId, database->autoFrequency, database->autoFrameAmount, database->nextId, database->nextGroundId, database->nextAutoId, database->cameraParameters.frameRate, database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.exposure, database->autoCameraParameters.frameRate, database->autoCameraParameters.adcGain, database->autoCameraParameters.pgaGain, database->autoCameraParameters.exposure);

	ImageDetails imageDetails[database->maxNumberOfGroundPictures + database->maxNumberOfAutoPictures];

	Boolean inOBC = FALSE;

	File_result = 0;

	unsigned int currentPosition = getDatabaseStart(database);

	for (unsigned int i = 0; i < database->maxNumberOfGroundPictures + database->maxNumberOfAutoPictures; i++)
	{
		FRAM_readAndProgress((unsigned char*)&imageDetails[i], &currentPosition, sizeof(ImageDetails));
		inOBC = FALSE;

		if (imageDetails[i].cameraId != 0 && (imageDetails[i].date >= start && imageDetails[i].date <= end)) // check if the current id is the requested id
		{
			// we will read from the database and then - write to file id, true(boolean) if on iOBC SD else false, score, type, date:
			File_result = f_write(&imageDetails[i].cameraId, sizeof(imageid), 1, DataBaseFile);
			File_result += f_write(&imageDetails[i].secondaryId, sizeof(imageid), 1, DataBaseFile);

			File_result += f_write(&imageDetails[i].firstPictureInSequance, sizeof(unsigned int), 1, DataBaseFile);
			File_result += f_write(&imageDetails[i].numberOfPictureInSequance, sizeof(unsigned int), 1, DataBaseFile);

			File_result += f_write(&imageDetails[i].autoIterationId, sizeof(unsigned int), 1, DataBaseFile);

			File_result += f_write(&imageDetails[i].date, sizeof(unsigned int), 1, DataBaseFile);

			printf("cameraId: %d, secondaryId: %u, first in sequence: %u, number of pictures in sequence: %u, auto iteration id: %u, date: %u, inOBC:", imageDetails[i].cameraId, imageDetails[i].secondaryId, imageDetails[i].firstPictureInSequance, imageDetails[i].numberOfPictureInSequance, imageDetails[i].autoIterationId, imageDetails[i].date);

			for (int j = 0; j < NumberOfFileTypes; j++)
			{
				if (imageDetails[i].fileTypes[j])
					inOBC = TRUE;
				else
					inOBC = FALSE;

				printf(" %u", inOBC);

				File_result += f_write(&inOBC, sizeof(Boolean), 1, DataBaseFile);
			}

			File_result += f_write(&imageDetails[i].angles, sizeof(imageDetails[i].angles), 1, DataBaseFile);

			printf(", angles = %s\n", imageDetails[i].angles);

			if (File_result != (7 + NumberOfFileTypes)) {
				printf("DB ERROR getDatabase - FRAM read or file write");
				return NULL;
			}
		}
	}

	return DataBaseFile;
}

static uint8_t GetBinnedPixel(uint8_t binPixelSize, int x, int y)
{
	uint16_t value = 0;
	int i,j;
	for(j = 0; j < binPixelSize; j++)
	{
		for(i = 0; i < binPixelSize; i++)
		{
			value += imageBuffer[y+j*2][x+i*2]; // need to jump 2 because of bayer pattern
		}
	}
	return (uint8_t)(value/(binPixelSize*binPixelSize));

	return 0;
}

DataBaseResult BinImage(DataBase database, imageid id, unsigned int reductionLevel)	// where 2^(bin level) is the size reduction of the image
{
	unsigned int currentPosition = getDatabaseStart(database);
	ImageDetails currentImageDetails; // will contain our current ID while going through the ImageDescriptor

	while(currentPosition < getDatabaseEnd(database)) // as long as we are not in the end of the file ,we will continue
	{
		FRAM_readAndProgress((unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

		if (currentImageDetails.cameraId == id) // check if the current id is the requested id
		{
			if (currentImageDetails.fileTypes[raw + reductionLevel])
				return DataBasealreadyInSD;
			else
			{
				currentImageDetails.fileTypes[raw + reductionLevel] = TRUE;

				currentPosition -= sizeof(ImageDetails);
				FRAM_writeAndProgress((unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails));

				break;
			}
		}
	}

	char fileName[FILENAMESIZE];
	getFileName(id, raw, fileName);

	F_FILE *file = f_open(fileName, "r" ); // open file for writing in safe mode
	if(file == NULL)
	{
		return DataBaseFail;
	}

	unsigned int br = f_read(imageBuffer, 1, sizeof(imageBuffer), file);
	if( sizeof(imageBuffer) != br )
	{
		TRACE_ERROR("f_read pb: %d\n\r", f_getlasterror() ); // if bytes to write doesn't equal bytes written, get the error
	}

	int err1 = f_close( file ); // data is also considered safe when file is closed
	if( err1 )
	{
		TRACE_ERROR("f_close pb: %d\n\r", err1);
	}

	// bin data:
	uint8_t binPixelSize = (1 << reductionLevel);
	uint8_t (*binBuffer)[IMAGE_HEIGHT / binPixelSize][IMAGE_WIDTH / binPixelSize] = (uint8_t(*)[][IMAGE_WIDTH/binPixelSize])imageBuffer;

	for(unsigned int y = 0; y < IMAGE_HEIGHT / binPixelSize; y += 2)
	{
		for(unsigned int x = 0; x < IMAGE_WIDTH / binPixelSize; x += 2)
		{
			(*binBuffer)[y][x] = GetBinnedPixel(binPixelSize,x*binPixelSize,y*binPixelSize);         // Red pixel
			(*binBuffer)[y][x+1] = GetBinnedPixel(binPixelSize,x*binPixelSize+1,y*binPixelSize);     // Green pixel
			(*binBuffer)[y+1][x] = GetBinnedPixel(binPixelSize,x*binPixelSize,y*binPixelSize+1);     // Green pixel
			(*binBuffer)[y+1][x+1] = GetBinnedPixel(binPixelSize,x*binPixelSize+1,y*binPixelSize+1); // Blue
		}
	}

	// save data
	getFileName(id, raw + reductionLevel, fileName);

	file = f_open( fileName, "w+" ); /* open file for writing in safe mode */
	if( !file )
	{
		return DataBaseFail;
	}

	unsigned int bw = f_write(imageBuffer, 1,sizeof(*binBuffer), file );
	if(sizeof(*binBuffer) != bw )
	{
		return DataBaseFail;
	}

	f_flush( file ); /* only after flushing can data be considered safe */

	int err = f_close( file ); /* data is also considered safe when file is closed */
	if( err )
	{
		return DataBaseFail;
	}

	updateGeneralParameters(database);

	return DataBaseSuccess;
}

DataBaseResult compressImage(DataBase database, imageid id, unsigned int imageFactor)
{
	unsigned int currentPosition = getDatabaseStart(database);
	ImageDetails currentImageDetails; // will contain our current ID while going through the ImageDescriptor

	while(currentPosition < getDatabaseEnd(database)) // as long as we are not in the end of the file ,we will continue
	{
		FRAM_readAndProgress( (unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

		if (currentImageDetails.cameraId == id) // check if the current id is the requested id
		{
			if (currentImageDetails.fileTypes[jpg])
			{
				return DataBasealreadyInSD;
			}
			else
			{
				currentImageDetails.fileTypes[jpg] = TRUE;

				currentPosition -= sizeof(ImageDetails);
				FRAM_writeAndProgress((unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails));

				break;
			}
		}
	}

	fileType fileType = raw + imageFactor - 1;

	char inputFile[FILENAMESIZE];
	getFileName(id, fileType, inputFile);

	char midFile[FILENAMESIZE];
	getFileName(id, bmp, midFile);

	char outputFile[FILENAMESIZE];
	getFileName(id, jpg, outputFile);

	CompressImg(inputFile, midFile, outputFile, imageFactor);

	return DataBaseSuccess;
}

DataBaseResult newAutoImageing(DataBase database)
{
	database->nextAutoIterationId++;

	updateGeneralParameters(database);

	return DataBaseSuccess;
}

DataBaseResult updateMaxNumberOfGroundPictures(DataBase database, unsigned int maxNumberOfGroundPictures)
{
	database->maxNumberOfGroundPictures = maxNumberOfGroundPictures;

	updateGeneralParameters(database);

	return DataBaseSuccess;
}
DataBaseResult updateMaxNumberOfAutoPictures(DataBase database, unsigned int maxNumberOfAutoPictures)
{
	database->maxNumberOfAutoPictures = maxNumberOfAutoPictures;

	updateGeneralParameters(database);

	return DataBaseSuccess;
}

// ----------HIGHER LEVEL FUNCTIONS----------

DataBaseResult DeleteImage_bySecondaryID(DataBase database, imageid secondaryId, fileType fileType, Boolean Auto)
{
	/*
	raw, // raw file for picture on OBC SD
	t02, // thumbnail, factor 2
	t04, // thumbnail, factor 4
	t08, // thumbnail, factor 8
	jpg,
	Gecko SD // will delete the image from the database gecko nd obc
	*/

	imageid cameraId = getCameraId_bySecondaryId(database, secondaryId, Auto);

	DataBaseResult DB_result = DataBaseSuccess;

	if (fileType == bmp) // Gecko.... just look at "הקסם TLM&TLC" and u wil understand how stupid it is abd why I did it this way.
	{
		DB_result = DeleteImageFromPayload(database, cameraId, Auto);
	}
	else
	{
		DB_result = DeleteImageFromOBC(database, cameraId, fileType, Auto);
	}

	return DB_result;
}
DataBaseResult DeleteImage_byCameraID(DataBase database, imageid cameraId, fileType fileType, Boolean Auto)
{
	/*
	raw, // raw file for picture on OBC SD
	t02, // thumbnail, factor 2
	t04, // thumbnail, factor 4
	t08, // thumbnail, factor 8
	jpg,
	Gecko SD // will delete the image from the database gecko nd obc
	*/

	DataBaseResult DB_result = DataBaseSuccess;

	if (fileType == bmp) // Gecko.... just look at "הקסם TLM&TLC" and u wil understand how stupid it is abd why I did it this way.
	{
		DB_result = DeleteImageFromPayload(database, cameraId, Auto);
	}
	else
	{
		DB_result = DeleteImageFromOBC(database, cameraId, fileType, Auto);
	}

	return DB_result;
}

DataBaseResult TransferImage_bySecondaryID(DataBase database, imageid secondaryId, Boolean mode, Boolean Auto)
{
	/*
	raw, // raw file for picture on OBC SD
	t02, // thumbnail, factor 2
	t04, // thumbnail, factor 4
	t08, // thumbnail, factor 8
	jpg,
	Gecko SD // will delete the image from the database gecko nd obc
	*/

	imageid cameraId = getCameraId_bySecondaryId(database, secondaryId, Auto);

	DataBaseResult DB_result = transferImageToSD(database, cameraId, mode, Auto);

	return DB_result;
}
