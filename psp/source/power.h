/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  power.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef POWER_H
#define POWER_H


#include <pspsdk.h>
#include <psppower.h>

#include "common.h"
#include "debug.h"
#include "kernel/kernel.h"


int g_idleCounter;


bool powerInitBrightnessModule();
int powerGetBrightness();
void powerSetBrightness(int brightness);
void powerSetDefaultClockSpeed();
void powerEnablePowerSaveMode(bool savePower);
void powerInit();
bool powerSetClockSpeedByCurrentMode();
bool powerSetClockSpeed(int speedOption);
void powerIdleThread(SceSize args, void *argp);
void powerEnablePowerSaveMode(bool savePower);
void powerStartIdleThread();
void powerTermIdleThread();


#endif