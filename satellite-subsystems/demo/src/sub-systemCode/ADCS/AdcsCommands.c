#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <hal/Drivers/I2C.h>
#include <hal/errors.h>


#include <satellite-subsystems/cspaceADCS_types.h>
#include <satellite-subsystems/cspaceADCS.h>
#include "../COMM/GSC.h"
#include "../TRXVU.h"
#include "sub-systemCode/Global/logger.h"
#include "sub-systemCode/COMM/splTypes.h"	// for ADCS command subtypes
#include "sub-systemCode/ADCS/AdcsGetDataAndTlm.h"

#include "AdcsCommands.h"
#include "AdcsMain.h"

#define MAX_CONFIG_PARAM_LENGTH 64


#define ADCS_ACK_PROCESSED_FLAG_INDEX 1
#define ADCS_ACK_TC_ERR_INDEX 2

byte I2cData[ADCS_CMD_MAX_DATA_LENGTH]; //global for splited I2C TC (272 bytes long)
unsigned char Adcs_raw_12c_command[ADCS_CMD_MAX_DATA_LENGTH];

/*!
 * @brief allows the user to send a read request directly to the I2C.
 * @param[out] rv return value from the ADCS I2C ACK request
 * @return Errors according to "<hal/Drivers/I2C.h>"
 */
int AdcsReadI2cAck(AdcsTcErrorReason *rv)
{
	int err = 0;
	unsigned char data[4] = {0};
	unsigned char id = ADCS_I2C_TELEMETRY_ACK_REQUEST;

	unsigned char processed_flag = 0;
	unsigned char counter = 0;
	do{
		err = I2C_write(ADCS_I2C_ADRR, &id, 1);
		if(0 != err){
			return err;
		}
		err = I2C_read(ADCS_I2C_ADRR,  data, sizeof(data));
		if(0 != err){
			return err;
		}
		processed_flag = data[ADCS_ACK_PROCESSED_FLAG_INDEX];
		if(0 != processed_flag ){
			break;
		}
		vTaskDelay(5);
		if(10 <= counter ){	// try 10 times and then exit
			return E_COMMAND_NACKED;
		}
		counter++;
	}while(1);
	if(NULL != rv){
		*rv = (AdcsTcErrorReason)data[ADCS_ACK_TC_ERR_INDEX];
	}
	return 0;
}

int AdcsGenericI2cCmd(adcs_i2c_cmd *i2c_cmd)
{
	int err = 0;
	if(NULL == i2c_cmd){
		return E_INPUT_POINTER_NULL;
	}
	char is_tlm = (i2c_cmd->id & 0x80); // if MSB is 1 then it is TLM. if 0 then TC
	if(!is_tlm){ // is command
		memcpy(&Adcs_raw_12c_command[1],i2c_cmd->data,i2c_cmd->length);	// data bytes of the CMD(length can be 0)
		Adcs_raw_12c_command[0] = i2c_cmd->id;								// first byte is the CMD ID

		err = I2C_write(ADCS_I2C_ADRR, (unsigned char *)Adcs_raw_12c_command, i2c_cmd->length + 1); // +1 in case of 0 length CMD
		if(0 != err){
			WriteAdcsLog(LOG_ADCS_I2C_WRITE_ERR,err);
			return err;
		}
		err = AdcsReadI2cAck(&i2c_cmd->ack);

	}else{ // is tlm
		err = I2C_write(ADCS_I2C_ADRR, (unsigned char *)&i2c_cmd->id, sizeof(i2c_cmd->id));
		if(0 != err){
			WriteAdcsLog(LOG_ADCS_I2C_WRITE_ERR,err);
			return err;
		}
		err = I2C_read(ADCS_I2C_ADRR, (unsigned char *)i2c_cmd->data, i2c_cmd->length);
		if(0 != err){
			WriteAdcsLog(LOG_ADCS_I2C_READ_ERR,err);
			return err;
		}
		err = AdcsReadI2cAck(&i2c_cmd->ack);
	}
	return err;
}

void SendAdcsTlm(byte *info, unsigned int length, int subType){
	TM_spl tm;
	time_unix time_now;
	Time_getUnixEpoch(&time_now);	//get time
	tm.time = time_now;
	tm.type = TM_ADCS_T;
	tm.subType = subType;

	tm.length = length;
	memcpy(tm.data,info,tm.length);

	byte rawData[SPL_TM_HEADER_SIZE + tm.length];
	unsigned int rawDataLength = 0;
	encode_TMpacket(rawData, (int*)&rawDataLength, tm);
	TRX_sendFrame(rawData, (unsigned char)rawDataLength);
#ifdef TESTING
	printf("\n[%d",rawData[0]);
	for(unsigned int i = 1; i < rawDataLength;i++){
		printf(",%X",rawData[i]);
	}
	printf("]\n");
#endif
}

TroubleErrCode AdcsExecuteCommand(TC_spl *cmd)
{
	if(NULL == cmd || NULL == cmd->data){
		WriteAdcsLog(LOG_ADCS_NULL_DATA,-1);
		return TRBL_NULL_DATA;
	}
	int err = TRBL_SUCCESS;
	int temp = 0;
	byte sub_type = cmd->subType;

	byte data[ADCS_CMD_MAX_DATA_LENGTH] = {0};
	memset(data,0xFF,ADCS_CMD_MAX_DATA_LENGTH);

	cspace_adcs_bootprogram bootindex;
	cspace_adcs_runmode_t runmode;
	cspace_adcs_estmode_sel att_est;

	cspace_adcs_clearflags_t clear;
	cspace_adcs_magmode_t magmode;
	cspace_adcs_savimag_t savimag_param;

	adcs_i2c_cmd i2c_cmd = {0};
	switch(sub_type)
	{
		//generic I2C command
		case ADCS_I2C_GENRIC_ST:
				memcpy(&i2c_cmd,cmd->data,cmd->length);
				if ((i2c_cmd.length > ADCS_CMD_MAX_DATA_LENGTH / 2)&&(i2c_cmd.id < 128)){

					if ((i2c_cmd.data[0] & 0x0F) == 0){	// 4 LSB indicate which half of the CMD did we send.
						memcpy(I2cData, &i2c_cmd.data[1], ADCS_CMD_MAX_DATA_LENGTH/2);
					}
					else{
						memcpy(&I2cData[ADCS_CMD_MAX_DATA_LENGTH/2], &i2c_cmd.data[1], i2c_cmd.length - ADCS_CMD_MAX_DATA_LENGTH/2);
					}
					if((i2c_cmd.data[0] & 0xF0) != 0xF0){	// 4MSB indicate if CMD is assembled
						break;
					}
					memcpy(i2c_cmd.data,I2cData,i2c_cmd.length);
				}
				err = AdcsGenericI2cCmd(&i2c_cmd);

				if (i2c_cmd.id >= 128){// is TLM
					if (i2c_cmd.length > ADCS_CMD_MAX_DATA_LENGTH/2){
						SendAdcsTlm(i2c_cmd.data, ADCS_CMD_MAX_DATA_LENGTH/2, ADCS_I2C_GENRIC_ST);//send first half
						SendAdcsTlm(&(i2c_cmd.data[ADCS_CMD_MAX_DATA_LENGTH/2]), ADCS_CMD_MAX_DATA_LENGTH/2, ADCS_I2C_GENRIC_ST);//send second half

					}else{
						SendAdcsTlm((byte*)i2c_cmd.data,ADCS_CMD_MAX_DATA_LENGTH/2, ADCS_I2C_GENRIC_ST);
					}
				}
				WriteAdcsLog(LOG_ADCS_GENERIC_I2C_ACK,i2c_cmd.ack);
				break;
		
		//ADCS telecommand
		case ADCS_COMPONENT_RESET_ST:
			err = cspaceADCS_componentReset(ADCS_ID);
			break;
		case ADCS_SET_CURR_UNIX_TIME_ST:
			err = cspaceADCS_setCurrentTime(ADCS_ID,(cspace_adcs_unixtm_t*)cmd->data);
			break;
		case ADCS_CACHE_ENABLE_ST:
			err = cspaceADCS_cacheStateADCS(ADCS_ID, cmd->data[0]);
			break;
		case ADCS_RESET_BOOT_REGISTER_ST:
			i2c_cmd.id = RESET_BOOT_REGISTER_CMD_ID;
			i2c_cmd.length = RESET_BOOT_REGISTER_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);//length is 0
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_DEPLOY_MAG_BOOM_ST:
			err = cspaceADCS_deployMagBoomADCS(ADCS_ID,cmd->data[0]);
			break;
		case ADCS_RUN_MODE_ST:
			memcpy(&runmode,cmd->data,sizeof(runmode));
			err = cspaceADCS_setRunMode(ADCS_ID, runmode);
			break;
		case ADCS_SET_PWR_CTRL_DEVICE_ST:
			err = cspaceADCS_setPwrCtrlDevice(ADCS_ID, (cspace_adcs_powerdev_t*)cmd->data);
			break;
		case ADCS_CLEAR_ERRORS_ST:
			memcpy(&clear,cmd->data,sizeof(clear));
			err = cspaceADCS_clearErrors(ADCS_ID, clear);
			break;
		case ADCS_SET_ATT_CTRL_MODE_ST:
			err = cspaceADCS_setAttCtrlMode(ADCS_ID, (cspace_adcs_attctrl_mod_t*)cmd->data);
			break;

		case ADCS_SET_EST_MODE_ST:
			memcpy(&att_est,cmd->data,sizeof(att_est));//copy for ENUM?
			err = cspaceADCS_setAttEstMode(ADCS_ID,att_est);
			break;

		case ADCS_SET_MAG_OUTPUT_ST:
			err = cspaceADCS_setMagOutput(ADCS_ID, (cspace_adcs_magnetorq_t*)cmd->data);
			break;
		case ADCS_SET_WHEEL_SPEED_ST:
			err = cspaceADCS_setWheelSpeed(ADCS_ID, (cspace_adcs_wspeed_t*)cmd->data);
			break;
		case ADCS_SET_MTQ_CONFIG_ST:
			i2c_cmd.id = MTQ_CONFIG_CMD_ID;
			i2c_cmd.length = MTQ_CONFIG_CMD_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_RW_CONFIG_ST:
			i2c_cmd.id = SET_WHEEL_CONFIG_CMD_ID;
			i2c_cmd.length = SET_WHEEL_CONFIG_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_GYRO_CONFIG_ST:
			i2c_cmd.id = SET_GYRO_CONFIG_CMD_ID;
			i2c_cmd.length = SET_GYRO_CONFIG_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_CSS_CONFIG_ST:
			i2c_cmd.id = SET_CSS_CONFIG_CMD_ID;
			i2c_cmd.length = SET_CSS_CONFIG_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_CSS_RELATIVE_SCALE_ST:
			i2c_cmd.id = SET_CSS_SCALE_CMD_ID;
			i2c_cmd.length = SET_CSS_SCALE_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
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
		case ADCS_RATE_SENSOR_OFFSET_ST:
			err = cspaceADCS_setRateSensorConfig(ADCS_ID,(cspace_adcs_ratesencfg_t*)cmd->data);
			break;
		case ADCS_SET_STAR_TRACKER_CONFIG_ST:
			err = cspaceADCS_setStarTrackerConfig(ADCS_ID,(cspace_adcs_startrkcfg_t*)cmd->data);
			break;
		case ADCS_SET_DETUMB_CTRL_PARAM_ST:
			i2c_cmd.id = DETUMB_CTRL_PARAM_CMD_ID;
			i2c_cmd.length = DETUMB_CTRL_PARAM_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_YWHEEL_CTRL_PARAM_ST:
			i2c_cmd.id = SET_YWHEEL_CTRL_PARAM_CMD_ID;
			i2c_cmd.length = SET_YWHEEL_CTRL_PARAM_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_RWHEEL_CTRL_PARAM_ST:
			i2c_cmd.id = SET_RWHEEL_CTRL_PARAM_CMD_ID;
			i2c_cmd.length = SET_RWHEEL_CTRL_PARAM_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_MOMENT_INTERTIA_ST:
			i2c_cmd.id = SET_MOMENT_INERTIA_CMD_ID;
			i2c_cmd.length = SET_MOMENT_INERTIA_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_PROD_INERTIA_ST:
			i2c_cmd.id = SET_PROD_INERTIA_CMD_ID;
			i2c_cmd.length = SET_PROD_INERTIA_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_ESTIMATION_PARAM1_ST:
			err =  cspaceADCS_setEstimationParam1(ADCS_ID,(cspace_adcs_estparam1_t*)cmd->data);
			break;
		case ADCS_ESTIMATION_PARAM2_ST:
			err =  cspaceADCS_setEstimationParam2(ADCS_ID,(cspace_adcs_estparam2_t*)cmd->data);
			break;
		case ADCS_SET_SGP4_ORBIT_PARAMS_ST:
			err = cspaceADCS_setSGP4OrbitParameters(ADCS_ID, (double*)cmd->data);
			break;
		case ADCS_SET_SGP4_ORBIT_INC_ST:
			i2c_cmd.id = SET_SGP4_ORBIT_INC_CMD_ID;
			i2c_cmd.length = SET_SGP4_ORBIT_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_SGP4_ORBIT_ECC_ST:
			i2c_cmd.id = SET_SGP4_ORBIT_ECC_CMD_ID;
			i2c_cmd.length = SET_SGP4_ORBIT_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_SGP4_ORBIT_RAAN_ST:
			i2c_cmd.id = SET_SGP4_ORBIT_RAAN_CMD_ID;
			i2c_cmd.length = SET_SGP4_ORBIT_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_SGP4_ORBIT_ARG_OF_PER_ST:
			i2c_cmd.id = SET_SGP4_ORBIT_ARG_OF_PER_CMD_ID;
			i2c_cmd.length = SET_SGP4_ORBIT_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_SGP4_ORBIT_BSTAR_DRAG_ST:
			i2c_cmd.id = SET_SGP4_ORBIT_BSTAR_DRAG_CMD_ID;
			i2c_cmd.length = SET_SGP4_ORBIT_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_SGP4_ORBIT_MEAN_MOT_ST:
			i2c_cmd.id = SET_SGP4_ORBIT_MEAN_MOT_CMD_ID;
			i2c_cmd.length = SET_SGP4_ORBIT_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_SGP4_ORBIT_MEAN_ANOM_ST:
			i2c_cmd.id = SET_SGP4_ORBIT_MEAN_ANOM_CMD_ID;
			i2c_cmd.length = SET_SGP4_ORBIT_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_SGP4_ORBIT_EPOCH_ST:
			i2c_cmd.id = SET_SGP4_ORBIT_EPOCH_CMD_ID;
			i2c_cmd.length = SET_SGP4_ORBIT_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			break;
		case ADCS_SET_MAGNETOMETER_MODE_ST:
			memcpy(&magmode,cmd->data,sizeof(magmode));
			err = cspaceADCS_setMagnetometerMode(ADCS_ID, magmode);
			break;
		case ADCS_SAVE_CONFIG_ST:
			err = cspaceADCS_saveConfig(ADCS_ID);
			break;
		case ADCS_SAVE_ORBIT_PARAM_ST:
			err = cspaceADCS_saveOrbitParam(ADCS_ID);
			break;
		case ADCS_SAVE_IMAGE_ST:
			memcpy(&savimag_param,cmd->data,sizeof(savimag_param));
			err = cspaceADCS_saveImage(ADCS_ID, savimag_param);
			break;
		case ADCS_UPDATE_TLM_ELEMENT_AT_INDEX_ST:
			err = UpdateTlmElementAtIndex(cmd->data[0],cmd->data[1],cmd->data[2]);
			break;
		case ADCS_RESET_TLM_ELEMENTS_ST:
			err = RestoreDefaultTlmElement();
			break;
		case ADCS_SET_TLM_OVERRIDE_FLAG_ST:
			memcpy(&temp,cmd->data,sizeof(Boolean));
			err = AdcsSetTlmOverrideFlag((Boolean)temp);
			break;
		case ADCS_SET_ADCS_LOOP_PARAMETERS_ST:
			err = UpdateAdcsFramParameters(cmd->data[0],cmd->data+1);
			break;
		case ADCS_UPDATE_TLM_PERIOD_VEC_ST:
			err = UpdateTlmPeriodVector(cmd->data);
			break;
		case ADCS_UPDATE_TLM_SAVE_VEC_ST:
			err = UpdateTlmToSaveVector((Boolean8bit*)cmd->data);
			break;
		case ADCS_SET_BOOT_INDEX_ST:
			memcpy(&bootindex,cmd->data,sizeof(bootindex));
			err = cspaceADCS_BLSetBootIndex(ADCS_ID,bootindex);
			break;
		case ADCS_RUN_BOOT_PROGRAM_ST:
			err = cspaceADCS_BLRunSelectedProgram(ADCS_ID);
			break;


			
/*******************************************************************************/
/****************************     Telemetry     ********************************/
/*******************************************************************************/


		case ADCS_GET_GENERAL_INFO_ST:
			err = cspaceADCS_getGeneralInfo(ADCS_ID,(cspace_adcs_geninfo_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_geninfo_t),ADCS_GET_GENERAL_INFO_ST);
			break;
		case ADCS_GET_BOOT_PROGRAM_INFO_ST:
			err = cspaceADCS_getBootProgramInfo(ADCS_ID,(cspace_adcs_bootinfo_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_bootinfo_t),ADCS_GET_BOOT_PROGRAM_INFO_ST);
			break;
		case ADCS_GET_CURR_UNIX_TIME_ST:
			err = cspaceADCS_getCurrentTime(ADCS_ID,(cspace_adcs_unixtm_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_unixtm_t),ADCS_GET_CURR_UNIX_TIME_ST);
			break;
		case ADCS_GET_SRAM_LATCHUP_COUNTERS_ST:
			err = cspaceADCS_getSRAMLatchupCounters(ADCS_ID,(cspace_adcs_sramlatchupcnt_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_sramlatchupcnt_t),ADCS_GET_SRAM_LATCHUP_COUNTERS_ST);
			break;
		case ADCS_GET_EDAC_COUNTERS_ST:
			err = cspaceADCS_getEDACCounters(ADCS_ID,(cspace_adcs_edaccnt_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_edaccnt_t),ADCS_GET_EDAC_COUNTERS_ST);
			break;
		case ADCS_GET_COMM_STATUS_ST:
			err = cspaceADCS_getCommStatus(ADCS_ID,(cspace_adcs_commstat_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_commstat_t),ADCS_GET_COMM_STATUS_ST);
			break;
		case ADCS_GET_CURRENT_STATE_ST:
			err = cspaceADCS_getCurrentState(ADCS_ID,(cspace_adcs_currstate_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_currstate_t),ADCS_GET_CURRENT_STATE_ST);
			break;
		case ADCS_GET_EST_ANG_ST:
			i2c_cmd.id = GET_ADCS_EST_ANG_CMD_ID;
			i2c_cmd.length = GET_ADCS_EST_ANG_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			SendAdcsTlm(i2c_cmd.data,GET_ADCS_EST_ANG_DATA_LENGTH,ADCS_GET_EST_ANG_ST);
			break;
		case ADCS_GET_EST_ANG_RATE_ST: 
			i2c_cmd.id = GET_ADCS_EST_ANG_CMD_ID;
			i2c_cmd.length = GET_ADCS_EST_ANG_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			SendAdcsTlm(i2c_cmd.data,i2c_cmd.length,ADCS_GET_EST_ANG_RATE_ST);
			break;
		case ADCS_GET_SATELLITE_POSITION_ST:
			i2c_cmd.id = ADCS_GET_SATELLITE_POSITION_CMD_ID;
			i2c_cmd.length = ADCS_GET_SATELLITE_POSITION_DATA_LENGTH;
			memcpy(i2c_cmd.data,cmd->data,i2c_cmd.length);
			err = AdcsGenericI2cCmd(&i2c_cmd);
			SendAdcsTlm(i2c_cmd.data,i2c_cmd.length,ADCS_GET_SATELLITE_POSITION_ST);
			break;
		case ADCS_GET_MAGNETIC_FIELD_VEC_ST:
			err = cspaceADCS_getMagneticFieldVec(ADCS_ID,(cspace_adcs_magfieldvec_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_magfieldvec_t),ADCS_GET_MAGNETIC_FIELD_VEC_ST);
			break;
		case ADCS_GET_COARSE_SUN_VEC_ST:
			err = cspaceADCS_getCoarseSunVec(ADCS_ID,(cspace_adcs_sunvec_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_sunvec_t),ADCS_GET_COARSE_SUN_VEC_ST);
			break;
		case ADCS_GET_FINE_SUN_VEC_ST:
			err = cspaceADCS_getFineSunVec(ADCS_ID,(cspace_adcs_sunvec_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_sunvec_t),ADCS_GET_FINE_SUN_VEC_ST);
			break;
		case ADCS_GET_NADIR_VECTOR_ST:
			err = cspaceADCS_getNadirVector(ADCS_ID,(cspace_adcs_nadirvec_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_nadirvec_t),ADCS_GET_NADIR_VECTOR_ST);
			break;
		case ADCS_GET_SENSOR_RATES_ST:
			err = cspaceADCS_getSensorRates(ADCS_ID,(cspace_adcs_angrate_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_angrate_t),ADCS_GET_SENSOR_RATES_ST);
			break;
		case ADCS_GET_WHEEL_SPEED_ST:
			err = cspaceADCS_getWheelSpeed(ADCS_ID,(cspace_adcs_wspeed_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_wspeed_t),ADCS_GET_WHEEL_SPEED_ST);
			break;
		case ADCS_GET_MAGNETORQUER_CMD_ST:
			err = cspaceADCS_getMagnetorquerCmd(ADCS_ID,(cspace_adcs_magtorqcmd_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_magtorqcmd_t),ADCS_GET_MAGNETORQUER_CMD_ST);
			break;
		case ADCS_GET_WHEEL_SPEED_CMD_ST:
			err = cspaceADCS_getWheelSpeedCmd(ADCS_ID,(cspace_adcs_wspeed_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_wspeed_t),ADCS_GET_WHEEL_SPEED_CMD_ST);
			break;
		case ADCS_GET_RAW_CSS_1_6_MEASURMENTS_ST:
			err = cspaceADCS_getRawCss1_6Measurements(ADCS_ID,(cspace_adcs_rawcss1_6_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawcss1_6_t),ADCS_GET_RAW_CSS_1_6_MEASURMENTS_ST);
			break;
		case ADCS_GET_RAW_CSS_7_10_MEASURMENTS_ST:
			err = cspaceADCS_getRawCss7_10Measurements(ADCS_ID,(cspace_adcs_rawcss7_10_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawcss7_10_t),ADCS_GET_RAW_CSS_7_10_MEASURMENTS_ST);
			break;
		case ADCS_GET_RAW_MAGNETOMETER_MEAS_ST:
			err = cspaceADCS_getRawMagnetometerMeas(ADCS_ID,(cspace_adcs_rawmagmeter_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawmagmeter_t),ADCS_GET_RAW_MAGNETOMETER_MEAS_ST);
			break;
		case ADCS_GET_C_SENSE_CURRENT_MEASUREMENTS_ST:
			err = cspaceADCS_getCSenseCurrentMeasurements(ADCS_ID,(cspace_adcs_csencurrms_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_csencurrms_t),ADCS_GET_C_SENSE_CURRENT_MEASUREMENTS_ST);
			break;
		case ADCS_GET_C_CONTROL_CURRENT_MEASUREMENTS_ST:
			err = cspaceADCS_getCControlCurrentMeasurements(ADCS_ID,(cspace_adcs_cctrlcurrms_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_cctrlcurrms_t),ADCS_GET_C_CONTROL_CURRENT_MEASUREMENTS_ST);
			break;
		case ADCS_GET_WHEEL_CURRENTS_TLM_ST:
			err = cspaceADCS_getWheelCurrentsTlm(ADCS_ID,(cspace_adcs_wheelcurr_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_wheelcurr_t),ADCS_GET_WHEEL_CURRENTS_TLM_ST);
			break;
		case ADCS_GET_ADCS_TEMPERATURE_TLM_ST:
			err = cspaceADCS_getADCSTemperatureTlm(ADCS_ID,(cspace_adcs_msctemp_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_msctemp_t),ADCS_GET_ADCS_TEMPERATURE_TLM_ST);
			break;
		case ADCS_GET_RATE_SENSOR_TEMP_TLM_ST:
			err = cspaceADCS_getRateSensorTempTlm(ADCS_ID,(cspace_adcs_ratesen_temp_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_ratesen_temp_t),ADCS_GET_RATE_SENSOR_TEMP_TLM_ST);
			break;
		case ADCS_GET_RAW_GPS_STATUS_ST:
			err = cspaceADCS_getRawGpsStatus(ADCS_ID,(cspace_adcs_rawgpssta_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawgpssta_t),ADCS_GET_RAW_GPS_STATUS_ST);
			break;
		case ADCS_GET_RAW_GPS_TIME_ST:
			err = cspaceADCS_getRawGpsTime(ADCS_ID,(cspace_adcs_rawgpstm_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawgpstm_t),ADCS_GET_RAW_GPS_TIME_ST);
			break;
		case ADCS_GET_RAW_GPS_X_ST:
			err = cspaceADCS_getRawGpsX(ADCS_ID,(cspace_adcs_rawgps_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawgps_t),ADCS_GET_RAW_GPS_X_ST);
			break;
		case ADCS_GET_RAW_GPS_Y_ST:
			err = cspaceADCS_getRawGpsY(ADCS_ID,(cspace_adcs_rawgps_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawgps_t),ADCS_GET_RAW_GPS_Y_ST);
			break;
		case ADCS_GET_RAW_GPS_Z_ST:
			err = cspaceADCS_getRawGpsZ(ADCS_ID,(cspace_adcs_rawgps_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawgps_t),ADCS_GET_RAW_GPS_Z_ST);
			break;
		case ADCS_GET_STATE_TLM_ST:
			err = cspaceADCS_getStateTlm(ADCS_ID,(cspace_adcs_statetlm_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_statetlm_t),ADCS_GET_STATE_TLM_ST);
			break;
		case ADCS_GET_ADCS_MEASUREMENTS_ST:
			err = cspaceADCS_getADCSMeasurements(ADCS_ID,(cspace_adcs_measure_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_measure_t),ADCS_GET_ADCS_MEASUREMENTS_ST);
			break;
		case ADCS_GET_ACTUATORS_CMDS_ST:
			err = cspaceADCS_getActuatorsCmds(ADCS_ID,(cspace_adcs_actcmds_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_actcmds_t),ADCS_GET_ACTUATORS_CMDS_ST);
			break;
		case ADCS_GET_ESTIMATION_METADATA_ST:
			err = cspaceADCS_getEstimationMetadata(ADCS_ID,(cspace_adcs_estmetadata_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_estmetadata_t),ADCS_GET_ESTIMATION_METADATA_ST);
			break;
		case ADCS_GET_SENSOR_MEASUREMENTS_ST:
			err = cspaceADCS_getRawSensorMeasurements(ADCS_ID,(cspace_adcs_rawsenms_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawsenms_t),ADCS_GET_SENSOR_MEASUREMENTS_ST);
			break;
		case ADCS_GET_POW_TEMP_MEAS_TLM_ST:
			err = cspaceADCS_getPowTempMeasTLM(ADCS_ID,(cspace_adcs_pwtempms_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_pwtempms_t),ADCS_GET_POW_TEMP_MEAS_TLM_ST);
			break;
		case ADCS_GET_ADCS_EXC_TIMES_ST:
			err = cspaceADCS_getADCSExcTimes(ADCS_ID,(cspace_adcs_exctm_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_exctm_t),ADCS_GET_ADCS_EXC_TIMES_ST);
			break;
		case ADCS_GET_PWR_CTRL_DEVICE_ST:
			err = cspaceADCS_getPwrCtrlDevice(ADCS_ID,(cspace_adcs_powerdev_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_powerdev_t),ADCS_GET_PWR_CTRL_DEVICE_ST);
			break;
		case ADCS_GET_MISC_CURRENT_MEAS_ST:
			err = cspaceADCS_getMiscCurrentMeas(ADCS_ID,(cspace_adcs_misccurr_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_misccurr_t),ADCS_GET_MISC_CURRENT_MEAS_ST);
			break;
		case ADCS_GET_COMMANDED_ATTITUDE_ANGLES_ST:
			err = cspaceADCS_getCommandedAttitudeAngles(ADCS_ID,(cspace_adcs_cmdangles_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_cmdangles_t),ADCS_GET_COMMANDED_ATTITUDE_ANGLES_ST);
			break;
		case ADCS_GET_FULL_CONFIG_ST:
			err = cspaceADCS_getADCSConfiguration(ADCS_ID,(unsigned char*)data);
			SendAdcsTlm(data, ADCS_CMD_MAX_DATA_LENGTH/2, ADCS_GET_FULL_CONFIG_ST);//send first half
			SendAdcsTlm(&(data[ADCS_CMD_MAX_DATA_LENGTH/2]), ADCS_CMD_MAX_DATA_LENGTH/2, ADCS_GET_FULL_CONFIG_ST);//send second half
			break;
		case ADCS_GET_SGP4_ORBIT_PARAMETERS_ST:
			err = cspaceADCS_getSGP4OrbitParameters(ADCS_ID,(double*)data);
			SendAdcsTlm(data, ADCS_SGP4_ORBIT_PARAMS_DATA_LENGTH,ADCS_GET_SGP4_ORBIT_PARAMETERS_ST);
			break;
		case ADCS_GET_ADCS_RAW_GPS_MEAS_ST:
			err = cspaceADCS_getADCSRawGPSMeas(ADCS_ID,(cspace_adcs_rawgpsms_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawgpsms_t),ADCS_GET_ADCS_RAW_GPS_MEAS_ST);
			break;
		case ADCS_GET_STAR_TRACKER_TLM_ST:
			err = cspaceADCS_getStartrackerTLM(ADCS_ID,(cspace_adcs_startrackertlm_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_startrackertlm_t),ADCS_GET_STAR_TRACKER_TLM_ST);
			break;
		case ADCS_GET_ADC_RAW_RED_MAG_ST:
			err = cspaceADCS_getADCRawRedMag(ADCS_ID,(cspace_adcs_rawmagmeter_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_rawmagmeter_t),ADCS_GET_ADC_RAW_RED_MAG_ST);
			break;
		case ADCS_GET_ACP_EXECUTION_STATE_ST:
			err = cspaceADCS_getACPExecutionState(ADCS_ID,(cspace_adcs_acp_info_t*)data);
			SendAdcsTlm(data, sizeof(cspace_adcs_acp_info_t),ADCS_GET_ACP_EXECUTION_STATE_ST);
			break;

		case ADCS_GET_TLM_SAVE_VECTOR_ST:
			err = GetTlmToSaveVector((Boolean8bit*)data);
			SendAdcsTlm(data,NUM_OF_ADCS_TLM * sizeof(Boolean8bit),ADCS_GET_TLM_SAVE_VECTOR_ST);
			break;

		case ADCS_GET_PERIOD_VECTOR_ST:
			err = GetTlmPeriodVector(data);
			SendAdcsTlm(data,NUM_OF_ADCS_TLM * sizeof(unsigned char),ADCS_GET_PERIOD_VECTOR_ST);
			break;

		case ADCS_GET_LAST_SAVE_TIMES_ST:
			err = GetLastSaveTimes((time_unix*)data);
			SendAdcsTlm(data,NUM_OF_ADCS_TLM * sizeof(time_unix),ADCS_GET_LAST_SAVE_TIMES_ST);
			break;
		case ADCS_GET_OPERATING_FLAGS_ST:
			err = GetOperationgFlags(data);
			SendAdcsTlm(data,NUM_OF_ADCS_TLM * sizeof(Boolean8bit),ADCS_GET_OPERATING_FLAGS_ST);
			break;

		case ADCS_GET_TLM_ELEM_AT_INDEX_ST:
			GetTlmElementAtIndex((AdcsTlmElement_t*)data,cmd->data[0]);
			SendAdcsTlm(data, sizeof(AdcsTlmElement_t),ADCS_GET_TLM_ELEM_AT_INDEX_ST);
			break;

		case ADCS_GET_ADCS_CONFIG_PARAM_ST:
			i2c_cmd.id = GET_ADCS_FULL_CONFIG_CMD_ID;
			i2c_cmd.length = ADCS_FULL_CONFIG_DATA_LENGTH;
			err = AdcsGenericI2cCmd(&i2c_cmd);
			memcpy(data,&(i2c_cmd.data[(cmd->data[0]<<8) + cmd->data[1]]),cmd->data[2]);
			SendAdcsTlm(data,MAX_CONFIG_PARAM_LENGTH,ADCS_GET_ADCS_CONFIG_PARAM_ST);
			break;
		case ADCS_GET_TLM_OVERRIDE_FLAG_ST:
			err = AdcsGetTlmOverrideFlag((Boolean*)&temp);
			SendAdcsTlm((byte*)&temp,sizeof(temp),ADCS_GET_TLM_OVERRIDE_FLAG_ST);
			break;
		case ADCS_SET_DATA_LOG_ST:
			break;
		default:
			break;
	}
	unsigned int error_codes[] = {sub_type,err,i2c_cmd.ack};
	SendAdcsTlm((byte*)error_codes,sizeof(error_codes),ADCS_ACK_DATA_ST);
	return err;
}
