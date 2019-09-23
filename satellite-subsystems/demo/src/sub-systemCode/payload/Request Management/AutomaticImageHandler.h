/*
 * 	ImageAutomaticHandler.c
 *
 * 	Created on: Sep 19, 2019
 * 	Author: Roy Harel
 */

#ifndef IMAGEAUTOMATICHANDLER_H_
#define IMAGEAUTOMATICHANDLER_H_

Boolean8bit auto_thumbnail_creation;

void AutomaticImageHandlerTaskMain();

void KickStartAutomaticImageHandlerTask();

int stopAction();
int resumeAction();
void handleErrors(int error);

#endif /* IMAGEAUTOMATICHANDLER_H_ */
