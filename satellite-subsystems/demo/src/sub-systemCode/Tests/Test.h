/*
 * Test.h
 *
 *  Created on: Mar 3, 2019
 *      Author: elain
 */

#ifndef TEST_H_
#define TEST_H_

#define UPDATE_TIME_TEST_ST 133
#define DUMP_TEST_ST 246
#define SAVE_DATA_TEST_T 11

#include "../payload/DataBase.h"
#include "../payload/Butchering.h"

void collect_TM_during_deploy();
void reset_FIRST_activation();
void readAntsState();
void imageDump_test(void* param);

void ButcheringTest(unsigned int StartIndex, unsigned int EndIndex, unsigned int id);
void GetChunkFromImageReal(chunk_t chunk, unsigned short index, pixel_t image[IMAGE_SIZE], fileType im_type, unsigned int height, unsigned int width);
void GetChunkFromImageTest(chunk_t chunk, unsigned short index, pixel_t image[IMAGE_SIZE], unsigned int ImageSize);


#endif /* TEST_H_ */
