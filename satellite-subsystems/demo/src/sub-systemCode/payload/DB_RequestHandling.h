/*
 * DB_RequestHandling.h
 *
 *  Created on: Jun 6, 2019
 *      Author: Roy's Laptop
 */

#ifndef DB_REQUESTHANDLING_H_
#define DB_REQUESTHANDLING_H_

#include <freertos/FreeRTOS.h>

#include "../Global/Global.h"

#include "ImageConversion.h"
#include "DataBase.h"

typedef enum
{
	RawImage,
	ThumbnailLevel1,
	ThumbnailLevel2,
	ThumbnailLevel3,
	ThumbnailLevel4,
	ThumbnailLevel5,
	ThumbnailLevel6,
	JPG,
	GeckoSD
} GroundFileTypes;

ImageDataBaseResult TakePicture(ImageDataBase database, unsigned char data[]);
ImageDataBaseResult TakeSpecialPicture(ImageDataBase database, unsigned char data[]);
ImageDataBaseResult DeletePicture(ImageDataBase database, unsigned char data[]);
ImageDataBaseResult TransferPicture(ImageDataBase database, unsigned char data[]);
byte* GetImageDataBase(ImageDataBase database, unsigned char data[]);
ImageDataBaseResult CreateThumbnail(unsigned char* data);
ImageDataBaseResult CreateJPG(unsigned char* data);
ImageDataBaseResult UpdatePhotographyValues(ImageDataBase database, unsigned char data[]);

#endif /* DB_REQUESTHANDLING_H_ */
