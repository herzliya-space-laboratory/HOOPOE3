/*
 * Macros.h
 *
 *  Created on: 25 ���� 2019
 *      Author: I7COMPUTER
 */

#ifndef MACROS_H_
#define MACROS_H_

// ToDo: add error log!

#define CMP_AND_RETURN(value1, value2, return_value)	\
		if (value1 != value2)							\
			return return_value;

#define CHECK_FOR_NULL(value, return_value)	\
		CMP_AND_RETURN(value, NULL, return_value);

#endif /* FILEMACROS_H_ */
