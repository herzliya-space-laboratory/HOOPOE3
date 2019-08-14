/*
 * jpeg_encoder.h
 *
 *  Created on: 12 αιεμ 2019
 *      Author: Tamir
 */

#ifndef JPEG_ENCODER_H_
#define JPEG_ENCODER_H_

#include "BooleanJpeg.h"
#include <stdint.h>
#include "sys/types.h"
#include "jpeg_params.h"
#include "output_stream.h"

#define JPGE_MAX(a,b) (((a)>(b))?(a):(b))
#define JPGE_MIN(a,b) (((a)<(b))?(a):(b))
#define JPGE_OUT_BUF_SIZE (2048)

typedef int32_t sample_array_t;

// Lower level jpeg_encoder class - useful if more control is needed than the above helper functions.
typedef struct
{
    output_stream m_pStream;
    jpeg_params_t m_params;
    uint8_t m_num_components;
    uint8_t m_comp_h_samp[3], m_comp_v_samp[3];
    int m_image_x, m_image_y, m_image_bpp, m_image_bpl;
    int m_image_x_mcu, m_image_y_mcu;
    int m_image_bpl_xlt, m_image_bpl_mcu;
    int m_mcus_per_row;
    int m_mcu_x, m_mcu_y;
    uint8_t * *m_mcu_lines;
    uint8_t m_mcu_y_ofs;
    sample_array_t *m_sample_array;
    int16_t *m_coefficient_array;
    int32_t *m_quantization_tables[2];
    uint *m_huff_codes[4];
    uint8_t *m_huff_code_sizes[4];
    uint8_t *m_huff_bits[4];
    uint8_t *m_huff_val[4];
    int m_last_dc_val[3];
    uint8_t *m_out_buf;
    uint8_t *m_out_buf_ofs;
    uint m_out_buf_left;
    uint32_t m_bit_buffer;
    uint8_t m_bits_in;
    BooleanJpeg m_all_stream_writes_succeeded;
} jpeg_encoder_t;

void clear(jpeg_encoder_t * enc);

// Initializes the compressor.
// pStream: The stream object to use for writing compressed data.
// params - Compression parameters structure, defined above.
// width, height  - Image dimensions.
// channels - May be 1, or 3. 1 indicates grayscale, 3 indicates RGB source data.
// Returns false on out of memory or if a stream write fails.
BooleanJpeg init(jpeg_encoder_t * enc, output_stream * stream, int width, int height, int src_channels, const jpeg_params_t * comp_params);

// Deinitializes the compressor, freeing any allocated memory. May be called at any time.
void deinit(jpeg_encoder_t * enc);

// Call this method with each source scanline.
// width * src_channels bytes per scanline is expected (RGB or Y format).
// You must call with NULL after all scanlines are processed to finish compression.
// Returns false on out of memory or if a stream write fails.
BooleanJpeg process_scanline(jpeg_encoder_t * enc, const void* pScanline);

#endif /* JPEG_ENCODER_H_ */
