// jpge.cpp - C++ class for JPEG compression.
// Public domain, Rich Geldreich <richgel99@gmail.com>
// v1.00 - Last updated Dec. 18, 2010
// Note: The current DCT function was derived from an old version of libjpeg.

#include "jpeg.h"
#include "output_stream.h"
#include "jpeg_encoder.h"


// Writes JPEG image to file.
BooleanJpeg compress_image_to_jpeg_file(const char *pFilename, int width, int height, int num_channels, const uint8_t *pImage_data, const jpeg_params_t * comp_params)
{
	output_stream dst_stream = {.type = JPEG_FILE, .data.file_stream = {.m_pFile = NULL, .m_bStatus = FALSE_JPEG}};
	if (!open_jpeg_file(&dst_stream, pFilename))
		return FALSE_JPEG;

	jpeg_encoder_t dst_image;
	clear(&dst_image);
	if (!init(&dst_image, &dst_stream, width, height, num_channels, comp_params))
		return FALSE_JPEG;

	int i;
	for (i = 0; i < height; i++)
	{
		const uint8_t * pBuf = pImage_data + i * width * num_channels;
		if (!process_scanline(&dst_image, pBuf))
			return FALSE_JPEG;
	}
	if (!process_scanline(&dst_image, NULL))
		return FALSE_JPEG;

   deinit(&dst_image);

   return close_jpeg_file(&dst_stream);
}

BooleanJpeg compress_image_to_jpeg_file_in_memory(uint8_t ** output_buf, int * output_buf_size,
												int width, int height, int num_channels,
												const uint8_t *pImage_data, const jpeg_params_t * comp_params)
{
	*output_buf = NULL;
	*output_buf_size = 0;

	output_stream dst_stream = {.type = JPEG_MEMORY,  .data.mem_stream = { .m_pBuf = NULL, .m_buf_size = 0 , .m_buf_ofs = 0} };

	int buf_size = width * height * 3; // allocate a buffer that's hopefully big enough (this is way overkill for jpeg)
	if (buf_size < 1024) buf_size = 1024;
	if (!init_jpeg_mem(&dst_stream, buf_size))
		return FALSE_JPEG;


	jpeg_encoder_t dst_image;
	clear(&dst_image);
	if (!init(&dst_image, &dst_stream, width, height, num_channels, comp_params))
		return FALSE_JPEG;

	int i;
	for (i = 0; i < height; i++)
	{
		const uint8_t * pBuf = pImage_data + i * width * num_channels;
		if (!process_scanline(&dst_image, pBuf))
			return FALSE_JPEG;
	}
	if (!process_scanline(&dst_image, NULL))
		return FALSE_JPEG;

	deinit(&dst_image);

	*output_buf_size = get_size(&dst_stream);
	*output_buf = dst_stream.data.mem_stream.m_pBuf;

	return TRUE_JPEG;
}

