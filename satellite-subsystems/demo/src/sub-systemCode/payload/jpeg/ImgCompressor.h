/*
 * ImgCompressor.h
 *
 *  Created on: Jul 14, 2019
 *      Author: Roy's Laptop
 */

#ifndef IMGCOMPRESSOR_H_
#define IMGCOMPRESSOR_H_

#include "../DataBase.h"

typedef enum
{
	JpegCompressionSuccess,
	JpegCompression_qualityFactor_outOFRange,
	JpegCompression_ImageLoadingFailure,
	JpegCompression_FailedWritingToOutputFile
} JpegCompressionResult;

/*
 * Usage: jpge <sourcefile> <destfile> <quality_factor>
 * sourcefile: Source image file, in any format stb_image.c supports.
 * destfile: Destination JPEG file.
 * quality_factor: 1-100, higher=better
 */
JpegCompressionResult CompressImage(imageid id, fileType reductionLevel, unsigned int quality_factor);

#endif /* IMGCOMPRESSOR_H_ */
