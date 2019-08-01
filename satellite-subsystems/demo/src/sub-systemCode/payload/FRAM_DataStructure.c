/*
 * FRAM_DataStructure.c
 *
 *  Created on: Jul 26, 2019
 *      Author: Roy's Laptop
 */

#include <hal/Storage/FRAM.h>
#include <stdlib.h>
#include <string.h>

#include "FRAM_Extended.h"

#include "FRAM_DataStructure.h"

int FRAM_DataStructure_search(uint32_t dataStructure_startAddress, uint32_t dataStructure_endAddress,
		uint32_t element_size, void* element, uint32_t* element_address,
		Boolean (*searchCondition)(void*, void*), void* search_param)
{
	uint32_t currentPosition = dataStructure_startAddress;

	while(currentPosition < dataStructure_endAddress)
	{
		FRAM_readAndProgress((unsigned char*)element, (unsigned int*)&currentPosition, (unsigned int)element_size);

		if (searchCondition(element, search_param))
		{
			currentPosition -= element_size;
			memcpy(element_address, &currentPosition, sizeof(uint32_t));
			return 0;
		}
	}

	return -1;
}
