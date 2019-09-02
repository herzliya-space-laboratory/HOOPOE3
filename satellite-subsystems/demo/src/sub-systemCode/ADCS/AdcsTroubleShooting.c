#include "AdcsTroubleShooting.h"
#include "../Main/CMD/ADCS_CMD.h"
#include "AdcsMain.h"
#include "../Main/CMD/General_CMD.h"

int AdcsTroubleShooting(TroubleErrCode trbl)
{
	switch(trbl)
		{
		case(TRBL_FAIL):
			break;
		case(TRBL_ADCS_INIT_ERR):
			break;
		case(TRBL_BOOT_ERROR):
			break;
		case(TRBL_CHANNEL_OFF):
				break;
		case(TRBL_CMD_ERR):
				break;
		case(TRBL_WRONG_SUB_TYPE):
				break;
		case(TRBL_WRONG_TYPE):
				break;
		case(TRBL_NULL_DATA):
				break;
		case(TRBL_FS_INIT_ERR):
				break;
		case(TRBL_FS_WRITE_ERR):
				break;
		case(TRBL_FS_READ_ERR):
				break;
		case(TRBL_FRAM_WRITE_ERR):
				break;
		case(TRBL_FRAM_READ_ERR):
				break;
		case(TRBL_INPUT_PARAM_ERR):
				break;
		case(TRBL_QUEUE_CREATE_ERR):
				break;
		case(TRBL_SEMAPHORE_CREATE_ERR):
				break;
		case(TRBL_TLM_ERR):
				break;
		case(TRBL_QUEUE_EMPTY):
				break;
		default:
			break;
		}
	return 0;
}

