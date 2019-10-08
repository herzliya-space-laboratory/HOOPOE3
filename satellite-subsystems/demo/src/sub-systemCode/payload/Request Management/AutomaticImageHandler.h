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

void create_xAIHS();
Boolean get_automatic_image_handling_state();
int stopAction();
int resumeAction();

#endif /* IMAGEAUTOMATICHANDLER_H_ */
