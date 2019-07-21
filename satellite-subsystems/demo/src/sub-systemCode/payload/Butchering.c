#include "Butchering.h"

#ifndef NULL
	#define NULL ((void*)0)
#endif

#ifdef TESTING
void printChunk(chunk_t chnk)
{
	for (unsigned int i = 0; i < CHUNK_SIZE; i++)
	{
		if (i%CHUNK_WIDTH == 0 && i != 0) printf("\n");
		printf("%d\t", chnk[i]);
	}
	printf("\n\n");
}

void initImage(pixel_t *im)
{
	for (unsigned int i = 0; i < IMAGE_SIZE; i++)
	{
		im[i] = i;
	}
}

#endif



int GetChunksFromRange(chunk_t *chunks, unsigned int index_start, unsigned int index_end, pixel_t image[IMAGE_SIZE], fileType im_type)
{
	if (NULL == image || NULL == chunks)
	{
		return BUTCHER_NULL_POINTER;
	}

	if (index_start > NUM_OF_CHUNKS || index_start > index_end)
	{
		return BUTHCER_PARAMATER_VALUE;
	}

	int err = 0;
	for (unsigned int i = index_start; i < index_end; i++)
	{
		err = GetChunkFromImage(chunks[i - index_start], i, image,im_type);
		if (0 != err)
		{
			return BUTCHER_UNDEFINED_ERR;
		}
	}
	return BUTCHER_SUCCSESS;
}

int GetChunkFromImage(chunk_t chunk, unsigned int index, pixel_t image[IMAGE_SIZE], fileType im_type)
{
	unsigned int factor = 1;
	switch(im_type)
	{
		case raw: factor = 1; break;
		case t02: factor = 2; break;
		case t04: factor = 4; break;
		case t08: factor = 8; break;
		case t16: factor = 16; break;
		case t32: factor = 32; break;
		case t64: factor = 64; break;
		default: return BUTHCER_PARAMATER_VALUE;

	}
	if (NULL == chunk || NULL == image)
	{
		return BUTCHER_NULL_POINTER;
	}

	if (index > NUM_OF_CHUNKS - 1)
	{
		return BUTHCER_PARAMATER_VALUE;
	}
	unsigned int effective_im_height = IMAGE_HEIGHT / factor;
	unsigned int effective_im_width = IMAGE_WIDTH / factor;

	unsigned int num_of_chunks_H = effective_im_height / CHUNK_HEIGHT;
	unsigned int num_of_chunks_X = effective_im_width / CHUNK_WIDTH;

	unsigned int height_offset = (index / num_of_chunks_X)* CHUNK_HEIGHT;
	unsigned int width_offset = (index % num_of_chunks_X)* CHUNK_WIDTH;

	for (int i = 0; i < CHUNK_WIDTH; ++i)
	{
		for (int j = 0; j < CHUNK_HEIGHT; ++j)
		{
			chunk[i + j * CHUNK_WIDTH] = image[(i + width_offset + (j + height_offset)*effective_im_width)];
		}
	}
	return BUTCHER_SUCCSESS;
}

int InsertChunkToImage(pixel_t image[IMAGE_SIZE], chunk_t chunk, int index)
{
	if (NULL == image || NULL == chunk)
	{
		return BUTCHER_NULL_POINTER;
	}
	if (index < 0 || index > NUM_OF_CHUNKS)
	{
		return BUTHCER_PARAMATER_VALUE;
	}
	unsigned int num_of_chunks_H = IMAGE_HEIGHT / CHUNK_HEIGHT;
	unsigned int num_of_chunks_X = IMAGE_WIDTH / CHUNK_WIDTH;

	unsigned int height_offset = (index / num_of_chunks_X)* CHUNK_HEIGHT;
	unsigned int width_offset = (index % num_of_chunks_X)* CHUNK_WIDTH;

	for (unsigned int i = 0; i < CHUNK_WIDTH; i++)
	{
		for (unsigned int j = 0; j < CHUNK_HEIGHT; j++)
		{
			image[(i + width_offset + (j + height_offset)*IMAGE_WIDTH)] = chunk[i + j * CHUNK_WIDTH];
		}
	}

	return BUTCHER_SUCCSESS;
}

int GetDataRegionFromBuffer(unsigned char *data, unsigned char *buffer, unsigned int *start_index,unsigned int data_length)
{
	if (NULL == data || NULL == buffer || NULL == start_index)
	{
		return BUTCHER_NULL_POINTER;
	}

	for (unsigned int i = 0; i < data_length; i++)
	{
		buffer[i] = data[i + *start_index];
	}
	UpdateDataCursur(start_index, data_length);
	return BUTCHER_SUCCSESS;

}

int UpdateDataCursur(unsigned int *start_index, unsigned int data_length)
{
	if (NULL == start_index)
	{
		return BUTCHER_NULL_POINTER;
	}
	*start_index += data_length;

	return BUTCHER_SUCCSESS;
}
