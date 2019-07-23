/*
 * commands.h
 *
 *  Created on: Dec 5, 2018
 *      Author: elain
 */

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <satellite-subsystems/IsisTRXVU.h>
#include <satellite-subsystems/IsisAntS.h>
#include <satellite-subsystems/GomEPS.h>
#include <satellite-subsystems/SCS_Gecko/gecko_use_cases.h>
#include <satellite-subsystems/SCS_Gecko/gecko_driver.h>
#include <satellite-subsystems/cspaceADCS.h>
#include <satellite-subsystems/cspaceADCS_types.h>
#include <hal/boolean.h>
#include <hal/errors.h>

#include "../COMM/GSC.h"
#include "../Global/sizes.h"
#include "../Global/Global.h"
#include "../Global/TM_managment.h"

//ADCS
#define ADCS_INDEX 1
#define COMMAND_LIST_SIZE 20

/**
 *@
 */
int init_command();
int add_command(TC_spl command);
int get_command(TC_spl* command);

void act_upon_command(TC_spl decode);


void AUC_COMM(TC_spl decode);

void AUC_general(TC_spl decode);

void AUC_payload(TC_spl decode);

void AUC_EPS(TC_spl decode);

void AUC_ADCS(TC_spl decode);

void AUC_GS(TC_spl decode);

void AUC_SW(TC_spl decode);

void AUC_special_operation(TC_spl decode);


void cmd_error(Ack_type* type, ERR_type* err);

void cmd_soft_reset_cmponent(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_reset_satellite(Ack_type* type, ERR_type* err);

void cmd_gracefull_reset_satellite(Ack_type* type, ERR_type* err);

void cmd_hard_reset_cmponent(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_mute(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_unmute(Ack_type* type, ERR_type* err);

void cmd_active_trans(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_shut_trans(Ack_type* type, ERR_type* err);

void cmd_change_trans_rssi(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_aprs_dump(Ack_type* type, ERR_type* err);

void cmd_stop_dump(Ack_type* type, ERR_type* err);

void cmd_time_frequency(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_upload_time(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_generic_I2C(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_dump(TC_spl cmd);

void cmd_image_dump(TC_spl cmd);

void cmd_delete_TM(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_dummy_func(Ack_type* type, ERR_type* err);

void cmd_ARM_DIARM(Ack_type* type, ERR_type* err, TC_spl cmd);

void cmd_deploy_ants(Ack_type* type, ERR_type* err);

void cmd_reset_APRS_list(Ack_type* type, ERR_type* err);

void cmd_reset_delayed_command_list(Ack_type* type, ERR_type* err);

void cmd_reset_FRAM(Ack_type* type, ERR_type* err, TC_spl cmd);
#endif /* COMMANDS_H_ */
