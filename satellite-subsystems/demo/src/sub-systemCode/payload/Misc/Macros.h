/*
 * Macros.h
 *
 *  Created on: 25 ���� 2019
 *      Author: I7COMPUTER
 */

#ifndef MACROS_H_
#define MACROS_H_

// ToDo: add error log! (use f_getlasterror())

#define CMP_AND_RETURN(value1, value2, return_value)	\
		if (value1 != value2)							\
			return return_value;

#define CHECK_FOR_NULL(value, return_value)	\
		if (value == NULL)					\
			return return_value;

#define OPEN_FILE(file, fileName, mode, errorMessage)	\
		f_enterFS();\
		file = f_open(fileName, mode);					\
		CHECK_FOR_NULL(file, errorMessage);

#define READ_FROM_FILE(file, buffer, size_of_element, number_of_elements, errorMessage)												\
		if ((uint32_t)f_read(buffer, size_of_element, number_of_elements, file) != (uint32_t)size_of_element * number_of_elements)	\
		{																															\
			printf("\n-F- file system error (%d)\n\n", f_getlasterror());															\
			return errorMessage;																									\
		}

#define WRITE_TO_FILE(file, buffer, size_of_element, number_of_elements, errorMessage)												\
		if ((uint32_t)f_write(buffer, size_of_element, number_of_elements, file) != (uint32_t)size_of_element * number_of_elements)	\
		{																															\
			printf("\n-F- file system error (%d)\n\n", f_getlasterror());															\
			return errorMessage;																									\
		}

#define FLUSH_FILE(file, errorMessage)										\
		if (f_flush(file) != 0)												\
		{																	\
			printf("\n-F- file system error (%d)\n\n", f_getlasterror());	\
			return errorMessage;											\
		}

#define CLOSE_FILE(file, errorMessage)	\
		if (f_close(file))				\
		{								\
			printf("\n-F- file system error (%d)\n\n", f_getlasterror());	\
			f_releaseFS();\
			return errorMessage;		\
		}\
		f_releaseFS();

#define DELETE_FILE(fileName, errorMessage)	\
		f_enterFS();\
		if (f_delete(fileName))				\
		{									\
			f_releaseFS();\
			return errorMessage;			\
		}\
		f_releaseFS();

#endif /* FILEMACROS_H_ */
