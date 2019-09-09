/*
 * jpeg_memory_alloc.c
 *
 *  Created on: 12 αιεμ 2019
 *      Author: Tamir
 */

#include <string.h>
#include "freertos/portable/MemMang/standardMemMang.h"
#include "jpeg_memory_alloc.h"

void *jpge_malloc(size_t nSize)
{
	return pvPortMalloc(nSize);
}
void *jpge_cmalloc(size_t nSize)
{
	void * p = jpge_malloc(nSize);
	if (p != NULL)
	{
		memset(p, 0, nSize);
	}
	return p;
}
void jpge_free(void *p)
{
	return vPortFree(p);
}
