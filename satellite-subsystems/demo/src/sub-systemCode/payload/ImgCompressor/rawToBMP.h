/*
 * rawToBMP.h
 *
 *  Created on: 31 במאי 2019
 *      Author: I7COMPUTER
 */

#ifndef RAWTOBMP_H_
#define RAWTOBMP_H_

#include <hcc/api_fat.h>

void TransformRawToBMP(const char * inputFile, const char * outputFile, int compfact);

#endif /* RAWTOBMP_H_ */
