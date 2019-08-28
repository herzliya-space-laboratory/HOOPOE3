/*
 * imageDump.h
 *
 *  Created on: Jun 23, 2019
 *      Author: elain
 */

#ifndef IMAGEDUMP_H_
#define IMAGEDUMP_H_

#include "../../../Global/sizes.h"
#include "../../../Global/Global.h"
#include "../../../Global/GlobalParam.h"
#include "../../../TRXVU.h"

#include "../CameraManeger.h"
#include "../../DataBase/DataBase.h"

#define DEFALT_CHUNK_HEIGHT 13
#define DEFALT_CHUNK_WIDTH 16

#define SIZE_OF_PARAMETERS_FOR_BIT_FIELD_DUMP (sizeof(imageid) + sizeof(byte) + sizeof(uint16_t))
#define BIT_FIELD_SIZE (SPL_TC_DATA_SIZE - SIZE_OF_PARAMETERS_FOR_BIT_FIELD_DUMP)

#define NUMBER_OF_CHUNKS_IN_CMD (BIT_FIELD_SIZE * 8)

ImageDataBaseResult setChunkDimensions_inFRAM(uint16_t width, uint16_t height);

/*
 * @brief		the function in charge of starting sending an image
 * @param[in]	a pointer to a request_image type
 * 				->type: the dump can be a cording to bitFiled with a starting chunk,
 * 						a field between two chunks, send all thumbnail5 between two times,
 * 						send image metadata between two times
 * 				->cmdId: the id of the command that started it all
 * 				->command_parameters: the parameters for all types of image dumps
 *
 */
void imageDump_task(void* param);

#endif /* IMAGEDUMP_H_ */
