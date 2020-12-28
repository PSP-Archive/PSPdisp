/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  common.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef COMMON_H
#define COMMON_H

#include <pspsdk.h>
#include <psptypes.h>


// Introduce a boolean type
#ifndef __cplusplus
#define true (1)
#define false (0)
typedef int bool;
#endif


// Basic states the PSP can be in
#define MAIN_STATE_MENU 0
#define MAIN_STATE_WLAN 1
#define MAIN_STATE_USB 2
#define MAIN_STATE_QUIT 3


// Prototypes for stuff that is implemented in main.c
int power_callback(int unknown, int pwrflags, void *common);
int exit_callback(int arg1, int arg2, void *common);
int callbackThread(SceSize args, void *argp);
int setupCallbacks(void);

int main(int argc, char *argv[]);

#endif









