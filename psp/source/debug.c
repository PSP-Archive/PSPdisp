/*
  ***************************************************
  PSPdisp (c) 2008 - 2009 Jochen Schleu

  debug.c - sio debugging

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "debug.h"

bool l_sioAvailable = 0;


/*
  debugInit
  ---------------------------------------------------
  Initialize the serial debugging output.
  ---------------------------------------------------
*/
void debugInit()
{
  pspDebugScreenInit();
  pspDebugScreenPrintf("DEBUG MODE\n\n");

  if (DEBUG_SIO)
  {
    pspDebugScreenPrintf("SIO OUTPUT ENABLED\n\n");

    SceUID result;

    if (l_sioAvailable)
      result = 1;
    else
      result = utilsLoadStartModule("sio.prx");

    if (result < 0)
    {
      l_sioAvailable = false;
      pspDebugScreenPrintf("ERROR: Couldn't start 'sio.prx'.\n");
      sceKernelDelayThread(1000 * 1000 * 5);
    }
    else
    {
      l_sioAvailable = true;
      kernelDebugSioInit(115200);
      pspDebugScreenPrintf("SIO initialized\n");
      printf("Ready\n");
    }
  }
  
  if (DEBUG_FILE)
  {
    pspDebugScreenPrintf("FILE OUTPUT ENABLED\n\n");

	__debug_file_handle = fopen(DEBUG_FILE_NAME, "wb");
  }
}
