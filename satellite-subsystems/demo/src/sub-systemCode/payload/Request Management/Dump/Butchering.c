/*
 *	Butchering.c
 *
 *	Created on: 7 ���� 2019
 *	Author: Roy Harel
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <math.h>

#include "Butchering.h"

#define TESTING
#ifdef TESTING
	void printChunk(pixel_t* chunk)
	{
		for (unsigned int i = 0; i < CHUNK_SIZE; i++)
		{
			if (i%CHUNK_WIDTH == 0 && i != 0) printf("\n");
			printf("%u\t", *(chunk + i));
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


ButcherError GetChunksFromRange(chunk_t* chunks, unsigned int index_start, unsigned int index_end, image_t image, fileType im_type, uint32_t image_size)
{
	if ( image == NULL || chunks == NULL )
	{
		return BUTCHER_NULL_POINTER;
	}

	unsigned int effective_im_size = IMAGE_SIZE / pow(pow(2,im_type), 2);
	if ( (im_type != jpg) && (index_start > ceil(effective_im_size / CHUNK_SIZE) || index_start > index_end) )
	{
		return BUTHCER_PARAMATER_VALUE;
	}

	int err = 0;
	for (unsigned int i = index_start; i < index_end; i++)
	{
		err = GetChunkFromImage(chunks[i - index_start], i, image, im_type, image_size);
		if (err != BUTCHER_SUCCSESS)
		{
			return err;
		}

		vTaskDelay(100);
	}
	return BUTCHER_SUCCSESS;
}

ButcherError GetChunkFromImage(chunk_t chunk, unsigned int index, image_t image, fileType img_type, uint32_t image_size)
{
	if (img_type == jpg)
	{
		pixel_t* temp_chunk = SimpleButcher((pixel_t*)image, image_size, CHUNK_SIZE, index);
		memcpy(chunk, temp_chunk, CHUNK_SIZE);
		return BUTCHER_SUCCSESS;
	}

	unsigned int factor = 1;
	switch(img_type)
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

	unsigned int effective_im_height = IMAGE_HEIGHT / factor;
	unsigned int effective_im_width = IMAGE_WIDTH / factor;
	unsigned int effective_im_size = IMAGE_SIZE / pow(factor,2);

	unsigned int num_of_chunks_Y = effective_im_height / CHUNK_HEIGHT;	// rounded down (without the reminder!)
	unsigned int num_of_chunks_X = effective_im_width / CHUNK_WIDTH;

	if ( effective_im_width % CHUNK_WIDTH != 0 )
		return Butcher_Parameter_Value;


	unsigned int height_offset = (index / num_of_chunks_X) * CHUNK_HEIGHT;
	unsigned int width_offset = (index % num_of_chunks_X) * CHUNK_WIDTH;

	if (effective_im_height - height_offset < CHUNK_HEIGHT)
	{
		unsigned int reminder_section_index = index - (num_of_chunks_Y * num_of_chunks_X);

		unsigned int reminder_height_offset = num_of_chunks_Y * CHUNK_HEIGHT;

		unsigned int reminder_section_offset = reminder_height_offset * effective_im_width;
		pixel_t* reminder_section_strarting_point = (pixel_t*)image + reminder_section_offset;

		pixel_t* buffer = SimpleButcher(reminder_section_strarting_point, effective_im_size, CHUNK_SIZE, reminder_section_index);
		memcpy(chunk, buffer, CHUNK_SIZE);
	}
	else
	{
		for (int i = 0; i < CHUNK_HEIGHT; i++)
		{
			for (int j = 0; j < CHUNK_WIDTH; j++)
			{
				chunk[i][j] = image[height_offset + i][width_offset + j];
			}
		}
	}

	return BUTCHER_SUCCSESS;
}

pixel_t* SimpleButcher(pixel_t* data, unsigned int sizeofData, unsigned int size, unsigned int index)
{
	if (size*index >= sizeofData + size)
		return NULL;

	pixel_t* buffer = malloc(size);

	memcpy(buffer, data + size*index, size);
/*
	printf("\n");
	for (unsigned int i = 0; i < size; i++)
	{
		printf("%u ", *(buffer + i));
	}
	printf("\n");
*/
	if (size*index > sizeofData)
	{
		byte zero = 0;
		memcpy(buffer + (sizeofData - size*index), &zero, size - (sizeofData - size*index));
	}

	return buffer;
}
