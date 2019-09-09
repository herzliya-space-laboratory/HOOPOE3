/*
 * color_conv.h
 *
 *  Created on: 12 αιεμ 2019
 *      Author: Tamir
 */

#ifndef COLOR_CONV_H_
#define COLOR_CONV_H_

#include "stdint.h"

void RGB_to_YCC(uint8_t * Pdst, const uint8_t *src, int num_pixels);

void RGB_to_Y(uint8_t * Pdst, const uint8_t *src, int num_pixels);

void Y_to_YCC(uint8_t * Pdst, const uint8_t * src, int num_pixels);




#endif /* COLOR_CONV_H_ */
