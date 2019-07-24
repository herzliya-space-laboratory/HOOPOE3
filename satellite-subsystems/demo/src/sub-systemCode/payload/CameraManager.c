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

#include <at91/utility/exithandler.h>
#include <at91/commons.h>
#include <at91/utility/trace.h>
#include <at91/peripherals/cp15/cp15.h>

#include <hal/Utility/util.h>
#include <hal/Timing/WatchDogTimer.h>
#include <hal/Timing/Time.h>
#include <hal/Drivers/I2C.h>
#include <hal/Drivers/LED.h>
#include <hal/errors.h>
#include <hal/Storage/FRAM.h>

#include <string.h>

#include "DataBase.h"
#include "CameraManager.h"
#include "Camera.h"
#include "DB_RequestHandling.h"

#include "../COMM/imageDump.h"

#include "../TRXVU.h"
#include "../Main/HouseKeeping.h"

#define CameraManagmentTask_StackDepth /*8192*/30000
#define CameraManagmentTask_Name ("Camera Management Task")
#define CameraDumpTask_Name ("Camera Dump Task")

#define MAX_NUMBER_OF_PICTURES_TO_BE_HANDLED 2

static xQueueHandle interfaceQueue;
xTaskHandle	camManeger_handler;

static time_unix lastPicture_time;
static time_unix timeBetweenPictures;
static uint32_t numberOfPicturesLeftToBeTaken;

command_id cmd_id_4_takePicturesWithTimeInBetween;

static imageid lastPicture_id;
static unsigned int numberOfFrames;

static time_unix turnedOnCamera;

DataBase imageDataBase;

static time_unix cameraActivation_duration; // the duration the camera will stay turned on after being activated

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
	int err = f_enterFS();
	if (err != 0)
		printf("\n*ERROR* - eentering file system (%d)\n\n", err);

	F_FILE* a = f_open("pic123.bla", "w+");
	if (a == NULL)
		printf("\n\n\n*ERROR* - FS\n\n\n");
	f_close(a);

	Camera_Request req;
	time_unix timeNow;

	F_FILE* b = f_open("pic123.bla", "w+");
	if (b == NULL)
		printf("\n\n\n*ERROR* - FS\n\n\n");
	f_close(b);

	while(TRUE)
	{
		F_FILE* e = f_open("pic123.bla", "w+");
		if (e == NULL)
			printf("\n\n\n*ERROR* - FS\n\n\n");
		f_close(e);

		if (removeRequestFromQueue(&req) > -1)
		{
			act_upon_request(req);
		}

		F_FILE* b = f_open("pic123.bla", "w+");
		if (b == NULL)
			printf("\n\n\n*ERROR* - FS\n\n\n");
		f_close(b);

		Time_getUnixEpoch(&timeNow);
		if (timeNow > ( turnedOnCamera + cameraActivation_duration ) && turnedOnCamera != 0)
		{
			TurnOffGecko();
			turnedOnCamera = 0;
		}

		F_FILE* c = f_open("pic123.bla", "w+");
		if (c == NULL)
			printf("\n\n\n*ERROR* - FS\n\n\n");
		f_close(c);

		Take_pictures_with_time_in_between();

		vTaskDelay(1000);

		F_FILE* d = f_open("pic123.bla", "w+");
		if (d == NULL)
			printf("\n\n\n*ERROR* - FS\n\n\n");
		f_close(d);


		if (!get_ground_conn())
		{
			DataBaseResult error = handleMarkedPictures(imageDataBase, MAX_NUMBER_OF_PICTURES_TO_BE_HANDLED, &turnedOnCamera);

			if (error != DataBase_thereWereNoPicturesToBeHandled)
				save_ACK(ACK_CAMERA, error + 30, cmd_id_4_takePicturesWithTimeInBetween);
		}

		vTaskDelay(SYSTEM_DEALY);
	}
}

int initCamera(Boolean firstActivation)
{
	int error = initGecko();
	if (error)
		return error;

	Initialized_GPIO();

	imageDataBase = initDataBase(firstActivation);
	if (imageDataBase == NULL)
		error = -1;

	return error;
}

int KickStartCamera(void)
{
	//Software initialize
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

	if ( (lastPicture_time + timeBetweenPictures <= currentTime) && (numberOfPicturesLeftToBeTaken != 0) )
	{
		lastPicture_time = currentTime;

		Time_getUnixEpoch(&turnedOnCamera);
		TurnOnGecko();

		takePicture(imageDataBase, FALSE_8BIT);

		imageid id = getLatestID(imageDataBase);
		markPicture(imageDataBase, id);

		numberOfPicturesLeftToBeTaken--;
	}
}

void handleDump(Camera_Request request)
{
	request_image DumpRequest;
	DumpRequest.cmdId = request.cmd_id;

	if (request.id == Image_Dump_chunkField)
	{
		DumpRequest.type = chunkField_imageDump_;

		memcpy(DumpRequest.command_parameters, request.data, MAX_DATA_CAM_REQ);
	}

	else if (request.id == Image_Dump_bitField)
	{
		DumpRequest.type = bitField_imageDump_;

		memcpy(DumpRequest.command_parameters, request.data, MAX_DATA_CAM_REQ);
	}

	else if (request.id == Thumbnail_Dump)
	{
		DumpRequest.type = thumbnail_imageDump_;

		time_unix startingTime = *((time_unix*)request.data);
		memcpy(&startingTime, request.data, sizeof(time_unix));
		time_unix endTime = *((time_unix*)request.data + 4);
		memcpy(&endTime, request.data + 4, sizeof(time_unix));

		imageid* ID_list = get_ID_list_withDefaltThumbnail(imageDataBase, startingTime, endTime);
		memcpy(DumpRequest.command_parameters, ID_list, sizeof(ID_list));
	}

	else if (request.id == DataBase_Dump)
	{
		DumpRequest.type = imageDataBaseDump_;

		time_unix startingTime = *((time_unix*)request.data);
		memcpy(&startingTime, request.data, sizeof(time_unix));
		time_unix endTime = *((time_unix*)request.data + 4);
		memcpy(&endTime, request.data + 4, sizeof(time_unix));

		byte* buffer = getDataBaseBuffer(imageDataBase, startingTime, endTime);
		memcpy(DumpRequest.command_parameters, buffer, sizeof(buffer));
	}

	//imageDump_task(&DumpRequest);
	xTaskCreate(imageDump_task, (const signed char*)CameraDumpTask_Name, CameraManagmentTask_StackDepth, &DumpRequest, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 1), NULL);
}

void act_upon_request(Camera_Request request)
{
	DataBaseResult error = DataBaseSuccess;

	F_FILE* a = f_open("pic123.bla", "w+");
	if (a == NULL)
		printf("\n\n\n*ERROR* - FS\n\n\n");
	f_close(a);

	F_FILE* l;

	switch (request.id)
	{
	case take_picture:
		error = TakePicture(imageDataBase, request.data);

		F_FILE* b = f_open("pic123.bla", "w+");
		if (b == NULL)
			printf("\n\n\n*ERROR* - FS\n\n\n");
		f_close(b);

		Time_getUnixEpoch(&turnedOnCamera);

		lastPicture_id = getLatestID(imageDataBase);
		numberOfFrames = getNumberOfFrames(imageDataBase);

		F_FILE* c = f_open("pic123.bla", "w+");
		if (c == NULL)
			printf("\n\n\n*ERROR* - FS\n\n\n");
		f_close(c);

		for (unsigned int i = 0; i < numberOfFrames; i++) {
			markPicture(imageDataBase, lastPicture_id - i);
		}

		break;


	case take_picture_with_special_values:
		Time_getUnixEpoch(&turnedOnCamera);

		error = TakeSpecialPicture(imageDataBase, request.data);

		lastPicture_id = getLatestID(imageDataBase);
		numberOfFrames = getNumberOfFrames(imageDataBase);
		for (unsigned int i = 1; i < numberOfFrames; i++) {
			markPicture(imageDataBase, lastPicture_id - i);
		}

		break;


	case take_pictures_with_time_in_between:
		memcpy(&numberOfPicturesLeftToBeTaken, request.data, sizeof(int));
		memcpy(&timeBetweenPictures, request.data + 4, sizeof(int));

		Time_getUnixEpoch(&lastPicture_time);

		cmd_id_4_takePicturesWithTimeInBetween = request.cmd_id;

		break;


	case delete_picture:
		error = DeletePicture(imageDataBase, request.data);

		Time_getUnixEpoch(&turnedOnCamera);

		break;


	case transfer_image_to_OBC:
		if (get_ground_conn())
		{
			addRequestToQueue(request);
		}
		else
		{
			error = TransferPicture(imageDataBase, request.data);
		}

		break;


	case create_thumbnail:
		error = CreateThumbnail(imageDataBase, request.data);
		break;


	case create_jpg:
		error = CreateJPG(imageDataBase, request.data);
		break;


	case update_photography_values:
		error = UpdatePhotographyValues(imageDataBase, request.data);
		break;


	case reset_DataBase:
		imageDataBase = resetDataBase(imageDataBase);
		break;


	case Image_Dump_chunkField:
	case Image_Dump_bitField:
	case Thumbnail_Dump:
	case DataBase_Dump:
		handleDump(request);
		break;


	case update_defult_duration:
		memcpy(&cameraActivation_duration, request.data, sizeof(time_unix));
		break;


	case Turn_On_Camera:

		l = f_open("pic123.bla", "w+");
		if (l == NULL)
			printf("\n\n\n*ERROR* - FS\n\n\n");
		f_close(l);

		TurnOnGecko();

		F_FILE* h = f_open("pic123.bla", "w+");
		if (h == NULL)
			printf("\n\n\n*ERROR* - FS\n\n\n");
		f_close(h);

		Time_getUnixEpoch(&turnedOnCamera);

		F_FILE* g = f_open("pic123.bla", "w+");
		if (g == NULL)
			printf("\n\n\n*ERROR* - FS\n\n\n");
		f_close(g);

		break;

	case Turn_Off_Camera:
		TurnOffGecko();
		break;


	default:
		return;
		break;
	}

	F_FILE* d = f_open("pic123.bla", "w+");
	if (d == NULL)
		printf("\n\n\n*ERROR* - FS\n\n\n");
	f_close(d);

	save_ACK(ACK_CAMERA, error + 30, request.cmd_id);
}
