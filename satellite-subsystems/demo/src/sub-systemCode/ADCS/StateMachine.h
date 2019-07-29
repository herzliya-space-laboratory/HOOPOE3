
#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

#include <hal/boolean.h>
#include "AdcsTroubleShooting.h"
/*!
 * @this enum continue all the states
 * @each number respond state
 */
 typedef enum
 {

	 NUM_OF_STATES
 }AdcsStates;

 typedef enum
 {
	 SetBootIndex 98
 }Adcs_Sub_types;


 typedef Boolean8bit StateMatrix[NUM_OF_STATES][NUM_OF_STATES];

/*! @init the State Machine
 *  @the state machine is the part in the code which active the ADCS system.
 *  @return:
 *  @	the err code by TroubleErrCode enum
 */
 TroubleErrCode InitStateMachine();

/*!
 * @Update ADCS State Machine to the new values from the ground/system request
 * @get:
 * @	commend type.
 * @	commend data.
 * @	commend length.
 * @return:
 * @	the err code by TroubleErrCode enum
 */
 TroubleErrCode UpdateAdcsStateMachine(AdcsStateMachineCMD cmd, unsigned char *data, unsigned int length);

/*!
 * @upate the stages matrix in the F-ram and sd
 * @get:
 * @	new stages matrix
 * @return:
 * @	the err code by TroubleErrCode enum
 */
 TroubleErrCode UpdateStateMatrix(StateMatrix New_mat);

/*!
 * @get the stage matrix from the F-ram.
 * @return:
 * @	the stage matrix
 */
 StateMatrix GetStateMatrix();


 /*! Get a full stage table from the ground and update the full stage table
  * @param[telemtry] contains the new stage table data
  * @param[stagetable] the stage table that will be updated
  */
 int translateCommandFULL(unsigned char telemtry[]);
 /*! Get the delay parameter stage table from the ground and update it to the stage table
  * @param[delay] contains the new delay
  * @param[stagetable] the stage table that will be updated
  */
 int translateCommandDelay(unsigned char delay[3]);
 /*! Get the control mode parameter stage table from the ground and update it to the stage table
  * @param[controlMode] contains the new control Mode
  * @param[stagetable] the stage table that will be updated
  */
 int translateCommandControlMode(unsigned char controlMode);
 /*! Get the Power parameter stage table from the ground and update it to the stage table
  * @param[power] contains the new power
  * @param[stagetable] the stage table that will be updated
  */
 int translateCommandPower( unsigned char power);
 /*! Get the Estimation mode parameter stage table from the ground and update it to the stage table
  * @param[estimationMode] contains the new estimation Mode
  * @param[stagetable] the stage table that will be updated
  */
 int translateCommandEstimationMode(unsigned char estimationMode);
 /*! Get the Telemtry parameter stage table from the ground and update it to the stage table
  *  @param[telemtry] contains the new telemtry
  * @param[stagetable] the stage table that will be updated
  *
  */
 int translateCommandTelemtry(unsigned char telemtry[3]);

 int Set_RunMode(unsigned char data);
 int Run_Selected_Boot_Program();
 int Set_Boot_Index(unsigned char* data);
 int get_Boot_Index(unsigned char * data);
 int ADCS_Config(unsigned char * data);
 int Mag_Config(unsigned char * data);
 int set_Detumbling_Param(unsigned char* data);
 int set_Commanded_Attitude(unsigned char* data);
 int Set_Y_Wheel_Param(unsigned char* data);
 int Set_REACTION_Wheel_Param(unsigned char* data);
 int Set_Moments_Inertia(unsigned char* data);
 int Set_Products_Inertia(unsigned char* data);
 int set_Mode_Of_Mag_OPeration(unsigned char * data);
 int SGP4_Oribt_Param(unsigned char * data);
 int SGP4_Oribt_Inclination(unsigned char * data);
 int SGP4_Oribt_Eccentricy(unsigned char * data);
 int SGP4_Oribt_RAAN(unsigned char * data);
 int SGP4_Oribt_Argument(unsigned char * data);
 int SGP4_Oribt_B_Star(unsigned char * data);
 int SGP4_Oribt_Mean_Motion(unsigned char * data);
 int SGP4_Oribt_Mean_Anomaly(unsigned char * data);
 int SGP4_Oribt_Epoc(unsigned char * data);
 int Reset_Boot_Registers();
 int Save_Config();
 int Save_Orbit_Param();
 int setUnixTime(unsigned char * unix_time);
 int Cashe_State(unsigned char state);
 int Set_Mag_Offest_Scale(unsigned char* config_data);
 int Set_Mag_Mount(unsigned char * config_data);
 int Set_Mag_Sense(unsigned char * config_data);

#endif /* STATEMACHINE_H_ */




