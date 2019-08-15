/*
 *	Butchering.c
 *
 *	Created on: 7 ���� 2019
 *	Author: Roy Harel
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <math.h>

#include "../../Misc/Macros.h"
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
#endif

ButcherError GetChunkFromImage(pixel_t* chunk, uint32_t index, pixel_t* image, fileType img_type, uint32_t image_size)
{
	CHECK_FOR_NULL(chunk, BUTCHER_NULL_POINTER);
	CHECK_FOR_NULL(image, BUTCHER_NULL_POINTER);

	if (img_type == jpg)
	{
		pixel_t* temp_chunk = SimpleButcher(image, image_size, CHUNK_SIZE, index);
		memcpy(chunk, temp_chunk, CHUNK_SIZE);
		free(temp_chunk);
		return BUTCHER_SUCCSESS;
	}

	unsigned int factor = GetImageFactor(img_type);

	uint32_t effective_im_height = IMAGE_HEIGHT / factor;
	uint32_t effective_im_width = IMAGE_WIDTH / factor;
	uint32_t effective_im_size = IMAGE_SIZE / pow(factor,2);

	uint32_t num_of_chunks_Y = effective_im_height / CHUNK_HEIGHT;	// rounded down (without the reminder!)
	uint32_t num_of_chunks_X = effective_im_width / CHUNK_WIDTH;

	if ( effective_im_width % CHUNK_WIDTH != 0 )
		return Butcher_Parameter_Value;


	uint32_t height_offset = (index / num_of_chunks_X) * CHUNK_HEIGHT;
	uint32_t width_offset = (index % num_of_chunks_X) * CHUNK_WIDTH;

	if (effective_im_height - height_offset < CHUNK_HEIGHT)
	{
		uint32_t reminder_section_index = index - (num_of_chunks_Y * num_of_chunks_X);

		uint32_t reminder_height_offset = num_of_chunks_Y * CHUNK_HEIGHT;

		uint32_t reminder_section_offset = reminder_height_offset * effective_im_width;
		pixel_t* reminder_section_strarting_point = (pixel_t*)image + reminder_section_offset;

		pixel_t* buffer = SimpleButcher(reminder_section_strarting_point, effective_im_size, CHUNK_SIZE, reminder_section_index);
		memcpy(chunk, buffer, CHUNK_SIZE);
	}
	else
	{
		for (uint32_t y = 0; y < CHUNK_HEIGHT; y++)
		{
			/*
			for (uint32_t x = 0; x < CHUNK_WIDTH; x++)
			{
				*(chunk + (y*CHUNK_WIDTH) + (x) ) = *(image + (height_offset + y*CHUNK_WIDTH) + (width_offset + x) );
			}
			*/
			memcpy(chunk + (y*CHUNK_WIDTH), image + (height_offset + y*CHUNK_WIDTH) + (width_offset), CHUNK_WIDTH);
		}
	}

	return BUTCHER_SUCCSESS;
}

pixel_t* SimpleButcher(pixel_t* data, uint32_t sizeofData, uint32_t size, uint32_t index)
{
	if (size*index >= sizeofData + size)
		return NULL;

	pixel_t* buffer = malloc(size);

	memcpy(buffer, data + size*index, size);

	if (size*index > sizeofData)
	{
		byte zero = 0;
		memcpy(buffer + (sizeofData - size*index), &zero, size - (sizeofData - size*index));
	}

	return buffer;
}
