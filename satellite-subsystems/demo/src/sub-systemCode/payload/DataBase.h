/*
 * DataBase.h
 *
 *  Created on: 7 במאי 2019
 *      Author: I7COMPUTER
 */

#ifndef DATABASE_H_
#define DATABASE_H_

#include <hal/Boolean.h>
#include <hcc/api_fat.h>

#include "../Global/FRAMadress.h"
#include "../Global/Global.h"

#define FILE_NAME_SIZE 13			// The size of the filename parameter. i0000001.raw (+1 for '/0')

#define TESTING	// ToDo: remove MAX_NUMBER_OF_PICTURES testing mode.

#ifdef TESTING
	#define MAX_NUMBER_OF_PICTURES 20		// The maximum number of pictures the database supports
#else
	#define MAX_NUMBER_OF_PICTURES 1000		// The maximum number of pictures the database supports
#endif

#define DEFALT_FRAME_RATE 1
#define DEFALT_FRAME_AMOUNT 1
#define DEFALT_ADC_GAIN 53
#define DEFALT_PGA_GAIN 3
#define DEFALT_EXPOSURE 2048

#define DEFALT_REDUCTION_LEVEL 5

// ToDo: get rid of once all code is together!
//! the file name of the estimated angles
#define ESTIMATED_ANGLES_FILE ("ESTIMATED_ANGLES")

typedef enum
{
	raw,	// the raw file of the picture (reduction level 0), 1088x2048
	t02,	// thumbnail, factor 02 (reduction level 1), 544x1024
	t04,	// thumbnail, factor 04 (reduction level 2), 272x512
	t08,	// thumbnail, factor 08 (reduction level 3), 136x256
	t16,	// thumbnail, factor 16 (reduction level 4), 68x128
	t32,	// thumbnail, factor 32 (reduction level 5), 34x64
	t64,	// thumbnail, factor 64 (reduction level 6), 17x32
	jpg,	// a compressed picture
	NumberOfFileTypes,
	bmp,	// bitmap for jpg, unavailable - deleted and created for jpg
} fileType;

typedef unsigned int imageid;
typedef struct DataBase_t* DataBase;

typedef enum
{
	//------General returns------
	DataBaseSuccess,
	DataBaseNullPointer,
	DataBaseNotInSD,
	DataBaseTransferFailed,
	DataBasealreadyInSD,
	DataBaseGradeCalculationError,
	DataBaseIllegalId,
	DataBaseVectorNotExist,
	DataBaseFramFail,
	DataBaseFull,
	DataBaseFail,
	DataBaseTimeError,
	DataBaseShotPicNotLongAgo,
	DataBaseAlreadyMarked,
	DataBase_SmallerThanTheCurrentMax,
	DataBase_sizeBeyondFRAMBounderies,
	//------GECKO_UC_TakeImage returns------
	GECKO_Take_Success,						// (0) completed successfully
	GECKO_Take_Error_TurnOffSensor,			// (-1) could not turn off sensor
	GECKO_Take_Error_Set_ADC_Gain,			// (-2) could not set ADC gain
	GECKO_Take_Error_Set_PGA_Gain,			// (-3) could not set PGA gain
	GECKO_Take_Error_setExposure,			// (-4) could not set exposure
	GECKO_Take_Error_setFrameAmount,		// (-5) could not set frame amount
	GECKO_Take_Error_setFrameRate,			// (-6) could not set frame rate
	GECKO_Take_Error_turnOnSensor,			// (-7) could not turn on sensor
	GECKO_Take_sensorTurnOnTimeout,			// (-8) sensor turn on timeout
	GECKO_Take_trainingTimeout, 			// (-9) training timeout
	GECKO_Take_trainingError,				// (-10) training error
	GECKO_Take_Error_notInitialiseFlash,	// (-11) could not initialize flash
	GECKO_Take_Error_setImageID,			// (-12) could not set image ID
	GECKO_Take_Error_disableTestPattern,	// (-13) could not disable test pattern
	GECKO_Take_Error_startSampling,			// (-14) could not start sampling
	GECKO_Take_samplingTimeout,				// (-15) sampling timeout
	GECKO_Take_Error_clearSampleFlag,		// (-16) could not clear sample flag
	GECKO_Take_Error_turnOfSensor,			// (-17) could not turn of sensor
	//------GECKO_UC_ReadImage returns------
	GECKO_Read_Success,						// (0) completed successfully
	GECKO_Read_Error_InitialiseFlash,		// (-1) could not initialize flash
	GECKO_Read_Error_SetImageID,			// (-2) could not set image ID
	GECKO_Read_Error_StartReadout,			// (-3) could not start readout
	GECKO_Read_readTimeout,					// (-4) data read timeout
	GECKO_Read_wordCountMismatch,			// (-5) word count mismatch during read
	GECKO_Read_pageCountMismatch,			// (-6) page count mismatch during read
	GECKO_Read_readDoneFlagNotSet,			// (-7) read done flag not set
	GECKO_Read_Error_ClearReadDoneFlag,		// (-8) could not clear read done flag
	//------GECKO_UC_EraseImage returns------
	GECKO_Erase_Success,					// (0) completed successfully
	GECKO_Erase_Error_SetImageID,			// (-1) could not set image ID
	GECKO_Erase_StartErase,					// (-2) could not start erase
	GECKO_Erase_Timeout,					// (-3) erase timeout
	GECKO_Erase_Error_ClearEraseDoneFlag	// (-4) could not clear erase done flag
} DataBaseResult;

imageid getLatestID(DataBase database);
unsigned int getNumberOfFrames(DataBase database);

DataBaseResult markPicture(DataBase database, imageid id);
DataBaseResult handleMarkedPictures(DataBase database);

/*!
 * init image data base
 * allocating the memory for the database's variable containing its address, its array_vector, the number of pictures on the satelite and the next Id we will use for a picture
 * @return data base handler,
 * NULL on fail.
 */
DataBase initDataBase(Boolean8bit reset);
/*
 * Ressets the whole database: the FRAM, the handler and deletes all of the pictures saved on OBC sd
 * @param database the DataBase's handler
 * @param shouldDelete a pointer to a function that decides whether or not a picture should be deleted
 * @return
 */
DataBase resetDataBase(DataBase database);

void updateCameraParameters(DataBase database, uint32_t frameRate, uint8_t adcGain, uint8_t pgaGain, uint32_t exposure, uint32_t frameAmount);

/*!
 * transfer image from payload SD To OBC SD
 * @param id index of image.
 * @param mode - when the drivers arrive
 * @return DataBaseNotInSD if Image not in payload SD,
 * DataBaseTransferFailed fail to transfer image,
 * DataBasealreadyInSD if Image already in payload SD,
 * DataBaseIllegalId,
 * DataBaseFail,
 * DataBaseNullPointer if database = NULL.
 * 28 - 37: Camera errors (lines 64 - 73)
 * DataBaseSuccess on success.
 */
DataBaseResult transferImageToSD(DataBase database, imageid cameraId);

/*!
 * Delete images from OBC SD
 * @param database
 * @param id the requested images's id, also if 0 will mean all)
 * @param type file type, NumberOfFileTypes = all types
 * @return DataBaseNullPointer if database = NULL,
 * DataBaseFail,
 * DataBaseSuccess on success.
 */
DataBaseResult DeleteImageFromOBC(DataBase database, imageid id, fileType type);
/*!
 * Delete images from payload SD by index
 * @param id index of image.
 * @return DataBaseNullPointer if database = NULL,
 * DataBaseFail on unknown failure,
 * 38 - 43: Camera errors (lines 74 - 79)
 * DataBaseSuccess on success.
 */
DataBaseResult DeleteImageFromPayload(DataBase database,imageid id);

/*!
 * takes a picture and writes info about it
 * @param database were we will save the info about the image
 * @return DataBaseFail on unknown failure,
 * 12 - 30: Camera errors (lines 45 - 63)
 * DataBaseSuccess on success.
 */
DataBaseResult takePicture(DataBase database, Boolean8bit testPattern);

DataBaseResult takePicture_withSpecialParameters(DataBase database, uint32_t frameRate, uint8_t adcGain, uint8_t pgaGain, uint32_t exposure, uint32_t frameAmount, Boolean8bit testPattern);

/*!
 * get Image file descriptor
 * @param id index of image.
 * @return File descriptor
 * @note User have to close the file
 */
char* GetImageFileName(imageid cameraId, fileType fileType);

/*
 * will contain the data about the picture with the id given, at the start is the size if the array (the size is uint32)
 * @note User have to free the array
 */
byte* GetImageMetaData_byID(DataBase database, imageid id);
/*
 * GetImageMetaData but has the general data that the DB has before the metadata about the images, at the start is the size if the array (the size is uint32)
 * @note User have to free the array
*/
byte* getDataBaseBuffer(DataBase database, time_unix start, time_unix end);

DataBaseResult BinImage(DataBase database, imageid id, byte reductionLevel, Boolean Skipping);

/*!
 * get Image file descriptor
 * @param id index of image,
 * imageFactor 0 for raw
 * @note
 */
DataBaseResult compressImage(DataBase database, imageid id, unsigned int quality_factor, fileType reductionLevel);

/*
 * delete all images
 */
DataBaseResult clearDataBase(DataBase database);

imageid* get_ID_list_withDefaltThumbnail(DataBase database, time_unix startingTime, time_unix endTime);

#endif /* DataBase_H_ */
