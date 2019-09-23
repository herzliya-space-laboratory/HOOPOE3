/*
 * 	ImageAutomaticHandler.c
 *
 * 	Created on: Sep 19, 2019
 * 	Author: Roy Harel
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <hal/boolean.h>

#include "../../Global/GlobalParam.h"
#include "../../Global/TLM_management.h"
#include "../../Global/logger.h"

#include "../DataBase/DataBase.h"
#include "../Misc/Macros.h"

#include "AutomaticImageHandler.h"

#define AutomaticImageHandlerTask_Name ("Automatic Image Handling Task")
#define AutomaticImageHandlerTask_StackDepth 8192

#define NUMBER_OF_PICTURES_TO_BE_HANDLED_AT_A_TIME 2

Boolean finished;

void AutomaticImageHandlerTaskMain()
{
	int error = f_managed_enterFS();
	check_int("CameraManagerTaskMain, enter FS", error);

	while(TRUE)
	{
		if ( !get_ground_conn() && auto_thumbnail_creation )
		{
			finished = FALSE;
			error = handleMarkedPictures();
			finished = TRUE;
		}

		Gecko_TroubleShooter(error);

		if (error != DataBaseSuccess)
		{
			ImageMetadata image_metadata;
			ImageDataBaseResult result = SearchDataBase_forLatestMarkedImage(&image_metadata);

			if (result != DataBaseSuccess || result != DataBaseIdNotFound)
				WriteErrorLog(error, SYSTEM_PAYLOAD_AUTO_HANDLING, (uint32_t)0);
			else
				WriteErrorLog(error, SYSTEM_PAYLOAD_AUTO_HANDLING, (uint32_t)image_metadata.cameraId);
		}

		vTaskDelay(SYSTEM_DEALY);
	}
}

void KickStartAutomaticImageHandlerTask()
{
	xTaskCreate(AutomaticImageHandlerTaskMain, (const signed char*)AutomaticImageHandlerTask_Name, AutomaticImageHandlerTask_StackDepth, NULL, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, NULL);
	vTaskDelay(SYSTEM_DEALY);
}

int stopAction()
{
	// Stopping Future Thumbnail Creation:
	auto_thumbnail_creation = FALSE_8BIT;

	int error = setStopFlag(TRUE_8BIT);
	CMP_AND_RETURN(error, 0, -1);

	Boolean8bit stop_flag = TRUE_8BIT;

	do
	{
		vTaskDelay(SYSTEM_DEALY);
	} while ( finished != TRUE );

	error = setStopFlag(FALSE_8BIT);
	CMP_AND_RETURN(error, 0, -1);

	return 0;
}

int resumeAction()
{
	auto_thumbnail_creation = TRUE_8BIT;

	int error = setStopFlag(FALSE_8BIT);
	CMP_AND_RETURN(error, 0, -1);

	return 0;
}

void handleErrors(int error)
{
	if (error != 0)
		WriteErrorLog(error, SYSTEM_PAYLOAD_AUTO_HANDLING, (uint32_t)-1);
}
