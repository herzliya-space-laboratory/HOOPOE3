/*
 * ImgCompressor.c
 *
 *  Created on: Jul 14, 2019
 *      Author: Roy's Laptop
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "jpeg.h"

#include "../Macros.h"
#include "bmp/rawToBMP.h"

#include "ImgCompressor.h"

typedef unsigned int uint;

static byte bmpBuffer[BMP_FILE_DATA_SIZE];

JpegCompressionResult CompressImage(imageid id, fileType reductionLevel, unsigned int quality_factor)
{
	printf("jpeg_encoder class example program\n");

	unsigned int compfact = (unsigned int)pow(2, (double)reductionLevel);

	char pSrc_filename_raw[FILE_NAME_SIZE];
	GetImageFileName(id, reductionLevel, pSrc_filename_raw);

	char pSrc_filename[FILE_NAME_SIZE];
	GetImageFileName(id, bmp, pSrc_filename);

	TransformRawToBMP(pSrc_filename_raw, pSrc_filename, compfact);

	char pDst_filename[FILE_NAME_SIZE];
	GetImageFileName(id, jpg, pDst_filename);

	if ((quality_factor < 1) || (quality_factor > 100))
	{
		printf("Quality factor must range from 1-100!\n");
		return JpegCompression_qualityFactor_outOFRange;
	}

	// Load the source image:

	F_FILE* file;
	OPEN_FILE(file, pSrc_filename, "r", JpegCompression_ImageLoadingFailure);	// open file for writing in safe mode

	f_seek(file, BMP_FILE_HEADER_SIZE, SEEK_SET);	// skipping the file header

	READ_FROM_FILE(file, bmpBuffer, BMP_FILE_DATA_SIZE, 1, JpegCompression_ImageLoadingFailure);

	CLOSE_FILE(file, JpegCompression_ImageLoadingFailure);

	unsigned int image_width = IMAGE_WIDTH / compfact;
	unsigned int image_height = IMAGE_HEIGHT / compfact;
	printf("Source image resolution: %ix%i\n", image_width, image_height);

	// Fill in the compression parameter structure.
	jpeg_params_t params = { .m_quality = quality_factor, .m_subsampling = H2V2, .m_no_chroma_discrim_flag = FALSE_JPEG };

	printf("Writing JPEG image to file: %s\n", pDst_filename);

	// Now create the JPEG file.

	if (!compress_image_to_jpeg_file(pDst_filename, image_width, image_height, H2V2, bmpBuffer, &params))
	{
		printf("Failed writing to output file!\n");
		return JpegCompression_FailedWritingToOutputFile;
	}

	printf("Compressed file size: %u\n", (unsigned int)f_filelength(pDst_filename));

	f_delete(pSrc_filename);	// deleting the BMP file

	return JpegCompression_Success;
}
