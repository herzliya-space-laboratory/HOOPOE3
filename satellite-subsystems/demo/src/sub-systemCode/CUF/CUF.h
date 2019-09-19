/*
 * CUF.h
 *
 *  Created on: 15 ���� 2018
 *      Author: USER1
 */

#ifndef CUF_H_
#define CUF_H_

#include <hal/boolean.h>
#include "../Global/TLM_management.h"

#define TEMPORARYCUFSTORAGESIZE 50000 //size of the array that accumilates the temporary CUFs data
#define CUFARRSTORAGE 1000 //size of the array that accumilates the CUF functions
#define PERMINANTCUFLENGTH 100 //size of the array that accumilates the perminant CUF names
#define CUFNAMELENGTH 12 //length of the name of a CUF
#define uploadCodeLength 33120 //180 packets
#define PERCUFSAVEFILENAME "CUFPer"
//#define VTASKDELAY_CUF(xTicksToDelay) ((void (*)(portTickType))(CUFSwitch(0)))(xTicksToDelay)

//the struct of a CUF
typedef struct {
	char* name; //name of the CUF
	int* data; //its data
	unsigned int length; //the length of its data (in 4-byte int elements, not bytes!)
	unsigned long SSH; //the ssh associated with the CUF
	Boolean isTemporary; //is it temporary
	Boolean disabled; //is it disabled
} CUF;

/*!
 * Function to initiate the CUF functions array and reset the CUF slot array if needed
 * @param[in] isFirstRun boolean to indicate if the CUF slot array shuld be reset
 * @return 0 on success
 */
int InitCUF();

/*!
 * Function to enable CUF function use
 * @param[in] index the index of the function to be returned
 * @return the function with the given index in the array
 */
void* CUFSwitch(int index);

/*!
 * Function to generate an ssh to a data sample
 * @param[in] data the data to generate the ssh from
 * @return the generated ssh
 * 0 on fail
 */
unsigned long GenerateSSH(unsigned char* data);

/*!
 * Function to authenticate a CUF
 * @param[in] CUF the cuf to be authenticated
 * @return 1 if authenticated
 * 0 if not authenticated
 * -1 on fail
 */
int AuthenticateCUF(CUF* code);

/*!
 * Function to save the CUF slot array to FRAM
 * @return 0 on success
 */
int SavePerminantCUF();

/*!
 * Function to load the CUF slot array from FRAM
 * @return 0 on success
 */
int LoadPerminantCUF();

/*!
 * Function to manage the CUF satellite restart rutine
 * @return 0 on success
 */
int CUFManageRestart();

/*!
 * Function to add a perminant CUF to the CUF slot array
 * @param[in] CUF the cuf to be added to CUF slot array
 * @return 0 if successfull
 * -3 if failed
 */
int UpdatePerminantCUF(CUF* code);

/*!
 * Function remove a CUF
 * @param[in] name the name of the cuf to be removed
 * @return 0 if successfull
 * -1 if failed
 */
int RemoveCUF(char* name);

/*!
 * Function to be converted to binary and used as test
 * @param[in] CUFSwitch switch function with all the satellites functions
 * @return what CUFTestFunction2 returns + 1
 */
int CUFTestFunction(void* (*CUFSwitch)(int));

/*!
 * Function to be converted to binary and used as test for multipul function test
 * @param[in] index the index to be tested
 * @return the index + 1
 */
int CUFTestFunction2(int index);

/*!
 * Function to execute a CUF
 * @param[in] name the name of the cuf to be executed
 * @return what the CUF execution returns
 * -200 on fail (not to confuse with -1 which can be an output of the CUFs execution if the CUFs code failed)
 * -500 if cuf is un-authenticated
 */
int ExecuteCUF(char* name);

/*!
 * Function to integrate a new CUF into the system
 * @param[in] name name of cuf
 * @param[in] data data of cuf
 * @param[in] length length of cuf
 * @param[in] SSH SSH code of cuf
 * @param[in] isTemporary self explanatory
 * @return the integrated CUF on success
 * NULL on fail
 */
int IntegrateCUF(char* name, int* data, unsigned int length, unsigned long SSH, Boolean isTemporary, Boolean disabled);

/*!
 * Function to add CUF to the temp array
 * @param[in] temp the cuf to add
 */
void AddToCUFTempArray(CUF* temp);

/*!
 * Function to get CUF from the temp array
 * @param[in] name the name of the cuf to get
 * @return the cuf on success
 * NULL on fail (if its not in the array)
 */
CUF* GetFromCUFTempArray(char* name);

/*!
 * Function to test CUF  functionality
 * @return TRUE
 */
Boolean CUFTest();

/*!
 * disables given CUF from running on start
 */
void DisableCUF(char* name);

/*!
 * enables given CUF to run on start
 */
void EnableCUF(char* name);

unsigned int castCharPointerToInt(unsigned char* pointer);
unsigned int* castCharArrayToIntArray(unsigned char* array, int length);
char* getExtendedName(char* name);

#endif /* CUF_H_ */
