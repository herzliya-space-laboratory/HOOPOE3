/*
 * FileSystem.c
 *
 *  Created on: Aug 15, 2019
 *      Author: Roy's Laptop
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "FileSystem.h"

int ReadFromFile(F_FILE* file, byte* buffer, uint32_t size_of_element, uint32_t number_of_elements)
{
	uint32_t ret = f_read(buffer, size_of_element, number_of_elements, file);
	if (ret != number_of_elements)
	{
		return -1;
	}

	return f_getlasterror();
}
int WriteToFile(F_FILE* file, byte* buffer, uint32_t number_of_elements, uint32_t size_of_element)
{
	uint32_t ret = f_write(buffer, size_of_element, number_of_elements, file);
	if (ret != number_of_elements)
	{
		return -1;
	}

	f_flush(file);

	return f_getlasterror();
}
