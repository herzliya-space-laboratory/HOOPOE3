/*
 * General_CMD.h
 *
 *  Created on: Jun 22, 2019
 *      Author: Hoopoe3n
 */

#ifndef GENERAL_CMD_H_
#define GENERAL_CMD_H_

#include "../../COMM/GSC.h"
#include "../../Global/Global.h"

void cmd_delete_TM(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_reset_file(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_format_SD(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_dummy_func(Ack_type* type, ERR_type* err);

void cmd_generic_I2C(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_dump(TC_spl cmd);

void cmd_resume_TM(Ack_type* type, ERR_type* err);

void cmd_stop_TM(Ack_type* type, ERR_type* err);

void cmd_soft_reset_cmponent(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_reset_satellite(Ack_type* type, ERR_type* err);

void cmd_gracefull_reset_satellite(Ack_type* type, ERR_type* err);

void cmd_hard_reset_cmponent(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_reset_TLM_SD(Ack_type* type, ERR_type* err);

void cmd_upload_time(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_ARM_DIARM(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_deploy_ants(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_get_onlineTM(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_reset_off_line(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_add_item_off_line(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_delete_item_off_line(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_get_off_line_setting(Ack_type* type, ERR_type* err, TC_spl cmd);
#endif /* GENERAL_CMD_H_ */
