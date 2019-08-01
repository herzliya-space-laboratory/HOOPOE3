/*
 * FRAM_Extended.h
 *
 *  Created on: 7 ���� 2019
 *      Author: I7COMPUTER
 */

#ifndef FRAM_EXTENDED_H_
#define FRAM_EXTENDED_H_

#include <hal/boolean.h>

/*
 * Will read from the FRAM and progress the address variable according to the number of bytes that were read from the FRAM
 * @param data the data to be read,
 * address location in FRAM to start writing in,
 * size the size of the data
 * @return returns FRAM_read function's result
*/
int FRAM_readAndProgress(unsigned char *data, unsigned int *address, unsigned int size);

/*
 * Will write from the FRAM and progress the address variable according to the number of bytes that were written from the FRAM
 * @param data the data to be written,
 * address location in FRAM to start writing in,
 * size the size of the data
 * @return returns FRAM_write function's result
*/
int FRAM_writeAndProgress(unsigned char *data, unsigned int *address, unsigned int size);

#endif /* FRAM_EXTENDED_H_ */
