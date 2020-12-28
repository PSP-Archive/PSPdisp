/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  kernel.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef KERNEL_H
#define KERNEL_H

#include <pspsdk.h> 
#include <pspkernel.h>
#include <pspdisplay_kernel.h>
#include <pspctrl.h>

void kernelSetWlanLedState(int enabled);
void kernelDisplaySetBrightness(int level);
int kernelDisplayGetBrightness();
int kernelCtrlReadBufferPositive(SceCtrlData *pad_data, int count);
int kernelGetModel();

#endif