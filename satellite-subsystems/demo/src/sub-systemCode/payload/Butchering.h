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

enum BUTCHER_ERR
{
	BUTCHER_SUCCSESS,
	BUTCHER_NULL_POINTER,
	BUTHCER_PARAMATER_VALUE,
	BUTCHER_UNDEFINED_ERR,
};


typedef unsigned char pixel_t;						///< declare pixel type. each pixel is only one color(R or B or G)
typedef unsigned char chunk_t[CHUNK_SIZE];	///< declare chunk type with fixed size


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
 */
int GetChunksFromRange(chunk_t *chunks, unsigned int index_start, unsigned int index_end, pixel_t image[IMAGE_SIZE], fileType im_type);

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
int GetChunkFromImage(chunk_t chunk, unsigned int index, pixel_t image[IMAGE_SIZE], fileType im_type);

/*!
 * @brief copy chunk data to empty image
 * @param[in] image image to copy chunk data to
 * @param[in] chunk the chunk to insert into the image
 * @param[in] index the chunks index
 * @return	BUTCHER_SUCCSESS at succsess
 *			BUTCHER_NULL_POINTER is one or more parameters are NULL
 *			BUTHCER_PARAMATER_VALUE if indexes are out of bound
 */
int InsertChunkToImage(pixel_t image[IMAGE_SIZE], chunk_t chunk, int index);

/*!
 * @brief get data row chronologicly from 'data' input
 * @param[in] data the data to get infromation from
 * @param[out] buffer output data buffer with length 'data_length'
 * @param[in] start_index index to start saving data from
 * @param[in] data_length hoe many bytes of data to read from 'data'
 * @warning to use correctly one must update 'start_index' to keep track of the data.
 * @see see
 * @return	BUTCHER_SUCCSESS at succsess
 *			BUTCHER_NULL_POINTER is one or more parameters are NULL
 *			BUTHCER_PARAMATER_VALUE if indexes are out of bound
 */
int GetDataRegionFromBuffer(unsigned char *data, unsigned char *buffer, unsigned int *start_index, unsigned int data_length);

/*!
 * @brief updates where the starting index position is by incrementing it by a value of 'data_length'
 * @param[out] start_index buffer starting index cursur
 * @param[in] data_length the factor to increment 'start_index' by
 * @return	BUTCHER_SUCCSESS at succsess
 *			BUTCHER_NULL_POINTER is one or more parameters are NULL
 */
 int UpdateDataCursur(unsigned int *start_index, unsigned int data_length);
#endif
