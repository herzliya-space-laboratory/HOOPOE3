#ifndef ADCSTROUBLESHOOTING_H_
#define ADCSTROUBLESHOOTING_H_

//the err code for the trouble shooting
typedef enum TroubleErrCode
{
	TRBL_NO_ERROR,
	TRBL_BOOT_STAK,
	TRBL_CHANL_OFF,
	TRBL_CMD_ERR,
	ERR_WRONG_SUB_TYPE,
	ERR_WRONG_TYPE,
	ERR_NULL_DATA

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
