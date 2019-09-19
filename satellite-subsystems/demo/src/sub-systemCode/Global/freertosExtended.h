/*
 * freertosExtended.h
 *
 *  Created on: Aug 29, 2019
 *      Author: elain
 */

#ifndef FREERTOSEXTENDED_H_
#define FREERTOSEXTENDED_H_
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

portBASE_TYPE xSemaphoreGive_extended(xSemaphoreHandle semToGive);
portBASE_TYPE xSemaphoreTake_extended(xSemaphoreHandle semToTake, portTickType maxDelay);

#endif /* FREERTOSEXTENDED_H_ */
