/*
 * FileSystem.h
 *
 *  Created on: Aug 15, 2019
 *      Author: Roy's Laptop
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "../../Global/TLM_management.h"
#include "Macros.h"

void keepTryingTo_enterFS(void);
int OpenFile(F_FILE* file, const char* fileName, const char* mode);
int CloseFile(F_FILE* file);
int DeleteFile(const char* fileName);
int ReadFromFile(F_FILE* file, byte* buffer, uint32_t size_of_element, uint32_t number_of_elements);
int WriteToFile(F_FILE* file, byte* buffer, uint32_t size_of_element, uint32_t number_of_elements);

#endif /* FILESYSTEM_H_ */
