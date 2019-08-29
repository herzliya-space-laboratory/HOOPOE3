/*
 * freertosExtended.c
 *
 *  Created on: Aug 29, 2019
 *      Author: elain
 */
#include "freertosExtended.h"

portBASE_TYPE xSemaphoreGive_extended(xSemaphoreHandle semToGive)
{
	if (xSemaphoreGive(semToGive))
		return pdTRUE;
	else
		return pdFALSE;
}

portBASE_TYPE xSemaphoreTake_extended(xSemaphoreHandle semToTake, portTickType maxDelay)
{
	if (xSemaphoreTake(semToTake, maxDelay))
		return pdTRUE;
	else
		xSemaphoreGive_extended(semToTake);

	return pdFALSE;
}
