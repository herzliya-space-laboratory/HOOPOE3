#include <stdio.h>
#include <string.h>
#include "output_stream.h"
#include "jpeg_memory_alloc.h"
#include <hcc/api_fat.h>

#include "../../Misc/FileSystem.h"

BooleanJpeg open_jpeg_file(output_stream * stream, char *pFilename)
{
	if (stream->type != JPEG_FILE)
		return FALSE_JPEG;

	close_jpeg_file(stream);

	stream->data.file_stream.singles_count = 0;
	stream->data.file_stream.singles_buff = (uint8_t *)jpge_cmalloc(JPEG_SINGLES_SIZE);
	stream->data.file_stream.m_bStatus = (stream->data.file_stream.singles_buff != NULL);

	stream->data.file_stream.m_pFile = NULL;
	int error = OpenFile(&stream->data.file_stream.m_pFile, pFilename, "w");
	stream->data.file_stream.m_bStatus = stream->data.file_stream.m_bStatus && (stream->data.file_stream.m_pFile != NULL) && !error;
	return stream->data.file_stream.m_bStatus;
}

BooleanJpeg close_jpeg_file(output_stream * stream)
{
	if (stream->type != JPEG_FILE)
			return FALSE_JPEG;

	if (stream->data.file_stream.m_pFile)
	{
		if (stream->data.file_stream.singles_count > 0) // empty the singles if any
		{
			stream->data.file_stream.m_bStatus = stream->data.file_stream.m_bStatus &&
											(f_write((void *)stream->data.file_stream.singles_buff,
													stream->data.file_stream.singles_count, 1, stream->data.file_stream.m_pFile) == 1);
			stream->data.file_stream.singles_count = 0;
		}

		f_flush(stream->data.file_stream.m_pFile);
		if (CloseFile(&stream->data.file_stream.m_pFile) != 0)
		{
			stream->data.file_stream.m_bStatus = FALSE_JPEG;
		}
	}

	stream->data.file_stream.singles_count = 0;
	jpge_free(stream->data.file_stream.singles_buff);

	return stream->data.file_stream.m_bStatus;
}

BooleanJpeg init_jpeg_mem(output_stream * stream, uint buf_size)
{
	if (stream->type != JPEG_MEMORY)
		return FALSE_JPEG;

	stream->data.mem_stream.m_buf_size = buf_size;
	stream->data.mem_stream.m_buf_ofs = 0;
	stream->data.mem_stream.m_pBuf = (uint8_t *)jpge_cmalloc(buf_size);

	return stream->data.mem_stream.m_pBuf != NULL;
}

BooleanJpeg close_jpeg_mem(output_stream * stream)
{
	if (stream->type != JPEG_MEMORY)
			return FALSE_JPEG;

	stream->data.mem_stream.m_buf_size = 0;
	stream->data.mem_stream.m_buf_ofs = 0;
	jpge_free(stream->data.mem_stream.m_pBuf);
	return TRUE_JPEG;
}

BooleanJpeg put_buf(output_stream * stream, const void* pBuf, int len)
{
	if (stream->type == JPEG_FILE)
	{
		// write only one
		if (len == 1)
		{
			// no space
			if (stream->data.file_stream.singles_count >= JPEG_SINGLES_SIZE)
			{
				stream->data.file_stream.m_bStatus = stream->data.file_stream.m_bStatus &&
											(f_write((void *)stream->data.file_stream.singles_buff,
													stream->data.file_stream.singles_count, 1, stream->data.file_stream.m_pFile) == 1);
				stream->data.file_stream.singles_count = 0;
			}

			stream->data.file_stream.singles_buff[stream->data.file_stream.singles_count++] = *(char *)pBuf;

			return stream->data.file_stream.m_bStatus;
		}
		// else - write more than one byte

		// empty the singles if any
		if (stream->data.file_stream.singles_count > 0)
		{
			stream->data.file_stream.m_bStatus = stream->data.file_stream.m_bStatus &&
											(f_write((void *)stream->data.file_stream.singles_buff,
													stream->data.file_stream.singles_count, 1, stream->data.file_stream.m_pFile) == 1);
			stream->data.file_stream.singles_count = 0;
		}

		// write the many new bytes
		stream->data.file_stream.m_bStatus = stream->data.file_stream.m_bStatus && (f_write(pBuf, len, 1, stream->data.file_stream.m_pFile) == 1);

		return stream->data.file_stream.m_bStatus;
	}
	else if (stream->type == JPEG_MEMORY)
	{
		uint buf_remaining = stream->data.mem_stream.m_buf_size - stream->data.mem_stream.m_buf_ofs;
		if ((uint)len > buf_remaining)
			return FALSE_JPEG;
		memcpy(stream->data.mem_stream.m_pBuf + stream->data.mem_stream.m_buf_ofs, pBuf, len);
		stream->data.mem_stream.m_buf_ofs += len;
		return TRUE_JPEG;
	}

	return FALSE_JPEG;
}

uint get_size(output_stream * stream)
{
	if (stream->type == JPEG_FILE)
		return stream->data.file_stream.m_pFile ? f_tell(stream->data.file_stream.m_pFile) : 0;
	else if (stream->type == JPEG_MEMORY)
		return stream->data.mem_stream.m_buf_ofs;
	return 0;
}
