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

#include <hal/Timing/Time.h>
#include <hal/errors.h>
#include <hal/Storage/FRAM.h>

#include "../Global/Global.h"
#include "../COMM/imageDump.h"
#include "DataBase.h"
#include "CameraManeger.h"
#include "Camera.h"
#include "DB_RequestHandling.h"

#include "../TRXVU.h"
#include "../Main/HouseKeeping.h"

#define CameraManagmentTask_StackDepth /*8192*/30000
#define CameraManagmentTask_Name ("CMT")
#define create_task(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask) xTaskCreate( (pvTaskCode) , (pcName) , (usStackDepth) , (pvParameters), (uxPriority), (pxCreatedTask) ); vTaskDelay(10);

static xQueueHandle interfaceQueue;
xTaskHandle	camManeger_handler;

time_unix lastPicture_time;
time_unix timeBetweenPictures;
uint32_t numberOfPicturesLeftToBeTaken;

command_id cmd_id_4_takePicturesWithTimeInBetween;

Boolean transition;

imageid lastPicture_id;
unsigned int numberOfFrames;

time_unix turnedOnCamera;

DataBase imageDataBase;

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
	f_enterFS();
	Camera_Request req;
	time_unix timeNow;
	while(TRUE)
	{
		transition = get_ground_conn();

		if (removeRequestFromQueue(&req) > -1)
		{
			act_upon_request(req);
		}

		Time_getUnixEpoch(&timeNow);
		if (timeNow > ( turnedOnCamera + 15 ) && turnedOnCamera != 0)
		{
			TurnOffGecko();
			turnedOnCamera = 0;
		}

		Take_pictures_with_time_in_between();

		vTaskDelay(1000);

		if (!transition)
		{
			Time_getUnixEpoch(&turnedOnCamera);

			DataBaseResult error = handleMarkedPictures(imageDataBase);

			save_ACK(ACK_CAMERA, error + 30, cmd_id_4_takePicturesWithTimeInBetween); // ToDo: How do we handle errors? (error log...)
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

	xTaskCreate(imageDump_task, (const signed char*)"ID_Task", CameraManagmentTask_StackDepth, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 1), NULL);

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

void send_picture_to_earth_(Camera_Request request)
{
	char* name = GetPictureFile(imageDataBase, request.data);
	byte arg[40];
	memcpy(arg, &request.cmd_id, 4);
	memcpy(arg + 4, request.data, 23);
	memcpy(arg + 27, name, FILENAMESIZE);
	vTaskDelay(10);
}
void send_imageDataBase_to_earth_(Camera_Request request)
{
	byte* ImageDataBaseFile = GetDataBase(imageDataBase, request.data);
	free(ImageDataBaseFile);
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

void act_upon_request(Camera_Request request)
{
	DataBaseResult error = DataBaseSuccess;

	switch (request.id)
	{
	case take_picture:
		error = TakePicture(imageDataBase, request.data);

		Time_getUnixEpoch(&turnedOnCamera);

		lastPicture_id = getLatestID(imageDataBase);
		numberOfFrames = getNumberOfFrames(imageDataBase);
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
		if (transition)
		{
			addRequestToQueue(request);
		}
		else
		{
			error = TransferPicture(imageDataBase, request.data);
		}

		break;


	case create_thambnail:
		error = CreateThumbnail(imageDataBase, request.data);
		break;


	case create_jpg:
		error = CreateJPG(imageDataBase, request.data);
		break;


	case update_photography_values:
		error = UpdatePhotographyValues(imageDataBase, request.data);
		break;


	case update_max_number_of_pictures:
		error = UpdateMaxNumberOfPicturesInDB(imageDataBase, request.data);
		break;


	case send_picture_to_earth:
		send_picture_to_earth_(request);
		break;


	case send_imageDataBase_to_earth:
		send_imageDataBase_to_earth_(request);
		break;


	case reset_DataBase:
		imageDataBase = resetDataBase(imageDataBase);
		break;


	default:
		return;
		break;
	}

	save_ACK(ACK_CAMERA, error + 30, request.cmd_id);
}

