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

ButcherError GetChunkFromImage(pixel_t* chunk, uint16_t chunk_width, uint16_t chunk_height, uint16_t index, pixel_t* image, fileType img_type, uint32_t image_size)
{
	CHECK_FOR_NULL(chunk, BUTCHER_NULL_POINTER);
	CHECK_FOR_NULL(image, BUTCHER_NULL_POINTER);

	uint16_t chunk_size = chunk_width * chunk_height;

	if (img_type == jpg)
	{
		return SimpleButcher(image, chunk, image_size, chunk_size, index);
	}

	unsigned int factor = GetImageFactor(img_type);

	uint32_t effective_im_height = IMAGE_HEIGHT / factor;
	uint32_t effective_im_width = IMAGE_WIDTH / factor;
	uint32_t effective_im_size = IMAGE_SIZE / pow(factor,2);

	uint32_t num_of_chunks_Y = effective_im_height / chunk_height;	// rounded down (without the reminder!)
	uint32_t num_of_chunks_X = effective_im_width / chunk_width;

	if ( effective_im_width % chunk_width != 0 )
		return BUTCHER_PARAMATER_VALUE;

	uint32_t height_offset = (index / num_of_chunks_X) * chunk_height;
	uint32_t width_offset = (index % num_of_chunks_X) * chunk_width;

	if (effective_im_height - height_offset < chunk_height)
	{
		uint32_t reminder_section_index = index - (num_of_chunks_Y * num_of_chunks_X);

		uint32_t reminder_height = num_of_chunks_Y * chunk_height;

		uint32_t reminder_section_offset = reminder_height * effective_im_width;
		pixel_t* reminder_section_strarting_point = (pixel_t*)image + reminder_section_offset;

		ButcherError err = SimpleButcher(reminder_section_strarting_point, chunk, effective_im_size, chunk_size, reminder_section_index);
		CMP_AND_RETURN(err, BUTCHER_SUCCSESS, err);
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

ButcherError SimpleButcher(pixel_t* data, pixel_t* chunk, uint32_t sizeofData, uint32_t size, uint16_t index)
{
	if (size*index >= sizeofData + size)
		return BUTCHER_OUT_OF_BOUNDS;

	memcpy(chunk, data + size*index, size);

	if (size*index > sizeofData)
	{
		memset(chunk + (sizeofData - size*index), 0, size - (sizeofData - size*index));
	}

	return BUTCHER_SUCCSESS;
}
