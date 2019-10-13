/*
 * CameraManeger.c
 *
 *  Created on: Jun 2, 2019
 *      Author: DBTn
 */
#include <freertos/FreeRTOS.h>

#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <hal/Utility/util.h>
#include <hal/Timing/Time.h>
#include <hal/errors.h>
#include <hal/Storage/FRAM.h>

#include <string.h>

#include "../DataBase/DataBase.h"
#include "../Drivers/GeckoCameraDriver.h"
#include "DB_RequestHandling.h"
#include "Dump/imageDump.h"

#include "../../Global/logger.h"

#include "../../Global/TLM_management.h"

#include "../../Global/GlobalParam.h"
#include "../../TRXVU.h"
#include "../../Main/HouseKeeping.h"

#include "AutomaticImageHandler.h"
#include "CameraManeger.h"

#define CameraManagmentTask_StackDepth 30000
#define CameraManagmentTask_Name ("Camera Management Task")
#define CameraDumpTask_Name ("Camera Dump Task")

static xQueueHandle interfaceQueue;
xTaskHandle	camManeger_handler;

static time_unix lastPicture_time;
static time_unix timeBetweenPictures;
static uint32_t numberOfPicturesLeftToBeTaken;

static command_id cmd_id_for_takePicturesWithTimeInBetween;

static time_unix turnedOnCamera;
static time_unix cameraActivation_duration; ///< the duration the camera will stay turned on after being activated

static Boolean automatic_image_handler_disabled_permanently;

xTaskHandle xImageDumpTaskHandle;

ImageDataBase imageDataBase;


/*
 * Handle the functionality of every request.
 * @param[in]	the request to order
 *
 * @note the definition is for the functions written above the implementation
 */
void act_upon_request(Camera_Request request);
/*
 * remove request from the queue
 * @param[out]	the request removed from the queue
 * @return positive-number the number of requests in the Queue
 * @return -1 if the queue if empty
 *
 * @note the definition is for the functions written above the implementation
 */
int removeRequestFromQueue(Camera_Request *request);
/*
 * will check if needs to take picture and will take if it does
 *
 * @note the definition is for the functions written above the implementation
 */
void Take_pictures_with_time_in_between();

static void inner_init();

/*
 * The task who handle all the requests from the system
 */
void CameraManagerTaskMain()
{
	inner_init();

	Camera_Request req;

	int f_error = f_managed_enterFS();//task 5 enter fs
	check_int("CameraManagerTaskMain, enter FS", f_error);

	while(TRUE)
	{
		if (removeRequestFromQueue(&req) > -1)
		{
			act_upon_request(req);
		}

		// Handle case where EPS shut down automatic image handler:
		if (get_system_state(cam_operational_param) && !automatic_image_handler_disabled_permanently && turnedOnCamera == 0)
		{
			if (xImageDumpTaskHandle != NULL && eTaskGetState(xImageDumpTaskHandle) == eDeleted)
			{
				if (get_automatic_image_handling_task_suspension_flag())
				{
					xImageDumpTaskHandle = NULL;
					resumeAction();
				}
			}
		}

		Take_pictures_with_time_in_between();

		vTaskDelay(100);
	}
}

void setGeckoParametersInFRAM()
{
	FRAM_write_exte((unsigned char *)&automatic_image_handler_disabled_permanently, AUTOMATIC_IMAGE_HANDLER_STATE_ADDR, AUTOMATIC_IMAGE_HANDLER_STATE_SIZE);
	FRAM_write_exte((unsigned char *)&cameraActivation_duration, CAMERA_ACTIVATION_DURATION_ADDR, CAMERA_ACTIVATION_DURATION_SIZE);
}
void getGeckoParametersFromFRAM()
{
	FRAM_read_exte((unsigned char *)&automatic_image_handler_disabled_permanently, AUTOMATIC_IMAGE_HANDLER_STATE_ADDR, AUTOMATIC_IMAGE_HANDLER_STATE_SIZE);
	FRAM_read_exte((unsigned char *)&cameraActivation_duration, CAMERA_ACTIVATION_DURATION_ADDR, CAMERA_ACTIVATION_DURATION_SIZE);
}

int createFolders()
{
	char path[13];
	int error = 0;

	sprintf(path, "%s", GENERAL_PAYLOAD_FOLDER_NAME);
	error = f_mkdir(path);
	CMP_AND_RETURN_MULTIPLE(error, 0, 6, -1);

	sprintf(path, "%s/%s", GENERAL_PAYLOAD_FOLDER_NAME, RAW_IMAGES_FOLDER_NAME);
	error = f_mkdir(path);
	CMP_AND_RETURN_MULTIPLE(error, 0, 6, -1);

	sprintf(path, "%s/%s", GENERAL_PAYLOAD_FOLDER_NAME, JPEG_IMAGES_FOLDER_NAME);
	error = f_mkdir(path);
	CMP_AND_RETURN_MULTIPLE(error, 0, 6, -1);

	sprintf(path, "%s/%s", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_1_IMAGES_FOLDER_NAME);
	error = f_mkdir(path);
	CMP_AND_RETURN_MULTIPLE(error, 0, 6, -1);

	sprintf(path, "%s/%s", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_2_IMAGES_FOLDER_NAME);
	error = f_mkdir(path);
	CMP_AND_RETURN_MULTIPLE(error, 0, 6, -1);

	sprintf(path, "%s/%s", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_3_IMAGES_FOLDER_NAME);
	error = f_mkdir(path);
	CMP_AND_RETURN_MULTIPLE(error, 0, 6, -1);

	sprintf(path, "%s/%s", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_4_IMAGES_FOLDER_NAME);
	error = f_mkdir(path);
	CMP_AND_RETURN_MULTIPLE(error, 0, 6, -1);

	sprintf(path, "%s/%s", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_5_IMAGES_FOLDER_NAME);
	error = f_mkdir(path);
	CMP_AND_RETURN_MULTIPLE(error, 0, 6, -1);

	sprintf(path, "%s/%s", GENERAL_PAYLOAD_FOLDER_NAME, THUMBNAIL_LEVEL_6_IMAGES_FOLDER_NAME);
	error = f_mkdir(path);
	CMP_AND_RETURN_MULTIPLE(error, 0, 6, -1);

	return 0;
}

int initCamera(Boolean first_activation)
{
	TurnOffGecko();

	cameraActivation_duration = DEFALT_DURATION;
	automatic_image_handler_disabled_permanently = FALSE;
	setGeckoParametersInFRAM();

	int error = initGecko();
	CMP_AND_RETURN(error, 0, error);

	Initialized_GPIO();

	error = createFolders();
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	imageDataBase = initImageDataBase(first_activation);
	if (imageDataBase == NULL)
		error = -1;

	if (first_activation)
	{
		error = setChunkDimensions_inFRAM(DEFALT_CHUNK_WIDTH, DEFALT_CHUNK_HEIGHT);
		CMP_AND_RETURN(error, DataBaseSuccess, error);
	}

	return error;
}

static void inner_init()
{
	getGeckoParametersFromFRAM();

	numberOfPicturesLeftToBeTaken = 0;
	lastPicture_time = 0;
	timeBetweenPictures = 0;
}

int KickStartCamera(void)
{
	// Software initialize
	interfaceQueue = xQueueCreate(MAX_NUMBER_OF_CAMERA_REQ, sizeof(Camera_Request));
	if (interfaceQueue == NULL)
		return -1;

	xImageDumpTaskHandle = NULL;

	xTaskCreate(CameraManagerTaskMain, (const signed char*)CameraManagmentTask_Name, CameraManagmentTask_StackDepth, NULL, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, NULL);

	return 0;
}

int addRequestToQueue(Camera_Request request)
{
	int numberOfReq = uxQueueMessagesWaiting(interfaceQueue);

	if (numberOfReq == MAX_NUMBER_OF_CAMERA_REQ)
		return -1;

	signed portBASE_TYPE retVal = xQueueSend(interfaceQueue, &request, MAX_DELAY);

	if (retVal != pdTRUE)
		return -2;

	return ++numberOfReq;
}

int removeRequestFromQueue(Camera_Request *request)
{
	int count = uxQueueMessagesWaiting(interfaceQueue);
	if (count == 0)
		return -1;

	xQueueReceive(interfaceQueue, request, MAX_DELAY);

	return --count;
}

int resetQueue()
{
	xQueueReset(interfaceQueue);
	return 0;
}

void Take_pictures_with_time_in_between()
{
	time_unix currentTime;
	Time_getUnixEpoch(&currentTime);

	if ( (lastPicture_time + timeBetweenPictures <= currentTime) && (numberOfPicturesLeftToBeTaken != 0) )
	{
		lastPicture_time = currentTime;

		if (get_system_state(cam_operational_param))
		{
			stopAction();

			if ( get_automatic_image_handling_task_suspension_flag() == TRUE)
			{
				updateGeneralDataBaseParameters(imageDataBase);

				Time_getUnixEpoch(&turnedOnCamera);
				TurnOnGecko();

				ImageDataBaseResult error = takePicture(imageDataBase, FALSE_8BIT);
				if (error != DataBaseSuccess)
				{
					WriteErrorLog(error, SYSTEM_PAYLOAD, cmd_id_for_takePicturesWithTimeInBetween);
				}
			}

			TurnOffGecko();
			resumeAction();
		}

		numberOfPicturesLeftToBeTaken--;
	}
}

void act_upon_request(Camera_Request request)
{
	ImageDataBaseResult error = DataBaseSuccess;
	Boolean CouldNotExecute = FALSE;

	updateGeneralDataBaseParameters(imageDataBase);

	switch (request.id)
	{
	case take_image:
		if (get_system_state(cam_operational_param))
		{
			 stopAction();

			Time_getUnixEpoch(&turnedOnCamera);

			error = TakePicture(imageDataBase, request.data);
		}
		else
		{
			CouldNotExecute = TRUE;
		}
		break;

	case take_image_with_special_values:
		if (get_system_state(cam_operational_param))
		{
			stopAction();

			Time_getUnixEpoch(&turnedOnCamera);
			error = TakeSpecialPicture(imageDataBase, request.data);
		}
		else
		{
			CouldNotExecute = TRUE;
		}
		break;

	case take_image_with_time_intervals:
		memcpy(&numberOfPicturesLeftToBeTaken, request.data, sizeof(int));
		memcpy(&timeBetweenPictures, request.data + 4, sizeof(int));
		Time_getUnixEpoch(&lastPicture_time);
		cmd_id_for_takePicturesWithTimeInBetween = request.cmd_id;
		break;

	case delete_image_file:
		stopAction();
		error = DeletePictureFile(imageDataBase, request.data);
		break;

	case delete_image:
		if (get_system_state(cam_operational_param))
		{
			stopAction();

			Time_getUnixEpoch(&turnedOnCamera);
			error = DeletePicture(imageDataBase, request.data);
		}
		else
		{
			addRequestToQueue(request);
			CouldNotExecute = TRUE;
		}
		break;

	case transfer_image_to_OBC:
		if (get_system_state(cam_operational_param))
		{
			stopAction();

			Time_getUnixEpoch(&turnedOnCamera);
			error = TransferPicture(imageDataBase, request.data);
		}
		else
		{
			addRequestToQueue(request);
			CouldNotExecute = TRUE;
		}
		break;

	case create_thumbnail:
		stopAction();
		error = CreateThumbnail(request.data);
		break;

	case create_jpg:
		stopAction();
		error = CreateJPG(request.data);
		break;

	case update_photography_values:
		error = UpdatePhotographyValues(imageDataBase, request.data);
		break;

	case reset_DataBase:
		stopAction();
		error = resetImageDataBase(imageDataBase);
		break;

	case image_Dump_chunkField:
	case image_Dump_bitField:
	case DataBase_Dump:
	case fileType_Dump:
		stopAction();
		KickStartImageDumpTask(&request, &xImageDumpTaskHandle);
		break;

	case update_defult_duration:
		memcpy(&cameraActivation_duration, request.data, sizeof(time_unix));
		setGeckoParametersInFRAM();
		break;

	case set_chunk_size:
		error = setChunkDimensions(request.data);
		break;

	case stop_take_image_with_time_intervals:
		numberOfPicturesLeftToBeTaken = 0;
		timeBetweenPictures = 0;
		lastPicture_time = 0;
		break;

	case turn_on_camera:
		if (get_system_state(cam_operational_param))
		{
			stopAction();
			Time_getUnixEpoch(&turnedOnCamera);
			TurnOnGecko();
		}
		else
		{
			CouldNotExecute = TRUE;
		}
		break;

	case turn_off_camera:
		TurnOffGecko();
		turnedOnCamera = 0;
		break;

	case turn_off_future_AutoThumbnailCreation:
		setAutoThumbnailCreation(imageDataBase, FALSE_8BIT);
		break;
	case turn_on_future_AutoThumbnailCreation:
		setAutoThumbnailCreation(imageDataBase, TRUE_8BIT);
		break;

	case turn_off_AutoThumbnailCreation:
		automatic_image_handler_disabled_permanently = TRUE;
		setGeckoParametersInFRAM();

		CouldNotExecute = TRUE;
		if (get_automatic_image_handling_ready_for_long_term_stop() == TRUE)
		{
			stopAction();
			CouldNotExecute = FALSE;
			break;
		}
		vTaskDelay(100);

		setAutoThumbnailCreation(imageDataBase, FALSE_8BIT);

		if (CouldNotExecute)
		{
			addRequestToQueue(request);
		}

		break;
	case turn_on_AutoThumbnailCreation:
		automatic_image_handler_disabled_permanently = FALSE;
		setGeckoParametersInFRAM();

		resumeAction();

		setAutoThumbnailCreation(imageDataBase, TRUE_8BIT);
		break;

	case get_gecko_registers:
		error = SendGeckoRegisters();
		break;

	case set_gecko_registers:
		error = GeckoSetRegister(request.data);
		break;

	case re_init_cam_manager:
		resetQueue();
		inner_init();
		error = initCamera(TRUE);
		break;

	default:
		return;
		break;
	}

	Gecko_TroubleShooter(error);

	if ( !CouldNotExecute )
	{
		save_ACK(ACK_CAMERA, error + 30, request.cmd_id);

		if (error != DataBaseSuccess)
		{
			WriteErrorLog(error, SYSTEM_PAYLOAD, request.cmd_id);
		}
	}
}

int send_request_to_reset_database()
{
	Camera_Request request = { .cmd_id = 0, .id = reset_DataBase , .keepOnCamera = KEEP_ON_CAMERA };
	int error = addRequestToQueue(request);
	CMP_AND_RETURN(error, 0, error);
	vTaskDelay(100);
	return 0;
}
