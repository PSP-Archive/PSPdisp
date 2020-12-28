/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  utils.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef UTILS_H
#define UTILS_H


#include <pspsdk.h>
#include <pspkernel.h>
#include <pspsysmem.h>
#include <stdio.h>
#include <kubridge.h> // From the 4.01 M33 SDK
#include "common.h"


#define RGB_TO_ABGR(a) (0xFF000000 + ((a & 0x00FF0000) >> 16) + (a & 0x0000FF00) + ((a & 0x000000FF) << 16))
#define RGBA_TO_ABGR(a) (((a & 0x000000FF) << 24) + ((a & 0xFF000000) >> 24) + ((a & 0x00FF0000) >> 8) + ((a & 0x0000FF00) << 8))


// Kernel mode functions available?
bool g_utilsKernelFunctionsAvailable;


SceUID utilsLoadStartModule(char *path);
int utilsStopUnloadModule(SceUID modID);
unsigned int utilsGetUint32FromBuffer(void* buffer);
bool utilsCheckFirmwareVersion();
bool utilsInitKernelModule();

int utilsCtrlReadBufferPositive(SceCtrlData *pad_data, int count);


#endif