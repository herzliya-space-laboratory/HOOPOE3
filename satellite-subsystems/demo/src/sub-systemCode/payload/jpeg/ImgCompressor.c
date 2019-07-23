/*
 * ImgCompressor.c
 *
 *  Created on: Jul 14, 2019
 *      Author: Roy's Laptop
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ImgCompressor.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "jpeg.h"

#include "bmp/rawToBMP.h"

typedef unsigned int uint;

static byte imageBuffer[IMAGE_SIZE*3];

JpegCompressionResult CompressImage(imageid id, fileType reductionLevel, unsigned int quality_factor)
{
	printf("jpeg_encoder class example program\n");

	unsigned int compfact = (unsigned int)pow(2, (double)reductionLevel);

	const char* pSrc_filename_raw = GetImageFileName(id, reductionLevel);
	const char* pSrc_filename = GetImageFileName(id, bmp);

	TransformRawToBMP(pSrc_filename_raw, pSrc_filename, compfact);

	const char* pDst_filename = GetImageFileName(id, jpg);

	if ((quality_factor < 1) || (quality_factor > 100))
	{
		printf("Quality factor must range from 1-100!\n");
		return JpegCompression_qualityFactor_outOFRange;
	}

	// Load the source image.

	F_FILE* file = f_open(pSrc_filename, "r" ); // open file for writing in safe mode
	if(file == NULL)
	{
		return JpegCompression_ImageLoadingFailure;
	}
	f_seek(file, BMP_FILE_HEADER_SIZE, SEEK_SET);	// skipping the file header

	unsigned int br = f_read(imageBuffer, 1, IMAGE_SIZE * 3 / (compfact * compfact), file);
	if( sizeof(imageBuffer) != br )
	{
		TRACE_ERROR("f_read pb: %d\n\r", f_getlasterror() ); // if bytes to write doesn't equal bytes written, get the error
	}

	int err1 = f_close( file ); // data is also considered safe when file is closed
	if( err1 )
	{
		TRACE_ERROR("f_close pb: %d\n\r", err1);
	}

	unsigned int image_width = IMAGE_WIDTH / compfact;
	unsigned int image_height = IMAGE_HEIGHT / compfact;
	printf("Source image resolution: %ix%i\n", image_width, image_height);

	// Fill in the compression parameter structure.
	jpeg_params_t params = { .m_quality = quality_factor, .m_subsampling = H2V2, .m_no_chroma_discrim_flag = FALSE_JPEG };

	printf("Writing JPEG image to file: %s\n", pDst_filename);

	// Now create the JPEG file.

	if (!compress_image_to_jpeg_file(pDst_filename, image_width, image_height, H2V2, imageBuffer, &params))
	{
		printf("Failed writing to output file!\n");
		return JpegCompression_FailedWritingToOutputFile;
	}

	printf("Compressed file size: %u\n", (unsigned int)f_filelength(pDst_filename));

	// ToDo: f_delete(pSrc_filename);	// deleting the BMP file

	return JpegCompressionSuccess;
}
