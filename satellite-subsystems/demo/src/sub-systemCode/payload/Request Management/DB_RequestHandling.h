/*
 * DB_RequestHandling.h
 *
 *  Created on: Jun 6, 2019
 *      Author: Roy's Laptop
 */

#ifndef DB_REQUESTHANDLING_H_
#define DB_REQUESTHANDLING_H_

#include <freertos/FreeRTOS.h>

#include "../../Global/Global.h"

#include "Dump/imageDump.h"
#include "../Compression/ImageConversion.h"
#include "../DataBase/DataBase.h"

ImageDataBaseResult TakePicture(ImageDataBase database, unsigned char data[]);
ImageDataBaseResult TakeSpecialPicture(ImageDataBase database, unsigned char data[]);
ImageDataBaseResult DeletePictureFile(ImageDataBase database, unsigned char* data);
ImageDataBaseResult DeletePicture(ImageDataBase database, unsigned char data[]);
ImageDataBaseResult TransferPicture(ImageDataBase database, unsigned char data[]);
byte* GetImageDataBase(ImageDataBase database, unsigned char data[]);
ImageDataBaseResult CreateThumbnail(unsigned char* data);
ImageDataBaseResult CreateJPG(unsigned char* data);
ImageDataBaseResult UpdatePhotographyValues(ImageDataBase database, unsigned char data[]);
ImageDataBaseResult setChunkDimensions(unsigned char data[]);

int GeckoSetRegister(unsigned char* data);

#endif /* DB_REQUESTHANDLING_H_ */
