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

xTaskHandle automaticImageHandling_taskHandle;
xSemaphoreHandle xAutomaticImageHandling_taskHandle;

Boolean automatic_image_handling_ready_for_long_term_stop;
xSemaphoreHandle xAutomaticImageHandlingReadyForLongTermStop;

Boolean automatic_image_handling_task_suspension_flag;
xSemaphoreHandle xAutomaticImageHandlingTaskSuspensionFlag;

void create_xAutomaticImageHandlingTaskSuspensionFlag()
{
	vSemaphoreCreateBinary(xAutomaticImageHandlingTaskSuspensionFlag);
}
Boolean get_automatic_image_handling_task_suspension_flag()
{
	Boolean ret = FALSE;
	if (xSemaphoreTake_extended(xAutomaticImageHandlingTaskSuspensionFlag, 1000) == pdTRUE)
	{
		ret = automatic_image_handling_task_suspension_flag;
		xSemaphoreGive_extended(xAutomaticImageHandlingTaskSuspensionFlag);
	}
	return ret;
}
void set_automatic_image_handling_task_suspension_flag(Boolean param)
{
	if (xSemaphoreTake_extended(xAutomaticImageHandlingTaskSuspensionFlag, 1000) == pdTRUE)
	{
		automatic_image_handling_task_suspension_flag = param;
		xSemaphoreGive_extended(xAutomaticImageHandlingTaskSuspensionFlag);
	}
}

void create_xAutomaticImageHandlingReadyForLongTermStop()
{
	vSemaphoreCreateBinary(xAutomaticImageHandlingReadyForLongTermStop);
}
Boolean get_automatic_image_handling_ready_for_long_term_stop()
{
	Boolean ret = FALSE;
	if (xSemaphoreTake_extended(xAutomaticImageHandlingReadyForLongTermStop, 1000) == pdTRUE)
	{
		ret = automatic_image_handling_ready_for_long_term_stop;
		xSemaphoreGive_extended(xAutomaticImageHandlingReadyForLongTermStop);
	}
	return ret;
}
void set_automatic_image_handling_ready_for_long_term_stop(Boolean param)
{
	if (xSemaphoreTake_extended(xAutomaticImageHandlingReadyForLongTermStop, 1000) == pdTRUE)
	{
		automatic_image_handling_ready_for_long_term_stop = param;
		xSemaphoreGive_extended(xAutomaticImageHandlingReadyForLongTermStop);
	}
}

void create_xAutomaticImageHandling_taskHandle()
{
	vSemaphoreCreateBinary(xAutomaticImageHandling_taskHandle);
}
void suspendAutomaticImageHandlingTask()
{
	if (xSemaphoreTake_extended(xAutomaticImageHandling_taskHandle, 1000) == pdTRUE)
	{
		vTaskSuspend(automaticImageHandling_taskHandle);
		set_automatic_image_handling_task_suspension_flag(TRUE);

		xSemaphoreGive_extended(xAutomaticImageHandling_taskHandle);
	}
}
void resumeAutomaticImageHandlingTask()
{
	if (xSemaphoreTake_extended(xAutomaticImageHandling_taskHandle, 1000) == pdTRUE)
	{
		vTaskResume(automaticImageHandling_taskHandle);
		// Note: The Task Itself sets "automatic_image_handling_task_suspension_flag" back to "FALSE".

		xSemaphoreGive_extended(xAutomaticImageHandling_taskHandle);
	}
}

int resumeAction()
{
	resumeAutomaticImageHandlingTask();
	return 0;
}
int stopAction()
{
	suspendAutomaticImageHandlingTask();
	TurnOffGecko();
	vTaskDelay(100);
	return 0;
}

void AutomaticImageHandlerTaskMain()
{
	int error = f_managed_enterFS();
	check_int("AutoImageHandler, enter FS", error);

	while(TRUE)
	{
		set_automatic_image_handling_ready_for_long_term_stop(FALSE);
		error = 0;

		if ( get_system_state(cam_operational_param) )
		{
			set_automatic_image_handling_task_suspension_flag(FALSE);
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

		set_automatic_image_handling_ready_for_long_term_stop(TRUE);
		vTaskDelay(1000);
	}
}

void initAutomaticImageHandlerTask()
{
	create_xAutomaticImageHandlingTaskSuspensionFlag();
	create_xAutomaticImageHandlingReadyForLongTermStop();
	create_xAutomaticImageHandling_taskHandle();
}

void KickStartAutomaticImageHandlerTask()
{
	if (xSemaphoreTake_extended(xAutomaticImageHandling_taskHandle, 1000) == pdTRUE)
	{
		xTaskCreate(AutomaticImageHandlerTaskMain, (const signed char*)AutomaticImageHandlerTask_Name, AutomaticImageHandlerTask_StackDepth, NULL, (unsigned portBASE_TYPE)TASK_DEFAULT_PRIORITIES, &automaticImageHandling_taskHandle);
		vTaskDelay(SYSTEM_DEALY);

		xSemaphoreGive_extended(xAutomaticImageHandling_taskHandle);
	}
}
