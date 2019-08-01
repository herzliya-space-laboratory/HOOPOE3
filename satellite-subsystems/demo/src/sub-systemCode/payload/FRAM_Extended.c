/*
 * FRAM_Extended.c
 *
 *  Created on: 7 ���� 2019
 *      Author: I7COMPUTER
 */

#include <hal/Storage/FRAM.h>

#include "FRAM_Extended.h"

int FRAM_readAndProgress(unsigned char *data, unsigned int *address, unsigned int size)
{
	int result = FRAM_read( data, *address, size);
	*address += size;
	return result;
}

int FRAM_writeAndProgress(unsigned char *data, unsigned int *address, unsigned int size)
{
	int result = FRAM_write( data, *address, size);
	*address += size;
	return result;
}
