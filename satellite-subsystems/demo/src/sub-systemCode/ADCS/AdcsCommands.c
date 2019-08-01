#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <hal/Drivers/I2C.h>
#include <hal/errors.h>

#include "AdcsCommands.h"
#include "Adcs_Main.h"

#include <satellite-subsystems/cspaceADCS_types.h>
#include <satellite-subsystems/cspaceADCS.h>

#include "sub-systemCode/COMM/splTypes.h"	// for ADCS command subtypes


int AdcsReadI2cAck(int *rv)
{
	if(NULL == rv){
		return -2;
	}

	int err = 0;
	unsigned char data[3] = {0};
	unsigned char id = ADCS_I2C_TELEMETRY_ACK_REQUEST;
	err = I2C_write(ADCS_I2C_ADRR, &id, 1);
	if(0 != err){
		return err;
	}
	//vTaskDelay(5);				// todo: check if really need this delay. the delay can overwrite the requested data
	err = I2C_read(ADCS_I2C_ADRR,  data, sizeof(data) - 1);
	if(0 != err){
		return err;
	}
	*rv = data[2];
	return 0;
}

int AdcsGenericI2cCmd(unsigned char* data, unsigned int length , int *ack)
{
	if(ack == NULL || NULL == data){
		return -2;
	}
	if(0 == length){
		return -2;
	}

	int err = I2C_write(ADCS_I2C_ADRR, data, length);
	if(0 != err){
		//TODO: log I2c write Err
		return err;
	}
	err = AdcsReadI2cAck(ack);
	return err;
}

int AdcsI2cCmdWithID(unsigned char id,unsigned char* data, unsigned int length , int *ack)
{
	if(NULL == ack){
		return -2;
	}
	if(NULL == data ){
		return AdcsGenericI2cCmd(&id,sizeof(id),ack);
	}

	unsigned char *buffer = malloc(length+sizeof(id));
	if(NULL == buffer){
		return -2;
	}

	int err = 0;
	err = AdcsGenericI2cCmd(buffer,sizeof(buffer),ack);
	if(0 != err){
		free(buffer);
		return err;
	}
	free(buffer);
	return err;
}

int ResetBootRegisters()
{
	unsigned char reset_boot_reg_cmd_id = 6;
	int ack = 0;
	return AdcsGenericI2cCmd(&reset_boot_reg_cmd_id,sizeof(reset_boot_reg_cmd_id),&ack);
}

typedef enum {
	All_sgp4_params = 0,
	Inclination 	= 46,
	Eccentricity 	= 47,
	RAAN 			= 48,
	ArgOfPerigee 	= 49,
	BStardrag 		= 50,
	MeanMotion 		= 51,
	MeanAnomaly 	= 52,
	Epoch 			= 53
}Sgp4OrbitParam;
int SetSgp4OrbitParams(Sgp4OrbitParam params, unsigned char data[CSPACE_ADCS_SG4ORBITPARAMS_SIZE])
{
	if(NULL == data){
		return E_INPUT_POINTER_NULL;
	}
	int err = 0;
	if(All_sgp4_params == params){
		err = cspaceADCS_setSGP4OrbitParameters(ADCS_ID,(double*)data);
		return err;
	}
	AdcsI2cCmdWithID((unsigned char)params,data,sizeof(double),&err);
	return err;
}

int SetRateSensorOffset(unsigned char *data)
{
	if(NULL == data){
		return E_INPUT_POINTER_NULL;
	}
	//TODO: finsih- need to read the current config and change only the rate offset
	int err = 0;
	cspaceADCS_setRateSensorConfig rate_sensor;
	err = cspaceADCS_setRateSensorConfig(ADCS_ID,&data)
	//TODO: send ack with 'rv'
	return err;

}
TroubleErrCode AdcsExecuteCommand(TC_spl *cmd)
{
	if(NULL == cmd || NULL == cmd->data){
		//TODO: log err
		return TRBL_NULL_DATA;
	}
	//TODO: finish switch from the TLC dictionary
	int err = TRBL_SUCCESS;
	int rv = 0;
	unsigned char sub_type = cmd->subType;

	cspace_adcs_bootprogram bootindex;
	cspace_adcs_runmode_t runmode;
	cspace_adcs_unixtm_t time;
	cspace_adcs_estmode_sel att;
	switch(sub_type)
	{

	case ADCS_GENRIC_ST:
		err = AdcsGenericI2cCmd(cmd->data,cmd->length,&rv);
		//TODO: log 'rv'. maybe send it in ACK form
		break;
	case ADCS_SET_BOOT_INDEX_ST:
		memcpy(&bootindex,cmd->data,sizeof(bootindex));
		err = cspaceADCS_BLSetBootIndex(ADCS_ID,bootindex);
		break;

	case ADCS_RUN_BOOT_PROGRAM_ST:
		err = cspaceADCS_BLRunSelectedProgram(ADCS_ID);
		break;

	case ADCS_SET_SGP4_ORBIT_PARAMS_ST:
		err = SetSgp4OrbitParams(All_sgp4_params, cmd->data);
		break;

	case ADCS_SET_SGP4_ORBIT_INC_ST:
		err = SetSgp4OrbitParams(Inclination, cmd->data);
		break;

	case ADCS_SET_SGP4_ORBIT_ECC_ST:
		err = SetSgp4OrbitParams(Eccentricity, cmd->data);
		break;
	case ADCS_SET_SGP4_ORBIT_EPOCH_ST:
		 err = SetSgp4OrbitParams(Epoch, cmd->data);
		 return err;
	case ADCS_SET_SGP4_ORBIT_RAAN_ST:
		err = SetSgp4OrbitParams(RAAN, cmd->data);
		break;

	case ADCS_SET_SGP4_ORBIT_ARG_OF_PER_ST:
		err = SetSgp4OrbitParams(ArgOfPerigee, cmd->data);
		break;

	case ADCS_SET_SGP4_ORBIT_BSTAR_DRAG_ST:
		err = SetSgp4OrbitParams(BStardrag, cmd->data);
		break;

	case ADCS_SET_SGP4_ORBIT_MEAN_MOT_ST:
		err = SetSgp4OrbitParams(MeanMotion, cmd->data);
		break;

	case ADCS_SET_SGP4_ORBIT_MEAN_ANOM_ST:
		err = SetSgp4OrbitParams(MeanAnomaly, cmd->data);
		break;

	case ADCS_RESET_BOOT_REGISTER_ST:
		err = ResetBootRegisters();
		break;

	case ADCS_SAVE_CONFIG_ST:
		err = cspaceADCS_saveConfig(ADCS_ID);
		break;

	case ADCS_SAVE_ORBIT_PARAM_ST:
		err = cspaceADCS_saveOrbitParam(ADCS_ID);
		break;

	case ADCS_SET_UNIX_TIME_ST: //TODO: get unix time
		err = cspaceADCS_setCurrentTime(ADCS_ID,(cspace_adcs_unixtm_t*)cmd->data);
		break;

	case ADCS_CACHE_ENABLE_ST:
		err = cspaceADCS_cacheStateADCS(ADCS_ID, cmd->data[0]);
		break;

	case ADCS_DEPLOY_MAG_BOOM_ST:
		err = cspaceADCS_deployMagBoomADCS(ADCS_ID,cmd->data[0]);
		break;

	case ADCS_RUN_MODE_ST:
		memcpy(&runmode,cmd->data,sizeof(runmode));
		err = cspaceADCS_setRunMode(ADCS_ID, runmode);
		break;

	case ADCS_SET_MTQ_CONFIG_ST:
		err = AdcsI2cCmdWithID(MTQ_CONFIG_CMD_ID,cmd->data,MTQ_CONFIG_CMD_DATA_LENGTH,&rv);
		//TODO: send ack with 'rv'
		break;

	case ADCS_SET_MAGNETMTR_MOUNT_ST:
		err = cspaceADCS_setMagMountConfig(ADCS_ID,(cspace_adcs_magmountcfg_t*)cmd->data);
		break;

	case ADCS_SET_MAGNETMTR_OFFSET_ST:
		err = cspaceADCS_setMagOffsetScalingConfig(ADCS_ID,(cspace_adcs_offscal_cfg_t*)cmd->data);
		break;

	case ADCS_SET_MAGNETMTR_SENSTVTY_ST:
		err = cspaceADCS_setMagSensConfig(ADCS_ID,(cspace_adcs_magsencfg_t*)cmd->data);
		break;

	case ADCS_SET_DETUMB_CTRL_PARAM_ST:
		err = AdcsI2cCmdWithID(DETUMB_CTRL_PARAM_CMD_ID,cmd->data,DETUMB_CTRL_PARAM_DATA_LENGTH,&rv);
		//TODO: send ack with 'rv'
		break;

	case ADCS_SET_YWHEEL_CTRL_PARAM_ST:
		err = AdcsI2cCmdWithID(SET_YWHEEL_CTRL_PARAM_CMD_ID,cmd->data,SET_YWHEEL_CTRL_PARAM_DATA_LENGTH,&rv);
		//TODO: send ack with 'rv'
		break;

	case ADCS_SET_RWHEEL_CTRL_PARAM_ST:
		err = AdcsI2cCmdWithID(SET_RWHEEL_CTRL_PARAM_CMD_ID,cmd->data,SET_RWHEEL_CTRL_PARAM_DATA_LENGTH,&rv);
		//TODO: send ack with 'rv'
		break;
	case ADCS_SET_MOMENT_INTERTIA_ST:
		err = AdcsI2cCmdWithID(SET_MOMENT_INERTIA_CMD_ID,cmd->data,SET_MOMENT_INERTIA_DATA_LENGTH,&rv);
		//TODO: send ack with 'rv'
		break;
	case ADCS_PROD_INERTIA_ST:
		err = AdcsI2cCmdWithID(SET_PROD_INERTIA_CMD_ID,cmd->data,12,&rv);
		//TODO: send ack with 'rv'
		break;
	case ADCS_ESTIMATION_PARAM1_ST:
		err =  cspaceADCS_setEstimationParam1(ADCS_ID,(cspace_adcs_estparam1_t*)cmd->data);
		break;
	case ADCS_ESTIMATION_PARAM2_ST:
		err =  cspaceADCS_setEstimationParam2(ADCS_ID,(cspace_adcs_estparam2_t*)cmd->data);
		break;
	case ADCS_GET_CURR_UNIX_TIME_ST:
		err = cspaceADCS_getCurrentTime(ADCS_ID,&time);
		//TODO: send 'time' as ack or something
		break;

	case ADCS_MTQ_CONFIG_ST:
		//TODO: what's the difference between this(st = 135) and  sub_typeID = 117
		break;

	case ADCS_RW_CONFIG_ST:
		err = AdcsI2cCmdWithID(SET_WHEEL_CONFIG_CMD_ID,cmd->data,SET_WHEEL_CONFIG_DATA_LENGTH,&rv);
		//TODO: send ack with 'rv'
		break;

	case ADCS_GYRO_CONFIG_ST:
		err = AdcsI2cCmdWithID(SET_GYRO_CONFIG_CMD_ID,cmd->data,SET_GYRO_CONFIG_DATA_LENGTH,&rv);
		//TODO: send ack with 'rv'
		break;

	case ADCS_CSS_CONFIG_ST:
		err = AdcsI2cCmdWithID(SET_CSS_CONFIG_CMD_ID,cmd->data,SET_CSS_CONFIG_DATA_LENGTH,&rv);
		//TODO: send ack with 'rv'
		break;

	case ADCS_CSS_RELATIVE_SCALE_ST:
		err = AdcsI2cCmdWithID(SET_CSS_SCALE_CMD_ID,cmd->data,SET_CSS_SCALE_DATA_LENGTH,&rv);
		//TODO: send ack with 'rv'
		break;

	case ADCS_CSS_THRESHOLD_ST:
		// TODO: changes in the entire 272bytes config table(aka stage table)
		break;

	case ADCS_MAGNETMTR_MOUNT_TRNSFRM_ST:
		//TODO: check: already done in 'cspaceADCS_setMagMountConfig' in ADCS_SET_MAGNETMTR_MOUNT_ST
		break;

	case ADCS_MAGETMTR_CHNL_1_3_OFFST_ST:
		break;

	case ADCS_MAGNETMTR_SENS_MATRIX_ST:
		//TODO: check: already done in 'cspaceADCS_setMagSensConfig' in ADCS_SET_MAGNETMTR_SENSTVTY_ST
		break;

	case ADCS_RATE_SENSOR_OFFSET_ST:
		err = AdcsI2cCmdWithID(SET_RATE_SENSOR_CONFIG_CMD_ID,cmd->data,SET_RATE_SENSOR_CONFIG_DATA_LENGTH);
		//TODO: send ack with 'rv'
		break;

	case ADCS_RATE_SENSOR_MULT_ST:
		break;

	case ADCS_DETUMBLING_GAIN_CONFIG_ST: break;
	case ADCS_Y_MOMENT_GAIN_CONFIG_ST: break;
	case ADCS_REF_WHEEL_MOMENTUM_ST: break;
	case ADCS_RWHEEL_GAIN_ST: break;
	case ADCS_MOMENT_INERTIA_ST:break;
	case ADCS_NOISE_CONFIG_ST: break;

	case ADCS_USE_IN_EKF_ST:
		err = cspaceADCS_setAttEstMode(ADCS_ID,estmode_full_state_ekf);
		break;

	case ADCS_SET_EST_MODE:
		memcpy(&att,cmd->data,sizeof(att));
		err = cspaceADCS_setAttEstMode(ADCS_ID,att);
		break;

	default:
		//TODO: return unknown cmd
		break;
	}
	return err;
}

