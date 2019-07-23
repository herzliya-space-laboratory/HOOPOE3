/*
 * imageDump.h
 *
 *  Created on: Jun 23, 2019
 *      Author: elain
 */

#ifndef IMAGEDUMP_H_
#define IMAGEDUMP_H_

#include "../Global/sizes.h"
#include "../Global/Global.h"
#include "../Global/GlobalParam.h"
#include "../TRXVU.h"

#define SIZE_OF_PARAMETERS_FOR_BIT_FIELD_DUMP sizeof(imageid)+sizeof(byte)+sizeof(short)
#define BIT_FIELD_SIZE SPL_TC_DATA_SIZE - SIZE_OF_PARAMETERS_FOR_BIT_FIELD_DUMP

#define NUMBER_OF_CHUNKS_IN_CMD BIT_FIELD_SIZE * 8

typedef enum{
	bitField_imageDump_,
	chunkField_imageDump_,
	thumbnail_imageDump_,
	imageDataBaseDump_
}imageDump_t;

typedef struct __attribute__ ((__packed__))
{
	imageDump_t type;
	command_id cmdId;
	byte command_parameters[SPL_TC_DATA_SIZE];
}request_image;

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
