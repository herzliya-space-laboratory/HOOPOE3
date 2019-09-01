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

#include "../../Global/TLM_management.h"

#include "../../Global/GlobalParam.h"
#include "../../TRXVU.h"
#include "../../Main/HouseKeeping.h"

#include "CameraManeger.h"

#define CameraManagmentTask_StackDepth 30000
#define CameraManagmentTask_Name ("Camera Management Task")
#define CameraDumpTask_Name ("Camera Dump Task")

#define NUMBER_OF_PICTURES_TO_BE_HANDLED_AT_A_TIME 2

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

		if ( !get_ground_conn() )
		{
			Camera_Request request = { .cmd_id = 0, .id = Handle_Mark, .keepOnCamera = 10 };
			act_upon_request(request);
		}

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

	cameraActivation_duration = DEFALT_DURATION;

	xTaskCreate(CameraManagerTaskMain, (const signed char*)CameraManagmentTask_Name, CameraManagmentTask_StackDepth, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 1), NULL);

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

	if ( (lastPicture_time + timeBetweenPictures <= currentTime) && (numberOfPicturesLeftToBeTaken != 0)  && (get_system_state(cam_operational_param)) )
	{
		lastPicture_time = currentTime;

		Time_getUnixEpoch(&turnedOnCamera);
		TurnOnGecko();

		takePicture(imageDataBase, FALSE_8BIT);

		numberOfPicturesLeftToBeTaken--;
	}
}

void Gecko_TroubleShooter(ImageDataBaseResult error)
{
	Boolean should_reset_take = error > GECKO_Take_Success && error < GECKO_Read_Success;
	Boolean should_reset_read = error > GECKO_Read_Success && error < GECKO_Erase_Success;
	Boolean should_reset_erase = error > GECKO_Erase_Success && error <= GECKO_Erase_Error_ClearEraseDoneFlag;

	Boolean should_reset = should_reset_take || should_reset_read || should_reset_erase;

	if (should_reset)
	{
		TurnOffGecko();
		TurnOnGecko();
	}
}

void startDumpTask(Camera_Request request)
{
	xTaskCreate(imageDump_task, (const signed char*)CameraDumpTask_Name, CameraManagmentTask_StackDepth, &request, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 1), NULL);
	vTaskDelay(SYSTEM_DEALY);
}

void act_upon_request(Camera_Request request)
{
	ImageDataBaseResult error = DataBaseSuccess;
	error = f_managed_enterFS();
	// ToDo: log error

	switch (request.id)
	{
	case take_picture:
		Time_getUnixEpoch(&turnedOnCamera);
		error = TakePicture(imageDataBase, request.data);
		break;

	case take_picture_with_special_values:
		Time_getUnixEpoch(&turnedOnCamera);
		error = TakeSpecialPicture(imageDataBase, request.data);
		break;

	case take_pictures_with_time_in_between:
		memcpy(&numberOfPicturesLeftToBeTaken, request.data, sizeof(int));
		memcpy(&timeBetweenPictures, request.data + 4, sizeof(int));
		Time_getUnixEpoch(&lastPicture_time);
		cmd_id_for_takePicturesWithTimeInBetween = request.cmd_id;
		break;

	case delete_picture_file:
		error = DeletePictureFile(imageDataBase, request.data);
		break;

	case delete_picture:
		if (get_system_state(cam_operational_param))
		{
			Time_getUnixEpoch(&turnedOnCamera);
			error = DeletePicture(imageDataBase, request.data);
		}
		else
		{
			addRequestToQueue(request);
		}
		break;

	case transfer_image_to_OBC:
		if (get_ground_conn() && !get_system_state(cam_operational_param))
		{
			addRequestToQueue(request);
		}
		else
		{
			Time_getUnixEpoch(&turnedOnCamera);
			error = TransferPicture(imageDataBase, request.data);
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

	case Image_Dump_chunkField:
	case Image_Dump_bitField:
	case DataBase_Dump:
		startDumpTask(request);
		break;

	case update_defult_duration:
		memcpy(&cameraActivation_duration, request.data, sizeof(time_unix));
		break;

	case Turn_On_Camera:
		if (get_system_state(cam_operational_param))
		{
			Time_getUnixEpoch(&turnedOnCamera);
			TurnOnGecko();
		}
		break;

	case Turn_Off_Camera:
		TurnOffGecko();
		turnedOnCamera = 0;
		break;

	case Set_Chunk_Size:
		error = setChunkDimensions(request.data);
		break;

	case Handle_Mark:
		Time_getUnixEpoch(&turnedOnCamera);
		error = handleMarkedPictures(NUMBER_OF_PICTURES_TO_BE_HANDLED_AT_A_TIME);
		break;

	default:
		return;
		break;
	}

	Gecko_TroubleShooter(error);

	if ( !(request.id == Handle_Mark || request.id == transfer_image_to_OBC) )
		save_ACK(ACK_CAMERA, error + 30, request.cmd_id);

	error = f_managed_releaseFS();
	// ToDo: log error
}
