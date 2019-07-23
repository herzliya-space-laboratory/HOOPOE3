/*!
 * @file	Butchering.h
 * @brief	performes butchering and de-butchering procedures
 * @warning	THESE FUNCTIONS DOES NOT ALLOCATE MEMORY. PLEASE ALLOCATE MEMORY BEFOREHAND
 */

/*
index format:
	______________________
	| 0| 1| 2| 3| 4| 5| 6|
	| 7| 8| 9|10|11|12|13|
	|14|15|16|17|18|19|10|
	|21|22|23|24|25|26|27|
	---------------------
every box is a chunk and the whole is an image
*/

#ifndef BUTCHERING_H_
#define BUTCHERING_H_

#include "DataBase.h" // for fileType
#include "../Global/Global.h"

typedef enum BUTCHER_ERR
{
	BUTCHER_SUCCSESS,
	BUTCHER_NULL_POINTER,
	BUTHCER_PARAMATER_VALUE,
	BUTCHER_UNDEFINED_ERR,
	BUTCHER_BAD_WIDTH
} ButcherError;


typedef unsigned char pixel_t;						///< declare pixel type. each pixel is only one color(R or B or G)
typedef pixel_t chunk_t[CHUNK_HEIGHT][CHUNK_WIDTH];	///< declare chunk type with fixed size
typedef pixel_t image_t[IMAGE_HEIGHT][IMAGE_WIDTH];

/*!
 * @brief gets all chunks from image according to a specified index range
 * @param[out] chunks array of chunks to be filled with data from image
 * @param[in] index_start the starting index from which to start copying data
 * @param[in] index_end the index to stop reading after
 * @param[in] image the image from which to read data and copy to chunk array
 * @param[in] fileType the type of the image
 * @return	BUTCHER_SUCCSESS at succsess
 *			BUTCHER_NULL_POINTER is one or more parameters are NULL
 *			BUTHCER_PARAMATER_VALUE if indexes are out of bound
 * @note don't use for JPEG!
 */
ButcherError GetChunksFromRange(chunk_t* chunks, unsigned int index_start, unsigned int index_end, image_t image, fileType im_type);

ButcherError GetChunkFromImage(chunk_t chunk, unsigned int index, image_t image, fileType img_type);

/*!
 * @brief gets a chunk specified by index from image
 * @param[out] chunk the chunk from the image
 * @param[in] index the chunks index
 * @param[in] image the image from which to read data and copy to chunk
 * @param[in] fileType the type of the image
 * @return	BUTCHER_SUCCSESS at succsess
 *			BUTCHER_NULL_POINTER is one or more parameters are NULL
 *			BUTHCER_PARAMATER_VALUE if indexes are out of bound
 */
pixel_t* SimpleButcher(pixel_t* data, unsigned int sizeofData, unsigned int size, unsigned int index);

#endif
