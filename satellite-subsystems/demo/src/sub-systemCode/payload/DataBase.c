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
#include "Boolean_bit.h"
#include "FRAM_Extended.h"
#include "DataBase.h"

typedef struct
{
	// size = 66 bytes
	imageid cameraId;
	unsigned int firstPictureInSequance;
	unsigned int timestamp;

	char fileTypes;

	unsigned char angles[6];

	Boolean8bit markedFor_4thTumbnailCreation;
} ImageDetails;

typedef struct
{
	unsigned int frameAmount;
	unsigned int frameRate;
	uint8_t adcGain;
	uint8_t pgaGain;
	unsigned int exposure;
} CameraParameters;

struct DataBase_t
{
	// size = 28 bytes
	unsigned int maxNumberOfPictures;
	unsigned int numberOfPictures;		// current number of pictures saved on the satellite
	unsigned int nextId;				// the next id we will use for a picture, (camera id)
	CameraParameters cameraParameters;
};

static uint8_t imageBuffer[IMAGE_HEIGHT_][IMAGE_WIDTH_];

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

	char baseString[FILENAMESIZE] = "i0000000.raw";
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

/*
 * Will return the DataBases ending address in the FRAM
*/
static unsigned int getDatabaseEnd(DataBase database)
{
	return getDatabaseStart(database) + ((database->maxNumberOfPictures) * sizeof(ImageDetails));
}

/*
 * Will restart the database (only FRAM), deleting all of its contents
*/
DataBaseResult zeroDataBase(DataBase database)
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
	int FRAM_result1 = FRAM_writeAndProgress((unsigned char*)(&database->maxNumberOfPictures),  &currentPosition, sizeof(unsigned int));
	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->numberOfPictures),  &currentPosition, sizeof(unsigned int));
	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->nextId),  &currentPosition, sizeof(int));
	FRAM_result1 += FRAM_writeAndProgress((unsigned char*)(&database->cameraParameters),  &currentPosition, sizeof(CameraParameters));
	if(FRAM_result1 != 0)									// checking if the read from theFRAM succeeded
	{
		free(database);
		return DataBaseFramFail;
	}
	return DataBaseSuccess;
}

// ----------BASIC FUNCTIONS----------

imageid getLatestID(DataBase database)
{
	return database->nextId - 1;
}

unsigned int getNumberOfFrames(DataBase database)
{
	return database->cameraParameters.frameAmount;
}

DataBaseResult markPicture(DataBase database, imageid id)
{
	unsigned int currentPosition = getDatabaseStart(database);
	unsigned int endPosition = getDatabaseEnd(database);

	ImageDetails currentImageDetails; // will contain our current ID while going through the ImageDescriptor

	while(currentPosition < endPosition)  // as long as we are not in the end of the file ,we will continue
	{
		vTaskDelay(100);

		FRAM_readAndProgress( (unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

		if (currentImageDetails.cameraId == id) // check if the current id is the requested id
		{
			if (currentImageDetails.markedFor_4thTumbnailCreation)
				return DataBaseAlreadyMarked;

			currentImageDetails.markedFor_4thTumbnailCreation = TRUE_8BIT;

			currentPosition -= sizeof(ImageDetails);
			FRAM_writeAndProgress( (unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

			return DataBaseSuccess;
		}
	}

	return DataBaseIllegalId;
}

DataBaseResult handleMarkedPictures(DataBase database)
{
	unsigned int currentPosition = getDatabaseStart(database);
	unsigned int endPosition = getDatabaseEnd(database);

	ImageDetails currentImageDetails; // will contain our current ID while going through the ImageDescriptor

	DataBaseResult DB_result;

	while(currentPosition < endPosition)  // as long as we are not in the end of the file ,we will continue
	{
		vTaskDelay(100);

		FRAM_readAndProgress( (unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

		if (currentImageDetails.markedFor_4thTumbnailCreation && currentImageDetails.cameraId != 0)
		{
			DB_result = transferImageToSD(database, currentImageDetails.cameraId);
			if (DB_result != DataBaseSuccess && DB_result != DataBasealreadyInSD)
				return DB_result;

			vTaskDelay(1000);

			DB_result = BinImage(database, currentImageDetails.cameraId, 4);
			if (DB_result != DataBaseSuccess && DB_result != DataBasealreadyInSD)
				return DB_result;

			vTaskDelay(1000);

			DB_result = DeleteImageFromOBC(database, currentImageDetails.cameraId, raw);
			if (DB_result != DataBaseSuccess && DB_result != DataBasealreadyInSD)
				return DB_result;

			// making sure i wont lose the data written in the functions above to the FRAM:
			currentPosition -= sizeof(ImageDetails);
			FRAM_read( (unsigned char*)&currentImageDetails, (unsigned int)currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

			currentImageDetails.markedFor_4thTumbnailCreation = FALSE_8BIT;
			FRAM_writeAndProgress( (unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file
		}
	}
	return DataBaseSuccess;
}

DataBase initDataBase(Boolean8bit reset)
{
	DataBase database = malloc(sizeof(*database));	// allocate the memory for the database's variables

	if(database == NULL)
	{
		printf("\n *ERROR* - NULL POINTER\n");
		free(database);
		return NULL;
	}

	unsigned int currentPosition = DATABASEFRAMADDRESS;
	int FRAM_result1 = FRAM_readAndProgress((unsigned char*)(&database->maxNumberOfPictures),  &currentPosition, sizeof(unsigned int));
	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->numberOfPictures),  &currentPosition, sizeof(unsigned int));
	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->nextId),  &currentPosition, sizeof(int));
	FRAM_result1 += FRAM_readAndProgress((unsigned char*)(&database->cameraParameters),  &currentPosition, sizeof(CameraParameters));
	if(FRAM_result1 != 0)				// checking if the read from theFRAM succeeded
	{
		free(database);
		return NULL;
	}

	printf("maxNumberOfPictures = %u, numberOfPictures = %u, nextId = %u; CameraParameters: frameAmount = %u, frameRate = %u, adcGain = %u, pgaGain = %u, exposure = %u\n", database->maxNumberOfPictures, database->numberOfPictures, database->nextId, database->cameraParameters.frameAmount, database->cameraParameters.frameRate, database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.exposure);

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

		database->maxNumberOfPictures = MAXNUMBEROFPICTURES;
		database->numberOfPictures = 0;

		database->nextId = 1;
		updateCameraParameters(database, defaltFrameRate, defaltAdcGain, defaltPgaGain, defaltExposure, defaltFrameAmount);
		updateGeneralParameters(database);

		printf("\nRe: database->numberOfPictures = %u, database->nextId = %u\n", database->numberOfPictures, database->nextId);
	}

	return database;
}

DataBase resetDataBase(DataBase database)
{
	printf("\n----------resetDataBase----------\n");

	DataBaseResult DB_result = DataBaseSuccess;

	// Deleting all pictures saved on iOBC SD so we wont have doubles later
	for (int i = 0; i < NumberOfFileTypes; i++) {
		DB_result = DeleteImageFromOBC(database, 0, i);
		if(DB_result != DataBaseSuccess)
			return NULL;
	}

	// Reinitializing the database:
	free(database);

	return initDataBase(TRUE_8BIT);
}

void updateCameraParameters(DataBase database, unsigned int frameRate, unsigned char adcGain, unsigned char pgaGain, unsigned int exposure, unsigned int frameAmount)
{
	database->cameraParameters.frameAmount = frameAmount;
	database->cameraParameters.frameRate = frameRate;
	database->cameraParameters.adcGain = adcGain;
	database->cameraParameters.pgaGain = pgaGain;
	database->cameraParameters.exposure = exposure;

	updateGeneralParameters(database);
}

DataBaseResult transferImageToSD(DataBase database, imageid cameraId)
{
	printf("\n----------transferImageToSD----------\n");

	if(database == NULL)	// Checking for NULL pointer
	{
		return DataBaseNullPointer;
	}

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
	char fileName[FILENAMESIZE];
	getFileName(cameraId, raw, fileName);

	F_FILE *PictureFile = f_open(fileName, "w+" );	// open a new file for writing in safe mode
	if(PictureFile == NULL)
	{
		printf("transferImageToSD - f_open\n");	// if file pointer is NULL, get an error
		return DataBaseFail;
	}

	err = GomEpsResetWDT(0);
	printf("err DB eps: %d\n", err);


	unsigned int elementsWritten = f_write(imageBuffer, sizeof(uint8_t), IMAGE_SIZE_, PictureFile);

	if (elementsWritten != IMAGE_SIZE_)
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
	currentPosition = getDatabaseStart(database);
	endPosition = getDatabaseEnd(database);


	ImageDetails currentImageDetails;	// will contain our current ID while going through the ImageDescriptors at the database
	int FRAM_result;

	while(currentPosition < endPosition)	// as long as we are not in the end of the file ,we will continue
	{
		vTaskDelay(100);

		FRAM_result = FRAM_readAndProgress((unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails));	// reading the id from the ImageDescriptor file
		if (FRAM_result != 0)
		{
			printf("DB ERROR - FRAM fail\n");
			return DataBaseFramFail;
		}

		bit fileTypes[8];

		char2bits(currentImageDetails.fileTypes, fileTypes);

		printf("TransferImageToOBC - current id = %u, current position = %u\n", currentImageDetails.cameraId, (unsigned int)(currentPosition - sizeof(imageid)));

		if (currentImageDetails.cameraId == cameraId)	// check if the current id is the requested id
		{
			if (fileTypes[raw].value)
				return DataBasealreadyInSD;

			fileTypes[raw].value = TRUE_bit;

			currentPosition -= sizeof(ImageDetails);

			currentImageDetails.fileTypes = bits2char(fileTypes);

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

DataBaseResult DeleteImageFromOBC(DataBase database, imageid cameraId, fileType type)
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
	currentPosition = getDatabaseStart(database);
	endPosition = getDatabaseEnd(database);


    while (currentPosition < endPosition)	// as long as we are not in the end of the file ,we will continue
    {
    	vTaskDelay(100);

    	FRAM_readAndProgress((unsigned char*)&currentImageDetails, &currentPosition, sizeof(ImageDetails)); // coping data from image descriptor to ImageDetails

    	bit fileTypes[8];
    	char2bits(currentImageDetails.fileTypes, fileTypes);

    	printf("id = %u, file type = %u, file_name = %s\n", currentImageDetails.cameraId, type, fileName);

    	if (currentImageDetails.cameraId != 0)
    	{
    		if (cameraId == 0)
    		{
    			if (fileTypes[type].value)
    			{
    				int file_result = f_delete(fileName); // will delete the file
    				if(file_result != 0) // remove returns -1 if the deletion process failed
   					{
    					printf("DeleteImageFromOBC Fail - file deletion fail (%u)\n", file_result);
    					return DataBaseFail;
    				}
    			}
    		}
    		else if (currentImageDetails.cameraId == cameraId)	// if we dont skip on that id
    		{
    			if(!fileTypes[type].value)
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
    			fileTypes[type].value = FALSE_bit;

    			currentImageDetails.fileTypes = bits2char(fileTypes);

    			currentPosition -= sizeof(ImageDetails); // we jump back in the ImageDescriptor to the start of the filename (tight after id)
    			FRAM_writeAndProgress((unsigned char*)&currentImageDetails, &currentPosition, sizeof(ImageDetails)); // writing zeroes over the file name of the current ImageDescriptor
    		}

    		if (currentImageDetails.cameraId == cameraId)
    		{
    			printf("c id = %u, id = %u\n", currentImageDetails.cameraId, cameraId);
    			printf("id = %u, file type = %u, file_name = %u\ncurrentPosition = %u, DB end = %u\n", currentImageDetails.cameraId, type, fileTypes[type].value, currentPosition, getDatabaseEnd(database));
    			break;
    		}
    	}
    }

	updateGeneralParameters(database);

    return DataBaseSuccess;
}

DataBaseResult DeleteImageFromPayload(DataBase database, imageid id)
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
	currentPosition = getDatabaseStart(database);
	endPosition = getDatabaseEnd(database);

	ImageDetails currentImageDetails; // will contain our current ID while going through the ImageDescriptor

	Boolean Found = FALSE;

	while(currentPosition < endPosition)  // as long as we are not in the end of the file ,we will continue
	{
		vTaskDelay(100);

		FRAM_readAndProgress( (unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

    	bit fileTypes[8];
    	char2bits(currentImageDetails.fileTypes, fileTypes);

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
				if(fileTypes[i].value)
				{
					DataBaseResult DB_result = DeleteImageFromOBC(database, id, i);
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

DataBaseResult takePicture(DataBase database, Boolean8bit testPattern)
{
	printf("\n----------TAKE PICTURE----------\n");

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
				images[i].fileTypes[j].value = FALSE_bit;
			}
			for (unsigned int j = 0; j < sizeof(images[i].angles); j++) {
				*(images[i].angles + j) = 0;
			}
			images[i].autoIterationId = 0;
			images[i].cameraId = 0;
			images[i].timestamp = 0;
			images[i].firstPictureInSequance = 0;
			images[i].numberOfPictureInSequance = 0;
			images[i].secondaryId = 0;
		}

		searchDataBaseByTime(database, images, currentDate - database->autoFrequency, currentDate);
		if (images[0].cameraId != 0)
			return DataBaseShotPicNotLongAgo;
	}*/

	int err = 0;
	for (unsigned int i = 0; i < database->cameraParameters.frameAmount; i++)
	{
		err = GomEpsResetWDT(0);
		printf("err DB eps: %d\n", err);
		// Erasing previous image before taking one to clear this part of the SDs
		err = GECKO_EraseBlock(database->nextId + i);
		if (err)
		{
			printf("Error (%d) erasing image!\n",err);
			return (GECKO_Erase_Success - err);
		}
		vTaskDelay(500);
	}


	int WDerr = GomEpsResetWDT(0);
	printf("err DB eps: %d\n", WDerr);

	// Taking images:

	err = GECKO_TakeImage( database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.exposure, database->cameraParameters.frameAmount, database->cameraParameters.frameRate, database->nextId, testPattern);

	vTaskDelay(500);
	if(err)
	{
		printf("Error (%d) taking image!\n",err);
		return (GECKO_Take_Success - err);
	}


	// DataBase handling:

	err = 0;
	imageid firstPictureInSequance = 0;

	unsigned int numOfFramesTaken;
	for (numOfFramesTaken = 0; numOfFramesTaken < database->cameraParameters.frameAmount; numOfFramesTaken++)
	{
		vTaskDelay(100);

		printf("database->nextId = %u, database->numberOfPictures = %u\n", database->nextId, database->numberOfPictures);

		ImageDetails currentImageDetails; //id, filename, score, type, timestamp

		// Writing current time to the DataBase(in Unix):
		currentImageDetails.timestamp = currentDate + (database->cameraParameters.frameRate * numOfFramesTaken);

		//ImageDetails.timestamp += numOfFramesTaken*frameRate;	// handling the time stamp of multiple frames
		printf("ImageDetails.timestamp = %d\n", currentImageDetails.timestamp);

		// Finding space at the database(FRAM) for the image's ImageDetails:
		printf("\nFinding space at the database(FRAM) for the image's ImageDetails:\n");

		unsigned int currentPosition;
		currentPosition = getDatabaseStart(database);

		Boolean IsEmpty = FALSE;

		while(currentPosition < getDatabaseEnd(database)) // as long as we are not in the end of the file ,we will continue
		{
			int FRAM_result = FRAM_readAndProgress((unsigned char*)&currentImageDetails.cameraId, (unsigned int*)&currentPosition, (unsigned int)sizeof(imageid));
			if (FRAM_result != 0)
				return DataBaseFramFail;

			printf("\n currentPosition = %d, current id = %d\n", (unsigned int)(currentPosition - sizeof(imageid)), currentImageDetails.cameraId);

			if (currentImageDetails.cameraId == 0)
			{
				IsEmpty = TRUE;
				break;
			}
			else
			{
				currentPosition += sizeof(ImageDetails) - sizeof(imageid);
			}
		}

		if (IsEmpty == TRUE)
		{
			currentPosition -= sizeof(imageid);

			currentImageDetails.cameraId = database->nextId;

			if (numOfFramesTaken == 0)
				firstPictureInSequance = currentImageDetails.cameraId;

			currentImageDetails.firstPictureInSequance = firstPictureInSequance;

			// saving the angles to ImageDetails
			for (int h = 0; h < 6; h++) {
				*(currentImageDetails.angles + h) = *(angles + h);
			}

			if (numOfFramesTaken == 0)
				currentImageDetails.firstPictureInSequance = currentImageDetails.cameraId;

			currentImageDetails.markedFor_4thTumbnailCreation = FALSE_8BIT;

			database->nextId++;
			database->numberOfPictures++;

			printf("\nid: %d, first Picture In Sequance = %u, inOBC:", currentImageDetails.cameraId, currentImageDetails.firstPictureInSequance);

	    	bit fileTypes[8];

			for (int i = 0; i < NumberOfFileTypes; i++) {
				fileTypes[i].value = FALSE_bit;
				printf(" %u", fileTypes[i].value);
			}

			currentImageDetails.fileTypes = bits2char(fileTypes);

			FRAM_writeAndProgress((unsigned char*)&currentImageDetails, &currentPosition, sizeof(ImageDetails)); // writing data from image descriptor to ImageDetails

			printf(", timestamp: %u, angles = %s\n", currentImageDetails.timestamp, currentImageDetails.angles);
		}
		else	// If we got here and didn't find space at the database it is probably full:
		{
			database->nextId -= numOfFramesTaken;
			database->numberOfPictures -= numOfFramesTaken;

			updateGeneralParameters(database);

			return DataBaseFull;
		}
	}

	updateGeneralParameters(database);

	printf("\nSo guys we did it! we reached a quarter of a million subscribers!\n");

	return DataBaseSuccess;
}

DataBaseResult takePicture_withSpecialParameters(DataBase database, unsigned int frameRate, unsigned char adcGain, unsigned char pgaGain, unsigned int exposure, unsigned int frameAmount, Boolean8bit testPattern)
{
	CameraParameters regularParameters;
	regularParameters.adcGain = database->cameraParameters.adcGain;
	regularParameters.pgaGain = database->cameraParameters.pgaGain;
	regularParameters.exposure = database->cameraParameters.exposure;
	regularParameters.frameRate = database->cameraParameters.frameRate;
	regularParameters.frameAmount = database->cameraParameters.frameAmount;

	updateCameraParameters(database, frameRate, adcGain, pgaGain, exposure, frameAmount);

	DataBaseResult DB_result = takePicture(database, testPattern);

	updateCameraParameters(database, regularParameters.frameRate, regularParameters.adcGain, regularParameters.pgaGain, regularParameters.exposure, regularParameters.frameAmount);

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

    	bit fileTypes[8];
    	char2bits(currentImageDetails.fileTypes, fileTypes);

		if (currentImageDetails.cameraId == cameraId) // check if the current id is the requested id
		{
			if (fileTypes[fileType].value)
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

byte* GetDataBaseFile(DataBase database, unsigned int start, unsigned int end)
{
	byte* buffer = malloc(sizeof(int));
	if (buffer == NULL)
		return NULL;

	unsigned int size = 0; // will be the first value in the array and will indicate its length (including itself!)

	memcpy(buffer + 0, &size, sizeof(size));
	size += sizeof(size);

	memcpy(buffer + size, &database->maxNumberOfPictures, sizeof(database->maxNumberOfPictures));
	size += sizeof(database->maxNumberOfPictures);

	memcpy(buffer + size, &database->numberOfPictures, sizeof(database->maxNumberOfPictures));
	size+= sizeof(database->numberOfPictures);

	memcpy(buffer + size, &database->nextId, sizeof(database->nextId));
	size+= sizeof(database->nextId);

	printf("maxNumberOfPictures = %u, numberOfPictures = %u, nextId = %u, ", database->maxNumberOfPictures, database->numberOfPictures, database->nextId);

	memcpy(buffer + size, &database->cameraParameters.frameAmount, sizeof(database->cameraParameters.frameAmount));
	size+= sizeof(database->cameraParameters.frameAmount);
	memcpy(buffer + size, &database->cameraParameters.frameRate, sizeof(database->cameraParameters.frameRate));
	size+= sizeof(database->cameraParameters.frameRate);
	memcpy(buffer + size, &database->cameraParameters.adcGain, sizeof(database->cameraParameters.adcGain));
	size+= sizeof(database->cameraParameters.adcGain);
	memcpy(buffer + size, &database->cameraParameters.pgaGain, sizeof(database->cameraParameters.pgaGain));
	size+= sizeof(database->cameraParameters.pgaGain);
	memcpy(buffer + size, &database->cameraParameters.exposure, sizeof(database->cameraParameters.exposure));
	size+= sizeof(database->cameraParameters.exposure);

	printf("cameraParameters: frameAmount = %u, frameRate = %u, adcGain = %u, pgaGain = %u, exposure = %u\n", database->cameraParameters.frameAmount, database->cameraParameters.frameRate, database->cameraParameters.adcGain, database->cameraParameters.pgaGain, database->cameraParameters.exposure);

	// Write MetaData:

	ImageDetails imageDetails;

	// ADCS (for tests)
	short a = 0;
	short b = 0;
	short c = 0;

	unsigned int currentPosition = getDatabaseStart(database);

	while(currentPosition < getDatabaseEnd(database))
	{
		vTaskDelay(100);

		FRAM_readAndProgress((unsigned char*)&imageDetails, &currentPosition, sizeof(ImageDetails));

		if (imageDetails.cameraId != 0 && (imageDetails.timestamp >= start && imageDetails.timestamp <= end)) // check if the current id is the requested id
		{
			realloc(buffer, size + sizeof(ImageDetails));

			memcpy(buffer + size, &imageDetails.cameraId, sizeof(imageDetails.cameraId));
			size += sizeof(imageDetails.cameraId);

			memcpy(buffer + size, &imageDetails.firstPictureInSequance, sizeof(imageDetails.firstPictureInSequance));
			size += sizeof(imageDetails.firstPictureInSequance);

			memcpy(buffer + size, &imageDetails.timestamp, sizeof(imageDetails.timestamp));
			size += sizeof(imageDetails.timestamp);

			memcpy(buffer + size, &imageDetails.fileTypes, sizeof(imageDetails.fileTypes));
			size += sizeof(imageDetails.fileTypes);

			memcpy(buffer + size, &imageDetails.angles, sizeof(imageDetails.angles));
			size += sizeof(imageDetails.angles);

			memcpy(buffer + size, &imageDetails.markedFor_4thTumbnailCreation, sizeof(imageDetails.markedFor_4thTumbnailCreation));
			size += sizeof(imageDetails.markedFor_4thTumbnailCreation);

			// printing for tests:

			memcpy(&a, &imageDetails.angles + 0, sizeof(short));
			memcpy(&b, &imageDetails.angles + 2, sizeof(short));
			memcpy(&c, &imageDetails.angles + 4, sizeof(short));

			printf("cameraId: %d, first in sequence: %u, timestamp: %u, inOBC:", imageDetails.cameraId, imageDetails.firstPictureInSequance, imageDetails.timestamp);

			bit fileTypes[8];
			char2bits(imageDetails.fileTypes, fileTypes);

			printf(", files:");
			for (int j = 0; j < 8; j++) {
				printf(" %u", fileTypes[j].value);
			}
			printf(", ");

			printf(", angles: %u  %u  %u, markedFor_4thTumbnailCreation = %u\n", a, b, c, imageDetails.markedFor_4thTumbnailCreation);
		}
	}

	memcpy(buffer + 0, &size, sizeof(size));

	return buffer;
}

byte* GetImageMetaData_byID(DataBase database, imageid id)
{
	byte* buffer = malloc(sizeof(ImageDetails) + sizeof(int));
	if (buffer == NULL)
		return NULL;

	unsigned int size = 0;

	memcpy(buffer + 0, &size, sizeof(size));
	size += sizeof(size);

	ImageDetails imageDetails;

	// ADCS (for tests)
	short a = 0;
	short b = 0;
	short c = 0;

	unsigned int currentPosition = getDatabaseStart(database) + sizeof(*database);

	while(currentPosition < getDatabaseEnd(database))
	{
		vTaskDelay(100);

		FRAM_readAndProgress((unsigned char*)&imageDetails, &currentPosition, sizeof(ImageDetails));

		if (imageDetails.cameraId == id)
		{
			memcpy(buffer + size, &imageDetails.cameraId, sizeof(imageDetails.cameraId));
			size += sizeof(imageDetails.cameraId);

			memcpy(buffer + size, &imageDetails.firstPictureInSequance, sizeof(imageDetails.firstPictureInSequance));
			size += sizeof(imageDetails.firstPictureInSequance);

			memcpy(buffer + size, &imageDetails.timestamp, sizeof(imageDetails.timestamp));
			size += sizeof(imageDetails.timestamp);

			memcpy(buffer + size, &imageDetails.fileTypes, sizeof(imageDetails.fileTypes));
			size += sizeof(imageDetails.fileTypes);

			memcpy(buffer + size, &imageDetails.angles, sizeof(imageDetails.angles));
			size += sizeof(imageDetails.angles);

			memcpy(buffer + size, &imageDetails.markedFor_4thTumbnailCreation, sizeof(imageDetails.markedFor_4thTumbnailCreation));
			size += sizeof(imageDetails.markedFor_4thTumbnailCreation);

			memcpy(buffer + 0, &size, sizeof(size));

			// printing for tests:

			memcpy(&a, &imageDetails.angles + 0, sizeof(short));
			memcpy(&b, &imageDetails.angles + 2, sizeof(short));
			memcpy(&c, &imageDetails.angles + 4, sizeof(short));

			printf("cameraId: %d, first in sequence: %u, timestamp: %u, inOBC:", imageDetails.cameraId, imageDetails.firstPictureInSequance, imageDetails.timestamp);

	    	bit fileTypes[8];
	    	char2bits(imageDetails.fileTypes, fileTypes);

			printf(", files:");
			for (int j = 0; j < 8; j++) {
				printf(" %u", fileTypes[j].value);
			}
			printf(", ");

			printf(", angles: %u  %u  %u, markedFor_4thTumbnailCreation = %u\n", a, b, c, imageDetails.markedFor_4thTumbnailCreation);

			return buffer;
		}
	}

	return NULL;
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

DataBaseResult BinImage(DataBase database, imageid id, byte reductionLevel)	// where 2^(bin level) is the size reduction of the image
{
	unsigned int currentPosition = getDatabaseStart(database);
	ImageDetails currentImageDetails; // will contain our current ID while going through the ImageDescriptor

	while(currentPosition < getDatabaseEnd(database)) // as long as we are not in the end of the file ,we will continue
	{
		FRAM_readAndProgress((unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

    	bit fileTypes[8];
    	char2bits(currentImageDetails.fileTypes, fileTypes);

		if (currentImageDetails.cameraId == id) // check if the current id is the requested id
		{
			if (fileTypes[raw + reductionLevel].value)
				return DataBasealreadyInSD;
			else if (!fileTypes[raw ].value)
				return DataBaseNotInSD;
			else
			{
				fileTypes[raw + reductionLevel].value = TRUE_bit;

		    	currentImageDetails.fileTypes = bits2char(fileTypes);

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
	uint8_t (*binBuffer)[IMAGE_HEIGHT_ / binPixelSize][IMAGE_WIDTH_ / binPixelSize] = (uint8_t(*)[][IMAGE_WIDTH_/binPixelSize])imageBuffer;

	for(unsigned int y = 0; y < IMAGE_HEIGHT_ / binPixelSize; y += 2)
	{
		for(unsigned int x = 0; x < IMAGE_WIDTH_ / binPixelSize; x += 2)
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

	unsigned int bw = f_write(imageBuffer, 1, sizeof(*binBuffer), file );
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

DataBaseResult compressImage(DataBase database, imageid id)
{
	unsigned int currentPosition = getDatabaseStart(database);
	ImageDetails currentImageDetails; // will contain our current ID while going through the ImageDescriptor

	while(currentPosition < getDatabaseEnd(database)) // as long as we are not in the end of the file ,we will continue
	{
		FRAM_readAndProgress( (unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails)); // reading the id from the ImageDescriptor file

    	bit fileTypes[8];
    	char2bits(currentImageDetails.fileTypes, fileTypes);

		if (currentImageDetails.cameraId == id) // check if the current id is the requested id
		{
			if (fileTypes[jpg].value)
			{
				return DataBasealreadyInSD;
			}
			else
			{
				fileTypes[jpg].value = TRUE_bit;

				currentImageDetails.fileTypes = bits2char(fileTypes);

				currentPosition -= sizeof(ImageDetails);
				FRAM_writeAndProgress((unsigned char*)&currentImageDetails, (unsigned int*)&currentPosition, (unsigned int)sizeof(ImageDetails));

				break;
			}
		}
	}

	fileType fileType = raw;
	unsigned int imageFactor = 1;

	char inputFile[FILENAMESIZE];
	getFileName(id, fileType, inputFile);

	char midFile[FILENAMESIZE];
	getFileName(id, bmp, midFile);

	char outputFile[FILENAMESIZE];
	getFileName(id, jpg, outputFile);

	CompressImg(inputFile, midFile, outputFile, imageFactor);

	return DataBaseSuccess;
}

DataBaseResult updateMaxNumberOfPictures(DataBase database, unsigned int maxNumberOfPictures)
{
	if (maxNumberOfPictures < database->maxNumberOfPictures)
		return DataBase_SmallerThanTheCurrentMax;
	/*
	if ( (maxNumberOfPictures * sizeof(ImageDetails) + sizeof(*database)) > (FRAM_END - DATABASEFRAMADDRESS) )
		return DataBase_sizeBeyondFRAMBounderies;
	 */
	if (maxNumberOfPictures > 1000)
		return DataBase_sizeBeyondFRAMBounderies;

	database->maxNumberOfPictures = maxNumberOfPictures;
	updateGeneralParameters(database);
	return DataBaseSuccess;
}
