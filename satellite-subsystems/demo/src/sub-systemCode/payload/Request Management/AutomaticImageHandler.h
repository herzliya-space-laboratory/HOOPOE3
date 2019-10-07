/*
 * 	ImageAutomaticHandler.c
 *
 * 	Created on: Sep 19, 2019
 * 	Author: Roy Harel
 */

#ifndef IMAGEAUTOMATICHANDLER_H_
#define IMAGEAUTOMATICHANDLER_H_

void create_xAIHS();

void AutomaticImageHandlerTaskMain();

void KickStartAutomaticImageHandlerTask();

int stopAction();
int resumeAction();
void handleErrors(int error);

Boolean get_automatic_image_handling_state();

#endif /* IMAGEAUTOMATICHANDLER_H_ */
