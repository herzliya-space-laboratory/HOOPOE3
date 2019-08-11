#include "AdcsTroubleShooting.h"
#include "../Main/CMD/ADCS_CMD.h"
#include "AdcsMain.h"
#include "../Main/CMD/General_CMD.h"

int AdcsTroubleShooting(TroubleErrCode trbl)
{
	//todo: Add log
	byte *data;
	switch(trbl)
		{
		case(TRBL_FAIL):
			cspaceADCS_componentReset(ADCS_ID);
			break;
		case(TRBL_ADCS_INIT_ERR):
			cspaceADCS_componentReset(ADCS_ID);
			//todo: the init function return this err can't be called twice
			break;
		case(TRBL_BOOT_ERROR):
			cspaceADCS_componentReset(ADCS_ID);
			data = malloc(sizeof(cspace_adcs_geninfo_t));
			cspaceADCS_getGeneralInfo(ADCS_ID, data);
			cspaceADCS_BLSetBootIndex(ADCS_ID,1);
			cspaceADCS_BLRunSelectedProgram(ADCS_ID);
			break;
		case(TRBL_CHANNEL_OFF):
			while(SWITCH_OFF == get_system_state(ADCS_param)){
				vTaskDelay(CHANNEL_OFF_DELAY);
			}
				// todo: check channel while()

				break;
		case(TRBL_CMD_ERR):
				//todo:be more specific abut your err cods
				break;
		case(TRBL_WRONG_SUB_TYPE):

				break;
		case(TRBL_WRONG_TYPE):
				//that can never happen its in elai code
				break;
		case(TRBL_NULL_DATA):
				AdcsCmdQueueAddReset();
				break;
		case(TRBL_FS_INIT_ERR):
				//todo:check about FS errs
				break;
		case(TRBL_FS_WRITE_ERR):

				break;
		case(TRBL_FS_READ_ERR):

				break;
		case(TRBL_FRAM_WRITE_ERR):
				//todo:check abut fram errs
				break;
		case(TRBL_FRAM_READ_ERR):

				break;
		case(TRBL_INPUT_PARAM_ERR):

				break;
		case(TRBL_QUEUE_CREATE_ERR):
				if(TRBL_SUCCESS != AdcsCmdQueueInit()){
					//todo: find a different way to manage cmd to skip the Q
				}
				break;
		case(TRBL_SEMAPHORE_CREATE_ERR):
				//todo: SEMAPHORE?
				break;
		case(TRBL_TLM_ERR):
				//todo:make it more spacifc
				break;
		case(TRBL_QUEUE_EMPTY):
				//that good, no log for you!!! come back, one year!!!
				break;
		default:
			break;
		}
	return 0;
}


TroubleErrCode FindErr()
{

	return TRBL_SUCCESS;
}
