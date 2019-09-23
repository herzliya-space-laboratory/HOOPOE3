/*
 * CameraManeger.h
 *
 *  Created on: Jun 2, 2019
 *      Author: DBTn
 *
 *      this file is in charge off all the camera
 */

#ifndef CAMERAMANEGER_H_
#define CAMERAMANEGER_H_
#include "../../Global/Global.h"
#include "../../Global/GlobalParam.h"

#include "../../COMM/GSC.h"
#include "../../Global/sizes.h"

#include <hal/boolean.h>

#define MAX_NUMBER_OF_CAMERA_REQ	25

#define AUTO_CAMERA_SLEEP_TIMEOUT	CONVERT_MS_TO_SECONDS(30 * 60)

#define MAX_TIME_TO_KEEP_ON_CAMERA	(60 * 10)
#define	MIN_TIME_TO_KEEP_ON_CAMERA	10

#define DEFALT_DURATION 15	// seconds

typedef int duration_t;

typedef enum{
	take_image,
	take_image_with_special_values,
	take_image_with_time_intervals,
	delete_image_file,
	delete_image,
	transfer_image_to_OBC,
	create_thumbnail,
	create_jpg,
	update_photography_values,
	reset_DataBase,
	image_Dump_chunkField,
	image_Dump_bitField,
	DataBase_Dump,
	update_defult_duration,
	set_chunk_size,
	stop_take_image_with_time_intervals,
	turn_off_camera,
	turn_on_camera,
	turn_off_future_AutoThumbnailCreation,
	turn_on_future_AutoThumbnailCreation,
	turn_off_AutoThumbnailCreation,
	turn_on_AutoThumbnailCreation,
	number_of_request_types
}cam_Request_id_t;

typedef struct __attribute__ ((__packed__))
{
	command_id cmd_id;
	cam_Request_id_t id;
	duration_t keepOnCamera;
	byte data[SPL_TC_DATA_SIZE];
}Camera_Request;

void CameraManagerTaskMain();

void handleDump(Camera_Request request);

/*
 * Initialize all the components of the camera
 * @param[in]	If the first time the satellite code run is now
 * @return 		return error from initGecko()
 */
int initCamera(Boolean first_activation);

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
