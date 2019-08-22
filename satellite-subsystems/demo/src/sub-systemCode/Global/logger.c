/*
 * logger.c
 *
 *  Created on: 8 баев 2019
 *      Author: USER1
 */


#include "logger.h"
#include "TM_managment.h"
#include <stdio.h>

#define ERROR_LOG_FILENAME "error"
#define RESET_LOG_FILENAME "reset"
#define PAYLOAD_LOG_FILENAME "image"
#define EPS_LOG_FILENAME "EPS"
#define TRANSPONDER_LOG_FILENAME "TRXVU"

FileSystemResult WriteLog(void *log, char filename[5])
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

FileSystemResult WriteErrorLog(log_errors ErrNum)
{
	return WriteLog(&ErrNum, ERROR_LOG_FILENAME);
}

FileSystemResult WriteResetLog(log_systems SysNum)
{
	return WriteLog(&SysNum, RESET_LOG_FILENAME);
}

FileSystemResult WritePayloadLog(log_payload ImgNum)
{
	return WriteLog(&ImgNum, PAYLOAD_LOG_FILENAME);
}

FileSystemResult WriteEpsLog(log_states StateNum)
{
	return WriteLog(&StateNum, EPS_LOG_FILENAME);
}

FileSystemResult WriteTransponderLog(log_activation activity)
{
	return WriteLog(&activity, TRANSPONDER_LOG_FILENAME);
}
