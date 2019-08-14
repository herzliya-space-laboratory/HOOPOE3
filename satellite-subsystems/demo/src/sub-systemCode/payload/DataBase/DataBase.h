/*!
 * @file ImageDataBase.h
 * @brief ToDo: brief for database.h
 * @date 8 May 2019
 * ToDo: explain database.h
 *
 * @see ToDo DB capabilities file
 * @author Roy Harel
 */

#ifndef DATABASE_H_
#define DATABASE_H_

#include <hal/Boolean.h>
#include <hcc/api_fat.h>

#include "../Drivers/GeckoCameraDriver.h"
#include "../../Global/FRAMadress.h"
#include "../../Global/Global.h"

#define FILE_NAME_SIZE 12	///< The size of the filename parameter. i0000001.raw (+1 for '/0')

#ifdef TESTING
	#define MAX_NUMBER_OF_PICTURES 20
#else
	#define MAX_NUMBER_OF_PICTURES 1000	///< The maximum number of pictures the database supports
#endif

#define DEFALT_FRAME_RATE 1		///< The frame rate that will be used when taking a picture if the value wasn't updated with an command from the ground
#define DEFALT_FRAME_AMOUNT 1	///< The amount of frames that will be taken when taking a picture if the value wasn't updated with an command from the ground
#define DEFALT_ADC_GAIN 53		///< The ADC gain register value that will be used when taking a picture if the value wasn't updated with an command from the ground
#define DEFALT_PGA_GAIN 3		///< The PGA gain register value that will be used when taking a picture if the value wasn't updated with an command from the ground
#define DEFALT_EXPOSURE 2048	///< The exposure register value that will be used when taking a picture if the value wasn't updated with an command from the ground

#define DEFALT_REDUCTION_LEVEL 5	///< After taking an image an thumbnail of that level will be created automatically

// Macros:
// ToDo: Add error log
#define DB_RETURN_ERROR(error)			\
		if (error != DataBaseSuccess)	\
		{								\
			return error;				\
		}

typedef unsigned short imageid;
typedef struct ImageDataBase_t* ImageDataBase;	///< ADT architecture

typedef struct __attribute__ ((__packed__))
{
	// size = 66 bytes
	imageid cameraId;
	time_unix timestamp;
	char fileTypes;
	unsigned short angles[3];
	Boolean8bit markedFor_TumbnailCreation;
} ImageMetadata;

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
	GeckoSD
} fileType;

typedef enum
{
	// General DataBase Error Messages:
	DataBaseSuccess,
	DataBaseNullPointer,
	DataBaseNotInSD,
	DataBasealreadyInSD,
	DataBaseIllegalId,
	DataBaseIdNotFound,
	DataBaseFull,
	DataBaseJpegFail,
	DataBaseAlreadyMarked,
	DataBaseTimeError,
	DataBaseFramFail,
	DataBaseFileSystemError,
	DataBaseFail,

	// Gecko Drivers Image Taking Error Messages:
	GECKO_Take_Success,						///< (0) completed successfully
	GECKO_Take_Error_TurnOffSensor,			///< (-1) could not turn off sensor
	GECKO_Take_Error_Set_ADC_Gain,			///< (-2) could not set ADC gain
	GECKO_Take_Error_Set_PGA_Gain,			///< (-3) could not set PGA gain
	GECKO_Take_Error_setExposure,			///< (-4) could not set exposure
	GECKO_Take_Error_setFrameAmount,		///< (-5) could not set frame amount
	GECKO_Take_Error_setFrameRate,			///< (-6) could not set frame rate
	GECKO_Take_Error_turnOnSensor,			///< (-7) could not turn on sensor
	GECKO_Take_sensorTurnOnTimeout,			///< (-8) sensor turn on timeout
	GECKO_Take_trainingTimeout, 			///< (-9) training timeout
	GECKO_Take_trainingError,				///< (-10) training error
	GECKO_Take_Error_notInitialiseFlash,	///< (-11) could not initialize flash
	GECKO_Take_Error_setImageID,			///< (-12) could not set image ID
	GECKO_Take_Error_disableTestPattern,	///< (-13) could not disable test pattern
	GECKO_Take_Error_startSampling,			///< (-14) could not start sampling
	GECKO_Take_samplingTimeout,				///< (-15) sampling timeout
	GECKO_Take_Error_clearSampleFlag,		///< (-16) could not clear sample flag
	GECKO_Take_Error_turnOfSensor,			///< (-17) could not turn of sensor

	// Gecko Drivers Image Reading Error Messages:
	GECKO_Read_Success,						///< (0) completed successfully
	GECKO_Read_Error_InitialiseFlash,		///< (-1) could not initialize flash
	GECKO_Read_Error_SetImageID,			///< (-2) could not set image ID
	GECKO_Read_Error_StartReadout,			///< (-3) could not start readout
	GECKO_Read_readTimeout,					///< (-4) data read timeout
	GECKO_Read_wordCountMismatch,			///< (-5) word count mismatch during read
	GECKO_Read_pageCountMismatch,			///< (-6) page count mismatch during read
	GECKO_Read_readDoneFlagNotSet,			///< (-7) read done flag not set
	GECKO_Read_Error_ClearReadDoneFlag,		///< (-8) could not clear read done flag

	// Gecko Drivers Image Erasing Error Messages:
	GECKO_Erase_Success,					///< (0) completed successfully
	GECKO_Erase_Error_SetImageID,			///< (-1) could not set image ID
	GECKO_Erase_StartErase,					///< (-2) could not start erase
	GECKO_Erase_Timeout,					///< (-3) erase timeout
	GECKO_Erase_Error_ClearEraseDoneFlag,	///< (-4) could not clear erase done flag

	// Butcher Error Messages:
	Butcher_Success,
	Butcher_Null_Pointer,
	Butcher_Parameter_Value,
	Butcher_Undefined_Error
} ImageDataBaseResult;

uint8_t imageBuffer[IMAGE_HEIGHT][IMAGE_WIDTH];

uint32_t getDataBaseStart();
uint32_t getDataBaseEnd();

ImageDataBaseResult updateGeneralDataBaseParameters(ImageDataBase database);

/*!
 * init image data base
 * allocating the memory for the database's variable containing its address, its array_vector, the number of pictures on the satelite and the next Id we will use for a picture
 * @return data base handler,
 * NULL on fail.
 */
ImageDataBase initImageDataBase();
/*
 * Ressets the whole database: the FRAM, the handler and deletes all of the pictures saved on OBC sd
 * @param database the ImageDataBase's handler
 * @param shouldDelete a pointer to a function that decides whether or not a picture should be deleted
 * @return
 */
ImageDataBaseResult resetImageDataBase(ImageDataBase database);

imageid getLatestID(ImageDataBase database);
unsigned int getNumberOfFrames(ImageDataBase database);

ImageDataBaseResult handleMarkedPictures(uint32_t nuberOfPicturesToBeHandled);

/*!
 * takes a picture and writes info about it
 * @param database were we will save the info about the image
 * @return ImageDataBaseFail on unknown failure,
 * 12 - 30: Camera errors (lines 45 - 63)
 * ImageDataBaseSuccess on success.
 */
ImageDataBaseResult takePicture(ImageDataBase database, Boolean8bit testPattern);
/*
 * takePicture_withSpecialParameters
 */
ImageDataBaseResult takePicture_withSpecialParameters(ImageDataBase database, uint32_t frameRate, uint8_t adcGain, uint8_t pgaGain, uint32_t exposure, uint32_t frameAmount, Boolean8bit testPattern);
/*
 * updateCameraParameters
 */
void setCameraPhotographyValues(ImageDataBase database, uint32_t frameRate, uint8_t adcGain, uint8_t pgaGain, uint32_t exposure, uint32_t frameAmount);

/*!
 * transfer image from payload SD To OBC SD
 * @param id index of image.
 * @param mode - when the drivers arrive
 * @return ImageDataBaseNotInSD if Image not in payload SD,
 * ImageDataBaseTransferFailed fail to transfer image,
 * ImageDataBasealreadyInSD if Image already in payload SD,
 * ImageDataBaseIllegalId,
 * ImageDataBaseFail,
 * ImageDataBaseNullPointer if database = NULL.
 * 28 - 37: Camera errors (lines 64 - 73)
 * ImageDataBaseSuccess on success.
 */
ImageDataBaseResult transferImageToSD(ImageDataBase database, imageid cameraId);

/*!
 * Delete images from OBC SD
 * @param database
 * @param id the requested images's id, also if 0 will mean all)
 * @param type file type, NumberOfFileTypes = all types
 * @return ImageDataBaseNullPointer if database = NULL,
 * ImageDataBaseFail,
 * ImageDataBaseSuccess on success.
 */
ImageDataBaseResult DeleteImageFromOBC(ImageDataBase database, imageid id, fileType type);
/*!
 * Delete images from payload SD by index
 * @param id index of image.
 * @return ImageDataBaseNullPointer if database = NULL,
 * ImageDataBaseFail on unknown failure,
 * 38 - 43: Camera errors (lines 74 - 79)
 * ImageDataBaseSuccess on success.
 */
ImageDataBaseResult DeleteImageFromPayload(ImageDataBase database,imageid id);
/*
 * delete all images from OBC SD
 */
ImageDataBaseResult clearImageDataBase(void);

/*!
 * get Image file descriptor
 * @param id index of image.
 * @return File descriptor
 * @note User have to close the file
 */
ImageDataBaseResult GetImageFileName(imageid cameraId, fileType fileType, char fileName[FILE_NAME_SIZE]);

/*
 * GetImageMetaData but has the general data that the DB has before the metadata about the images, at the start is the size if the array (the size is uint32)
 * @note User have to free the array
*/
byte* getImageDataBaseBuffer(imageid start, imageid end);

imageid* get_ID_list_withDefaltThumbnail(imageid start, imageid end);

ImageDataBaseResult readImageToBuffer(imageid id, fileType image_type);
ImageDataBaseResult saveImageFromBuffer(imageid id, fileType image_type);
ImageDataBaseResult SearchDataBase_byID(imageid id, ImageMetadata* image_metadata, uint32_t* image_address, uint32_t database_current_address);
void updateFileTypes(ImageMetadata* image_metadata, uint32_t image_address, fileType reductionLevel, Boolean value);

#endif /* ImageDataBase_H_ */
