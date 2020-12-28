/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  main.c - program logic

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <psppower.h>
#include <pspimpose_driver.h>

#include "common.h"
#include "debug.h"
#include "utils.h"
#include "library/danzeff.h"
#include "config.h"
#include "osk.h"
#include "graphic.h"
#include "menu.h"
#include "usb.h"
#include "wlan.h"
#include "compress.h"
#include "msgdlg.h"
#include "kernel/kernel.h"



// Set to true by the exit callback
bool g_exitGame = false;
bool g_menuThreadRunning = false;

SceUID l_menuThreadId;

PSP_MODULE_INFO("pspdisp", 0, 1, 1); 
PSP_HEAP_SIZE_KB(18000);



/*
  exit_callback
  ---------------------------------------------------
  Callback function for quitting the game.
  ---------------------------------------------------
  Returns 0.
*/
int exit_callback(int arg1, int arg2, void *common)
{
  g_exitGame = true;
  configSaveConfiguration();
  sceKernelExitGame();
  return 0;
}



/*
  power_callback
  ---------------------------------------------------
  Callback function for suspending/resuming.
  Not used at the moment.
  ---------------------------------------------------
  Returns 0.
*/
int power_callback(int unknown, int pwrflags, void *common)
{
  if(pwrflags & PSP_POWER_CB_RESUME_COMPLETE)
  {
    // Returned from suspend
  }
  if (pwrflags & PSP_POWER_CB_POWER_SWITCH)
  {
    // About to suspend
  }

  return 0;
}



/*
  callBackThread
  ---------------------------------------------------
  Register all needed callback functions.
  ---------------------------------------------------
  Returns 0.
*/
int callbackThread(SceSize args, void *argp)
{
  int cbid;

//  cbid = sceKernelCreateCallback("Power Callback", power_callback, NULL);
//  scePowerRegisterCallback(0, cbid);

  // Only register the exit callback if the kernel mode prx is not available.
  // This usually only happens when run through HBL.
  if (!g_utilsKernelFunctionsAvailable)
  {
    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);

	sceKernelSleepThreadCB();
  }

  return 0;
}




/*
  setupCallbacks
  ---------------------------------------------------
  Set up the callback thread.
  ---------------------------------------------------
  Returns the callback thread id.
*/
SceUID setupCallbacks(void)
{
  SceUID thid = sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, THREAD_ATTR_USER, 0);
  if(thid > -1)
  {
    sceKernelStartThread(thid, 0, 0);
  }

  return thid;
}



/*
  enterComLoop
  ---------------------------------------------------
  Prepare, enter and shut down the com funcions.
  ---------------------------------------------------
*/
int enterComLoop(int mode)
{
  DEBUG_PRINTF("Entering main com loop\n");
  g_comMode = mode;
  
  // Start the menu thread
  sceKernelStartThread(l_menuThreadId, 0, NULL);

  // Initialize audio functions
  audioInit();

  // Initialize the decompression functions
  compressClearBuffers();

  // Switch to the configured clock speed
  powerSetClockSpeedByCurrentMode();

  // ** Enter the requested mode
  if (g_comMode == COM_MODE_USB)
  {
    // USB mode
    usbEnterLoop();  
  }
  else
  {
    // WLAN mode
    netStartWlanConnection();
  }

  g_comMode = COM_MODE_NONE;

  // Reset the cpu clock
  powerSetDefaultClockSpeed();

  // Reset the compression buffers
  compressClearBuffers();

  // Shutdown the menu thread
  g_menuThreadRunning = false;
  sceKernelWaitThreadEnd(l_menuThreadId, NULL);

  // Shutdown the audio functions
  audioTerm();

  DEBUG_PRINTF("going back to the main menu\n");
  return MAIN_STATE_MENU;
}





/*
  menuThread
  ---------------------------------------------------
  Thread running besides the communication functions
  for handling the menu system.
  ---------------------------------------------------
*/
void menuThread(SceSize args, void *argp)
{
  DEBUG_PRINTF("menuThread entered\n");

  SceCtrlData ctrlData;
  int oldButtons = 0;

  g_menuActive = false;
  g_menuThreadRunning = true;

  while (g_menuThreadRunning)
  {
    if (utilsCtrlReadBufferPositive(&ctrlData, 1) != 1)
      DEBUG_PRINTF("menuThread: utilsCtrlReadBufferPositive error\n");

    if (!g_msgActive && (ctrlData.Buttons & PSP_CTRL_HOME) && !(oldButtons & PSP_CTRL_HOME))
    {
      DEBUG_PRINTF("menuThread: Opening menu, buttons = 0x%08lX, last buttons = 0x%08lX\n", (long)ctrlData.Buttons, (long)oldButtons);
      
      powerEnablePowerSaveMode(false);
      
      g_menuActive = true;
      DEBUG_PRINTF("menuThread: to menuLoop\n");
      int result = menuLoop(MENU_INGAME);
      DEBUG_PRINTF("menuThread: from menuLoop\n");
      if (result == MENU_QUIT)
      {
        g_comRunning = false;
        menuDrawPleaseWaitScreen();
        int i = 0;
        while (g_menuThreadRunning)
        {
          i++;
          sceKernelDelayThread(100 * 1000);
          // Probably the usb cable is still plugged in and blocking, just kill the usb connection
          if ((g_comMode == COM_MODE_USB) && (i == 20))
          {
            DEBUG_PRINTF("menuThread: now force deactivating USB\n");
            usbDeactivate();
            DEBUG_PRINTF("menuThread: USB deactivated\n");
          }
        }
      }
      else
      {
        usbDrawWaitingForDataMessage(false, false, true);
        
        // Read in buttons, so that already pressed buttons can be
        // ignored for remote controlling
        sceCtrlReadBufferPositive(&ctrlData, 1);
        g_menuButtonsOnExit = ctrlData.Buttons;

        g_menuActive = false;
        g_comForceScreenUpdate = true;
      }

      utilsCtrlReadBufferPositive(&ctrlData, 1);
    }

    oldButtons = ctrlData.Buttons;

    // Prevent standby and screen power off
    scePowerTick(PSP_POWER_TICK_ALL);
  }
  g_menuActive = false;
}




/*
  main
  ---------------------------------------------------
  Main function.
  ---------------------------------------------------
*/
int main(int argc, char *argv[])
{
  // Register callbacks
  setupCallbacks();

  // Disable the HOME exit screen, only effective if the exit callback is registered
  sceImposeSetHomePopup(0);

  // Initialize SIO debugging if DEBUG is set
  DEBUG_INIT

  // Init controls
  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

  // Enable some graphic stuff
  graphicInit();
  graphicInitFontLibrary();
  
  // Check if this is running on 1.5 kernel
  utilsCheckFirmwareVersion();

  // Load the kernel module
  utilsInitKernelModule();

  // Read config file
  configLoadConfiguration();

  // Initialize power functions
  powerInit();

  // Initialize decompression
  compressInit();

  // Initialize the danzeff keyboard
  oskInitDanzeff();

  l_menuThreadId = sceKernelCreateThread("menuThread", (SceKernelThreadEntry)menuThread, 0x18, 0x10000, PSP_THREAD_ATTR_USER, 0);    

  // Set misc variables
  g_msgActive = g_oskActive = g_netDialogActive = false;
  g_comMode = COM_MODE_NONE;

  // Enter main loop
  int mainState = MAIN_STATE_MENU;
  bool mainLoopRunning = true;

  DEBUG_PRINTF("Entering main loop\n");

  // Show autostart screen if necessary
  if (configAutoMode != 0)
    mainState = menuDrawAutostartScreen();

  while (mainLoopRunning)
  {
    DEBUG_PRINTF("mainloop\n")
    switch (mainState)
    {
      case MAIN_STATE_MENU:
        mainState = menuLoop(MENU_MAIN);
        break;
      case MAIN_STATE_USB:
        mainState = enterComLoop(COM_MODE_USB);
        break;
      case MAIN_STATE_WLAN:
        mainState = enterComLoop(COM_MODE_WLAN);
        break;
      case MENU_QUIT:
        mainLoopRunning = false;
        break;
    }
  }

  compressTerm();

  configSaveConfiguration();

  // Reenable the HOME exit screen
  sceImposeSetHomePopup(1);

  sceKernelExitGame();
  return 0;
}
