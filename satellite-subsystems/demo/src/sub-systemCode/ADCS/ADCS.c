/*
 ============================================================================
 Name        : ADCS.c
 Author      : Michael
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
//! define True as 1 true in c
//#define TRUE 1
//! define False as 0 false in c

#include <stdio.h>
#include <stdlib.h>

#include <hcc/api_fat.h>

#include "../ADCS.h"
#include "../Global/GlobalParam.h"

// includes for camera tests:
#include <satellite-subsystems/SCS_gecko/gecko_use_cases.h>

// include for TRXVU test
#include <satellite-subsystems/IsisTRXVU.h>
//#include "../EPS.h"
#include <satellite-subsystems/GomEPS.h>
//include for TESTS
#include <at91/utility/exithandler.h>

//! define START_ESTIMATION_MODE as 0


//#include <pthread.h>



int setEstimation()
{
	printf("enter estimation mode:\n");
	printf("0) estmode_noatt_est = 0 < No attitude estimation\n");
	printf("1) estmode_mems_rate = 1 < MEMS rate sensing\n");
	printf("2) estmode_mag_ratefilt = 2 < Magnetometer rate filter\n");
	printf("3) estmode_mag_ratefilt_pitch_est = 3 < Magnetometer rate filter with pitch estimation\n");
	printf("4) estmode_mag_fine_suntriad = 4 < Magnetometer and Fine-sun TRIAD algorithm\n");
	printf("5) estmode_full_state_ekf = 5 < Full-state EKF\n");
	printf("6) estmode_mems_gyro_ekf = 6 < MEMS gyro EKF\n");
	unsigned int selection = 0;
	while(UTIL_DbguGetIntegerMinMax(&selection, 0,6) == 0);
	cspace_adcs_estmode_sel param = (cspace_adcs_estmode_sel)selection;
	int err = cspaceADCS_setAttEstMode(0, param);
	if (err != 0)
	{
		printf("cspaceADCS_setAttEstMode error = %d\n", err);
	}

	cspace_adcs_currstate_t param_1;
	err = cspaceADCS_getCurrentState(0, &param_1);
	if (err != 0)
	{
		printf("cspaceADCS_getCurrentState error = %d\n", err);
	}
	else
	{
		printf("attest_mode = %u\n", param_1.fields.attest_mode);
		printf("sunsensor_rangerr = %d\n", param_1.fields.sunsensor_rangerr);
		printf("coarsunsensor_err = %d\n", param_1.fields.coarsunsensor_err);
		printf("config_invalid = %d\n", param_1.fields.config_invalid);
	}

	return 0;
}

int setRunMode(cspace_adcs_runmode_t runmode)
{
	return cspaceADCS_setRunMode(ADCS_ID, runmode);
}

int BOOMStickDeploy(unsigned char timeout)
{
	return cspaceADCS_deployMagBoomADCS(ADCS_ID,timeout);
}
//!  a function that opens all of the ADCS files
void ADCS_CreateFiles()
{
	// opens file ... i don't know ... LOOK AT THE NAME OF THE GIVEN PARAMTER TO THE FUNCTION
	/*c_fileCreate(ESTIMATED_ANGLES_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(ESTIMATED_ANGULAR_RATE_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_ECI_POSITION_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_SATALLITE_VELOCITY_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_LLH_POSTION_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_MAGNETIC_FIELD_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(CSS_SUN_VECTOR_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(SENSOR_RATE_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(WHEEL_SPEED_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(MAGNETORQUER_CMD_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(WHEEL_SPEED_CMD_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(IGRF_MODEL_MAGNETIC_FIELD_VECTOR_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(GYRO_BIAS_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(INNOVATION_VECTOR_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(ERROR_VECTOR_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(QUATERNION_COVARIANCE_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(ANGULAR_RATE_COVARIANCE_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_CSS_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(RAW_MAGNETOMETER_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(ESTIMATED_QUATERNION_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);
	c_fileCreate(NAME_OF_ECEF_POSITION_DATA_FILE,SIZE_OF_TELEMTRY,DEFAULT_NUM_OF_FILES);*/
}
//! a function that is called once and it starts the adcs loop and an initialization of the cubeadcs
void ADCS_startLoop(Boolean activation)
{
	unsigned char I2C = ADCS_I2C_ADRR;
	// connects to the cubeADCS thorough I2C
	int err = cspaceADCS_initialize(&I2C,1);
	if(err != 0)
	{
		printf("%d",err);
	}
	vTaskDelay(2000);
	// well all files need to be created, don't they? this function creates them
	// Initializes the stage table. you now empty and alone like me :|

	//initialize the stage table
	stageTable ST = get_ST();
	byte raw_stageTable[STAGE_TABLE_SIZE];
	if (activation)
	{
		createTable(ST);
		getTableTo(get_ST(), raw_stageTable);
		FRAM_write(raw_stageTable, STAGE_TABLE_ADDR, STAGE_TABLE_SIZE);
	}
	else
	{
		FRAM_read(raw_stageTable, STAGE_TABLE_ADDR, STAGE_TABLE_SIZE);
		translateCommandFULL(raw_stageTable, ST);
	}
	// saves the stage table to the fram
	cspace_adcs_runmode_t Run = runmode_enabled;
	err = cspaceADCS_setRunMode(ADCS_ID,Run);
	if(err != 0)
	{
		printf("/n %d run",err);
	}
}
/*! a function that gathers the telemtry data
 *@param[StTa] holds what telmetry shuld be saved
 */
void gatherData(stageTable StTa)
{
	// a parameter that holds part of the adcs data
	cspace_adcs_estmetadata_t ADCSDataGenral;
	// get the data from the cubeADCS
	cspaceADCS_getEstimationMetadata(ADCS_ID, &ADCSDataGenral);
	// holds adcs data if you want more info go to the struct of the parameter
	cspace_adcs_statetlm_t ADCSStateData;
	// gets more data from the cube adcs
	cspaceADCS_getStateTlm(ADCS_ID, &ADCSStateData);
	// checks if it neccacery to save this param
	if(checkEstimatedAttitudeAngles(StTa))
	{
		// get the data from the adcs and put it in the according to the axis of the sat than save it in the file system
		cspace_adcs_attang_t satEstimatesAttitideAngles;
		satEstimatesAttitideAngles.fields.pitch = ADCSStateData.fields.estim_angles.fields.yaw;
		satEstimatesAttitideAngles.fields.roll = ADCSStateData.fields.estim_angles.fields.pitch;
		satEstimatesAttitideAngles.fields.yaw = -1 * ADCSStateData.fields.estim_angles.fields.roll;
		c_fileWrite(ESTIMATED_ANGLES_FILE, satEstimatesAttitideAngles.raw);
	}
	// checks if it neccacery to save this param
	if(checkEstimatedAngularRates(StTa))
	{
		// get the data from the adcs and put it in the according to the axis of the sat than save it in the file system
		 cspace_adcs_angrate_t satEstimatesAngularRates;
		satEstimatesAngularRates.fields.angrate_x = ADCSStateData.fields.estim_angrate.fields.angrate_z;
		satEstimatesAngularRates.fields.angrate_y = ADCSStateData.fields.estim_angrate.fields.angrate_y * -1;
		satEstimatesAngularRates.fields.angrate_z = ADCSStateData.fields.estim_angrate.fields.angrate_x;
		c_fileWrite(ESTIMATED_ANGULAR_RATE_FILE, satEstimatesAngularRates.raw);
	}
	// checks if it neccacery to save this param
	if(checkSatellitePositionECI(StTa))
	{
		// get the data from the adcs and put it in the according to the axis of the sat than save it in the file system
		cspace_adcs_pos_t satStallitePositionECI;
		satStallitePositionECI.fields.posvex_x = ADCSStateData.fields.adcs_pos.fields.posvec_z;
		satStallitePositionECI.fields.posvec_y = -1 * ADCSStateData.fields.adcs_pos.fields.posvec_y;
		satStallitePositionECI.fields.posvec_z = ADCSStateData.fields.adcs_pos.fields.posvex_x;
		c_fileWrite(NAME_OF_ECI_POSITION_DATA_FILE, satStallitePositionECI.raw);
	}
	// checks if it neccacery to save this param
	if(checkSatelliteVelocityECI(StTa))
	{
		// get the data from the adcs and put it in the according to the axis of the sat than save it in the file system
		cspace_adcs_ecirefvel_t satECIVelocity;
		satECIVelocity.fields.velocity_x = ADCSStateData.fields.adcs_vel.fields.velocity_z;
		satECIVelocity.fields.velocity_y = ADCSStateData.fields.adcs_vel.fields.velocity_y * -1;
		satECIVelocity.fields.velocity_z = ADCSStateData.fields.adcs_vel.fields.velocity_x;
		c_fileWrite(NAME_OF_SATALLITE_VELOCITY_DATA_FILE, satECIVelocity.raw);

	}
	// checks if it neccacery to save this param
	if(checkSatellitePositionLLH(StTa))
	{
		//save the data
		c_fileWrite(NAME_OF_LLH_POSTION_DATA_FILE, ADCSStateData.fields.adcs_coord.raw);
	}
	// checks if it neccacery to save this param
	if(checkMagneticFieldVector(StTa))
	{
		// get the data from the adcs and put it in the according to the axis of the sat than save it in the file system
		cspace_adcs_magfieldvec_t magneticFieldDataADCS;
		cspaceADCS_getMagneticFieldVec(ADCS_ID, &magneticFieldDataADCS);
		cspace_adcs_magfieldvec_t magneticFieldDataSat;
		magneticFieldDataSat.fields.magfield_x = magneticFieldDataADCS.fields.magfield_z;
		magneticFieldDataSat.fields.magfield_y = -1 * magneticFieldDataADCS.fields.magfield_y;
		magneticFieldDataSat.fields.magfield_z = magneticFieldDataADCS.fields.magfield_x;
		c_fileWrite(NAME_OF_MAGNETIC_FIELD_DATA_FILE, magneticFieldDataSat.raw);
	}
	// checks if it neccacery to save this param
	if(checkCoarseSunVector(StTa))
	{
		// get the data from the adcs and put it in the according to the axis of the sat than save it in the file system
		cspace_adcs_sunvec_t CSSSunVectorADCS;
		cspaceADCS_getCoarseSunVec(ADCS_ID, &CSSSunVectorADCS);
		cspace_adcs_sunvec_t CSSSunVectorSat;
		CSSSunVectorSat.fields.sunvec_x = CSSSunVectorADCS.fields.sunvec_z;
		CSSSunVectorSat.fields.sunvec_y = -1 * CSSSunVectorADCS.fields.sunvec_y;
		CSSSunVectorSat.fields.sunvec_z =  CSSSunVectorADCS.fields.sunvec_x;
		c_fileWrite(CSS_SUN_VECTOR_DATA_FILE, CSSSunVectorSat.raw);
	}
	// checks if it neccacery to save this param
	if(checkRateSensorRates(StTa))
	{
		// get the data from the adcs and put it in the according to the axis of the sat than save it in the file system
		cspace_adcs_angrate_t rateSensorADCS;
		cspaceADCS_getSensorRates(ADCS_ID, &rateSensorADCS);
		cspace_adcs_angrate_t rateSensorSat;
		rateSensorSat.fields.angrate_x = rateSensorADCS.fields.angrate_z;
		rateSensorSat.fields.angrate_y = -1 * rateSensorADCS.fields.angrate_y;
		rateSensorSat.fields.angrate_z = rateSensorADCS.fields.angrate_x;
		c_fileWrite(SENSOR_RATE_DATA_FILE, rateSensorSat.raw);
	}
	// checks if it neccacery to save this param
	if(checkWheelSpeedcheck(StTa))
	{
		// get the data from the adcs and put it in the according to the axis of the sat than save it in the file system
		cspace_adcs_wspeed_t wheelSpeedADCS;
		cspaceADCS_getWheelSpeed(ADCS_ID, &wheelSpeedADCS);
		cspace_adcs_wspeed_t wheelSpeedSat;
		wheelSpeedSat.fields.speed_x = wheelSpeedADCS.fields.speed_z;
		wheelSpeedSat.fields.speed_y = -1 * wheelSpeedADCS.fields.speed_y;
		wheelSpeedSat.fields.speed_z = wheelSpeedADCS.fields.speed_x;
		c_fileWrite(WHEEL_SPEED_DATA_FILE, wheelSpeedSat.raw);
	}
	// checks if it neccacery to save this param
	if(checkMagnetorquercmd(StTa))
	{
		// get the data from the adcs and put it in the according to the axis of the sat than save it in the file system
		cspace_adcs_magtorqcmd_t magnetorquerCmdADCS;
		cspaceADCS_getMagnetorquerCmd(ADCS_ID,&magnetorquerCmdADCS);
		cspace_adcs_magtorqcmd_t magnetorquerCmdSat;
		magnetorquerCmdSat.fields.magcmd_x = magnetorquerCmdADCS.fields.magcmd_z;
		magnetorquerCmdSat.fields.magcmd_y = -1 * magnetorquerCmdADCS.fields.magcmd_y;
		magnetorquerCmdSat.fields.magcmd_z = magnetorquerCmdADCS.fields.magcmd_x;
		c_fileWrite(MAGNETORQUER_CMD_FILE, magnetorquerCmdSat.raw);
	}
	// checks if it neccacery to save this param
	if(checkWheelSpeedcmd(StTa))
	{
		// get the data from the adcs and put it in the according to the axis of the sat than save it in the file system
		cspace_adcs_wspeed_t wheelSpeedCmdADCS;
		cspaceADCS_getWheelSpeedCmd(ADCS_ID,&wheelSpeedCmdADCS);
		cspace_adcs_wspeed_t wheelSpeedCmdSat;
		wheelSpeedCmdSat.fields.speed_x = wheelSpeedCmdADCS.fields.speed_z;
		wheelSpeedCmdSat.fields.speed_y = -1 * wheelSpeedCmdADCS.fields.speed_y;
		wheelSpeedCmdSat.fields.speed_z = wheelSpeedCmdADCS.fields.speed_x;
		c_fileWrite(WHEEL_SPEED_CMD_FILE, wheelSpeedCmdSat.raw);
	}
	// checks if it neccacery to save this param
	if(checkIGRFModelledMagneticFieldVector(StTa))
	{
		// are you reading this? there is a surprise at the end of the line 																																																						btw the code lines do the same as before                                                                          																										getting closer																																										hotter																																										HOT!!!!																																											well fine if you are this persistent																																										well here it is																																										thre is nothing its just a joke																																														no seriously this was a joke there is nothing ahead																																													don't go there is dangerous																																									pls stop 																																										trust there is nothing it a joke																																																																																																																																																																																																																																																																																																																																																																																																																																																																									:,(                  																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																							   its a joke m8 i got you HAHAHAHAHA
		cspace_adcs_igrf_magvec_t IGRFModelMagneticFieldVectorADCS = ADCSDataGenral.fields.igrf_magfield;
		cspace_adcs_igrf_magvec_t IGRFModelMagneticFieldVectorSat;
		IGRFModelMagneticFieldVectorSat.fields.igrf_magfield_x = IGRFModelMagneticFieldVectorADCS.fields.igrf_magfield_z;
		IGRFModelMagneticFieldVectorSat.fields.igrf_magfield_y = IGRFModelMagneticFieldVectorADCS.fields.igrf_magfield_y * -1;
		IGRFModelMagneticFieldVectorSat.fields.igrf_magfield_z = IGRFModelMagneticFieldVectorADCS.fields.igrf_magfield_x;
		c_fileWrite(IGRF_MODEL_MAGNETIC_FIELD_VECTOR_FILE, IGRFModelMagneticFieldVectorSat.raw);
	}
	// checks if it neccacery to save this param
	if(checkEstimatedGyroBiascheck(StTa))
	{
		cspace_adcs_estgyrovec_t gyroBiasADCS = ADCSDataGenral.fields.estgyrobias;
		cspace_adcs_estgyrovec_t gyroBiasSat;
		gyroBiasSat.fields.estgyrobias_x = gyroBiasADCS.fields.estgyrobias_z;
		gyroBiasSat.fields.estgyrobias_y = gyroBiasADCS.fields.estgyrobias_y * -1;
		gyroBiasSat.fields.estgyrobias_z = gyroBiasADCS.fields.estgyrobias_x;
		c_fileWrite(GYRO_BIAS_FILE, gyroBiasSat.raw);
	}
	// checks if it neccacery to save this param
	if(checkEstimationInnovationVector(StTa))
	{
		cspace_adcs_estvec_innv_t innovationVectorADCS = ADCSDataGenral.fields.innovationvec;
		cspace_adcs_estvec_innv_t innovationVectorSat;
		innovationVectorSat.fields.innovationvec_x = innovationVectorADCS.fields.innovationvec_z;
		innovationVectorSat.fields.innovationvec_y = innovationVectorADCS.fields.innovationvec_y * -1;
		innovationVectorSat.fields.innovationvec_z = innovationVectorADCS.fields.innovationvec_x;
		c_fileWrite(INNOVATION_VECTOR_FILE, innovationVectorSat.raw);
	}
	// checks if it neccacery to save this param
	if(checkQuaternionErrorVector(StTa))
	{
		c_fileWrite(ERROR_VECTOR_FILE, ADCSDataGenral.fields.errquaternion.raw);
	}
	// checks if it neccacery to save this param
	if(checkQuaternionCovariance(StTa))
	{
		c_fileWrite(QUATERNION_COVARIANCE_FILE, ADCSDataGenral.fields.quatcovariancerms.raw);
	}
	// checks if it neccacery to save this param
	if(checkAngularRateCovariance(StTa))
	{
		cspace_adcs_angrate_covvec_t satAngularRateCovariance;
				satAngularRateCovariance.fields.angratecov_x = ADCSDataGenral.fields.angratecov.fields.angratecov_z;
				satAngularRateCovariance.fields.angratecov_y = ADCSDataGenral.fields.angratecov.fields.angratecov_y * -1;
				satAngularRateCovariance.fields.angratecov_z = ADCSDataGenral.fields.angratecov.fields.angratecov_x;
				c_fileWrite(ANGULAR_RATE_COVARIANCE_FILE, satAngularRateCovariance.raw);
			}
			// checks if it neccacery to save this param
			if(checkRawCSS(StTa))
			{
				cspace_adcs_rawcss1_6_t rawCSSDataADCS;
				cspaceADCS_getRawCss1_6Measurements(ADCS_ID,  &rawCSSDataADCS);
				c_fileWrite(NAME_OF_CSS_DATA_FILE,rawCSSDataADCS.raw);
			}
			// checks if it neccacery to save this param
			if(checkRawMagnetometer(StTa))
			{
				cspace_adcs_rawmagmeter_t rawMagADCS;
				cspaceADCS_getRawMagnetometerMeas(ADCS_ID, &rawMagADCS);
				cspace_adcs_rawmagmeter_t rawMagSat;
				rawMagSat.fields.magnetic_x = rawMagADCS.fields.magnetic_z;
				rawMagSat.fields.magnetic_y = -1 * rawMagADCS.fields.magnetic_y;
				rawMagSat.fields.magnetic_z = rawMagADCS.fields.magnetic_x;
				c_fileWrite(RAW_MAGNETOMETER_FILE, rawMagSat.raw);
			}
			// checks if it neccacery to save this param
			if(checkEstimatedQuaternion(StTa))
			{
				c_fileWrite(ESTIMATED_QUATERNION_FILE, ADCSStateData.fields.est_quat.raw);
			}
			// checks if it neccacery to save this param
			if(checkECEFPosition(StTa))
			{
				cspace_adcs_ecef_pos_t satECEFPostion;
				satECEFPostion.fields.ecefpos_x = ADCSStateData.fields.ecef_pos.fields.ecefpos_z;
				satECEFPostion.fields.ecefpos_y = -1 * ADCSStateData.fields.ecef_pos.fields.ecefpos_y;
				satECEFPostion.fields.ecefpos_z = ADCSStateData.fields.ecef_pos.fields.ecefpos_x;
				c_fileWrite(NAME_OF_ECEF_POSITION_DATA_FILE, satECEFPostion.raw);

			}





		}

void eslADC_setGetWheelspeedTlmTest(void)
{
	cspace_adcs_wspeed_t cmdwheelspeed;
	cspace_adcs_wspeed_t getwheelspeed;
	cspaceADCS_getWheelSpeedCmd(0, &cmdwheelspeed);




	c_fileWrite(WHEEL_SPEED_CMD_FILE, cmdwheelspeed.raw);
	cspaceADCS_getWheelSpeed(0, &getwheelspeed);

	printf("Wheelspeed CMD y: %d [rpm]\r\n ", cmdwheelspeed.fields.speed_y);
	printf("Wheelspeed y: %d [rpm]\r\n ", getwheelspeed.fields.speed_y);
	printf("Wheelspeed cmd - speed: %d \r\n ",cmdwheelspeed.fields.speed_y - getwheelspeed.fields.speed_y);
	c_fileWrite(WHEEL_SPEED_DATA_FILE, getwheelspeed.raw);
}

void eslADC_setMTQTest(void)
{
	cspace_adcs_runmode_t run_Mode;
	cspace_adcs_magnetorq_t magpulse;
	cspace_adcs_rawmagmeter_t redmag;
	cspace_adcs_magfieldvec_t magmeter;
	int err;
	double duty_cycle = 0.90;
	for(duty_cycle = 0;duty_cycle < 1; duty_cycle+=0.05)
	{
		run_Mode = runmode_off;
		setRunMode(run_Mode);
		magpulse.fields.magduty_x = (short)(duty_cycle * 1000.0);
		magpulse.fields.magduty_y = (short)(duty_cycle * 1000.0);
		magpulse.fields.magduty_z = (short)(duty_cycle * 1000.0);
		err = cspaceADCS_setMagOutput(0, &magpulse);
		if(err != 0)
		{
			printf("%d",err);
		}
		run_Mode = runmode_enabled;
		setRunMode(run_Mode);
		vTaskDelay(1000);
		cspace_adcs_magtorqcmd_t mag_cmd;
		cspaceADCS_getMagnetorquerCmd(ADCS_ID, &mag_cmd);
		printf("cmd magduty_x: %d \r\n ", mag_cmd.fields.magcmd_x);
		printf("cmd magduty_y: %d \r\n ", mag_cmd.fields.magcmd_y);
		printf("cmd magduty_z: %d \r\n ", mag_cmd.fields.magcmd_z);
		c_fileWrite(MAGNETORQUER_CMD_FILE, mag_cmd.raw);
		cspaceADCS_getADCRawRedMag(ADCS_ID, &redmag);
		printf("Magnetometer vector x: %d \r\n", redmag.fields.magnetic_x);
		printf("Magnetometer vector y: %d \r\n", redmag.fields.magnetic_y);
		printf("Magnetometer vector z: %d \r\n", redmag.fields.magnetic_z);

		c_fileWrite(RAW_MAGNETOMETER_FILE, redmag.raw);
		cspaceADCS_getMagneticFieldVec(ADCS_ID, &magmeter);
		printf("magduty_x: %f [uT]\r\n ", (double) magmeter.fields.magfield_x * 0.01);
		printf("magduty_y: %f [uT]\r\n ", (double) magmeter.fields.magfield_y * 0.01);
		printf("magduty_z: %f [uT]\r\n ", (double) magmeter.fields.magfield_z * 0.01);
		printf("cmd magduty_x - magduty_x: %f \r\n",(double) magmeter.fields.magfield_x * 0.01 - redmag.fields.magnetic_x);
		printf("cmd magduty_y - magduty_y: %f \r\n",(double) magmeter.fields.magfield_y * 0.01 - redmag.fields.magnetic_y);
		printf("cmd magduty_Z - magduty_z: %f \r\n",(double) magmeter.fields.magfield_z * 0.01 - redmag.fields.magnetic_z);

		c_fileWrite(NAME_OF_MAGNETIC_FIELD_DATA_FILE, magmeter.raw);

	}
}

//waits for the given amount of seconds and print adcs information
void Wait_ADCS(int delay)
{
	cspace_adcs_magfieldvec_t redmag;
	cspace_adcs_wspeed_t getwheelspeed;
	int err,i;
	for(i = 0; i < delay; i+= 1000)
	{
		err = cspaceADCS_getMagneticFieldVec(ADCS_ID, &redmag);
		printf("magduty_x: %f [uT]\r\n ", (double) redmag.fields.magfield_x * 0.01);
		printf("magduty_y: %f [uT]\r\n ", (double) redmag.fields.magfield_y * 0.01);
		printf("magduty_z: %f [uT]\r\n ", (double) redmag.fields.magfield_z * 0.01);
		err = cspaceADCS_getWheelSpeed(0, &getwheelspeed);
		printf("Wheelspeed CMD y: %d [rpm]\r\n ", getwheelspeed.fields.speed_y);
		printf("%d",err);
		vTaskDelay(1000);
	}
}

//getts to the given speed and waits for aditional to sec
void Get_To_Speed(int _speed)
{
	cspace_adcs_magfieldvec_t redmag;
	cspace_adcs_wspeed_t Speed;
	Speed.fields.speed_y = _speed;
	Speed.fields.speed_x = 0;
	Speed.fields.speed_z = 0;
	int err = cspaceADCS_setWheelSpeed(ADCS_ID,&Speed);
	printf("err : %d\n",err);
	Speed.fields.speed_y = 0;
	while((_speed > 0 && Speed.fields.speed_y < _speed) || (_speed < 0 && Speed.fields.speed_y > _speed) || (_speed == 0 && Speed.fields.speed_y == _speed))
	{
		err = cspaceADCS_getMagneticFieldVec(ADCS_ID, &redmag);
		printf("magX: %f [uT]\r\n ", (double) redmag.fields.magfield_x * 0.01);
		printf("magY: %f [uT]\r\n ", (double) redmag.fields.magfield_y * 0.01);
		printf("magZ: %f [uT]\r\n ", (double) redmag.fields.magfield_z * 0.01);

		cspaceADCS_getWheelSpeed(0, &Speed);
		printf("Wheelspeed CMD y: %d [rpm]\r\n ", Speed.fields.speed_y);

		vTaskDelay(1000);
	}
	Wait_ADCS(2);
}

void magOff()
{
	cspace_adcs_magnetorq_t magpulse;
	magpulse.fields.magduty_x = 0;
	magpulse.fields.magduty_y = 0;
	magpulse.fields.magduty_z = 0;

cspaceADCS_setMagOutput(0, &magpulse);}

void ADCS_ERROR_TEST()
{
	int x = 20;
	int err;
	unsigned char *Alpha = "aaabbbb";
	unsigned char *DBT = "aaabbbb";
	unsigned char b[100];
	unsigned char _avil = 0;
	Boolean offerMoreTests = TRUE;
	unsigned int selection = 0;
	while(offerMoreTests){
					printf( "\n\r Select a test to perform: \n\r");
					printf("\t 666 Soft Reset(gracfulReset)\n\r");
					printf("\t 0) Get me out of here \n\r");
					printf("\t 1) EPS On \n\r");
					printf("\t 2) EPS Off \n\r");
					printf("\t 3) Print EPS \n\r");
					printf("\t 4) Set x = 10 \n\r");
					printf("\t 5) x += 10 \n\r");
					printf("\t 6) Send x bytes \n\r");
					printf("\t 7) Print x \n\r");

					while(UTIL_DbguGetIntegerMinMax(&selection, 0,666) == 0);

						switch (selection)
						{
						case 666: gracefulReset();	break;
						case 0:
									offerMoreTests = FALSE;
									break;
						case 1:
									Set_Power(1);
									break;
						case 2:
									Set_Power(0);
									break;
						case 3:
									//Print_EPS_Channel();
									break;

						case 4:
									x = 10 ;
									break;
						case 5:
									x += 5;
									break;
						case 6:
							IsisTrxvu_tcSetAx25Bitrate(0,trxvu_bitrate_9600);
							int pactAmunt = 0;
							printf("insirt pact amunt\n");
							while(UTIL_DbguGetIntegerMinMax(&pactAmunt, 0,666) == 0);
							for(int i = 0; i<pactAmunt;i++){
									vTaskDelay(x);
									IsisTrxvu_tcSetAx25Bitrate(0,trxvu_bitrate_9600);














									err = IsisTrxvu_tcSendAX25DefClSign(0,b,i,&_avil);
							}
									if(err != 0)
									{
										printf("err : %d\n ",err);
									}
									break;

						case 7:
									printf("#nX : %d\n ",x);
									break;

						}
		}
}

void Set_RunMode(unsigned char data)
{
	int err;
	err = setRunMode(data);
	if(err != 0)
	{
		printf("%d",err);
	}
	vTaskDelay(1000);
}

void Print_Run()
{
	int err;
	cspace_adcs_statetlm_t adcs_state;
	err =	cspaceADCS_getStateTlm(ADCS_ID,&adcs_state);
	if(err != 0)
	{
		printf("%d",err);
	}
	printf("Run mode enabled: %d \r\n", adcs_state.fields.curr_state.fields.run_mode);

}

void Set_Power(unsigned char data)
{
	gom_eps_channelstates_t switches_states;
	int err;
	switches_states.fields.quadbatSwitch = 0;
	switches_states.fields.quadbatHeater = 0;
	switches_states.fields.channel3V3_1 = data;
	switches_states.fields.channel3V3_2 = 0;
	switches_states.fields.channel3V3_3 = 0;
	switches_states.fields.channel5V_1 = data;
	switches_states.fields.channel5V_2 = 0;
	switches_states.fields.channel5V_3 = 0;
	err =  GomEpsSetOutput(0,switches_states);
	if(err != 0)
	{
		printf("%d\n",err);
	}
	vTaskDelay(2000);
}

void Print_EPS_Channel()
{
	gom_eps_hk_t myEpsTelemetry_hk;
	GomEpsGetHkData_general(0, &myEpsTelemetry_hk);
	printf("Output Status 1 = %d \r\n", myEpsTelemetry_hk.fields.output[0]);
	printf("Output Status 4 = %d \r\n", myEpsTelemetry_hk.fields.output[3]);
}

void Set_ADCS_Power(int data)
{
	int err;
	cspace_adcs_powerdev_t set_ctrldev;
	set_ctrldev.raw[0] = 0;
	set_ctrldev.raw[1] = 0;
	set_ctrldev.raw[2] = 0;
	printf("set signal mode\n");
	while(UTIL_DbguGetIntegerMinMax(&data, 0,2) == 0);
	set_ctrldev.fields.signal_cubecontrol = data;
	printf("set cube control motor  mode\n");
	while(UTIL_DbguGetIntegerMinMax(&data, 0,2) == 0);

	set_ctrldev.fields.motor_cubecontrol = data;
	printf("set motor mode\n");
	while(UTIL_DbguGetIntegerMinMax(&data, 0,2) == 0);

	set_ctrldev.fields.pwr_motor = data;
	err = cspaceADCS_setPwrCtrlDevice(0, &set_ctrldev);
	if(err != 0)
	{
		printf("%d\n",err);
	}
	err = cspaceADCS_setAttEstMode(ADCS_ID,estmode_mems_gyro_ekf);
	if(err != 0)
	{
		printf("%d\n",err);
	}
}

void Print_Power()
{
	int err;
	cspace_adcs_powerdev_t get_ctrldev;
	err = cspaceADCS_getPwrCtrlDevice(0, &get_ctrldev);
	if(err != 0)
	{
		printf("%d\n",err);
	}
	printf("ADCS Motor: %d \r\n", get_ctrldev.fields.pwr_motor);
	printf("Signal Cube Control: %d \r\n", get_ctrldev.fields.signal_cubecontrol);
	printf("Motor Cube Control: %d \r\n", get_ctrldev.fields.motor_cubecontrol);
}

void Wheel_Test()
{
	Wait_ADCS(10000);
	printf("Stage 1:0 --> 8000");
	Get_To_Speed(8000);
	printf("wait 5s");
	Wait_ADCS(5000);
	printf("Stage 2:8000 --> 0");
	Get_To_Speed(0);
	printf("wait 5s");
	Wait_ADCS(10000);
	printf("Stage 3:0 --> -8000");
	Get_To_Speed(-8000);
	printf("wait 5s");
	Wait_ADCS(5000);
	printf("Stage 4:-8000 --> 0");
	Get_To_Speed(0);
	printf("wait 5s");
	Wait_ADCS(10000);
	printf("Stage 5:0 --> 4000");
	Get_To_Speed(4000);
	printf("wait 5s");
	Wait_ADCS(5000);
	printf("Stage 6:4000 --> 0");
	Get_To_Speed(0);
	printf("wait 5s");
	Wait_ADCS(10000);
	printf("Stage 7:0 --> -4000");
	Get_To_Speed(-4000);
	printf("wait 5s");
	Wait_ADCS(5000);
	printf("Stage 8:-4000 --> 0");
	Get_To_Speed(0);
	printf("wait 5s");
	Wait_ADCS(10000);

}

void Stop_wheel()
{
	cspace_adcs_wspeed_t Speed;
	int err;
	Speed.fields.speed_y = 0;
	Speed.fields.speed_x = 0;
	Speed.fields.speed_z = 0;
	err = cspaceADCS_setWheelSpeed(ADCS_ID,&Speed);
	if(err != 0)
	{
		printf("%d",err);
	}
}

void print_Est()
{

}

void Mems_Test()
{
	cspace_adcs_estmetadata_t metadata;
	cspace_adcs_measure_t adcs_meas;
	cspace_adcs_rawsenms_t rawsensors;
	cspace_adcs_statetlm_t adcs_state;
	int i = 0;
	double conv_angrate = 0,conv_tempdata = 0,conv_magfield=0;
	for(i = 0;i <= 10; i++)
	{
		cspaceADCS_getStateTlm(0, &adcs_state);

		cspaceADCS_getEstimationMetadata(0, &metadata);
		cspaceADCS_getADCSMeasurements(0, &adcs_meas);
		cspaceADCS_getRawSensorMeasurements(0, &rawsensors);

		conv_angrate = (double) adcs_meas.fields.angular_rate.fields.angrate_x * 0.01; //Conversion from raw to eng values
		printf("Sensor rates x: %f [deg/s]\r\n", conv_angrate);
		conv_angrate = (double) adcs_meas.fields.angular_rate.fields.angrate_y * 0.01; //Conversion from raw to eng values
		printf("Sensor rates y: %f [deg/s]\r\n", conv_angrate);
		conv_angrate = (double) adcs_meas.fields.angular_rate.fields.angrate_z * 0.01; //Conversion from raw to eng values
		printf("Sensor rates z: %f [deg/s]\r\n", conv_angrate);

		conv_angrate = (double) adcs_meas.fields.angular_rate.fields.angrate_x * 0.01; //Conversion from raw to eng values
		printf("Sensor rates x: %f [deg/s]\r\n", conv_angrate);
		conv_angrate = (double) adcs_meas.fields.angular_rate.fields.angrate_y * 0.01; //Conversion from raw to eng values
		printf("Sensor rates y: %f [deg/s]\r\n", conv_angrate);
		conv_angrate = (double) adcs_meas.fields.angular_rate.fields.angrate_z * 0.01; //Conversion from raw to eng values
		printf("Sensor rates z: %f [deg/s]\r\n", conv_angrate);

		conv_tempdata = (double) metadata.fields.igrf_magfield.fields.igrf_magfield_x * 0.01; //Conversion from raw to eng values
		printf("IGRF magfield x: %f \r\n ", conv_tempdata);
		conv_tempdata = (double) metadata.fields.igrf_magfield.fields.igrf_magfield_y * 0.01; //Conversion from raw to eng values
		printf("IGRF magfield y: %f \r\n ", conv_tempdata);
		conv_tempdata = (double) metadata.fields.igrf_magfield.fields.igrf_magfield_z * 0.01; //Conversion from raw to eng values
		printf("IGRF magfield z: %f \r\n ", conv_tempdata);

		conv_magfield = (double) adcs_meas.fields.magfield.fields.magfield_x * 0.01; //Conversion from raw to eng values
		printf("Magnetic field x: %f [uT] \r\n", conv_magfield);
		conv_magfield = (double) adcs_meas.fields.magfield.fields.magfield_y * 0.01; //Conversion from raw to eng values
		printf("Magnetic field y: %f [uT] \r\n", conv_magfield);
		conv_magfield = (double) adcs_meas.fields.magfield.fields.magfield_z * 0.01; //Conversion from raw to eng values
		printf("Magnetic field z: %f [uT] \r\n", conv_magfield);

		printf("Raw Magnetometer x: %d \r\n", rawsensors.fields.magmeter_raw.fields.magnetic_x);
		printf("Raw Magnetometer y: %d \r\n", rawsensors.fields.magmeter_raw.fields.magnetic_y);
		printf("Raw Magnetometer z: %d \r\n", rawsensors.fields.magmeter_raw.fields.magnetic_z);

		printf("est roll: %d \r\n", adcs_state.fields.estim_angles.fields.roll);
		printf("est pitch %d \r\n", adcs_state.fields.estim_angles.fields.pitch);
		printf("est yaw: %d \r\n", adcs_state.fields.estim_angles.fields.yaw);

		printf("est angle rate x: %d \r\n", adcs_state.fields.estim_angrate.fields.angrate_x);
		printf("est angle rate y: %d \r\n", adcs_state.fields.estim_angrate.fields.angrate_y);
		printf("est angle rate z: %d \r\n", adcs_state.fields.estim_angrate.fields.angrate_z);
		vTaskDelay(1000);

	}
}

void css()
{
	int err;
	int i = 0;
	cspace_adcs_rawcss1_6_t rawcss_group1;
	cspace_adcs_rawcss7_10_t rawcss_group2;
	cspace_adcs_sunvec_t raw_sunvec;
	for(i =0;i < 20;i++)
	{
		err = cspaceADCS_getRawCss1_6Measurements(0, &rawcss_group1);
		if (err == 0)
		{
			printf("Raw Course Sun Sensor 1: %d \r\n", rawcss_group1.fields.css_1);
			printf("Raw Course Sun Sensor 2: %d \r\n", rawcss_group1.fields.css_2);
			printf("Raw Course Sun Sensor 3: %d \r\n", rawcss_group1.fields.css_3);
			printf("Raw Course Sun Sensor 4: %d \r\n", rawcss_group1.fields.css_4);
			printf("Raw Course Sun Sensor 5: %d \r\n", rawcss_group1.fields.css_5);
			printf("Raw Course Sun Sensor 6: %d \r\n", rawcss_group1.fields.css_6);
			vTaskDelay(1000);
		}
		else
		{
			printf("error from getRawCss 1: %d\n", err);
		}
		/*err = cspaceADCS_getRawCss7_10Measurements(0, &rawcss_group2);
		if (err == 0)
		{
			printf("Raw Course Sun Sensor 7: %d \r\n", rawcss_group2.fields.css_7);
			printf("Raw Course Sun Sensor 8: %d \r\n", rawcss_group2.fields.css_8);
			printf("Raw Course Sun Sensor 9: %d \r\n", rawcss_group2.fields.css_9);
			printf("Raw Course Sun Sensor 10: %d \r\n", rawcss_group2.fields.css_10);
			vTaskDelay(1000);
		}
		else
		{
			printf("error from getRawCss 2: %d\n", err);
		}*/

		err = cspaceADCS_getCoarseSunVec(0,&raw_sunvec);
		if (err == 0)
		{
			printf("Raw sun vector, vector X: %d\r\n",raw_sunvec.fields.sunvec_x);
			printf("Raw sun vector, vector Y: %d\r\n",raw_sunvec.fields.sunvec_y);
			printf("Raw sun vector, vector Z: %d\r\n",raw_sunvec.fields.sunvec_z);
			vTaskDelay(1000);
		}
		else
		{
			printf("error from getCoarseSunVec 2: %d\n", err);
		}
	}

}


int test_data()
{
	int i;
	int user_id = -1;
	int num_pram = 0,sizeOfCmd;
	unsigned char *data;
	int count = 0;
	int err;
	int temp;
	while(user_id != 0)
	{
		printf("\nid:\n");
		while(UTIL_DbguGetIntegerMinMax(&user_id,0,255) == 0);
		printf("\nnum of param\n");
		while(UTIL_DbguGetIntegerMinMax(&num_pram,0,255) == 0);
		int len[num_pram];
		sizeOfCmd = 0;
		for(i = 0; i<num_pram;i++)
		{
			printf("\nprint len in place %d: \n", i);
			while(UTIL_DbguGetIntegerMinMax(&temp,0,8) == 0);
			sizeOfCmd += temp;
			len[i] = temp;
		}
		data = malloc(sizeOfCmd);
		int err = I2C_write(ADCS_I2C_ADRR, &user_id, 1);
		vTaskDelay(5);
		err = I2C_read(ADCS_I2C_ADRR,data,sizeOfCmd);
		count = 0;
		for(i = 0; i< num_pram;i++)
		{
			if(len[i] == 0)
			{

			}
			else if(len[i] == 4){
				printf("\nfloat val:%f \n" , data[count]);

			}else if(len[i] == 8)
			{
				printf("\ndouble val:%lf \n" , data[count]);
			}
			else{
				for(user_id = 0; user_id<len[i]; user_id++)
				{
					printf("\nint val:%d \n" , data[count+user_id]);
				}
			}
			count += len[i];
		}
		free(data);
		err = ReadACK(user_id);
		printf("err %d \n",err);

	}

}
void ADCS_PROPER_TEST()
{
	cspace_adcs_attctrl_mod_t contr;
	unsigned char * data;
	unsigned int id = 206;
	contr.fields.override_flag = 1;
	contr.fields.timeout = 0xFFF;
	Boolean offerMoreTests = TRUE;
	unsigned int selection = 0;
	cspace_adcs_currstate_t adcs_state;
	int est = 3;
	int err;
	double Data[8] = {25544,  51.6448, 108.6376, 1991, 140.1463,   4.1544, 15.55046168, 14633};
	while(offerMoreTests){
		GomEpsResetWDT(0);
					printf( "\n\r Select a test to perform: \n\r");
					printf("\t 666 Soft Reset(gracfulReset)\n\r");
					printf("\t 0) Get me out of here \n\r");
					printf("\t 1) EPS On \n\r");
					printf("\t 2) EPS Off \n\r");
					printf("\t 3) Print EPS \n\r");
					printf("\t 4) Run Mode on \n\r");
					printf("\t 5) Run Mode off \n\r");
					printf("\t 6) PRINT Run Mode \n\r");
					printf("\t 7) set Power  \n\r");
					printf("\t 8)  \n\r");
					printf("\t 9) PRINT Power \n\r");
					printf("\t 10) Wheel Test \n\r");
					printf("\t 11) Mag Test \n\r");
					printf("\t 12) Mems Test \n\r");
					printf("\t 13) stop all \n\r");
					printf("\t 14) Magnet read test\n\r");
					printf("\t 15) Print estimation \n\r");

					printf("\t 18) Send TLE Dat and save \n\r");
					printf("\t 19) css test \n\r");
					printf("\t 21) estimation mode \n\r");
					printf("\t 22) test def data\n\r");

					while(UTIL_DbguGetIntegerMinMax(&selection, 0,666) == 0);

						switch (selection)
						{
						case 666: gracefulReset();	break;
						case 0:
									offerMoreTests = FALSE;
									break;
						case 1:
									Set_Power(1);
									break;
						case 2:
									Set_Power(0);


									break;
						case 3:
									Print_EPS_Channel();
									break;

						case 4:
									Set_RunMode(1);
									break;
						case 5:
									Set_RunMode(0);
									break;
						case 6:
									Print_Run();
									break;

						case 7:
									Set_ADCS_Power(0);
									break;
						case 8:
									break;
						case 9:
									Print_Power();
									break;
						case 10:
									Wheel_Test();
									break;
						case 11:
									eslADC_setMTQTest();
									break;
						case 12:
									Mems_Test();
									break;
						case 13:
									Stop_wheel();
									magOff();
									break;
						case 14:

									break;
						case 15:
									cspaceADCS_getCurrentState(0,&adcs_state );
									printf("Est Mode %d\n\r",adcs_state.fields.attest_mode);
									break;
						case 16:
									printf("Set cntrol: \n");
									while(UTIL_DbguGetIntegerMinMax(&est, 0,13) == 0);
									contr.fields.ctrl_mode = est;
									cspaceADCS_setAttCtrlMode(ADCS_ID,&contr);
									vTaskDelay(1000);
									break;
						case 17:
									cspaceADCS_getCurrentState(0,&adcs_state );
									printf("ctrol Mode %d\n\r",adcs_state.fields.ctrl_mode);
									break;

						case 18:
									err = cspaceADCS_setSGP4OrbitParameters(ADCS_ID,Data);
									printf("Tle ERR: %d",err);
									vTaskDelay(2000);
									err = cspaceADCS_saveOrbitParam(ADCS_ID);
									printf("Save ERR: %d",err);

									break;
						case 19:
									css();
									break;

						case 20:
							err = cspaceADCS_BLSetBootIndex(0,1);
							vTaskDelay(1000);
							err = cspaceADCS_BLRunSelectedProgram(0);
							break;
						case 21:
							setEstimation();
							break;
						case 22:
							test_data();
							break;
						case 23:
							data = malloc(277);
							int err = I2C_write(ADCS_I2C_ADRR, &id, 1);
							vTaskDelay(5);
							err = I2C_read(ADCS_I2C_ADRR,data,277);
							for(id = 0;id< 277;id++)
							{
								printf("i:%d\nChar: %u\n",id,data[id]);
								printf("int: %d\n", data[id]);

							}
							break;
						}
	}
}



void ADCS_Stat(unsigned char data)
{
	int err;
	cspace_adcs_estmode_sel EST = estmode_mems_gyro_ekf;

	Set_Power(data);

	Set_RunMode(data);

	Set_Power(data);

	Print_Power();
	err = cspaceADCS_setAttEstMode(ADCS_ID,EST);
	if(err != 0)
	{
		printf("%d\n",err);
	}
}

void ADCS_PRINT()
{
	Print_EPS_Channel();
	Print_Run();
	Print_Power();
}

void Delay_Test(stageTable StTa)
{
	int end = 0;
	int delay = 666;
	unsigned char * Data = &delay;
	translateCommandDelay(Data,StTa);


	printf("Given: %d\n\r",delay);
	printf("Data: %d,%d,%d\n\r",Data[0],Data[1],Data[2]);
}
void Control_Test(stageTable StTa)
{
	unsigned char Data[STAGE_TABLE_SIZE];
	unsigned char end = 0;
	unsigned char Start = 1;
	cspace_adcs_currstate_t adcs_state;
	translateCommandControlMode(Start,StTa);
	getTableTo(StTa,Data);
	end = Data[3];
	printf("Given: %d,Goten: %d \n\r",Start,end);
	vTaskDelay(1000);
	cspaceADCS_getCurrentState(0,&adcs_state );
	printf("ctrol Mode %d\n\r",adcs_state.fields.ctrl_mode);
}
void Power_Test(stageTable StTa)
{
	unsigned char Data[STAGE_TABLE_SIZE];
	unsigned char end = 0;
	unsigned char start = 85;//01010101 1 + 4 + 16 + 64 00011111
	translateCommandPower(start,StTa);
	getTableTo(StTa,Data);
	end = Data[4];
	printf("Given: %d,Goten: %d \n\r",start,end);
	int err;
	cspace_adcs_powerdev_t get_ctrldev;
	vTaskDelay(1000);
	err = cspaceADCS_getPwrCtrlDevice(0, &get_ctrldev);
	if(err != 0)
	{
		printf("%d\n",err);
	}
	printf("ADCS Motor: %d \r\n", get_ctrldev.fields.pwr_motor);
	printf("Signal Cube Control: %d \r\n", get_ctrldev.fields.signal_cubecontrol);
	printf("Motor Cube Control: %d \r\n", get_ctrldev.fields.motor_cubecontrol);
}

void EST_Test(stageTable StTa)
{
	unsigned char Data[STAGE_TABLE_SIZE];
	unsigned char end = 0;
	unsigned char Start = 1;
	cspace_adcs_currstate_t adcs_state;
	translateCommandEstimationMode(Start,StTa);
	getTableTo(StTa,Data);
	end = Data[5];
	printf("Given: %d,Goten: %d \n\r",Start,end);
	vTaskDelay(1000);
	cspaceADCS_getCurrentState(0,&adcs_state );

	printf("est Mode %d\n\r",adcs_state.fields.attest_mode);
}

void TM_Test(stageTable StTa)
{
	unsigned char Start[3] = {0xff,0xf,0xf0};
	unsigned char end[STAGE_TABLE_SIZE];
	cspace_adcs_currstate_t adcs_state;
	cspaceADCS_getCurrentState(0,&adcs_state );
}

void Logic_Test(stageTable StTa )
{
	unsigned int selection = 0;

					Boolean offerMoreTests = TRUE;

	while(offerMoreTests){

					printf( "\n\r Select a test to perform: \n\r");
					printf("\t 666 Soft Reset(gracfulReset)\n\r");
					printf("\t 0) Get me out of here \n\r");
					printf("\t 1) Delay \n\r");
					printf("\t 2) conreol \n\r");
					printf("\t 3) power \n\r");
					printf("\t 4) est \n\r");
					printf("\t 5) tm \n\r");
					printf("\t 6) full \n\r");

					while(UTIL_DbguGetIntegerMinMax(&selection, 0,666) == 0);

						switch (selection)
						{
						case 666: gracefulReset();	break;
						case 0:
									offerMoreTests = FALSE;
									break;
						case 1:
									Delay_Test(StTa);
									break;
						case 2:
									Control_Test(StTa);
									break;
						case 3:
									Power_Test(StTa);
									break;
						case 4:
									EST_Test(StTa);
									break;
						case 5:
									TM_Test(StTa);
									break;
						case 6:
									Control_Test(StTa);
									break;


						}
}
}
void ADCS_Tests(stageTable StTa )
		{

				vTaskDelay(2000);


				unsigned int selection = 0;

				Boolean offerMoreTests = TRUE;

while(offerMoreTests){
				GomEpsResetWDT(0);
				printf( "\n\r Select a test to perform: \n\r");
				printf("\t 666 Soft Reset(gracfulReset)\n\r");
				printf("\t 0) Get me out of here \n\r");
				printf("\t 1) ADCS on \n\r");
				printf("\t 2) ADCS off \n\r");
				printf("\t 3) PRINT ADCS \n\r");
				printf("\t 4) ADCS Tests \n\r");
				printf("\t 5) Trouble shooting \n\r");
				printf("\t 6) Logic Tests \n\r");

				while(UTIL_DbguGetIntegerMinMax(&selection, 0,666) == 0);

					switch (selection)
					{
					case 666: gracefulReset();	break;
					case 0:
								offerMoreTests = FALSE;
								break;
					case 1:
								ADCS_Stat(1);
								break;
					case 2:
								ADCS_Stat(0);
								break;
					case 3:
								ADCS_PRINT();
								break;
					case 4:
								ADCS_PROPER_TEST();
								break;
					case 5:
								ADCS_ERROR_TEST();
								break;
					case 6:
								Logic_Test(StTa);
								break;
					default:
								break;

					}
				}
		}
void Command_Test()
{
	unsigned int selection = 0;
	cspace_adcs_bootinfo_t Boot;
	cspace_adcs_unixtm_t Time;
	unsigned char Data = 1;
	printf( "\n\r Select a test to perform: \n\r");
	printf("\t 666 Soft Reset(gracfulReset)\n\r");
	printf("\t 0) Get me out of here \n\r");
	printf("\t 1) reset Boot Registers \n\r");
	printf("\t 2) Set Unix Time \n\r");
	printf("\t 3) Cashe \n\r");
	printf("\t 4) est \n\r");
	printf("\t 5) tm \n\r");
	printf("\t 6) full \n\r");
	while(UTIL_DbguGetIntegerMinMax(&selection, 0,666) == 0);
	switch (selection)
	{
		case 1:
			cspaceADCS_getBootProgramInfo(0, &Boot);
			printf("boot %d\n", Boot.fields.bootcounter);
			Reset_Boot_Registers();
			vTaskDelay(1000);
			cspaceADCS_getBootProgramInfo(0, &Boot);
			printf("boot %d\n", Boot.fields.bootcounter);
			break;
		case 2:
			Time.fields.unix_time_sec = 1577836800;
			cspaceADCS_setCurrentTime(0,&Time);
			vTaskDelay(1000);
			cspaceADCS_getCurrentTime(0,&Time);
			printf("time ms --- s %d --- %d",Time.fields.unix_time_millsec,Time.fields.unix_time_sec);
			break;
	}
}
//! the adcs main loop
void ADCS_Task()
{
	int err;
	unsigned char data[STAGE_TABLE_SIZE];
	int ret = f_enterFS(); /* Register this task with filesystem */
	check_int("adcs_task enter FileSystem", ret);

	// the adcs loop
	while (TRUE)
	{
		//enter if you dear but know unless the cubeadcs is supplied with electricity from the eps no one can enter
		if (get_system_state(ADCS_param)) // change to Does systems works
		{
			// read the current stage table from the fram
			// gathers the telemtry according to the stage table
			gatherData(get_ST());
			getTableTo(get_ST(), data);
			FRAM_write(data, STAGE_TABLE_ADDR, STAGE_TABLE_SIZE);
			vTaskDelay(GetDelay(get_ST()));
			cspace_adcs_runmode_t Run = runmode_enabled;
			err = cspaceADCS_setRunMode(ADCS_ID,Run);
			if(err != 0)
			{
				printf("/n %d run",err);
			}
		}
	}
}
/*! fully translates the command  from the ground
 * @param[Telemtry] holds the new stage table
 */



//! calc Pid Parameters
/*!
 * @param[prvious] pointer to an int that holds the privious values of the eror
 * @param[integral] pointer to an int thaht hold the integral values of the erors
 */

int biasZ = 666; //change paramater
int biasX = 666; //change paramater
int kpZ = 666; //change paramater
int kpX = 666; //change paramater
int kiZ = 666; //change paramater
int kiX = 666; //change paramater
int kdZ = 666; //change paramater
int kdX = 666; //change paramater
int prvious[2];
int integral[2];


int ADCS_command(unsigned char id, unsigned char* data, unsigned int dat_len)
{
	unsigned char arr[dat_len + 1];
	unsigned int i;
	arr[0] = id;

	for (i = 1; i < dat_len + 1; i++)
	{
		arr[i] = data[i - 1];
	}

	I2C_write(ADCS_I2C_ADRR, &arr[0], dat_len + 1);
	return ReadACK(id);
}

cspace_adcs_magnetorq_t calculateMgnet()
{
	cspace_adcs_statetlm_t ADCSStateData;
	cspaceADCS_getStateTlm(ADCS_ID, &ADCSStateData);
	cspace_adcs_magnetorq_t PIDData;
	int xError = ADCSStateData.fields.estim_angles.fields.pitch; //chack placess
	PIDData.fields.magduty_x = (xError * kpX + kdX * (xError - prvious[X_AXIS]) + kiX * integral[X_AXIS] - biasX) / 1000;
	int zError = ADCSStateData.fields.estim_angles.fields.yaw; // check places
	PIDData.fields.magduty_z = (zError * kpZ + kdZ * (zError - prvious[Z_AXIS]) + kiZ * integral[Z_AXIS] - biasZ) / 1000;
	PIDData.fields.magduty_y = 0;
	prvious[X_AXIS] = xError;
	prvious[Z_AXIS] = zError;
	integral[X_AXIS] += xError;
	integral[Z_AXIS] += zError;
	return PIDData;
}
int Set_Boot_Index(unsigned char* data)
{
	return ADCS_command(100, data, 1);
}
int Run_Selected_Boot_Program()
{
	return ADCS_command(101, NULL, 0);
}
void get_Boot_Index(unsigned char * data)
{
	int id = 130;
	I2C_write(ADCS_I2C_ADRR, &id, 1);
	vTaskDelay(5);
	I2C_read(ADCS_I2C_ADRR, data, 2);
	printf("boot Index: %d", data[0]);
}

int ADCS_Config(unsigned char * data)
{
	return ADCS_command(20, data, 272);
}
int Mag_Config(unsigned char * data)
{
	return ADCS_command(21, data, 3);
}
int set_Detumbling_Param(unsigned char* data)
{
	return ADCS_command(38, data, 6);
}
int set_Commanded_Attitude(unsigned char* data)
{
	return ADCS_command(18, data, 14);
}
int Set_Y_Wheel_Param(unsigned char* data)
{
	return ADCS_command(39, data, 14);
}
int Set_REACTION_Wheel_Param(unsigned char* data)
{
	return ADCS_command(40, data, 8);
}
int Set_Moments_Inertia(unsigned char* data)
{
	return ADCS_command(41, data, 12);
}
int Set_Products_Inertia(unsigned char* data)
{
	return ADCS_command(42, data, 12);
}
int set_Mode_Of_Mag_OPeration(unsigned char * data)
{
	return ADCS_command(56, data, 1);
}
int SGP4_Oribt_Param(unsigned char * data)
{
	return ADCS_command(45, data, 64);
}
int SGP4_Oribt_Inclination(unsigned char * data)
{
	return ADCS_command(46, data, 8);
}
int SGP4_Oribt_Eccentricy(unsigned char * data)
{
	return ADCS_command(47, data, 8);
}
int SGP4_Oribt_RAAN(unsigned char * data)
{
	return ADCS_command(48, data, 8);
}
int SGP4_Oribt_Argument(unsigned char * data)
{
	return ADCS_command(49, data, 8);
}
int SGP4_Oribt_B_Star(unsigned char * data)
{
	return ADCS_command(50, data, 8);
}
int SGP4_Oribt_Mean_Motion(unsigned char * data)
{
	return ADCS_command(51, data, 8);
}
int SGP4_Oribt_Mean_Anomaly(unsigned char * data)
{
	return ADCS_command(47, data, 8);
}
int SGP4_Oribt_Epoc(unsigned char * data)
{
	return ADCS_command(53, data, 8);
}
int Reset_Boot_Registers()
{
	char data = "Null";
	return ADCS_command((unsigned char)6, &data, 0);
}
int ReadACK(int cmd)
{
	unsigned char * data = malloc(sizeof(unsigned char) * 3);
	unsigned char id = 240;
	I2C_write(ADCS_I2C_ADRR, &id, 1);
	vTaskDelay(5);
	I2C_read(ADCS_I2C_ADRR, data, 2);
	if (data[0] != cmd)
	{
		return 666;// cmd didn't go to the ADCS
	}
	char ret = data[2];
	free(data);
	return ret;
}



//! PID mode
void PIDMODE()
{
	int size = 0;
	unsigned char data[SIZE_OF_TELEMTRY];
	time_unix Todo;
	c_fileRead(ESTIMATED_ANGLES_FILE, data, 1, LAST_ELEMENT_IN_C_FILE, LAST_ELEMENT_IN_C_FILE, &size, &Todo);
	prvious[X_AXIS	] = GET_X_AXIS;
	prvious[Z_AXIS] = GET_Z_AXIS;
	integral[X_AXIS] = prvious[X_AXIS];
	integral[Z_AXIS] = prvious[Z_AXIS];

	//vDelay(changed from the ground)
	while(TRUE)
	{
		//cspaceADCS_setMagOutput(ADCS_ID, calculateMgnet());
		//vDelay(changed from the ground)
	}

}
int Save_Config()
{
	return cspaceADCS_saveConfig(ADCS_ID);
}
int Save_Orbit_Param()
{
	return cspaceADCS_saveOrbitParam(ADCS_ID);
}
int Set_Mag_Mount(cspace_adcs_magmountcfg_t * config_data)
{
	return cspaceADCS_setMagMountConfig(ADCS_ID, config_data);
}
int Set_Mag_Offest_Scale(cspace_adcs_offscal_cfg_t* config_data)
{
	return cspaceADCS_setMagOffsetScalingConfig(ADCS_ID, config_data);
}
int Set_Mag_Sense(cspace_adcs_magsencfg_t * config_data)
{
	return cspaceADCS_setMagSensConfig(ADCS_ID, config_data);
}

int setUnixTime(cspace_adcs_unixtm_t * unix_time)
{
	return cspaceADCS_setCurrentTime(ADCS_ID, unix_time);
}

int Cashe_State(unsigned char state)
{
	return cspaceADCS_cacheStateADCS(ADCS_ID, state);
}

