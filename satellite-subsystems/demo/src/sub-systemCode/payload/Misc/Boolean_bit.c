/*
 * Boolean_bit.c
 *
 *  Created on: Jun 19, 2019
 *      Author: Roy's Laptop
 */

#include "Boolean_bit.h"

unsigned char convert2char(byte_usingBits* bits)
{
	unsigned char num = 0;

	if (bits->bit0.value)
		num += 1;

	if (bits->bit1.value)
		num += 2;

	if (bits->bit2.value)
		num += 4;

	if (bits->bit3.value)
		num += 8;

	if (bits->bit4.value)
		num += 16;

	if (bits->bit5.value)
		num += 32;

	if (bits->bit6.value)
		num += 64;

	if (bits->bit7.value)
		num += 128;

	return num;
}

void convert2byte(unsigned char num, byte_usingBits* bits)
{
	if(num & 0b10000000)
		bits->bit7.value = TRUE_bit;
	else
		bits->bit7.value = FALSE_bit;

	if(num & 0b01000000)
		bits->bit6.value = TRUE_bit;
	else
		bits->bit6.value = FALSE_bit;

	if(num & 0b00100000)
		bits->bit5.value = TRUE_bit;
	else
		bits->bit5.value = FALSE_bit;

	if(num & 0b00010000)
		bits->bit4.value = TRUE_bit;
	else
		bits->bit4.value = FALSE_bit;

	if(num & 0b00001000)
		bits->bit3.value = TRUE_bit;
	else
		bits->bit3.value = FALSE_bit;

	if(num & 0b00000100)
		bits->bit2.value = TRUE_bit;
	else
		bits->bit2.value = FALSE_bit;

	if(num & 0b00000010)
		bits->bit1.value = TRUE_bit;
	else
		bits->bit1.value = FALSE_bit;

	if(num & 0b00000001)
		bits->bit0.value = TRUE_bit;
	else
		bits->bit0.value = FALSE_bit;
}

void bits2byte(bit bitArray[8], byte_usingBits* bits)
{
	bits->bit0.value = bitArray[0].value;
	bits->bit1.value = bitArray[1].value;
	bits->bit2.value = bitArray[2].value;
	bits->bit3.value = bitArray[3].value;
	bits->bit4.value = bitArray[4].value;
	bits->bit5.value = bitArray[5].value;
	bits->bit6.value = bitArray[6].value;
	bits->bit7.value = bitArray[7].value;
}

void byte2bits(byte_usingBits* bits, bit bitArray[8])
{
	bitArray[0].value = bits->bit0.value;
	bitArray[1].value = bits->bit1.value;
	bitArray[2].value = bits->bit2.value;
	bitArray[3].value = bits->bit3.value;
	bitArray[4].value = bits->bit4.value;
	bitArray[5].value = bits->bit5.value;
	bitArray[6].value = bits->bit6.value;
	bitArray[7].value = bits->bit7.value;
}

void char2bits(unsigned char num, bit bitArray[8])
{
	byte_usingBits byte;

	convert2byte(num, &byte);

	byte2bits(&byte, bitArray);
}

unsigned char bits2char(bit bitArray[8])
{
	byte_usingBits bits;

	bits2byte(bitArray, &bits);

	return convert2char(&bits);
}
