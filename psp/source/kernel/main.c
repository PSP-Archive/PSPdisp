/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  kernel.c - kernel mode functions

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "kernel.h"


PSP_MODULE_INFO("kernel", PSP_MODULE_KERNEL, 1, 1);
PSP_MAIN_THREAD_ATTR(0);


// http://forums.ps2dev.org/viewtopic.php?t=9485
void kernelSetWlanLedState(int enabled)
{
	u32 k1;
	k1 = pspSdkSetK1(0);

	sceSysconCtrlLED(1, enabled); 

	pspSdkSetK1(k1);
}




void kernelDisplaySetBrightness(int level)
{
  u32 k1;
  k1 = pspSdkSetK1(0);

  sceDisplaySetBrightness(level, 0);

  pspSdkSetK1(k1);
}




int kernelDisplayGetBrightness()
{
  u32 k1;
  k1 = pspSdkSetK1(0);

  int brightness = 0;
  int dummy = 0;
  sceDisplayGetBrightness(&brightness, &dummy);

  pspSdkSetK1(k1);
  return brightness;
}



int kernelCtrlReadBufferPositive(SceCtrlData *pad_data, int count)
{
  u32 k1;
  k1 = pspSdkSetK1(0);

  int result = sceCtrlReadBufferPositive(pad_data, count);

  pspSdkSetK1(k1);
  return result;
}


int kernelGetModel()
{
  u32 k1;
  k1 = pspSdkSetK1(0);
  
  int result = sceKernelGetModel();

  pspSdkSetK1(k1);
  return result;
}



int module_start(SceSize args, void *argp)
{
  return 0;
}



int module_stop(SceSize args, void *argp)
{
  return 0;
}
