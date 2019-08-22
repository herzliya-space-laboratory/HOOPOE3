/*
 * CameraManeger.c
 *
 *  Created on: Jun 2, 2019
 *      Author: elain
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

#include "DataBase.h"
#include "CameraManeger.h"
#include "Camera.h"
#include "DB_RequestHandling.h"

#include "../TRXVU.h"

static xQueueHandle interfaceQueue;
xTaskHandle	camManeger_handler;

#ifdef AUTO_CAMERA
time_unix lastPicture_time;
xSemaphoreHandle xLastPicture_time;
#endif

DataBase imageDataBase;

/*
 *	Handle the functionality of every request.
 *	@param[in]	the request to order
 */
void act_upon_request(Camera_Request request);
/*
 * remove request from the queue
 * @param[out]	the request removed from the queue
 * @return positive-number the number of requests in the Queue
 * @return -1 if the queue if empty
 */
int removeRequestFromQueue(Camera_Request *request);

/*
 * The task who handle all the requests from the system
 * @param[in]	Pointer to an unsigned short representing the number of seconds
 * 				to keep the camera on after the request have been done
 */
void camManeger_task(void* arg)
{
	int i_error;
	Camera_Request req;
	time_unix timeToShut = 0, timeNow;
	while(TRUE)
	{
		if (removeRequestFromQueue(&req) > -1)
		{
			req.data[0] = FALSE_8BIT;
			act_upon_request(req);

			if (timeToShut == 0)
			{
				i_error = Time_getUnixEpoch(&timeToShut);
				check_int("camManeger_task, Time_getUnixEpoch", i_error);
			}
			else if (MIN_TIME_TO_KEEP_ON_CAMERA <= req.keepOnCamera && req.keepOnCamera <= MAX_TIME_TO_KEEP_ON_CAMERA)
			{
				timeToShut += req.keepOnCamera;
			}
		}

		i_error = Time_getUnixEpoch(&timeNow);
		check_int("camManeger_task, Time_getUnixEpoch", i_error);
		if (timeToShut != 0 && timeToShut < timeNow)
		{
			TurnOffGecko();
			timeToShut = 0;
		}

		vTaskDelay(SYSTEM_DEALY);
	}
}

#ifdef AUTO_CAMERA
//The handler of the auto camera task
xTaskHandle autoCameraHandler;

/*
 * The task who handle the "Auto pilot" of the camera
 */
void autoCamera_task()
{
	portTickType xLastWakeTime = xTaskGetTickCount();
	const portTickType xFrequency = CONVERT_SECONDS_TO_MS(30 * 60);

	while(TRUE)
	{
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

/*
 * Returns the auto state if on/off
 * @return FALSE == off, On == true
 */
Boolean check_autoPilot()
{
	Boolean8bit state;
	int error = FRAM_read(&state, AUTO_PILOT_STATE_ADDR, 1);
	check_int("check_autoPilot, FRAM_read(AUTO_PILOT_STATE_ADDR)", error);
	return state;
}

/*
 * Change the auto pilot state on/off
 * @param[in]	New value to update
 */
void change_autoPilot(Boolean state)
{
	int error = FRAM_write(&state, AUTO_PILOT_STATE_ADDR, 1);
	check_int("check_autoPilot, FRAM_read(AUTO_PILOT_STATE_ADDR)", error);
}

/*
 * Turn off the auto pilot, delete the task,
 * change the auto pilot to off
 */
void turnOff_autoPilot()
{
	vTaskDelete(autoCameraHandler);
	change_autoPilot(SWITCH_OFF);
}

/*
 * Turn on the auto pilot, create the task,
 * change the auto pilot to on
 */
void turnOn_autoPilot()
{
	if (check_autoPilot())
		return;

	xTaskCreate(autoCamera_task, (const signed char*)("Auto cam task"), 8192, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), autoCameraHandler);
	change_autoPilot(SWITCH_ON);
}
#endif

int initCamera(Boolean firstActivation)
{
	Initialized_GPIO();
	int error = initGecko();
	imageDataBase = initDataBase(firstActivation);

	return error;
}

int KickStartCamera()
{
	//Software initialize
	interfaceQueue = xQueueCreate(MAX_NUMBER_OF_CAMERA_REQ, sizeof(Camera_Request));
	if (interfaceQueue == NULL)
		return -1;

#ifdef AUTO_CAMERA
	FRAM_read(&lastPicture_time, CAMERA_LAST_PICTUR_TIME_ADDR, TIME_SIZE);
	vSemaphoreCreateBinary(xLastPicture_time);
	if (xLastPicture_time == NULL)
		return -2;
#endif

	xTaskCreate(camManeger_task, (const signed char*)("cam manager task"), 8192, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), NULL);

#ifdef AUTO_CAMERA
	if (check_autoPilot())
		xTaskCreate(autoCamera_task, (const signed char*)("Auto cam task"), 8192, NULL, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), autoCameraHandler);
#endif

	return 0;
}

void reset_payload_FRAM()
{
	resetDataBase(imageDataBase);
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
	char *name = GetPictureFile(imageDataBase, request.data);
	byte arg[40];
	memcpy(arg, &request.cmd_id, 4);
	memcpy(arg + 4, request.data, 23);
	//xTaskCreate(Dump_image_task, (const signed char*)("Roy"), 8192, arg, (unsigned portBASE_TYPE)(configMAX_PRIORITIES - 2), NULL);
	vTaskDelay(10);
}

void act_upon_request(Camera_Request request)
{
	DataBaseResult error;
	switch (request.id)
	{
	case take_picture:
		error = TakePicture(imageDataBase, request.data);
		break;
	case delete_picture:
		error = DeletePicture(imageDataBase, request.data);
		break;
	case transfer_image_to_OBC:
		error = TransferPicture(imageDataBase, request.data);
		break;
	case create_thambnail:
		error = CreateThumbnail(imageDataBase, request.data);
		break;
	case update_photography_values:
		error = UpdatePhotographyValues(imageDataBase, request.data);
		break;
	case send_picture_to_earth:
		send_picture_to_earth_(request);
		break;
	case turn_off_auto_pilot:
		turnOff_autoPilot();
		break;
	case turn_on_auto_pilot:
		turnOn_autoPilot();
		break;
	case reset_FRAM_address:
		reset_payload_FRAM();
		break;
		default:
			return;
			break;
	}

	save_ACK(ACK_CAMERA, error + 30, request.cmd_id);
}

