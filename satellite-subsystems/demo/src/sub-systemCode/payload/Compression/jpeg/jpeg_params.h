/*
 * params.h
 *
 *  Created on: 12 αιεμ 2019
 *      Author: Tamir
 */

#ifndef PARAMS_H_
#define PARAMS_H_

#include "BooleanJpeg.h"
#include <stddef.h>

// JPEG subsampling factors. Y_ONLY (grayscale images) and H2V2 (color images) are the most common.
typedef enum
{
	Y_ONLY = 0,
	H1V1 = 1,
	H2V1 = 2,
	H2V2 = 3
} subsampling_t ;


// JPEG compression parameters structure.
typedef struct
{
    // Quality: 1-100, higher is better. Typical values are around 50-95.
    int m_quality; // default 85;

    // m_subsampling:
    // 0 = Y (grayscale) only
    // 1 = YCbCr, no subsampling (H1V1, YCbCr 1x1x1, 3 blocks per MCU)
    // 2 = YCbCr, H2V1 subsampling (YCbCr 2x1x1, 4 blocks per MCU)
    // 3 = YCbCr, H2V2 subsampling (YCbCr 4x1x1, 6 blocks per MCU-- very common)
    subsampling_t m_subsampling; // default H2V2;

    // Disables CbCr discrimination - only intended for testing.
    // If true, the Y quantization table is also used for the CbCr channels.
    BooleanJpeg m_no_chroma_discrim_flag; // default FALSE_JPEG;
} jpeg_params_t;

BooleanJpeg check(const jpeg_params_t * params);

#endif /* PARAMS_H_ */
