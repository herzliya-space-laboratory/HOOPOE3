/*
 * ImgCompressor.c
 *
 *  Created on: 31 במאי 2019
 *      Author: I7COMPUTER
 */

#include <stdio.h>
#include "rawToBMP.h"
#include "ejpgl.h"

int CompressImg(char* infile, char* bmpfile, char* jpgfile, int compfact)
{
	TransformRawToBMP(infile, bmpfile, compfact);
	JPEGCompress(bmpfile, jpgfile);
	return 0;
}
