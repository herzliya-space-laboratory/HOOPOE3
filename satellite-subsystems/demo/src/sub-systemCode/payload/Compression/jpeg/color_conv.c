/*
 * color_conv.c
 *
 *  Created on: 12 αιεμ 2019
 *      Author: Tamir
 */

#include "sys/types.h"
#include "color_conv.h"

const int YR = 19595, YG = 38470, YB = 7471, CB_R = -11059, CB_G = -21709, CB_B = 32768, CR_R = 32768, CR_G = -27439, CR_B = -5329;

uint8_t clamp(int i)
{
	if ((uint)i > 255U)
	{
		if (i < 0)
			i = 0;
		else if (i > 255)
			i = 255;
	}
	return (uint8_t)i;
}

void RGB_to_YCC(uint8_t * Pdst, const uint8_t *src, int num_pixels)
{
	for ( ; num_pixels; Pdst += 3, src += 3, num_pixels--)
	{
		const int r = src[0], g = src[1], b = src[2];
		Pdst[0] = (uint8_t)((r * YR + g * YG + b * YB + 32768) >> 16);
		Pdst[1] = clamp(128 + ((r * CB_R + g * CB_G + b * CB_B + 32768) >> 16));
		Pdst[2] = clamp(128 + ((r * CR_R + g * CR_G + b * CR_B + 32768) >> 16));
	}
}

void RGB_to_Y(uint8_t * Pdst, const uint8_t *src, int num_pixels)
{
	for ( ; num_pixels; Pdst++, src += 3, num_pixels--)
	{
		Pdst[0] = (uint8_t)((src[0] * YR + src[1] * YG + src[2] * YB + 32768) >> 16);
	}
}

void Y_to_YCC(uint8_t * Pdst, const uint8_t * src, int num_pixels)
{
	for( ; num_pixels; Pdst += 3, src++, num_pixels--)
	{
		Pdst[0] = src[0];
		Pdst[1] = 128;
		Pdst[2] = 128;
	}
}



