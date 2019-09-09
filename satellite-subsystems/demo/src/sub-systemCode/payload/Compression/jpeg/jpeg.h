// jpge.h - C++ class for JPEG compression.
// Public domain, Rich Geldreich <richgel99@gmail.com>
// v1.00 - Last updated Dec. 18, 2010

#ifndef JPEG_ENCODER_H
#define JPEG_ENCODER_H

#include <stdint.h>
#include "jpeg_params.h"

//typedef unsigned char  uint8;
//typedef signed short   int16;
//typedef signed int     int32;
//typedef unsigned int   uint32;
//typedef unsigned int   uint;

  
// Writes JPEG image to a file.
// num_channels must be 1 (Y) or 3 (RGB), image pitch must be width*num_channels.
BooleanJpeg compress_image_to_jpeg_file(char *pFilename, int width, int height, int num_channels,
												const uint8_t *pImage_data, const jpeg_params_t * comp_params);
// Writes JPEG image to memory buffer.
// On entry, buf_size is the size of the output buffer pointed at by pBuf, which should be at least ~1024 bytes.
// If return value is true, buf_size will be set to the size of the compressed data.
BooleanJpeg compress_image_to_jpeg_file_in_memory(uint8_t ** output_buf, int * output_buf_size,
												int width, int height, int num_channels,
												const uint8_t *pImage_data, const jpeg_params_t * comp_params);
#endif // JPEG_ENCODER

