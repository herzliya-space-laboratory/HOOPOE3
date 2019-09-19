/*!
 * @file ImageDataBase.h
 * @date 8 May 2019
 * @author Roy Harel
 */

#ifndef DATABASE_H_
#define DATABASE_H_

#include <hal/Boolean.h>
#include <hcc/api_fat.h>

#include "../Drivers/GeckoCameraDriver.h"
#include "../../Global/FRAMadress.h"
#include "../../Global/Global.h"
#include "../Compression/jpeg/bmp/rawToBMP.h"

#define DELAY 100
#define CAMERA_DELAY 1000

#define FILE_NAME_SIZE 11	///< The size of the filename parameter. i00001.raw (+1 for '\0')

#define MAX_NUMBER_OF_PICTURES 1000	///< The maximum number of pictures the database supports

#define DEFAULT_FRAME_RATE 1	///< The frame rate that will be used when taking a picture if the value wasn't updated with an command from the ground
#define DEFAULT_FRAME_AMOUNT 1	///< The amount of frames that will be taken when taking a picture if the value wasn't updated with an command from the ground
#define DEFAULT_ADC_GAIN 53		///< The ADC gain register value that will be used when taking a picture if the value wasn't updated with an command from the ground
#define DEFAULT_PGA_GAIN 1		///< The PGA gain register value that will be used when taking a picture if the value wasn't updated with an command from the ground
/*
 * Default Sensor Offset:
 * All info taken from datasheet v1.4
 * Contained in register 0x0D in bits 16 to 31,
 * during tests with ISIS's function for taking pictures we checked the registers' values, 0x0D value was 0x3FC30335
 * hence the number 0x3FC3
 */
#define DEFAULT_SENSOR_OFFSET 0x3FC3
#define DEFAULT_EXPOSURE 38	///< The exposure register value that will be used when taking a picture if the value wasn't updated with an command from the ground

#define DEFAULT_REDUCTION_LEVEL 4	///< After taking an image an thumbnail of that level will be created automatically

// Macros:
#define DB_RETURN_ERROR(error)			\
		if (error != DataBaseSuccess)	\
		{								\
			return error;				\
		}

typedef unsigned short imageid;
typedef struct ImageDataBase_t* ImageDataBase;	///< ADT architecture

typedef struct __attribute__ ((__packed__))
{
	// size = 24 bytes
	imageid cameraId;
	time_unix timestamp;
	byte fileTypes;
	uint16_t angle_rates[3];
	byte raw_css[10];
	Boolean8bit markedFor_TumbnailCreation;
} ImageMetadata;

typedef enum
{
	raw,				// the raw file of the picture (reduction level 0), 1088x2048
	t02,				// thumbnail, factor 02 (reduction level 1), 544x1024
	t04,				// thumbnail, factor 04 (reduction level 2), 272x512
	t08,				// thumbnail, factor 08 (reduction level 3), 136x256
	t16,				// thumbnail, factor 16 (reduction level 4), 68x128
	t32,				// thumbnail, factor 32 (reduction level 5), 34x64
	t64,				// thumbnail, factor 64 (reduction level 6), 17x32
	jpg,				// a compressed picture
	NumberOfFileTypes,
	bmp					// bitmap for jpg, unavailable - deleted and created for jpg
} fileType;

typedef enum
{
	// General DataBase Error Messages:
	DataBaseSuccess								= 0,
	DataBaseNullPointer							= 1,
	DataBaseNotInSD								= 2,
	DataBaseRawDoesNotExist						= 3,
	DataBasealreadyInSD							= 4,
	DataBaseIllegalId							= 5,
	DataBaseIdNotFound							= 6,
	DataBaseFull								= 7,
	DataBaseJpegFail							= 8,
	DataBaseAlreadyMarked						= 9,
	DataBaseTimeError							= 10,
	DataBaseFramFail							= 11,
	DataBaseFileSystemError						= 12,
	DataBaseFail								= 13,

	DataBaseAdcsError_gettingAngleRates			= 14,
	DataBaseAdcsError_gettingCssVector			= 15,

	// Gecko Drivers Image Taking Error Messages:
	GECKO_Take_Success							= 16,	///< (0) completed successfully
	GECKO_Take_Error_TurnOffSensor				= 17,	///< (-1) could not turn off sensor
	GECKO_Take_Error_Set_ADC_Gain				= 18,	///< (-2) could not set ADC gain
	GECKO_Take_Error_Set_PGA_Gain				= 19,	///< (-3) could not set PGA gain
	GECKO_Take_Error_setExposure				= 20,	///< (-4) could not set exposure
	GECKO_Take_Error_setFrameAmount				= 21,	///< (-5) could not set frame amount
	GECKO_Take_Error_setFrameRate				= 22,	///< (-6) could not set frame rate
	GECKO_Take_Error_turnOnSensor				= 23,	///< (-7) could not turn on sensor
	GECKO_Take_sensorTurnOnTimeout				= 24,	///< (-8) sensor turn on timeout
	GECKO_Take_trainingTimeout					= 25, 	///< (-9) training timeout
	GECKO_Take_trainingError					= 26,	///< (-10) training error
	GECKO_Take_Error_notInitialiseFlash			= 27,	///< (-11) could not initialize flash
	GECKO_Take_Error_setImageID					= 28,	///< (-12) could not set image ID
	GECKO_Take_Error_disableTestPattern			= 29,	///< (-13) could not disable test pattern
	GECKO_Take_Error_startSampling				= 30,	///< (-14) could not start sampling
	GECKO_Take_samplingTimeout					= 31,	///< (-15) sampling timeout
	GECKO_Take_Error_clearSampleFlag			= 32,	///< (-16) could not clear sample flag
	GECKO_Take_Error_turnOfSensor				= 33,	///< (-17) could not turn of sensor

	// Gecko Drivers Image Reading Error Messages:
	GECKO_Read_Success							= 34,	///< (0) completed successfully
	GECKO_Read_Error_InitialiseFlash			= 35,	///< (-1) could not initialize flash
	GECKO_Read_Error_SetImageID					= 36,	///< (-2) could not set image ID
	GECKO_Read_Error_StartReadout				= 37,	///< (-3) could not start readout
	GECKO_Read_readTimeout						= 38,	///< (-4) data read timeout
	GECKO_Read_wordCountMismatch				= 39,	///< (-5) word count mismatch during read
	GECKO_Read_pageCountMismatch				= 40,	///< (-6) page count mismatch during read
	GECKO_Read_readDoneFlagNotSet				= 41,	///< (-7) read done flag not set
	GECKO_Read_Error_ClearReadDoneFlag			= 42,	///< (-8) could not clear read done flag

	// Gecko Drivers Image Erasing Error Messages:
	GECKO_Erase_Success							= 43,	///< (0) completed successfully
	GECKO_Erase_Error_SetImageID				= 44,	///< (-1) could not set image ID
	GECKO_Erase_StartErase						= 45,	///< (-2) could not start erase
	GECKO_Erase_Timeout							= 46,	///< (-3) erase timeout
	GECKO_Erase_Error_ClearEraseDoneFlag		= 47,	///< (-4) could not clear erase done flag

	// Butcher Error Messages:
	Butcher_Success								= 48,
	Butcher_Null_Pointer						= 49,
	Butcher_Parameter_Value						= 50,
	Butcher_Out_of_Bounds						= 51,
	Butcher_Undefined_Error						= 52,

	// JPEG Error Messages:
	JpegCompression_Success						= 53,
	JpegCompression_Failure						= 54,
	JpegCompression_qualityFactor_outOfRange	= 55
} ImageDataBaseResult;

uint8_t imageBuffer[BMP_FILE_DATA_SIZE];

uint32_t getDataBaseStart();
uint32_t getDataBaseEnd();

ImageDataBaseResult updateGeneralDataBaseParameters(ImageDataBase database);

/*!
 * init image data base
 * allocating the memory for the database's variable containing its address, its array_vector, the number of pictures on the satelite and the next Id we will use for a picture
 * @return data base handler,
 * NULL on fail.
 */
ImageDataBase initImageDataBase(Boolean first_activation);
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
ImageDataBaseResult takePicture_withSpecialParameters(ImageDataBase database, uint32_t frameAmount, uint32_t frameRate, uint8_t adcGain, uint8_t pgaGain, uint16_t sensorOffset, uint32_t exposure, Boolean8bit testPattern);
/*
 * updateCameraParameters
 */
void setCameraPhotographyValues(ImageDataBase database, uint32_t frameAmount, uint32_t frameRate, uint8_t adcGain, uint8_t pgaGain, uint16_t sensorOffset, uint32_t exposure);

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
ImageDataBaseResult getImageDataBaseBuffer(imageid start, imageid end, byte buffer[], uint32_t* size);

ImageDataBaseResult readImageFromBuffer(imageid id, fileType image_type);
ImageDataBaseResult saveImageToBuffer(imageid id, fileType image_type);
ImageDataBaseResult SearchDataBase_byID(imageid id, ImageMetadata* image_metadata, uint32_t* image_address, uint32_t database_current_address);
void updateFileTypes(ImageMetadata* image_metadata, uint32_t image_address, fileType reductionLevel, Boolean value);
uint32_t GetImageFactor(fileType image_type);
uint32_t getDataBaseSize();
void setAutoThumbnailCreation(ImageDataBase database, Boolean8bit new_AutoThumbnailCreation);

#endif /* ImageDataBase_H_ */
