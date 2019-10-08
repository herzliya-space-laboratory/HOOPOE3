/*
 * 	ImageAutomaticHandler.c
 *
 * 	Created on: Sep 19, 2019
 * 	Author: Roy Harel
 */

#ifndef IMAGEAUTOMATICHANDLER_H_
#define IMAGEAUTOMATICHANDLER_H_


void AutomaticImageHandlerTaskMain();

void KickStartAutomaticImageHandlerTask();
void initAutomaticImageHandlerTask();

Boolean get_automatic_image_handling_task_suspension_flag();
void set_automatic_image_handling_task_suspension_flag(Boolean param);

Boolean get_automatic_image_handling_ready_for_long_term_stop();
void set_automatic_image_handling_ready_for_long_term_stop(Boolean param);

int stopAction();
int resumeAction();

#endif /* IMAGEAUTOMATICHANDLER_H_ */
