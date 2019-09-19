/*
 * output_stream.h
 *
 *  Created on: 12 αιεμ 2019
 *      Author: Tamir
 */

#ifndef OUTPUT_STREAM_H_
#define OUTPUT_STREAM_H_

#include "BooleanJpeg.h"
#include <stdio.h>
#include <stdint.h>

#include <hcc/api_fat.h>

#define JPEG_SINGLES_SIZE	2048

typedef enum
{
	JPEG_FILE,
	JPEG_MEMORY,
} JPEG_OUTPUT_TYPE;

typedef struct
{
	F_FILE * m_pFile;
	BooleanJpeg m_bStatus;

	int singles_count;
	uint8_t * singles_buff;
} jpeg_file_output_stream;
typedef struct
{
	uint8_t * m_pBuf;
	uint m_buf_size, m_buf_ofs;
} jpeg_memory_output_stream;

typedef struct
{
	JPEG_OUTPUT_TYPE type;
	union
	{
		jpeg_file_output_stream file_stream;
		jpeg_memory_output_stream mem_stream;
	} data;
} output_stream;

// Output stream abstract class - used by the jpeg_encoder class to write to the output stream.
// put_buf() is generally called with len==JPGE_OUT_BUF_SIZE bytes, but for headers it'll be called with smaller amounts.

BooleanJpeg open_jpeg_file(output_stream * stream, char *pFilename);

BooleanJpeg close_jpeg_file(output_stream * stream);

BooleanJpeg init_jpeg_mem(output_stream * stream, uint buf_size);

BooleanJpeg close_jpeg_mem(output_stream * stream);

BooleanJpeg put_buf(output_stream * stream, const void* pBuf, int len);

uint get_size(output_stream * stream);

#endif /* OUTPUT_STREAM_H_ */
