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
	printf("-F- enter FS (%u)", f_enterFS());
	/*
	int error = 0;
	do
	{
		error = f_managed_enterFS();
		vTaskDelay(1000);
	} while (error != F_NO_ERROR);
	*/
}

int releaseFS(void)
{
	f_releaseFS();
	/*
	int error = f_managed_releaseFS();
	if (error != 0)
		return error;
	else
		return f_getlasterror();*/
	return 0;
}

int OpenFile(F_FILE** file, const char* fileName, const char* mode)
{
	keepTryingTo_enterFS();

	int errorMessage = 0;/*f_managed_open(fileName, mode, file)*/;
	*file = f_open(fileName, mode);
	errorMessage =f_getlasterror();
	printf("\n-F- OPEN, file system error (%d)\n\n", errorMessage);
	return errorMessage;
}

int CloseFile(F_FILE* file)
{
	int errorMessage = 0;/*f_managed_close(file);*/

	f_close(file);
	printf("\n-F- CLOSE, file system error (%d)\n\n", f_getlasterror());
	f_managed_releaseFS();
	return errorMessage;
}

int DeleteFile(const char* fileName)
{
	keepTryingTo_enterFS();

	int error = f_delete(fileName);
	printf("\n-F- OPEN, file system error (%d)\n\n", f_getlasterror());

	if (error != F_NO_ERROR)
		return error;

	releaseFS();
	return error;
}

int ReadFromFile(F_FILE* file, byte* buffer, uint32_t size_of_element, uint32_t number_of_elements)
{
	uint32_t a = f_read(buffer, size_of_element, number_of_elements, file);
	if (a != number_of_elements)
	{
		printf("\n-F- READ, file system error (%d)\n\n", f_getlasterror());
	}
	return f_getlasterror();
}

int WriteToFile(F_FILE* file, byte* buffer, uint32_t number_of_elements, uint32_t size_of_element)
{
	if ((uint32_t)f_write(buffer, size_of_element, number_of_elements, file) != number_of_elements)
	{
		printf("\n-F- WRITE, file system error (%d)\n\n", f_getlasterror());
		return f_getlasterror();
	}

	int error = f_flush(file);
	if (error)
	{
		printf("\n-F- FLUSH, error (%d)\n\n", error);
		return error;
	}

	return f_getlasterror();
}
