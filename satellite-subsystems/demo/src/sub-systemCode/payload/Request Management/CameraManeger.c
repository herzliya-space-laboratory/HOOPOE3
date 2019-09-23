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

command_id cmd_id_for_takePicturesWithTimeInBetween;

static time_unix turnedOnCamera;
static time_unix cameraActivation_duration; ///< the duration the camera will stay turned on after being activated

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

/*
 * The task who handle all the requests from the system

 * @param[in]	Pointer to an unsigned short representing the number of seconds
 * 				to keep the camera on after the request have been done
 */

void CameraManagerTaskMain()
{
	Camera_Request req;
	time_unix timeNow;

	int f_error = f_managed_enterFS();//task 3 enter
	check_int("CameraManagerTaskMain, enter FS", f_error);

	while(TRUE)
	{
		if (removeRequestFromQueue(&req) > -1)
		{
			act_upon_request(req);
		}

		Time_getUnixEpoch(&timeNow);
		if (timeNow > ( turnedOnCamera + cameraActivation_duration ) && turnedOnCamera != 0)
		{
			TurnOffGecko();
			turnedOnCamera = 0;
		}

		Take_pictures_with_time_in_between();

		Camera_Request request = { .cmd_id = 0, .id = 70, .keepOnCamera = 10 };
		act_upon_request(request);

		vTaskDelay(SYSTEM_DEALY);
	}
}

int initCamera(Boolean first_activation)
{
	int error = initGecko();
	CMP_AND_RETURN(error, 0, error);

	Initialized_GPIO();

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

int KickStartCamera(void)
{
	// Software initialize
	interfaceQueue = xQueueCreate(MAX_NUMBER_OF_CAMERA_REQ, sizeof(Camera_Request));
	if (interfaceQueue == NULL)
		return -1;

	numberOfPicturesLeftToBeTaken = 0;
	lastPicture_time = 0;
	timeBetweenPictures = 0;

	auto_thumbnail_creation = FALSE_8BIT;

	cameraActivation_duration = DEFALT_DURATION;

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

	int auto_handler_error = 0;

	if ( (lastPicture_time + timeBetweenPictures <= currentTime) && (numberOfPicturesLeftToBeTaken != 0) )
	{
		lastPicture_time = currentTime;

		if (get_system_state(cam_operational_param))
		{
			auto_handler_error = stopAction();
			handleErrors(auto_handler_error);

			Time_getUnixEpoch(&turnedOnCamera);
			TurnOnGecko();
			ImageDataBaseResult error = takePicture(imageDataBase, FALSE_8BIT);
			if (error != DataBaseSuccess)
			{
				WriteErrorLog(error, SYSTEM_PAYLOAD, cmd_id_for_takePicturesWithTimeInBetween);
			}

			auto_handler_error = resumeAction();
			handleErrors(auto_handler_error);
		}

		numberOfPicturesLeftToBeTaken--;
	}
}

void act_upon_request(Camera_Request request)
{
	ImageDataBaseResult error = DataBaseSuccess;

	Boolean CouldNotExecute = FALSE;
	int auto_handler_error = 0;

	switch (request.id)
	{
	case take_image:
		if (get_system_state(cam_operational_param))
		{
			auto_handler_error = stopAction();
			handleErrors(auto_handler_error);

			Time_getUnixEpoch(&turnedOnCamera);

			error = TakePicture(imageDataBase, request.data);

			auto_handler_error = resumeAction();
			handleErrors(auto_handler_error);
		}
		else
		{
			CouldNotExecute = TRUE;
		}
		break;

	case take_image_with_special_values:
		if (get_system_state(cam_operational_param))
		{
			auto_handler_error = stopAction();
			handleErrors(auto_handler_error);

			Time_getUnixEpoch(&turnedOnCamera);
			error = TakeSpecialPicture(imageDataBase, request.data);

			auto_handler_error = resumeAction();
			handleErrors(auto_handler_error);
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
		error = DeletePictureFile(imageDataBase, request.data);
		break;

	case delete_image:
		if (get_system_state(cam_operational_param))
		{
			auto_handler_error = stopAction();
			handleErrors(auto_handler_error);

			Time_getUnixEpoch(&turnedOnCamera);
			error = DeletePicture(imageDataBase, request.data);

			auto_handler_error = resumeAction();
			handleErrors(auto_handler_error);
		}
		else
		{
			addRequestToQueue(request);
			CouldNotExecute = TRUE;
		}
		break;

	case transfer_image_to_OBC:
		if (get_ground_conn() || !get_system_state(cam_operational_param))
		{
			addRequestToQueue(request);
			CouldNotExecute = TRUE;
		}
		else
		{
			auto_handler_error = stopAction();
			handleErrors(auto_handler_error);

			Time_getUnixEpoch(&turnedOnCamera);
			error = TransferPicture(imageDataBase, request.data);

			auto_handler_error = resumeAction();
			handleErrors(auto_handler_error);
		}
		break;

	case create_thumbnail:
		error = CreateThumbnail(request.data);
		break;

	case create_jpg:
		error = CreateJPG(request.data);
		break;

	case update_photography_values:
		error = UpdatePhotographyValues(imageDataBase, request.data);
		break;

	case reset_DataBase:
		error = resetImageDataBase(imageDataBase);
		break;

	case image_Dump_chunkField:
	case image_Dump_bitField:
	case DataBase_Dump:
		KickStartImageDumpTask(&request);
		break;

	case update_defult_duration:
		memcpy(&cameraActivation_duration, request.data, sizeof(time_unix));
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
		auto_thumbnail_creation = FALSE_8BIT;
		setAutoThumbnailCreation(imageDataBase, FALSE_8BIT);
		break;
	case turn_on_AutoThumbnailCreation:
		auto_thumbnail_creation = TRUE_8BIT;
		setAutoThumbnailCreation(imageDataBase, TRUE_8BIT);
		break;

	case (cam_Request_id_t)70:
		break;

	default:
		return;
		break;
	}

	Gecko_TroubleShooter(error);

	if ( !CouldNotExecute )
	{
		//save_ACK(ACK_CAMERA, error + 30, request.cmd_id);

		if (error != DataBaseSuccess)
		{
			WriteErrorLog(error, SYSTEM_PAYLOAD, request.cmd_id);
		}
	}
}
