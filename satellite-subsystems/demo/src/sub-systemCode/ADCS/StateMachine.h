
#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

#include <hal/boolean.h>
#include "AdcsTroubleShooting.h"
#include "sub-systemCode/COMM/GSC.h"

/*!
 * @this enum continue all the states
 * @each number respond state
 */

typedef enum
{
	MANUAL_MODE,
	AUTONMOUS_MODE
}OperationMode;

typedef enum
 {
	NO_CONTROL = 0,


	NUM_OF_CONTROL_MODES
 }AdcsControlModes;

typedef enum
{
	NO_ESTIMATION,

	NUM_OF_ESTIMATION_MODES
}AdcsEstimationModes;


/*! @brief init the State Machine
 *  the state machine is the part in the code which active the ADCS system.
 *  @return	Errors according to TroubleErrCode enum
 */
 TroubleErrCode InitStateMachine();

/*!
 * @brief Update ADCS State Machine
 * @param[in] decode the command packet
 * @param[in] Mode Operation mode of the system - Manual or Autonomous
 * @return	Errors according to TroubleErrCode enum
 */
 TroubleErrCode UpdateAdcsStateMachine();


#endif /* STATEMACHINE_H_ */




