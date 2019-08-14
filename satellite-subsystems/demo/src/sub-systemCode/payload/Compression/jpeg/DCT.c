/*
 * DCT.c
 *
 *  Created on: 12 αιεμ 2019
 *      Author: Tamir
 */

#include "DCT.h"

// Forward DCT
#define CONST_BITS 13
#define PASS1_BITS 2
#define FIX_0_298631336 ((int32_t)2446)    /* FIX(0.298631336) */
#define FIX_0_390180644 ((int32_t)3196)    /* FIX(0.390180644) */
#define FIX_0_541196100 ((int32_t)4433)    /* FIX(0.541196100) */
#define FIX_0_765366865 ((int32_t)6270)    /* FIX(0.765366865) */
#define FIX_0_899976223 ((int32_t)7373)    /* FIX(0.899976223) */
#define FIX_1_175875602 ((int32_t)9633)    /* FIX(1.175875602) */
#define FIX_1_501321110 ((int32_t)12299)   /* FIX(1.501321110) */
#define FIX_1_847759065 ((int32_t)15137)   /* FIX(1.847759065) */
#define FIX_1_961570560 ((int32_t)16069)   /* FIX(1.961570560) */
#define FIX_2_053119869 ((int32_t)16819)   /* FIX(2.053119869) */
#define FIX_2_562915447 ((int32_t)20995)   /* FIX(2.562915447) */
#define FIX_3_072711026 ((int32_t)25172)   /* FIX(3.072711026) */
#define DCT_DESCALE(x, n) (((x) + (((int32_t)1) << ((n) - 1))) >> (n))
#define DCT_MUL(var, const)  (((int16_t) (var)) * ((int32_t) (const)))

void dct(int32_t *data)
{
	int c;
	int32_t z1, z2, z3, z4, z5, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp10, tmp11, tmp12, tmp13, *data_ptr;

	data_ptr = data;

	for (c = 7; c >= 0; c--)
	{
		tmp0 = data_ptr[0] + data_ptr[7];
		tmp7 = data_ptr[0] - data_ptr[7];
		tmp1 = data_ptr[1] + data_ptr[6];
		tmp6 = data_ptr[1] - data_ptr[6];
		tmp2 = data_ptr[2] + data_ptr[5];
		tmp5 = data_ptr[2] - data_ptr[5];
		tmp3 = data_ptr[3] + data_ptr[4];
		tmp4 = data_ptr[3] - data_ptr[4];
		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		data_ptr[0] = (int32_t)((tmp10 + tmp11) << PASS1_BITS);
		data_ptr[4] = (int32_t)((tmp10 - tmp11) << PASS1_BITS);
		z1 = DCT_MUL(tmp12 + tmp13, FIX_0_541196100);
		data_ptr[2] = (int32_t)DCT_DESCALE(z1 + DCT_MUL(tmp13, FIX_0_765366865), CONST_BITS-PASS1_BITS);
		data_ptr[6] = (int32_t)DCT_DESCALE(z1 + DCT_MUL(tmp12, - FIX_1_847759065), CONST_BITS-PASS1_BITS);
		z1 = tmp4 + tmp7;
		z2 = tmp5 + tmp6;
		z3 = tmp4 + tmp6;
		z4 = tmp5 + tmp7;
		z5 = DCT_MUL(z3 + z4, FIX_1_175875602);
		tmp4 = DCT_MUL(tmp4, FIX_0_298631336);
		tmp5 = DCT_MUL(tmp5, FIX_2_053119869);
		tmp6 = DCT_MUL(tmp6, FIX_3_072711026);
		tmp7 = DCT_MUL(tmp7, FIX_1_501321110);
		z1   = DCT_MUL(z1, -FIX_0_899976223);
		z2   = DCT_MUL(z2, -FIX_2_562915447);
		z3   = DCT_MUL(z3, -FIX_1_961570560);
		z4   = DCT_MUL(z4, -FIX_0_390180644);
		z3 += z5;
		z4 += z5;
		data_ptr[7] = (int32_t)DCT_DESCALE(tmp4 + z1 + z3, CONST_BITS-PASS1_BITS);
		data_ptr[5] = (int32_t)DCT_DESCALE(tmp5 + z2 + z4, CONST_BITS-PASS1_BITS);
		data_ptr[3] = (int32_t)DCT_DESCALE(tmp6 + z2 + z3, CONST_BITS-PASS1_BITS);
		data_ptr[1] = (int32_t)DCT_DESCALE(tmp7 + z1 + z4, CONST_BITS-PASS1_BITS);
		data_ptr += 8;
	}

	data_ptr = data;

	for (c = 7; c >= 0; c--)
	{
		tmp0 = data_ptr[8*0] + data_ptr[8*7];
		tmp7 = data_ptr[8*0] - data_ptr[8*7];
		tmp1 = data_ptr[8*1] + data_ptr[8*6];
		tmp6 = data_ptr[8*1] - data_ptr[8*6];
		tmp2 = data_ptr[8*2] + data_ptr[8*5];
		tmp5 = data_ptr[8*2] - data_ptr[8*5];
		tmp3 = data_ptr[8*3] + data_ptr[8*4];
		tmp4 = data_ptr[8*3] - data_ptr[8*4];
		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		data_ptr[8*0] = (int32_t)DCT_DESCALE(tmp10 + tmp11, PASS1_BITS+3);
		data_ptr[8*4] = (int32_t)DCT_DESCALE(tmp10 - tmp11, PASS1_BITS+3);
		z1 = DCT_MUL(tmp12 + tmp13, FIX_0_541196100);
		data_ptr[8*2] = (int32_t)DCT_DESCALE(z1 + DCT_MUL(tmp13, FIX_0_765366865), CONST_BITS+PASS1_BITS+3);
		data_ptr[8*6] = (int32_t)DCT_DESCALE(z1 + DCT_MUL(tmp12, - FIX_1_847759065), CONST_BITS+PASS1_BITS+3);
		z1 = tmp4 + tmp7;
		z2 = tmp5 + tmp6;
		z3 = tmp4 + tmp6;
		z4 = tmp5 + tmp7;
		z5 = DCT_MUL(z3 + z4, FIX_1_175875602);
		tmp4 = DCT_MUL(tmp4, FIX_0_298631336);
		tmp5 = DCT_MUL(tmp5, FIX_2_053119869);
		tmp6 = DCT_MUL(tmp6, FIX_3_072711026);
		tmp7 = DCT_MUL(tmp7, FIX_1_501321110);
		z1   = DCT_MUL(z1, -FIX_0_899976223);
		z2   = DCT_MUL(z2, -FIX_2_562915447);
		z3   = DCT_MUL(z3, -FIX_1_961570560);
		z4   = DCT_MUL(z4, -FIX_0_390180644);
		z3 += z5;
		z4 += z5;
		data_ptr[8*7] = (int32_t)DCT_DESCALE(tmp4 + z1 + z3, CONST_BITS + PASS1_BITS + 3);
		data_ptr[8*5] = (int32_t)DCT_DESCALE(tmp5 + z2 + z4, CONST_BITS + PASS1_BITS + 3);
		data_ptr[8*3] = (int32_t)DCT_DESCALE(tmp6 + z2 + z3, CONST_BITS + PASS1_BITS + 3);
		data_ptr[8*1] = (int32_t)DCT_DESCALE(tmp7 + z1 + z4, CONST_BITS + PASS1_BITS + 3);
		data_ptr++;
	}
}
