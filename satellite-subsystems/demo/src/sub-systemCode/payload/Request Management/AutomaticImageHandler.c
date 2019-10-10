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

Boolean automatic_image_handling_ready_for_long_term_stop;
xSemaphoreHandle xAutomaticImageHandlingReadyForLongTermStop;	// 1

xTaskHandle automaticImageHandling_taskHandle;
xSemaphoreHandle xAutomaticImageHandling_taskHandle;			// 2

Boolean automatic_image_handling_task_suspension_flag;
xSemaphoreHandle xAutomaticImageHandlingTaskSuspensionFlag;		// 3

void create_xAutomaticImageHandlingTaskSuspensionFlag()
{
	vSemaphoreCreateBinary(xAutomaticImageHandlingTaskSuspensionFlag);
}
Boolean get_automatic_image_handling_task_suspension_flag()
{
	Boolean ret = FALSE;
	if (xSemaphoreTake_extended(xAutomaticImageHandlingTaskSuspensionFlag, 1000) == pdTRUE)
	{
		printf("-S-\tTook Cam Semaphor 3\n");
		ret = automatic_image_handling_task_suspension_flag;
		xSemaphoreGive_extended(xAutomaticImageHandlingTaskSuspensionFlag);
		printf("-S-\tGave Cam Semaphor 3\n");
	}
	return ret;
}
void set_automatic_image_handling_task_suspension_flag(Boolean param)
{
	if (xSemaphoreTake_extended(xAutomaticImageHandlingTaskSuspensionFlag, 1000) == pdTRUE)
	{
		printf("-S-\tTook Cam Semaphor 3\n");
		automatic_image_handling_task_suspension_flag = param;
		xSemaphoreGive_extended(xAutomaticImageHandlingTaskSuspensionFlag);
		printf("-S-\tGave Cam Semaphor 3\n");
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
		printf("-S-\tTook Cam Semaphor 1\n");
		ret = automatic_image_handling_ready_for_long_term_stop;
		xSemaphoreGive_extended(xAutomaticImageHandlingReadyForLongTermStop);
		printf("-S-\tGave Cam Semaphor 1\n");
	}
	return ret;
}
void set_automatic_image_handling_ready_for_long_term_stop(Boolean param)
{
	if (xSemaphoreTake_extended(xAutomaticImageHandlingReadyForLongTermStop, 1000) == pdTRUE)
	{
		printf("-S-\tTook Cam Semaphor 1\n");
		automatic_image_handling_ready_for_long_term_stop = param;
		xSemaphoreGive_extended(xAutomaticImageHandlingReadyForLongTermStop);
		printf("-S-\tGave Cam Semaphor 1\n");
	}
}

void create_xAutomaticImageHandling_taskHandle()
{
	vSemaphoreCreateBinary(xAutomaticImageHandling_taskHandle);
	automaticImageHandling_taskHandle = NULL;
}
void suspendAutomaticImageHandlingTask()
{
	if (xSemaphoreTake_extended(xAutomaticImageHandling_taskHandle, 1000) == pdTRUE && automaticImageHandling_taskHandle != NULL)
	{
		printf("-S-\tTook Cam Semaphor 2\n");
		vTaskSuspend(automaticImageHandling_taskHandle);
		set_automatic_image_handling_task_suspension_flag(TRUE);

		xSemaphoreGive_extended(xAutomaticImageHandling_taskHandle);
		printf("-S-\tGave Cam Semaphor 2\n");
	}
}
void resumeAutomaticImageHandlingTask()
{
	if (xSemaphoreTake_extended(xAutomaticImageHandling_taskHandle, 1000) == pdTRUE && automaticImageHandling_taskHandle != NULL)
	{
		printf("-S-\tTook Cam Semaphor 2\n");
		vTaskResume(automaticImageHandling_taskHandle);
		// Note: The Task Itself sets "automatic_image_handling_task_suspension_flag" back to "FALSE".

		xSemaphoreGive_extended(xAutomaticImageHandling_taskHandle);
		printf("-S-\tGave Cam Semaphor 2\n");
	}
}
Boolean checkIfInAutomaticImageHandlingTask()
{
	Boolean ret = FALSE;
	if (xSemaphoreTake_extended(xAutomaticImageHandling_taskHandle, 1000) == pdTRUE && automaticImageHandling_taskHandle != NULL)
	{
		printf("-S-\tTook Cam Semaphor 2\n");
		xTaskHandle current_task_handle = xTaskGetCurrentTaskHandle();
		if (current_task_handle == automaticImageHandling_taskHandle)
			ret = TRUE;
		else
			ret = FALSE;

		xSemaphoreGive_extended(xAutomaticImageHandling_taskHandle);
		printf("-S-\tGave Cam Semaphor 2\n");
	}

	return ret;
}

int resumeAction()
{
	Boolean state = get_automatic_image_handling_task_suspension_flag();
	if (state == TRUE)
	{
		resumeAutomaticImageHandlingTask();
	}
	return 0;
}
int stopAction()
{
	Boolean state = get_automatic_image_handling_task_suspension_flag();
	if (state == FALSE)
	{
		suspendAutomaticImageHandlingTask();
		TurnOffGecko();
	}
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
		set_automatic_image_handling_task_suspension_flag(FALSE);

		error = 0;

		if ( get_system_state(cam_operational_param) )
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

		set_automatic_image_handling_ready_for_long_term_stop(TRUE);
		vTaskDelay(1000);
	}
}

void initAutomaticImageHandlerTask()
{
	create_xAutomaticImageHandlingTaskSuspensionFlag();
	create_xAutomaticImageHandlingReadyForLongTermStop();
	create_xAutomaticImageHandling_taskHandle();

	create_xGeckoStateSemaphore();
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
