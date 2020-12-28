/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  sio.c - kernel mode functions for serial I/O

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include <pspsdk.h> 
#include <pspkernel.h>



PSP_MODULE_INFO("sio", PSP_MODULE_KERNEL, 1, 1);
PSP_MAIN_THREAD_ATTR(0);



void kernelDebugSioInit(int baudrate)
{
  u32 k1;
  k1 = pspSdkSetK1(0); 
  pspDebugSioInit();
  pspDebugSioSetBaud(baudrate);
//  pspDebugSioInstallKprintf();
//  pspDebugInstallStdoutHandler(NULL);
  pspDebugInstallStdoutHandler(pspDebugSioPutData);
  pspSdkSetK1(k1);  
}


int kernelDebugSioTerm()
{
  u32 k1;
  k1 = pspSdkSetK1(0);
  pspDebugInstallStdoutHandler(NULL);
  //pspDebugSioDisableKprintf();
  pspSdkSetK1(k1);
}


int kernelDebugSioPutText(const char *data, int len)
{
  u32 k1;
  k1 = pspSdkSetK1(0); 
  int result = pspDebugSioPutText(data, len);
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
