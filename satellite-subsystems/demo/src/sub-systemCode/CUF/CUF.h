/*
 * CUF.h
 *
 *  Created on: 15 ���� 2018
 *      Author: USER1
 */

#ifndef CUF_H_
#define CUF_H_

#include <hal/boolean.h>
#include <hal/Storage/FRAM.h>
#include <satellite-subsystems/GomEPS.h>
#include "../Global/TLM_management.h"

#define TEMPORARYCUFSTORAGESIZE 50000 //size of the array that accumilates the temporary CUFs data
#define CUFARRSTORAGE 1000 //size of the array that accumilates the CUF functions
#define PERMINANTCUFLENGTH 100 //size of the array that accumilates the perminant CUF names
#define CUFNAMELENGTH 12 //length of the name of a CUF
#define uploadCodeLength 33120 //180 packets
#define PERCUFSAVEFILENAME "CUFPer.cuf" //name of file to save cuf slot array to
#define RESETSTHRESHOLD 5 //resets threshold before failsafe
#define CUFRAMADRESS 10 //The FRAM adress for the CUF data
//#define VTASKDELAY_CUF(xTicksToDelay) ((void (*)(portTickType))(CUFSwitch(0)))(xTicksToDelay)

//defines for AUC_CUF switch for ground commands
#define CUF_HEADER_ST 0 //header ahndle
#define CUF_ARRAY_ST 1 //add to array
#define CUF_INTEGRATE_ST 2 //integrate cuf
#define CUF_EXECUTE_ST 3 //execute cuf
#define CUF_SAVEBACKUP_ST 4 //save backup
#define CUF_LOADBACKUP_ST 5 //load backup
#define CUF_REMOVEFILES_ST 6 //remove files
#define CUF_REMOVE_ST 7 //remove cuf
#define CUF_DISABLE_ST 8 //disable cuf
#define CUF_ENABLE_ST 9 //enable cuf

//the struct of a CUF
typedef struct {
	char name[13]; //name of the CUF
	int* data; //its data
	unsigned int length; //the length of its data (in 4-byte int elements, not bytes!)
	unsigned long SSH; //the ssh associated with the CUF
	Boolean isTemporary; //is it temporary
	Boolean disabled; //is it disabled
} CUF;

/*!
 * Function to initiate the CUF functions array and reset the CUF slot array if needed
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
 * @param[in] len the length of the data (in bytes)
 * @return the generated ssh
 * 0 on fail
 */
unsigned long GenerateSSH(char* data, int len);

/*!
 * Function to authenticate a CUF
 * @param[in] CUF the cuf to be authenticated
 * @return 1 if authenticated
 * 0 if not authenticated
 * -1 on fail
 */
int AuthenticateCUF(CUF* code);

/*!
 * Function to save the CUF slot array to per save file
 * @return 0 on success
 */
int SavePerminantCUF();

/*!
 * Function to load the CUF slot array from per save file
 * @return 0 on success
 */
int LoadPerminantCUF();

/*!
 * Function to manage the CUF satellite restart routine
 * @return 0 on success
 */
int CUFManageRestart();

/*!
 * Function to add a permanent CUF to the CUF slot array
 * @param[in] CUF the cuf to be added to CUF slot array
 * @return 0 if successful
 * -3 if failed
 */
int UpdatePerminantCUF(CUF* code);

/*!
 * Function remove a CUF
 * @param[in] name the name of the cuf to be removed
 * @return 0 on success
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
 * -300 if cuf is un-authenticated
 */
int ExecuteCUF(char* name);

/*!
 * Function to integrate a new CUF into the system
 * @param[in] name name of cuf
 * @param[in] data data of cuf
 * @param[in] length length of cuf
 * @param[in] SSH SSH code of cuf
 * @param[in] isTemporary self explanatory
 * @return 0 on success
 * -1 on fail
 * -2 if cuf is un-authenticated
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
 * Function to test CUF functionality
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

/*!
 * checks if the satellite has had RESETSTHRESHOLD resets since first run (for CUF failsafe)
 * param[in] isFirst is this the first run of the satellite
 * @return the truth value of the statement above
 */
Boolean CheckResetThreashold(Boolean isFirst);

/*!
 * self explanatory
 * param[in] pointer the pointer to convert to int
 * @return the int
 */
unsigned int castCharPointerToInt(unsigned char* pointer);

/*!
 * self explanatory
 * param[in] array the char array to convert to int array
 * param[in] the length of the char array
 * @return the int array
 */
unsigned int* castCharArrayToIntArray(unsigned char* array, int length);

/*
 * adds the .cuf extension to a cuf name (string)
 * param[in] name the name to add the .cuf extension to
 * @return the extended name (with the .cuf extension)
 */
char* getExtendedName(char* name);

#endif /* CUF_H_ */
