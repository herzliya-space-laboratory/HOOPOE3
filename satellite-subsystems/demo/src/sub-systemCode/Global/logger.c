/*
 * logger.c
 *
 *  Created on: 8 баев 2019
 *      Author: USER1
 */

#include <stdio.h>
#include "logger.h"

typedef struct
{
	int log_num;
	int info;
}LogStruct;

static FileSystemResult WriteLog(void *log, char filename[5])
{
	FileSystemResult fs = c_fileWrite(filename, log);
	if (fs == FS_SUCCSESS)
	{
		return FS_SUCCSESS;
	}
	if (fs == FS_NOT_EXIST)
	{
		fs = c_fileCreate(filename, sizeof(int));
		if (fs == FS_SUCCSESS)
		{
			return c_fileWrite(filename, log);
		}
		return fs;
	}
	return fs;
}

FileSystemResult WriteErrorLog(log_errors log_num, log_systems system, int info)
{
	LogStruct log;
	log.info = info;
	log.log_num = log_num + system * 50;
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

FileSystemResult WriteEpsLog(log_states log_num, int info)
{
	LogStruct log;
	log.info = info;
	log.log_num = log_num + EPS_LOG_OFFSET;
	return WriteLog(&log, EVENT_LOG_FILENAME);
}

FileSystemResult WriteTransponderLog(log_activation log_num, int info)
{
	LogStruct log;
	log.info = info;
	log.log_num = log_num + EPS_LOG_OFFSET;
	return WriteLog(&log, EVENT_LOG_FILENAME);
}
