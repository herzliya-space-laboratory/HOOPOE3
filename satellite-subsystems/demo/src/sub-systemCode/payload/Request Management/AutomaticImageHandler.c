/*
 * 	ImageAutomaticHandler.c
 *
 * 	Created on: Sep 19, 2019
 * 	Author: Roy Harel
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <hal/boolean.h>

#include "../../Global/freertosExtended.h"
#include "../../Global/GlobalParam.h"
#include "../../Global/TLM_management.h"
#include "../../Global/logger.h"

#include "../DataBase/DataBase.h"
#include "../Misc/Macros.h"

#include "AutomaticImageHandler.h"

#define AutomaticImageHandlerTask_Name ("Automatic Image Handling Task")
#define AutomaticImageHandlerTask_StackDepth 8192

Boolean automatic_image_handling_state;
xSemaphoreHandle xAIHS;

void create_xAIHS()
{
	vSemaphoreCreateBinary(xAIHS);
}
Boolean get_automatic_image_handling_state()
{
	Boolean ret = FALSE;
	if (xSemaphoreTake_extended(xAIHS, 1000) == pdTRUE)
	{
		ret = automatic_image_handling_state;
		xSemaphoreGive_extended(xAIHS);
	}
	return ret;
}
void set_get_automatic_image_handling_state(Boolean param)
{
	if (xSemaphoreTake_extended(xAIHS, 1000) == pdTRUE)
	{
		automatic_image_handling_state = param;
		xSemaphoreGive_extended(xAIHS);
	}
}

int resumeAction()
{
	set_get_automatic_image_handling_state(TRUE);
	return 0;
}
int stopAction()
{
	TurnOffGecko();
	vTaskDelay(10);
	set_get_automatic_image_handling_state(FALSE);
	return 0;
}

void AutomaticImageHandlerTaskMain()
{
	int error = f_managed_enterFS();
	check_int("AutoImageHandler, enter FS", error);

	while(TRUE)
	{
		error = 0;

		if ( get_system_state(cam_operational_param) && get_automatic_image_handling_state() )
		{
			error = handleMarkedPictures();
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
