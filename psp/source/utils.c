/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  utils.c - misc functions

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "utils.h"

#include "menu.h"
#include "msgdlg.h"


/*
  utilsLoadStartModule
  ---------------------------------------------------
  Load and start a module.
  ---------------------------------------------------
  Returns UID of module or kernel error code.
*/
SceUID utilsLoadStartModule(char *path) 
{ 
  SceUID moduleId;
  
  moduleId = pspSdkLoadStartModule(path, PSP_MEMORY_PARTITION_KERNEL);

  if (moduleId >= 0)
  {
    return moduleId;
  }
  else
  {
    // Loading failed (probably SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE)
    // Try the kubridge version instead
    // This should restore functionality on 3.71 M33 and 5.50 GEN-B

    moduleId = kuKernelLoadModule(path, 0, NULL);

    if (moduleId >= 0)
    { 
      int status;
      SceUID result = sceKernelStartModule(moduleId, 0, NULL, &status, NULL);
      if (result >= 0)
      {
        return moduleId;
      }
      else
      {
        return result;
      }
    }
    else
    {
      return moduleId;
    }
  }
}




/*
  utilsStopUnloadModule
  ---------------------------------------------------
  Stops a module and removes it from memory.
  ---------------------------------------------------
  Returns 0.
*/
int utilsStopUnloadModule(SceUID modID)
{ 
  int result = 0;
  int status;

  result = sceKernelStopModule(modID, 0, NULL, &status, NULL);
  if (result < 0)
    return result;

  return sceKernelUnloadModule(modID);
} 




/*
  utilsGetUint32FromBuffer
  ---------------------------------------------------
  Extract an unsigned int from a buffer.
  ---------------------------------------------------
  Returns the converted number.
*/
unsigned int utilsGetUint32FromBuffer(void* buffer)
{
  return *((unsigned int*)buffer);
}





/*
  utilsCheckFirmwareVersion
  ---------------------------------------------------
  Check if we are running under the 1.50 kernel.
  ---------------------------------------------------
  Returns true if the firmware has a higher number.
*/
bool utilsCheckFirmwareVersion()
{
  if (sceKernelDevkitVersion() == 0x01050001)
  {
    msgShowError("PSPdisp cannot run from the 1.50 kernel game folder.\n\nPlease move the game files to the newer kernel game folder or change the kernel mode for the \\PSP\\GAME folder in the recovery menu.", true, false);
    sceKernelExitGame();
    return false;
  }
  else
  {
    return true;
  }
}



/*
  utilsInitKernelModule
  ---------------------------------------------------
  Load the kernel mode prx for various kernel tasks.
  ---------------------------------------------------
  Returns false on error.
*/
bool utilsInitKernelModule()
{
  SceUID result = utilsLoadStartModule("kernel.prx");
  if (result < 0)
  {
    g_utilsKernelFunctionsAvailable = false;

/*
    char message[100];
    sprintf(message, "Cannot load or start module \"kernel.prx\" (error 0x%02lX).", (long unsigned int)result);
    msgShowError(message, true);
    sceKernelExitGame();
*/

    return false;
  }

  g_utilsKernelFunctionsAvailable = true;
  return true;
}



/*
  utilsCtrlReadBufferPositive
  ---------------------------------------------------
  Read the button state either in kernel or user mode
  depending on the availability of the kernel library.
  ---------------------------------------------------
  Returns sceCtrlReadBufferPositive() return code.
*/
int utilsCtrlReadBufferPositive(SceCtrlData *pad_data, int count)
{
  if (g_utilsKernelFunctionsAvailable)
    return kernelCtrlReadBufferPositive(pad_data, count);
  else
    return sceCtrlReadBufferPositive(pad_data, count);
}