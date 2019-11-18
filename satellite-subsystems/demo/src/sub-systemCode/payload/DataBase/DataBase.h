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

#define FILE_NAME_SIZE							(8 + 1 + 3 + 7 + 3)	///< The size of the filename parameter. i00001.raw (+1 for '\0')
#define GENERAL_PAYLOAD_FOLDER_NAME				"PAYLOAD"
#define RAW_IMAGES_FOLDER_NAME					"RAW"
#define THUMBNAIL_LEVEL_1_IMAGES_FOLDER_NAME	"T02"
#define THUMBNAIL_LEVEL_2_IMAGES_FOLDER_NAME	"T04"
#define THUMBNAIL_LEVEL_3_IMAGES_FOLDER_NAME	"T08"
#define THUMBNAIL_LEVEL_4_IMAGES_FOLDER_NAME	"T16"
#define THUMBNAIL_LEVEL_5_IMAGES_FOLDER_NAME	"T32"
#define THUMBNAIL_LEVEL_6_IMAGES_FOLDER_NAME	"T64"
#define JPEG_IMAGES_FOLDER_NAME					"JPG"

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
	uint16_t estimated_angles[3];
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
	DataBaseSuccess									= 0,
	DataBaseNullPointer								= 1,
	DataBaseNotInSD									= 2,
	DataBaseRawDoesNotExist							= 3,
	DataBasealreadyInSD								= 4,
	DataBaseIllegalId								= 5,
	DataBaseIdNotFound								= 6,
	DataBaseFull									= 7,
	DataBaseJpegFail								= 8,
	DataBaseAlreadyMarked							= 9,
	DataBaseTimeError								= 10,
	DataBaseFramFail								= 11,
	DataBaseFileSystemError							= 12,
	DataBaseFail									= 13,

	DataBaseAdcsError_gettingAngleRates				= 14,
	DataBaseAdcsError_gettingEstimatedAngles		= 15,
	DataBaseAdcsError_gettingCssVector				= 16,

	// Gecko Drivers Image Taking Error Messages:
	GECKO_Take_Success								= 17,	///< (0) completed successfully
	GECKO_Take_Error_TurnOffSensor					= 18,	///< (-1) could not turn off sensor
	GECKO_Take_Error_Set_ADC_Gain					= 19,	///< (-2) could not set ADC gain
	GECKO_Take_Error_Set_PGA_Gain					= 20,	///< (-3) could not set PGA gain
	GECKO_Take_Error_setExposure					= 21,	///< (-4) could not set exposure
	GECKO_Take_Error_setFrameAmount					= 22,	///< (-5) could not set frame amount
	GECKO_Take_Error_setFrameRate					= 23,	///< (-6) could not set frame rate
	GECKO_Take_Error_turnOnSensor					= 24,	///< (-7) could not turn on sensor
	GECKO_Take_sensorTurnOnTimeout					= 25,	///< (-8) sensor turn on timeout
	GECKO_Take_trainingTimeout						= 26, 	///< (-9) training timeout
	GECKO_Take_trainingError						= 27,	///< (-10) training error
	GECKO_Take_Error_notInitialiseFlash				= 28,	///< (-11) could not initialize flash
	GECKO_Take_Error_setImageID						= 29,	///< (-12) could not set image ID
	GECKO_Take_Error_disableTestPattern				= 30,	///< (-13) could not disable test pattern
	GECKO_Take_Error_startSampling					= 31,	///< (-14) could not start sampling
	GECKO_Take_samplingTimeout						= 32,	///< (-15) sampling timeout
	GECKO_Take_Error_clearSampleFlag				= 33,	///< (-16) could not clear sample flag
	GECKO_Take_Error_turnOfSensor					= 34,	///< (-17) could not turn of sensor

	// Gecko Drivers Image Reading Error Messages:
	GECKO_Read_Success								= 35,	///< (0) completed successfully
	GECKO_Read_Error_InitialiseFlash				= 36,	///< (-1) could not initialize flash
	GECKO_Read_Error_SetImageID						= 37,	///< (-2) could not set image ID
	GECKO_Read_Error_StartReadout					= 38,	///< (-3) could not start readout
	GECKO_Read_readTimeout							= 39,	///< (-4) data read timeout
	GECKO_Read_wordCountMismatch					= 40,	///< (-5) word count mismatch during read
	GECKO_Read_pageCountMismatch					= 41,	///< (-6) page count mismatch during read
	GECKO_Read_readDoneFlagNotSet					= 42,	///< (-7) read done flag not set
	GECKO_Read_Error_ClearReadDoneFlag				= 43,	///< (-8) could not clear read done flag

	GECKO_Read_CouldNotReadStopFlag					= 44,	///< (-9)
	GECKO_Read_StoppedAsPerRequest					= 45,	///< (-10)

	// Gecko Drivers Image Erasing Error Messages:
	GECKO_Erase_Success								= 46,	///< (0) completed successfully
	GECKO_Erase_Error_SetImageID					= 47,	///< (-1) could not set image ID
	GECKO_Erase_StartErase							= 48,	///< (-2) could not start erase
	GECKO_Erase_Timeout								= 49,	///< (-3) erase timeout
	GECKO_Erase_Error_ClearEraseDoneFlag			= 50,	///< (-4) could not clear erase done flag

	// Butcher Error Messages:
	Butcher_Success									= 51,
	Butcher_Null_Pointer							= 52,
	Butcher_Parameter_Value							= 53,
	Butcher_Out_of_Bounds							= 54,
	Butcher_Undefined_Error							= 55,

	// JPEG Error Messages:
	JpegCompression_Success							= 56,
	JpegCompression_Failure							= 57,
	JpegCompression_qualityFactor_outOfRange		= 58,

	// EPS doesn't let me take images freely:
	EPS_didntLetMeTurnTheGeckoOn					= 59
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

ImageDataBaseResult handleMarkedPictures();

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
ImageDataBaseResult SearchDataBase_forLatestMarkedImage(ImageMetadata* image_metadata);
ImageDataBaseResult SearchDataBase_forImageFileType_byTimeRange(time_unix lower_barrier, time_unix higher_barrier, fileType file_type, ImageMetadata* image_metadata, uint32_t* image_address, uint32_t database_starting_address);
ImageDataBaseResult updateFileTypes(ImageMetadata* image_metadata, uint32_t image_address, fileType reductionLevel, Boolean value);
uint32_t GetImageFactor(fileType image_type);
uint32_t getDataBaseSize();
void setAutoThumbnailCreation(ImageDataBase database, Boolean8bit new_AutoThumbnailCreation);
void Gecko_TroubleShooter(ImageDataBaseResult error);
ImageDataBaseResult SearchDataBase_forImageFileType_byTimeRange(time_unix lower_barrier, time_unix higher_barrier, fileType file_type, ImageMetadata* image_metadata, uint32_t* image_address, uint32_t database_starting_address);

#endif /* ImageDataBase_H_ */
