/*
 * ImageConversion.c
 *
 *  Created on: 25 ���� 2019
 *      Author: I7COMPUTER
 */

#include <math.h>

#include <satellite-subsystems/GomEPS.h>

#include "../../Global/logger.h"

#include "../Misc/Boolean_bit.h"
#include "jpeg/ImgCompressor.h"

#include "ImageConversion.h"

static uint8_t GetBayerPixel(uint8_t binPixelSize, int x, int y)
{
	uint16_t value = 0;

	for (uint32_t j = 0; j < binPixelSize; j++)
	{
		for (uint32_t i = 0; i < binPixelSize; i++)
		{
			value += imageBuffer[ (y+j*2)*IMAGE_WIDTH + (x+i*2) ]; // need to jump 2 because of the bayer pattern
		}
	}

	return (uint8_t)(value/(binPixelSize*binPixelSize));
}

void BinImage(fileType reductionLevel)	// where 2^(bin level) is the size reduction of the image
{
	// Binning data:
	uint8_t binPixelSize = (1 << reductionLevel);
	uint8_t (*binBuffer)[IMAGE_HEIGHT / binPixelSize][IMAGE_WIDTH / binPixelSize] = (uint8_t(*)[][IMAGE_WIDTH/binPixelSize])imageBuffer;

	for(unsigned int y = 0; y < IMAGE_HEIGHT / binPixelSize; y += 2)
	{
		for(unsigned int x = 0; x < IMAGE_WIDTH / binPixelSize; x += 2)
		{
			(*binBuffer)[y][x] = GetBayerPixel(binPixelSize,x*binPixelSize,y*binPixelSize);         // Red pixel
			(*binBuffer)[y][x+1] = GetBayerPixel(binPixelSize,x*binPixelSize+1,y*binPixelSize);     // Green pixel
			(*binBuffer)[y+1][x] = GetBayerPixel(binPixelSize,x*binPixelSize,y*binPixelSize+1);     // Green pixel
			(*binBuffer)[y+1][x+1] = GetBayerPixel(binPixelSize,x*binPixelSize+1,y*binPixelSize+1); // Blue pixel
		}
	}
}

void ImageSkipping(fileType reductionLevel)	// where 2^(bin level) is the size reduction of the image
{
	unsigned int imageFactor = (unsigned int)pow(2, (double)reductionLevel);
	uint8_t (*binBuffer)[IMAGE_HEIGHT / imageFactor][IMAGE_WIDTH / imageFactor] = (uint8_t(*)[IMAGE_HEIGHT/imageFactor][IMAGE_WIDTH/imageFactor])imageBuffer;

	for(unsigned int y = 0; y < IMAGE_HEIGHT / imageFactor; y += 2)
	{
		for(unsigned int x = 0; x < IMAGE_WIDTH / imageFactor; x += 2)
		{
			(*binBuffer)[y][x] = imageBuffer[ (y*imageFactor)*IMAGE_WIDTH + (x*imageFactor) ];				// Red pixel
			(*binBuffer)[y][x+1] = imageBuffer[ (y*imageFactor)*IMAGE_WIDTH + (x*imageFactor + 1) ];		// Green pixel
			(*binBuffer)[y+1][x] = imageBuffer[ (y*imageFactor + 1)*IMAGE_WIDTH + (x*imageFactor) ];		// Green pixel
			(*binBuffer)[y+1][x+1] = imageBuffer[ (y*imageFactor + 1)*IMAGE_WIDTH + (x*imageFactor + 1) ];	// Blue pixel
		}
	}
}

ImageDataBaseResult CreateImageThumbnail_withoutSearch(imageid id, fileType reductionLevel, Boolean Skipping, uint32_t image_address, ImageMetadata image_metadata)
{
	FRAM_read_exte((unsigned char*)&image_metadata, image_address, sizeof(ImageMetadata));

	bit fileTypes[8];
	char2bits(image_metadata.fileTypes, fileTypes);

	if (fileTypes[reductionLevel].value)	// check if the requested type was already created
		return DataBasealreadyInSD;
	else if (!fileTypes[raw].value)			// if it was not created already, check if the raw is available of the creation process
		return DataBaseRawDoesNotExist;

	int result = readImageFromBuffer(id, raw);
	DB_RETURN_ERROR(result);

	// Creating the Thumbnail on imageBuffer:
	if (Skipping)
		ImageSkipping(reductionLevel);
	else
		BinImage(reductionLevel);

	updateFileTypes(&image_metadata, image_address, reductionLevel, TRUE);

	// Saving data:
	result = saveImageToBuffer(id, reductionLevel);
	DB_RETURN_ERROR(result);

	return DataBaseSuccess;

}

ImageDataBaseResult CreateImageThumbnail(imageid id, fileType reductionLevel, Boolean Skipping)
{
	ImageMetadata image_metadata;
	uint32_t image_address;

	ImageDataBaseResult result = SearchDataBase_byID(id, &image_metadata, &image_address, getDataBaseStart());
	DB_RETURN_ERROR(result);

	return CreateImageThumbnail_withoutSearch(id, reductionLevel, Skipping, image_address, image_metadata);
}

ImageDataBaseResult compressImage(imageid id, unsigned int quality_factor)
{
	ImageMetadata image_metadata;
	uint32_t image_address;

	ImageDataBaseResult result = SearchDataBase_byID(id, &image_metadata, &image_address, getDataBaseStart());
	DB_RETURN_ERROR(result);

	bit fileTypes[8];
	char2bits(image_metadata.fileTypes, fileTypes);

	if (fileTypes[jpg].value)	// check if the requested type was already created
		return DataBasealreadyInSD;
	else if (!fileTypes[raw].value)			// if it was not created already, check if the raw is available of the creation process
		return DataBaseNotInSD;

	int err = GomEpsResetWDT(0);
	printf("err DB eps: %d\n", err);

	updateFileTypes(&image_metadata, image_address, jpg, TRUE);

	ImageDataBaseResult JPEG_result = CompressImage(id, raw, quality_factor);
	if (JPEG_result != JpegCompression_Success)
	{
		updateFileTypes(&image_metadata, image_address, jpg, FALSE);
		return DataBaseJpegFail;
	}

	int log_data = 0;
	memcpy(&image_metadata.cameraId, &log_data, 2);
	memcpy(&quality_factor, &log_data + 2, 2);

	WritePayloadLog(PAYLOAD_COMPRESSED_IMAGE, log_data);

	return DataBaseSuccess;
}
