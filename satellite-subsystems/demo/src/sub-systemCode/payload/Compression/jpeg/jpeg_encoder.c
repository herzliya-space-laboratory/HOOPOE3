/*
 * jpeg_encoder.c
 *
 *  Created on: 12 αιεμ 2019
 *      Author: Tamir
 */

#include <string.h>
#include "jpeg_encoder.h"
#include "output_stream.h"
#include "jpeg_tables.h"
#include "DCT.h"
#include "color_conv.h"
#include "jpeg_memory_alloc.h"

// JPEG marker generation.
void emit_marker(jpeg_encoder_t * enc, int marker)
{
	uint8_t num = 0xFF;
	enc->m_all_stream_writes_succeeded = enc->m_all_stream_writes_succeeded && put_buf(&enc->m_pStream, &num, 1);

	num = marker;
	enc->m_all_stream_writes_succeeded = enc->m_all_stream_writes_succeeded && put_buf(&enc->m_pStream, &num, 1);
}

void emit_byte(jpeg_encoder_t * enc, uint8_t i)
{
	enc->m_all_stream_writes_succeeded = enc->m_all_stream_writes_succeeded && put_buf(&enc->m_pStream, &i, 1);
}

void emit_word(jpeg_encoder_t * enc, uint i)
{
	uint8_t num = (i >> 8);
	enc->m_all_stream_writes_succeeded = enc->m_all_stream_writes_succeeded && put_buf(&enc->m_pStream, &num, 1);

	num = (i & 0xFF);
	enc->m_all_stream_writes_succeeded = enc->m_all_stream_writes_succeeded && put_buf(&enc->m_pStream, &num, 1);
}

void emit_jfif_app0(jpeg_encoder_t * enc)
{
	emit_marker(enc, M_APP0);
	emit_word(enc, 2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1);
	emit_byte(enc, 0x4A); emit_byte(enc, 0x46); emit_byte(enc, 0x49); emit_byte(enc, 0x46); /* Identifier: ASCII "JFIF" */
	emit_byte(enc, 0);
	emit_byte(enc, 1);      /* Major version */
	emit_byte(enc, 1);      /* Minor version */
	emit_byte(enc, 0);      /* Density unit */
	emit_word(enc, 1);
	emit_word(enc, 1);
	emit_byte(enc, 0);      /* No thumbnail image */
	emit_byte(enc, 0);
}

void emit_dqt(jpeg_encoder_t * enc)
{
	int i, j;
	for (i = 0; i < ((enc->m_num_components == 3) ? 2 : 1); i++)
	{
		emit_marker(enc, M_DQT);
		emit_word(enc, 64 + 1 + 2);
		emit_byte(enc, i);
		for (j = 0; j < 64; j++)
		{
			emit_byte(enc, enc->m_quantization_tables[i][j]);
		}
	}
}

void emit_sof(jpeg_encoder_t * enc)
{
  emit_marker(enc, M_SOF0);                           /* baseline */
  emit_word(enc, 3 * enc->m_num_components + 2 + 5 + 1);
  emit_byte(enc, 8);                                  /* precision */
  emit_word(enc, enc->m_image_y);
  emit_word(enc, enc->m_image_x);
  emit_byte(enc, enc->m_num_components);

  int i;
  for (i = 0; i < enc->m_num_components; i++)
  {
    emit_byte(enc, i + 1);                                   /* component ID     */
    emit_byte(enc, (enc->m_comp_h_samp[i] << 4) + enc->m_comp_v_samp[i]);  /* h and v sampling */
    emit_byte(enc, i > 0);                                   /* quant. table num */
  }
}

void emit_dht(jpeg_encoder_t * enc, uint8_t *bits, uint8_t *val, int index, BooleanJpeg ac_flag)
{
  int i, length;

  emit_marker(enc, M_DHT);

  length = 0;

  for (i = 1; i <= 16; i++)
    length += bits[i];

  emit_word(enc, length + 2 + 1 + 16);
  emit_byte(enc, index + (ac_flag << 4));

  for (i = 1; i <= 16; i++)
    emit_byte(enc, bits[i]);

  for (i = 0; i < length; i++)
    emit_byte(enc, val[i]);
}

void emit_dhts(jpeg_encoder_t * enc)
{
  emit_dht(enc, enc->m_huff_bits[0+0], enc->m_huff_val[0+0], 0, FALSE_JPEG);
  emit_dht(enc, enc->m_huff_bits[2+0], enc->m_huff_val[2+0], 0, TRUE_JPEG);
  if (enc->m_num_components == 3)
  {
    emit_dht(enc, enc->m_huff_bits[0+1], enc->m_huff_val[0+1], 1, FALSE_JPEG);
    emit_dht(enc, enc->m_huff_bits[2+1], enc->m_huff_val[2+1], 1, TRUE_JPEG);
  }
}

void emit_sos(jpeg_encoder_t * enc)
{
  emit_marker(enc, M_SOS);
  emit_word(enc, 2 * enc->m_num_components + 2 + 1 + 3);
  emit_byte(enc, enc->m_num_components);

  int i;
  for (i = 0; i < enc->m_num_components; i++)
  {
    emit_byte(enc, i + 1);

    if (i == 0)
      emit_byte(enc, (0 << 4) + 0);
    else
      emit_byte(enc, (1 << 4) + 1);
  }

  emit_byte(enc, 0);     /* spectral selection */
  emit_byte(enc, 63);
  emit_byte(enc, 0);
}

void emit_markers(jpeg_encoder_t * enc)
{
  emit_marker(enc, M_SOI);
  emit_jfif_app0(enc);
  emit_dqt(enc);
  emit_sof(enc);
  emit_dhts(enc);
  emit_sos(enc);
}

BooleanJpeg compute_huffman_table(uint * *codes, uint8_t * *code_sizes, uint8_t *bits, uint8_t *val)
{
	int p, i, l, last_p, si;
	uint8_t huff_size[257];
	uint huff_code[257];
	uint code;

	p = 0;

	for (l = 1; l <= 16; l++)
	{
		for (i = 1; i <= bits[l]; i++)
		{
			huff_size[p++] = (char)l;
		}
	}

	huff_size[p] = 0; last_p = p;

	code = 0; si = huff_size[0]; p = 0;

	while (huff_size[p])
	{
		while (huff_size[p] == si)
		{
			huff_code[p++] = code++;
		}

		code <<= 1;
		si++;
	}

	for (p = 0, i = 0; p < last_p; p++)
	{
		i = JPGE_MAX(i, val[p]);
	}

	*codes = (uint *)(jpge_cmalloc((i + 1) * sizeof(uint)));
	*code_sizes = (uint8_t *)(jpge_cmalloc((i + 1) * sizeof(uint8_t)));

	if (!(*codes) || !(*code_sizes))
	{
		jpge_free(*codes); *codes = NULL;
		jpge_free(*code_sizes); *code_sizes = NULL;
		return FALSE_JPEG;
	}

	for (p = 0; p < last_p; p++)
	{
		(*codes)[val[p]]      = huff_code[p];
		(*code_sizes)[val[p]] = huff_size[p];
	}

	return TRUE_JPEG;
}

// Quantization table generation.
void compute_quant_table(jpeg_encoder_t * enc, int32_t *dst, int16_t *src)
{
	int32_t q;

	if (enc->m_params.m_quality < 50)
		q = 5000 / enc->m_params.m_quality;
	else
		q = 200 - enc->m_params.m_quality * 2;

	int i;
	for (i = 0; i < 64; i++)
	{
		int32_t j = *src++;

		j = (j * q + 50L) / 100L;

		if (j < 1)
			j = 1;
		else if (j > 255)
			j = 255;

		*dst++ = j;
	}
}

// Higher-level methods.
void first_pass_init(jpeg_encoder_t * enc)
{
	enc->m_bit_buffer = 0; enc->m_bits_in = 0;

	memset(enc->m_last_dc_val, 0, 3 * sizeof(int));

	enc->m_mcu_y_ofs = 0;
}

BooleanJpeg second_pass_init(jpeg_encoder_t * enc)
{
	if (!compute_huffman_table(&enc->m_huff_codes[0+0], &enc->m_huff_code_sizes[0+0], enc->m_huff_bits[0+0], enc->m_huff_val[0+0])) return FALSE_JPEG;
	if (!compute_huffman_table(&enc->m_huff_codes[2+0], &enc->m_huff_code_sizes[2+0], enc->m_huff_bits[2+0], enc->m_huff_val[2+0])) return FALSE_JPEG;
	if (!compute_huffman_table(&enc->m_huff_codes[0+1], &enc->m_huff_code_sizes[0+1], enc->m_huff_bits[0+1], enc->m_huff_val[0+1])) return FALSE_JPEG;
	if (!compute_huffman_table(&enc->m_huff_codes[2+1], &enc->m_huff_code_sizes[2+1], enc->m_huff_bits[2+1], enc->m_huff_val[2+1])) return FALSE_JPEG;
	first_pass_init(enc);
	emit_markers(enc);
	return TRUE_JPEG;
}

BooleanJpeg jpg_open(jpeg_encoder_t * enc, int p_x_res, int p_y_res, int src_channels)
{
	switch (enc->m_params.m_subsampling)
	{
		case 0:
		{
			enc->m_num_components = 1;
			enc->m_comp_h_samp[0] = 1; enc->m_comp_v_samp[0] = 1;
			enc->m_mcu_x          = 8; enc->m_mcu_y          = 8;
			break;
		}
		case 1:
		{
			enc->m_num_components = 3;
			enc->m_comp_h_samp[0] = 1; enc->m_comp_v_samp[0] = 1;
			enc->m_comp_h_samp[1] = 1; enc->m_comp_v_samp[1] = 1;
			enc->m_comp_h_samp[2] = 1; enc->m_comp_v_samp[2] = 1;
			enc->m_mcu_x          = 8; enc->m_mcu_y          = 8;
			break;
		}
		case 2:
		{
			enc->m_num_components = 3;
			enc->m_comp_h_samp[0] = 2; enc->m_comp_v_samp[0] = 1;
			enc->m_comp_h_samp[1] = 1; enc->m_comp_v_samp[1] = 1;
			enc->m_comp_h_samp[2] = 1; enc->m_comp_v_samp[2] = 1;
			enc->m_mcu_x          = 16; enc->m_mcu_y         = 8;
			break;
		}
		case 3:
		{
			enc->m_num_components = 3;
			enc->m_comp_h_samp[0] = 2; enc->m_comp_v_samp[0] = 2;
			enc->m_comp_h_samp[1] = 1; enc->m_comp_v_samp[1] = 1;
			enc->m_comp_h_samp[2] = 1; enc->m_comp_v_samp[2] = 1;
			enc->m_mcu_x          = 16; enc->m_mcu_y         = 16;
		}
	}

	enc->m_image_x        = p_x_res;
	enc->m_image_y        = p_y_res;
	enc->m_image_bpp      = src_channels;
	enc->m_image_bpl      = enc->m_image_x * src_channels;

	enc->m_image_x_mcu    = (enc->m_image_x + enc->m_mcu_x - 1) & (~(enc->m_mcu_x - 1));
	enc->m_image_y_mcu    = (enc->m_image_y + enc->m_mcu_y - 1) & (~(enc->m_mcu_y - 1));
	enc->m_image_bpl_xlt  = enc->m_image_x * enc->m_num_components;
	enc->m_image_bpl_mcu  = enc->m_image_x_mcu * enc->m_num_components;
	enc->m_mcus_per_row   = enc->m_image_x_mcu / enc->m_mcu_x;



	JPGE_ALLOC_CHECK(enc->m_mcu_lines = (uint8_t **)(jpge_cmalloc(enc->m_mcu_y * sizeof(uint8_t *))));

	int i;
	for (i = 0; i < enc->m_mcu_y; i++)
		JPGE_ALLOC_CHECK(enc->m_mcu_lines[i] = (uint8_t *)(jpge_malloc(enc->m_image_bpl_mcu)));

	JPGE_ALLOC_CHECK(enc->m_sample_array = (sample_array_t *)(jpge_malloc(64 * sizeof(sample_array_t))));
	JPGE_ALLOC_CHECK(enc->m_coefficient_array = (int16_t *)(jpge_malloc(64 * sizeof(int16_t))));

	JPGE_ALLOC_CHECK(enc->m_quantization_tables[0]        = (int32_t *)(jpge_malloc(64 * sizeof(int32_t))));
	JPGE_ALLOC_CHECK(enc->m_quantization_tables[1]        = (int32_t *)(jpge_malloc(64 * sizeof(int32_t))));

	compute_quant_table(enc, enc->m_quantization_tables[0], s_std_lum_quant);
	compute_quant_table(enc, enc->m_quantization_tables[1], enc->m_params.m_no_chroma_discrim_flag ? s_std_lum_quant : s_std_croma_quant);

	JPGE_ALLOC_CHECK(enc->m_out_buf_ofs = enc->m_out_buf = (uint8_t *)(jpge_malloc(enc->m_out_buf_left = JPGE_OUT_BUF_SIZE)));

	JPGE_ALLOC_CHECK(enc->m_huff_bits[0+0] = (uint8_t *)(jpge_cmalloc(17)));
	JPGE_ALLOC_CHECK(enc->m_huff_val [0+0] = (uint8_t *)(jpge_cmalloc(DC_LUM_CODES)));
	JPGE_ALLOC_CHECK(enc->m_huff_bits[2+0] = (uint8_t *)(jpge_cmalloc(17)));
	JPGE_ALLOC_CHECK(enc->m_huff_val [2+0] = (uint8_t *)(jpge_cmalloc(AC_LUM_CODES)));
	JPGE_ALLOC_CHECK(enc->m_huff_bits[0+1] = (uint8_t *)(jpge_cmalloc(17)));
	JPGE_ALLOC_CHECK(enc->m_huff_val [0+1] = (uint8_t *)(jpge_cmalloc(DC_CHROMA_CODES)));
	JPGE_ALLOC_CHECK(enc->m_huff_bits[2+1] = (uint8_t *)(jpge_cmalloc(17)));
	JPGE_ALLOC_CHECK(enc->m_huff_val [2+1] = (uint8_t *)(jpge_cmalloc(AC_CHROMA_CODES)));

	memcpy(enc->m_huff_bits[0+0], s_dc_lum_bits, 17);
	memcpy(enc->m_huff_val [0+0], s_dc_lum_val, DC_LUM_CODES);
	memcpy(enc->m_huff_bits[2+0], s_ac_lum_bits, 17);
	memcpy(enc->m_huff_val [2+0], s_ac_lum_val, AC_LUM_CODES);
	memcpy(enc->m_huff_bits[0+1], s_dc_chroma_bits, 17);
	memcpy(enc->m_huff_val [0+1], s_dc_chroma_val, DC_CHROMA_CODES);
	memcpy(enc->m_huff_bits[2+1], s_ac_chroma_bits, 17);
	memcpy(enc->m_huff_val [2+1], s_ac_chroma_val, AC_CHROMA_CODES);

	if (!second_pass_init(enc)) return FALSE_JPEG;

	return enc->m_all_stream_writes_succeeded;
}

void load_block_8_8_grey(jpeg_encoder_t * enc, int x)
{
	uint8_t *src;
	sample_array_t *dst = enc->m_sample_array;

	x <<= 3;

	int i;
	for (i = 0; i < 8; i++, dst += 8)
	{
		src = enc->m_mcu_lines[i] + x;

		dst[0] = src[0] - 128; dst[1] = src[1] - 128; dst[2] = src[2] - 128; dst[3] = src[3] - 128;
		dst[4] = src[4] - 128; dst[5] = src[5] - 128; dst[6] = src[6] - 128; dst[7] = src[7] - 128;
	}
}

void load_block_8_8(jpeg_encoder_t * enc, int x, int y, int c)
{
	uint8_t * src;
	sample_array_t *dst = enc->m_sample_array;

	x = (x * (8 * 3)) + c;
	y <<= 3;

	int i;
	for (i = 0; i < 8; i++, dst += 8)
	{
		src = enc->m_mcu_lines[y + i] + x;

		dst[0] = src[0 * 3] - 128; dst[1] = src[1 * 3] - 128; dst[2] = src[2 * 3] - 128; dst[3] = src[3 * 3] - 128;
		dst[4] = src[4 * 3] - 128; dst[5] = src[5 * 3] - 128; dst[6] = src[6 * 3] - 128; dst[7] = src[7 * 3] - 128;
	}
}

void load_block_16_8(jpeg_encoder_t * enc, int x, int c)
{
	uint8_t *src1, *src2;
	sample_array_t *dst = enc->m_sample_array;

	x = (x * (16 * 3)) + c;

	int a = 0, b = 2, i;
	for (i = 0; i < 16; i += 2, dst += 8)
	{
		src1 = enc->m_mcu_lines[i + 0] + x;
		src2 = enc->m_mcu_lines[i + 1] + x;

		dst[0] = ((src1[ 0 * 3] + src1[ 1 * 3] + src2[ 0 * 3] + src2[ 1 * 3] + a) >> 2) - 128;
		dst[1] = ((src1[ 2 * 3] + src1[ 3 * 3] + src2[ 2 * 3] + src2[ 3 * 3] + b) >> 2) - 128;
		dst[2] = ((src1[ 4 * 3] + src1[ 5 * 3] + src2[ 4 * 3] + src2[ 5 * 3] + a) >> 2) - 128;
		dst[3] = ((src1[ 6 * 3] + src1[ 7 * 3] + src2[ 6 * 3] + src2[ 7 * 3] + b) >> 2) - 128;
		dst[4] = ((src1[ 8 * 3] + src1[ 9 * 3] + src2[ 8 * 3] + src2[ 9 * 3] + a) >> 2) - 128;
		dst[5] = ((src1[10 * 3] + src1[11 * 3] + src2[10 * 3] + src2[11 * 3] + b) >> 2) - 128;
		dst[6] = ((src1[12 * 3] + src1[13 * 3] + src2[12 * 3] + src2[13 * 3] + a) >> 2) - 128;
		dst[7] = ((src1[14 * 3] + src1[15 * 3] + src2[14 * 3] + src2[15 * 3] + b) >> 2) - 128;

		int temp = a; a = b; b = temp;
	}
}

void load_block_16_8_8(jpeg_encoder_t * enc, int x, int c)
{
	uint8_t *src1;
	sample_array_t *dst = enc->m_sample_array;

	x = (x * (16 * 3)) + c;

	int a = 0, b = 2, i;
	for (i = 0; i < 8; i++, dst += 8)
	{
		src1 = enc->m_mcu_lines[i + 0] + x;

		dst[0] = ((src1[ 0 * 3] + src1[ 1 * 3] + a) >> 1) - 128;
		dst[1] = ((src1[ 2 * 3] + src1[ 3 * 3] + b) >> 1) - 128;
		dst[2] = ((src1[ 4 * 3] + src1[ 5 * 3] + a) >> 1) - 128;
		dst[3] = ((src1[ 6 * 3] + src1[ 7 * 3] + b) >> 1) - 128;
		dst[4] = ((src1[ 8 * 3] + src1[ 9 * 3] + a) >> 1) - 128;
		dst[5] = ((src1[10 * 3] + src1[11 * 3] + b) >> 1) - 128;
		dst[6] = ((src1[12 * 3] + src1[13 * 3] + a) >> 1) - 128;
		dst[7] = ((src1[14 * 3] + src1[15 * 3] + b) >> 1) - 128;

		int temp = a; a = b; b = temp;
	}
}

void load_coefficients(jpeg_encoder_t * enc, int component_num)
{
	int32_t *q = enc->m_quantization_tables[component_num > 0];
	int16_t *dst = enc->m_coefficient_array;
	int i;
	for (i = 0; i < 64; i++)
	{
		sample_array_t j = enc->m_sample_array[s_zag[i]];

		if (j < 0)
		{
			if ((j = -j + (*q >> 1)) < *q)
				*dst++ = 0;
			else
				*dst++ = -(j / *q);
		}
		else
		{
			if ((j = j + (*q >> 1)) < *q)
				*dst++ = 0;
			else
				*dst++ = (j / *q);
		}

		q++;
	}
}

void flush_output_buffer(jpeg_encoder_t * enc)
{
	if (enc->m_out_buf_left != JPGE_OUT_BUF_SIZE)
	{
		enc->m_all_stream_writes_succeeded = enc->m_all_stream_writes_succeeded &&
											put_buf(&enc->m_pStream, enc->m_out_buf, JPGE_OUT_BUF_SIZE - enc->m_out_buf_left);
	}

	enc->m_out_buf_ofs  = enc->m_out_buf;
	enc->m_out_buf_left = JPGE_OUT_BUF_SIZE;
}

void put_bits(jpeg_encoder_t * enc, uint bits, uint8_t len)
{
	bits &= ((1UL << len) - 1);

	enc->m_bit_buffer |= ((uint32_t)bits << (24 - (enc->m_bits_in += len)));

	while (enc->m_bits_in >= 8)
	{
		uint8_t c;

		#define JPGE_PUT_BYTE(c) { *((enc->m_out_buf_ofs)++) = (c); if (--(enc->m_out_buf_left) == 0) flush_output_buffer(enc); }
		JPGE_PUT_BYTE(c = (uint8_t)((enc->m_bit_buffer >> 16) & 0xFF));
		if (c == 0xFF) JPGE_PUT_BYTE(0);

		enc->m_bit_buffer <<= 8;
		enc->m_bits_in -= 8;
	}
}

void code_coefficients_pass_two(jpeg_encoder_t * enc, int component_num)
{
	int i, j, run_len, nbits, temp1, temp2;
	int16_t *src = enc->m_coefficient_array;
	uint *codes[2];
	uint8_t *code_sizes[2];

	if (component_num == 0)
	{
		codes[0] = enc->m_huff_codes[0 + 0];
		codes[1] = enc->m_huff_codes[2 + 0];
		code_sizes[0] = enc->m_huff_code_sizes[0 + 0];
		code_sizes[1] = enc->m_huff_code_sizes[2 + 0];
	}
	else
	{
		codes[0] = enc->m_huff_codes[0 + 1];
		codes[1] = enc->m_huff_codes[2 + 1];
		code_sizes[0] = enc->m_huff_code_sizes[0 + 1];
		code_sizes[1] = enc->m_huff_code_sizes[2 + 1];
	}

	temp1 = temp2 = src[0] - enc->m_last_dc_val[component_num];
	enc->m_last_dc_val[component_num] = src[0];

	if (temp1 < 0)
	{
		temp1 = -temp1;
		temp2--;
	}

	nbits = 0;
	while (temp1)
	{
		nbits++;
		temp1 >>= 1;
	}

	put_bits(enc, codes[0][nbits], code_sizes[0][nbits]);
	if (nbits)
		put_bits(enc, temp2, nbits);

	for (run_len = 0, i = 1; i < 64; i++)
	{
		if ((temp1 = enc->m_coefficient_array[i]) == 0)
			run_len++;
		else
		{
			while (run_len >= 16)
			{
				put_bits(enc, codes[1][0xF0], code_sizes[1][0xF0]);
				run_len -= 16;
			}

			if ((temp2 = temp1) < 0)
			{
				temp1 = -temp1;
				temp2--;
			}

			nbits = 1;
			while (temp1 >>= 1)
				nbits++;

			j = (run_len << 4) + nbits;

			put_bits(enc, codes[1][j], code_sizes[1][j]);
			put_bits(enc, temp2, nbits);

			run_len = 0;
		}
	}

	if (run_len)
		put_bits(enc, codes[1][0], code_sizes[1][0]);
}

void code_block(jpeg_encoder_t * enc, int component_num)
{
	dct(enc->m_sample_array);
	load_coefficients(enc, component_num);
	code_coefficients_pass_two(enc, component_num);
}

void process_mcu_row(jpeg_encoder_t * enc)
{
	int i;
	if (enc->m_num_components == 1)
	{
		for (i = 0; i < enc->m_mcus_per_row; i++)
		{
			load_block_8_8_grey(enc, i); code_block(enc, 0);
		}
	}
	else if ((enc->m_comp_h_samp[0] == 1) && (enc->m_comp_v_samp[0] == 1))
	{
		for (i = 0; i < enc->m_mcus_per_row; i++)
		{
			load_block_8_8(enc, i, 0, 0); code_block(enc, 0);
			load_block_8_8(enc, i, 0, 1); code_block(enc, 1);
			load_block_8_8(enc, i, 0, 2); code_block(enc, 2);
		}
	}
	else if ((enc->m_comp_h_samp[0] == 2) && (enc->m_comp_v_samp[0] == 1))
	{
		for (i = 0; i < enc->m_mcus_per_row; i++)
		{
			load_block_8_8(enc, i * 2 + 0, 0, 0); code_block(enc, 0);
			load_block_8_8(enc, i * 2 + 1, 0, 0); code_block(enc, 0);
			load_block_16_8_8(enc, i, 1); code_block(enc, 1);
			load_block_16_8_8(enc, i, 2); code_block(enc, 2);
		}
	}
	else if ((enc->m_comp_h_samp[0] == 2) && (enc->m_comp_v_samp[0] == 2))
	{
		for (i = 0; i < enc->m_mcus_per_row; i++)
		{
			load_block_8_8(enc, i * 2 + 0, 0, 0); code_block(enc, 0);
			load_block_8_8(enc, i * 2 + 1, 0, 0); code_block(enc, 0);
			load_block_8_8(enc, i * 2 + 0, 1, 0); code_block(enc, 0);
			load_block_8_8(enc, i * 2 + 1, 1, 0); code_block(enc, 0);
			load_block_16_8(enc, i, 1); code_block(enc, 1);
			load_block_16_8(enc, i, 2); code_block(enc, 2);
		}
	}
}

BooleanJpeg terminate_pass_two(jpeg_encoder_t * enc)
{
	put_bits(enc, 0x7F, 7);
	flush_output_buffer(enc);
	emit_marker(enc, M_EOI);
	return TRUE_JPEG;
}

BooleanJpeg process_end_of_image(jpeg_encoder_t * enc)
{
	if (enc->m_mcu_y_ofs)
	{
		int i;
		for (i = enc->m_mcu_y_ofs; i < enc->m_mcu_y; i++)
			memcpy(enc->m_mcu_lines[i], enc->m_mcu_lines[enc->m_mcu_y_ofs - 1], enc->m_image_bpl_mcu);

		process_mcu_row(enc);
	}

	return terminate_pass_two(enc);
}

void load_mcu(jpeg_encoder_t * enc, const void *src)
{
	const uint8_t * Psrc = (const uint8_t *)src;

	// OK to write up to m_image_bpl_xlt bytes to Pdst
	uint8_t * Pdst = enc->m_mcu_lines[enc->m_mcu_y_ofs];

	if (enc->m_num_components == 1)
	{
		if (enc->m_image_bpp == 3)
			RGB_to_Y(Pdst, Psrc, enc->m_image_x);
		else
			memcpy(Pdst, Psrc, enc->m_image_x);
	}
	else
	{
		if (enc->m_image_bpp == 3)
			RGB_to_YCC(Pdst, Psrc, enc->m_image_x);
		else
			Y_to_YCC(Pdst, Psrc, enc->m_image_x);
	}

	if (enc->m_num_components == 1)
		memset(enc->m_mcu_lines[enc->m_mcu_y_ofs] + enc->m_image_bpl_xlt, Pdst[enc->m_image_bpl_xlt - 1], enc->m_image_x_mcu - enc->m_image_x);
	else
	{
		const uint8_t y  = Pdst[enc->m_image_bpl_xlt - 3 + 0];
		const uint8_t cb = Pdst[enc->m_image_bpl_xlt - 3 + 1];
		const uint8_t cr = Pdst[enc->m_image_bpl_xlt - 3 + 2];
		uint8_t *dst = enc->m_mcu_lines[enc->m_mcu_y_ofs] + enc->m_image_bpl_xlt;

		int i;
		for (i = enc->m_image_x; i < enc->m_image_x_mcu; i++)
		{
			*dst++ = y;
			*dst++ = cb;
			*dst++ = cr;
		}
	}

	if (++enc->m_mcu_y_ofs == enc->m_mcu_y)
	{
		process_mcu_row(enc);
		enc->m_mcu_y_ofs = 0;
	}
}

void clear(jpeg_encoder_t * enc)
{
	enc->m_num_components = 0;
	enc->m_image_x = 0;
	enc->m_image_y = 0;
	enc->m_image_bpp = 0;
	enc->m_image_bpl = 0;
	enc->m_image_x_mcu = 0;
	enc->m_image_y_mcu = 0;
	enc->m_image_bpl_xlt = 0;
	enc->m_image_bpl_mcu = 0;
	enc->m_mcus_per_row = 0;
	enc->m_mcu_x = 0;
	enc->m_mcu_y = 0;
	enc->m_mcu_lines = NULL;
	enc->m_mcu_y_ofs = 0;
	enc->m_sample_array = NULL;
	enc->m_coefficient_array = NULL;
	enc->m_out_buf = NULL;
	enc->m_out_buf_ofs = NULL;
	enc->m_out_buf_left = 0;
	enc->m_bit_buffer = 0;
	enc->m_bits_in = 0;

	memset(enc->m_comp_h_samp, 0, sizeof(enc->m_comp_h_samp));
	memset(enc->m_comp_v_samp, 0, sizeof(enc->m_comp_v_samp));
	memset(enc->m_quantization_tables, 0, sizeof(enc->m_quantization_tables));
	memset(enc->m_huff_codes, 0, sizeof(enc->m_huff_codes));
	memset(enc->m_huff_code_sizes, 0, sizeof(enc->m_huff_code_sizes));
	memset(enc->m_huff_bits, 0, sizeof(enc->m_huff_bits));
	memset(enc->m_huff_val, 0, sizeof(enc->m_huff_val));
	memset(enc->m_last_dc_val, 0, sizeof(enc->m_last_dc_val));

	enc->m_all_stream_writes_succeeded = TRUE_JPEG;
}


BooleanJpeg init(jpeg_encoder_t * enc, output_stream *pStream, int width, int height, int src_channels, const jpeg_params_t * comp_params)
{
	deinit(enc);

	if ((!pStream) || (width < 1) || (height < 1)) return FALSE_JPEG;
	if ((src_channels != 1) && (src_channels != 3)) return FALSE_JPEG;
	if (!check(comp_params)) return FALSE_JPEG;

	enc->m_pStream = *pStream;
	enc->m_params = *comp_params;

	return jpg_open(enc, width, height, src_channels);
}

void deinit(jpeg_encoder_t * enc)
{
	int i;
	if (enc->m_mcu_lines)
	{
		for (i = 0; i < enc->m_mcu_y; i++)
			jpge_free(enc->m_mcu_lines[i]);
		jpge_free(enc->m_mcu_lines);
	}

	jpge_free(enc->m_sample_array);
	jpge_free(enc->m_coefficient_array);
	jpge_free(enc->m_quantization_tables[0]);
	jpge_free(enc->m_quantization_tables[1]);

	for (i = 0; i < 4; i++)
	{
		jpge_free(enc->m_huff_codes[i]);
		jpge_free(enc->m_huff_code_sizes[i]);
		jpge_free(enc->m_huff_bits[i]);
		jpge_free(enc->m_huff_val[i]);
	}
	jpge_free(enc->m_out_buf);
	clear(enc);
}

BooleanJpeg process_scanline(jpeg_encoder_t * enc, const void* pScanline)
{
	if (enc->m_all_stream_writes_succeeded)
	{
		if (!pScanline)
		{
			if (!process_end_of_image(enc))
				return FALSE_JPEG;
		}
		else
		{
			load_mcu(enc, pScanline);
		}
	}

	return enc->m_all_stream_writes_succeeded;
}
