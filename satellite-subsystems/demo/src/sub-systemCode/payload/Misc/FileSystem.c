/*
 * FileSystem.c
 *
 *  Created on: Aug 15, 2019
 *      Author: Roy's Laptop
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "FileSystem.h"

void keepTryingTo_enterFS(void)
{
	int error = 0;
	do
	{
		error = f_managed_enterFS();
		vTaskDelay(1000);
	} while (error != F_NO_ERROR);
}
int releaseFS(void)
{
	int error = f_managed_releaseFS();
	if (error != 0)
		return error;
	else
		return f_getlasterror();
}

int OpenFile(F_FILE** file, char* fileName, char* mode)
{
	keepTryingTo_enterFS();

	return f_managed_open(fileName, mode, file);
}
int CloseFile(F_FILE** file)
{
	int errorMessage = f_managed_close(file);
	if (errorMessage)
	{
		return errorMessage;
	}

	return f_managed_releaseFS();
}
int DeleteFile(char* fileName)
{
	keepTryingTo_enterFS();

	int error = f_delete(fileName);
	if (error != F_NO_ERROR)
		return error;

	error = releaseFS();
	return error;
}

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
