#ifndef ADCSTROUBLESHOOTING_H_
#define ADCSTROUBLESHOOTING_H_

//the err code for the trouble shooting
typedef enum TroubleErrCode
{
	TRBL_NO_ERROR,
	TRBL_BOOT_ERROR,
	TRBL_CHANL_OFF,
	TRBL_CMD_ERR,
	TRBL_WRONG_SUB_TYPE,
	TRBL_WRONG_TYPE,
	TRBL_NULL_DATA,
	TRBL_FS_WRITE_ERR,
	TRBL_FS_READ_ERR,
	TRBL_FRAM_WRITE_ERR,
	TRBL_FRAM_READ_ERR,
	TRBL_INPUT_PARAM_ERR
}TroubleErrCode;


/*!
 * @solve the ADCS SYSTEM problems by their spaspic err code
 *
 * get:
 * 		problems err codes
 * return:
 * 		log code todo:check what log get?
 */
int AdcsTroubleShooting(TroubleErrCode *trbl);

TroubleErrCode FindErr();

#endif /* ADCSTROUBLESHOOTING_H_ */
