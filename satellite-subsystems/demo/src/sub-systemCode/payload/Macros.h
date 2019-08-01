/*
 * Macros.h
 *
 *  Created on: 25 αιεμ 2019
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
		file = f_open(fileName, mode);					\
		CHECK_FOR_NULL(file, errorMessage);

#define READ_FROM_FILE(file, buffer, size_of_element, number_of_elements, errorMessage)							\
		if (f_read(buffer, size_of_element, number_of_elements, file) != size_of_element * number_of_elements)	\
			return errorMessage;

#define WRITE_TO_FILE(file, buffer, size_of_element, number_of_elements, errorMessage)							\
		if (f_write(buffer, size_of_element, number_of_elements, file) != (long)size_of_element * number_of_elements)	\
			return errorMessage;

#define CLOSE_FILE(file, errorMessage)	\
		if (f_close(file))				\
			return errorMessage;

#define DELETE_FILE(fileName, errorMessage)	\
		if (f_delete(fileName))				\
			return errorMessage;

#endif /* FILEMACROS_H_ */
