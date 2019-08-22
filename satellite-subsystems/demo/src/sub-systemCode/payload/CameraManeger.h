/*
 * CameraManeger.h
 *
 *  Created on: Jun 2, 2019
 *      Author: elain
 *
 *      this file is in charge off all the camera
 */

#ifndef CAMERAMANEGER_H_
#define CAMERAMANEGER_H_
#include "../Global/Global.h"
#include "../Global/GlobalParam.h"

#include "../COMM/GSC.h"

#include <hal/boolean.h>

#define AUTO_CAMERA

#define MAX_NUMBER_OF_CAMERA_REQ	10
#define MAX_DATA_CAM_REQ			20
#define AUTO_CAMERA_SLEEP_TIMEOUT	CONVERT_MS_TO_SECONDS(30 * 60)

#define MAX_TIME_TO_KEEP_ON_CAMERA	60 * 10
#define	MIN_TIME_TO_KEEP_ON_CAMERA	10

typedef int duration_t;

typedef enum{
	take_picture,
	delete_picture,
	transfer_image_to_OBC,
	create_thambnail,
	update_photography_values,
	send_picture_to_earth,
	turn_off_auto_pilot,
	turn_on_auto_pilot,
	reset_FRAM_address,
	far_more_requests
}cam_Request_id_t;

typedef struct __attribute__ ((__packed__))
{
	command_id cmd_id;
	cam_Request_id_t id;
	duration_t keepOnCamera;
	byte data[MAX_DATA_CAM_REQ];
}Camera_Request;


/*
 * Initialize all the components of the camera
 * @param[in]	If the first time the satellite code run is now
 * @return 		return error from initGecko()
 */
int initCamera(Boolean firstActivation);

/*
 * create the software components for the camera:
 * starts the tasks, create the queue of requests, create semaphore
 */
int KickStartCamera();

/*
 * The interface of the camera is throw this command, every action you need from the camera
 * and all the other components that related to her need to pass throw this function.
 * @param[in]	The requested action of the camera
 * @return		positive number indicate the number of commands in the Queue
 * @return
 */
int addRequestToQueue(Camera_Request request);

/*
 * Reset the interface Queue
 * @return	0 if reset done correctly
 */
int resetQueue();

#endif /* CAMERAMANEGER_H_ */
