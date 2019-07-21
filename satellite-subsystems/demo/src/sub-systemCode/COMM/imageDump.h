/*
 * imageDump.h
 *
 *  Created on: Jun 23, 2019
 *      Author: Hoopoe3n
 */

#ifndef IMAGEDUMP_H_
#define IMAGEDUMP_H_

#include "../Global/Global.h"
#include "../Global/GlobalParam.h"
#include "../TRXVU.h"

#define NUMBER_OF_CHUNKS_IN_CMD 185 * 8

typedef enum{
	bitField_imageDump_,
	chunkField_imageDump_,
	thumbnail_imageDump_
}imageDump_t;

typedef struct __attribute__ ((__packed__))
{
	imageDump_t type;
	command_id cmdId;
	byte command_parameters[MAX_SIZE_OF_PARAM_IMAGE_DUMP];
}request_image;

/*
 * @brief		the function in charge of starting sending an image
 * @param[in]	a pointer to a request_image type
 * 				->type: the dump can be a cording to bitFiled with a starting chunk,
 * 						a field between two chunks, send all thumbnail4 between two times
 * 				->cmdId: the id of the command that started it all
 * 				->command_parameters: the parameters for all types of image dumps
 *
 */
void imageDump_task(void* param);

/*
 * @brief		logic to download image by bit field from ground
 * @param[in]
 */
int bitField_imageDump(time_unix image_id, fileType comprasionType, command_id cmdId,
		unsigned int firstChunk_index, byte packetsToSend[NUMBER_OF_CHUNKS_IN_CMD / 8]);

#endif /* IMAGEDUMP_H_ */
