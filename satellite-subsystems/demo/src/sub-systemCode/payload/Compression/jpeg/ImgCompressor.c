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

#include <satellite-subsystems/GomEPS.h>

#include "../../Misc/Macros.h"
#include "../../Misc/FileSystem.h"
#include "bmp/rawToBMP.h"

#include "ImgCompressor.h"

typedef unsigned int uint;

ImageDataBaseResult JPEG_compressor(uint32_t compfact, unsigned int quality_factor, char pDst_filename[FILE_NAME_SIZE], byte* buffer)
{
	unsigned int image_width = IMAGE_WIDTH / compfact;
	unsigned int image_height = IMAGE_HEIGHT / compfact;
	printf("Source image resolution: %ix%i\n", image_width, image_height);

	// Fill in the compression parameter structure.
	jpeg_params_t params = { .m_quality = quality_factor, .m_subsampling = H2V2, .m_no_chroma_discrim_flag = FALSE_JPEG };

	printf("Writing JPEG image to file: %s\n", pDst_filename);

	// Now create the JPEG file.

	if (!compress_image_to_jpeg_file(pDst_filename, image_width, image_height, H2V2, buffer, &params))
	{
		printf("Failed writing to output file!\n");
		return DataBaseFileSystemError;
	}

	uint32_t file_size = f_filelength(pDst_filename);
	printf("Compressed file size: %u\n", (unsigned int)file_size);

	return JpegCompression_Success;
}

ImageDataBaseResult Create_BMP_File(char pSrc_filename_raw[FILE_NAME_SIZE], char pSrc_filename[FILE_NAME_SIZE], unsigned int compfact, byte* buffer)
{
	TransformRawToBMP(pSrc_filename_raw, pSrc_filename, compfact);

	// Load the bmp:

	F_FILE* file = NULL;
	int error = f_managed_open(pSrc_filename, "r", &file);	// open file for reading in safe mode
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);
	CHECK_FOR_NULL(file, DataBaseNullPointer);

	f_seek(file, BMP_FILE_HEADER_SIZE, SEEK_SET);	// skipping the file header

	error = ReadFromFile(file, buffer, BMP_FILE_DATA_SIZE, 1);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	error = f_managed_close(&file);
	CMP_AND_RETURN(error, 0, DataBaseFileSystemError);

	return JpegCompression_Success;
}

ImageDataBaseResult CompressImage(imageid id, fileType reductionLevel, unsigned int quality_factor)
{
	printf("jpeg_encoder class example program\n");

	if ((quality_factor < 1) || (quality_factor > 100))
	{
		printf("Quality factor must range from 1-100!\n");
		return JpegCompression_qualityFactor_outOFRange;
	}

	unsigned int compfact = (unsigned int)pow(2, (double)reductionLevel);
	byte* buffer = imageBuffer;

	char pSrc_filename_raw[FILE_NAME_SIZE];
	ImageDataBaseResult result = GetImageFileName(id, reductionLevel, pSrc_filename_raw);
	CMP_AND_RETURN(result, DataBaseSuccess, result);

	char pSrc_filename_bmp[FILE_NAME_SIZE];
	result = GetImageFileName(id, bmp, pSrc_filename_bmp);
	CMP_AND_RETURN(result, DataBaseSuccess, result);

	GomEpsResetWDT(0);

	keepTryingTo_enterFS();

	result = Create_BMP_File(pSrc_filename_raw, pSrc_filename_bmp, compfact, buffer);
	CMP_AND_RETURN(result, JpegCompression_Success, result);

	char pDst_filename[FILE_NAME_SIZE];
	result = GetImageFileName(id, jpg, pDst_filename);
	CMP_AND_RETURN(result, DataBaseSuccess, result);

	result = JPEG_compressor(compfact, quality_factor, pDst_filename, buffer);
	CMP_AND_RETURN(result, JpegCompression_Success, result);

	f_delete(pSrc_filename_bmp);	// deleting the BMP file

	int error = releaseFS();
	if (error)
	{
		return DataBaseFileSystemError;
	}

	return JpegCompression_Success;
}
