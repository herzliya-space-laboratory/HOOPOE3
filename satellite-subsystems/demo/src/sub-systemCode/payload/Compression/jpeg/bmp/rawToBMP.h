/*
 * rawToBMP.h
 *
 *  Created on: 31 במאי 2019
 *      Author: I7COMPUTER
 */

#ifndef RAWTOBMP_H_
#define RAWTOBMP_H_

#include <hcc/api_fat.h>

#define BMP_FILE_HEADER_SIZE	54
#define BMP_FILE_DATA_SIZE		IMAGE_SIZE * 3
#define BMP_FILE_SIZE			BMP_FILE_HEADER_SIZE + BMP_FILE_DATA_SIZE

void TransformRawToBMP(char * inputF_FILE, char * outputF_FILE, int compfact);

#endif /* RAWTOBMP_H_ */
