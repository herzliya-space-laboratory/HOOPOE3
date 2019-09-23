/*
 * logger.c
 *
 *  Created on: 8 ���� 2019
 *      Author: USER1
 */

#include <stdio.h>
#include "logger.h"

#include "GenericTaskSave.h"

static FileSystemResult WriteLog(void *log, char filename[5])
{
	int error;

	saveRequest_task element;
	strcpy(element.file_name, filename);
	element.elementSize = LOG_STRUCT_ELEMENT_SIZE;
	memcpy(element.buffer, log, LOG_STRUCT_ELEMENT_SIZE);

	error = add_GenericElement_queue(element);
	if (error == -1)
	{
		printf("could not write to queue LOG\n\n");
		return -1;
	}

	return 0;
}

FileSystemResult WriteErrorLog(log_errors log_num, log_systems system, int info)
{
	LogStruct log;
	log.info = info;
	log.log_num = log_num + system * 500;
	return WriteLog(&log, ERROR_LOG_FILENAME);
}

FileSystemResult WriteResetLog(log_systems log_num, int info)
{
	LogStruct log;
	log.info = info;
	log.log_num = log_num + RESETS_LOG_OFFSET;
	return WriteLog(&log, EVENT_LOG_FILENAME);
}

FileSystemResult WritePayloadLog(log_payload log_num, int info)
{
	LogStruct log;
	log.info = info;
	log.log_num = log_num + PAYLOAD_LOG_OFFSET;
	return WriteLog(&log, EVENT_LOG_FILENAME);
}

FileSystemResult WriteEpsLog(log_eps log_num, int info)
{
	LogStruct log;
	log.info = info;
	log.log_num = log_num + EPS_LOG_OFFSET;
	return WriteLog(&log, EVENT_LOG_FILENAME);
}

FileSystemResult WriteTransponderLog(log_transponder log_num, int info)
{
	LogStruct log;
	log.info = info;
	log.log_num = log_num + TRANSPONDER_LOG_OFFSET;
	return WriteLog(&log, EVENT_LOG_FILENAME);
}

FileSystemResult WriteAdcsLog(log_adcs log_num, int info)
{
	LogStruct log;
	log.info = info;
	log.log_num = log_num + ADCS_LOG_OFFSET;
	return WriteLog(&log, EVENT_LOG_FILENAME);
}

FileSystemResult WriteCUFLog(log_cuf log_num, int info)
{
	LogStruct log;
	log.info = info;
	log.log_num = log_num + ADCS_LOG_OFFSET;
	return WriteLog(&log, EVENT_LOG_FILENAME);
}
