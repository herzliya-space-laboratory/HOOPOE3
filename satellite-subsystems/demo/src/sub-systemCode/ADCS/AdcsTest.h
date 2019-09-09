/*
 * AdcsTest.h
 *
 *  Created on: Aug 11, 2019
 *      Author: ����
 */

#ifndef ADCSTEST_H_
#define ADCSTEST_H_

typedef enum tests
{
	SEND_CMD,
	MAG_TEST,
	ERR_FLAG_TEST,
	PRINT_ERR_FLAG,
	TEST_AMOUNT
}tests;
#endif /* ADCSTEST_H_ */

void AdcsTestTask();
void printErrFlag();
void Mag_Test();
void AddCommendToQ();
