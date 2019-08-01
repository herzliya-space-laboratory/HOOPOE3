/*
 * FRAM_DataStructure_Tools.h
 *
 *  Created on: Jul 26, 2019
 *      Author: Roy's Laptop
 */

#ifndef FRAM_DATASTRUCTURE_TOOLS_H_
#define FRAM_DATASTRUCTURE_TOOLS_H_

#include <hal/Boolean.h>
#include <stdint.h>

int FRAM_DataStructure_search(uint32_t dataStructure_startAddress, uint32_t dataStructure_endAddress,
		uint32_t element_size, void* element, uint32_t* element_address,
		Boolean (*searchCondition)(void*, void*), void* search_param);

#endif /* FRAM_DATASTRUCTURE_TOOLS_H_ */
