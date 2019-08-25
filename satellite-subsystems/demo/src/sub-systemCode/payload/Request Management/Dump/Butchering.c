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
	void printChunk(pixel_t* chunk, uint16_t chunk_width, uint16_t chunk_height)
	{
		uint16_t chunk_size = chunk_width * chunk_height;
		for (unsigned int i = 0; i < chunk_size; i++)
		{
			if (i%chunk_width == 0 && i != 0) printf("\n");
			printf("%u\t", *(chunk + i));
		}
		printf("\n\n");
	}
#endif

ButcherError GetChunkFromImage(pixel_t* chunk, uint16_t chunk_width, uint16_t chunk_height, uint16_t index, pixel_t* image, fileType img_type, uint32_t image_size)
{
	CHECK_FOR_NULL(chunk, BUTCHER_NULL_POINTER);
	CHECK_FOR_NULL(image, BUTCHER_NULL_POINTER);

	uint16_t chunk_size = chunk_width * chunk_height;

	if (img_type == jpg)
	{
		pixel_t* temp_chunk = SimpleButcher(image, image_size, chunk_size, index);
		memcpy(chunk, temp_chunk, chunk_size);
		free(temp_chunk);
		return BUTCHER_SUCCSESS;
	}

	unsigned int factor = GetImageFactor(img_type);

	uint32_t effective_im_height = IMAGE_HEIGHT / factor;
	uint32_t effective_im_width = IMAGE_WIDTH / factor;
	uint32_t effective_im_size = IMAGE_SIZE / pow(factor,2);

	uint32_t num_of_chunks_Y = effective_im_height / chunk_height;	// rounded down (without the reminder!)
	uint32_t num_of_chunks_X = effective_im_width / chunk_width;

	if ( effective_im_width % chunk_width != 0 )
		return Butcher_Parameter_Value;

	uint32_t height_offset = (index / num_of_chunks_X) * chunk_height;
	uint32_t width_offset = (index % num_of_chunks_X) * chunk_width;

	if (effective_im_height - height_offset < chunk_height)
	{
		uint32_t reminder_section_index = index - (num_of_chunks_Y * num_of_chunks_X);

		uint32_t reminder_height_offset = num_of_chunks_Y * chunk_height;

		uint32_t reminder_section_offset = reminder_height_offset * effective_im_width;
		pixel_t* reminder_section_strarting_point = (pixel_t*)image + reminder_section_offset;

		pixel_t* buffer = SimpleButcher(reminder_section_strarting_point, effective_im_size, chunk_size, reminder_section_index);
		memcpy(chunk, buffer, chunk_size);
	}
	else
	{
		for (uint32_t y = 0; y < chunk_height; y++)
		{
			memcpy(chunk + (y*chunk_width), image + ((height_offset + y) * effective_im_width) + (width_offset), chunk_width);
		}
	}

	return BUTCHER_SUCCSESS;
}

pixel_t* SimpleButcher(pixel_t* data, uint32_t sizeofData, uint32_t size, uint16_t index)
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
