/*
 * Boolean_bit.h
 *
 *  Created on: Jun 16, 2019
 *  Author: Roy
 */

#ifndef BOOLEAN_BIT_H_
#define BOOLEAN_BIT_H_

typedef struct __attribute__((__packed__))
{
    unsigned value :1;
} bit;

typedef struct __attribute__((__packed__))
{
    bit bit0;
    bit bit1;
    bit bit2;
    bit bit3;
    bit bit4;
    bit bit5;
    bit bit6;
    bit bit7;
} byte_usingBits;

#define FALSE_bit 0
#define TRUE_bit 1

unsigned char convert2char(byte_usingBits* bits);

void convert2byte(unsigned char num, byte_usingBits* bits);

void bits2byte(bit bitArray[8], byte_usingBits* bits);

void byte2bits(byte_usingBits* bits, bit bitArray[8]);

void char2bits(unsigned char num, bit bitArray[8]);

unsigned char bits2char(bit bitArray[8]);

#endif /* BOOLEAN_BIT_H_ */
