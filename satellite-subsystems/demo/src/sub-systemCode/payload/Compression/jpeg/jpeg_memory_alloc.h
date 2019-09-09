/*
 * jpeg_memory_alloc.h
 *
 *  Created on: 12 αιεμ 2019
 *      Author: Tamir
 */

#ifndef JPEG_MEMORY_ALLOC_H_
#define JPEG_MEMORY_ALLOC_H_

#include <stdlib.h>

// Low-level helper functions.
#define JPGE_ALLOC_CHECK(exp) do { if ((exp) == NULL) return FALSE_JPEG; } while(0)
void *jpge_malloc(size_t nSize);
void *jpge_cmalloc(size_t nSize);
void jpge_free(void *p);

#endif /* JPEG_MEMORY_ALLOC_H_ */
