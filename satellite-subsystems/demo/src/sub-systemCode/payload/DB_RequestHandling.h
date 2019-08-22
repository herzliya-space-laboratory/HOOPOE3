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

#include "DataBase.h"

DataBaseResult TakePicture(DataBase database, unsigned char data[]);
DataBaseResult TakeSpecialPicture(DataBase database, unsigned char data[]);
DataBaseResult DeletePicture(DataBase database, unsigned char data[]);
DataBaseResult TransferPicture(DataBase database, unsigned char data[]);
F_FILE* GetDataBaseFile(DataBase database, unsigned char data[]);
DataBaseResult CreateThumbnail(DataBase database, unsigned char data[]);
DataBaseResult CreateJPG(DataBase database, unsigned char data[]);
DataBaseResult UpdatePhotographyValues(DataBase database, unsigned char data[]);
DataBaseResult UpdateMaxNumberOfPicturesInDB(DataBase database, unsigned char data[]);
char* GetPictureFile(DataBase database, unsigned char data[]);
#endif /* DB_REQUESTHANDLING_H_ */

