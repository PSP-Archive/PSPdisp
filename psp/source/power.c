/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  power.c - power save mode related functions

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "power.h"

#include "audio.h"
#include "config.h"
#include "menu.h"
#include "com.h"
#include "msgdlg.h"


#define POWER_IDLE_TIMEOUT_SCREEN_OFF 10


int l_previousCpuClock; // CPU clock before going into power save mode
int l_previousBusClock; // Bus clock before going into power save mode

int l_defaultCpuClock; // CPU clock on startup
int l_defaultBusClock; // Bus clock on startup

bool l_powerSaveModeEnabled; // Are we in power save mode?
int l_previousBrightness; // Display brightness before power save mode
int l_loweredBrightness; // Display brightness in power save mode

SceUID l_powerSaveThreadId;
bool l_runPowerSaveThread;


/*
  powerSetBrightness
  ---------------------------------------------------
  Set display brightness.
  ---------------------------------------------------
*/
void powerSetBrightness(int brightness)
{
  if (g_utilsKernelFunctionsAvailable)
    kernelDisplaySetBrightness(brightness);
}




/*
  powerGetBrightness
  ---------------------------------------------------
  Retrieve current display brightness.
  ---------------------------------------------------
  Returns the brightness (0-100)
*/
int powerGetBrightness()
{
  if (g_utilsKernelFunctionsAvailable)
    return kernelDisplayGetBrightness();
  else
    return 100;
}




/*
  powerSetClockSpeedByCurrentMode
  ---------------------------------------------------
  Change the PSP cpu and bus clock according to
  the communication mode.
  ---------------------------------------------------
  Returns result of powerSetClockSpeed().
*/
bool powerSetClockSpeedByCurrentMode()
{
  switch (g_comMode)
  {
    case COM_MODE_USB:
      return powerSetClockSpeed(configUsbClockSpeed);
    case COM_MODE_WLAN:
      return powerSetClockSpeed(configWlanClockSpeed);
    case COM_MODE_NONE:
    default:    
      return powerSetClockSpeed(0);
  }
}




/*
  powerSetClockSpeed
  ---------------------------------------------------
  Change the PSP cpu and bus clock.
  ---------------------------------------------------
  Returns true.
*/
bool powerSetClockSpeed(int speedOption)
{
//  int cpuClock = scePowerGetCpuClockFrequencyInt();
//  int busClock = scePowerGetBusClockFrequencyInt();

  DEBUG_PRINTF("Setting clockspeed to option: %d\n", speedOption);

  switch (speedOption)
  {
    case 0:
      scePowerSetClockFrequency(l_defaultCpuClock, l_defaultCpuClock, l_defaultBusClock);
      break;
    case 1:
      scePowerSetClockFrequency(33, 33, 16);
      break;
    case 2:
      scePowerSetClockFrequency(66, 66, 33);
      break;
    case 3:
      scePowerSetClockFrequency(111, 111, 55);
      break;
    case 4: 
      scePowerSetClockFrequency(222, 222, 111);
      break;
    case 5: 
      scePowerSetClockFrequency(333, 333, 166);
      break;
  }

  return true;
}





/*
  powerInit
  ---------------------------------------------------
  Initialize power functions, must be called on
  program start.
  ---------------------------------------------------
*/
void powerInit()
{
  // Get default CPU and bus speed
  l_defaultCpuClock = 222;//scePowerGetCpuClockFrequencyInt();
  l_defaultBusClock = 111;//scePowerGetBusClockFrequencyInt();

  l_powerSaveModeEnabled = false;

  // Lower brightness is depending on the PSP model
  if (g_utilsKernelFunctionsAvailable)
  {
    switch (kernelGetModel())
    {
      case 2: // 3000 03g
      case 3: // 3000 04g
      case 6: // 3000 07g
      case 8: // 3000 09g
        l_loweredBrightness = 16;
        break;

      case 4: // GO
        l_loweredBrightness = 24;
        break;

      case 0: // 1000
      case 1: // 2000
      default:
        l_loweredBrightness = 8;
        break;
    }
  }
  else
    l_loweredBrightness = 8;

  // Set brightness
  if (configAutosetBrightness)
    powerSetBrightness(configBrightness);
}




/*
  powerSetDefaultClockSpeeds
  ---------------------------------------------------
  Set the clock speeds according to the config file.
  ---------------------------------------------------
*/
void powerSetDefaultClockSpeed()
{
  scePowerSetClockFrequency(l_defaultCpuClock, l_defaultCpuClock, l_defaultBusClock);
}




/*
  powerEnablePowerSaveMode
  ---------------------------------------------------
  Start and stop the power saving mode.
  ---------------------------------------------------
*/
void powerEnablePowerSaveMode(bool savePower)
{
  if (savePower)
  {
    if (!l_powerSaveModeEnabled)
    {
      audioTerm();

      if (configUsbSaveClearScreen)
      {
        // Clear screen
        graphicClearScreen(0);
        graphicSwitchBuffers();
      }

      if (configUsbSaveDownClock)
      {
        l_previousCpuClock = scePowerGetCpuClockFrequencyInt();
        l_previousBusClock = scePowerGetBusClockFrequencyInt();

        // More conservative setting now, since the 2 Mhz caused problems
        // with waking up
		    scePowerSetClockFrequency(20, 20, 10);
/*
        scePowerSetClockFrequency(19, 2, 2);

        int cpuClock = scePowerGetCpuClockFrequencyInt();
        int busClock = scePowerGetBusClockFrequencyInt();

        if (!((cpuClock <= 2) && (busClock <= 95)))
        {
          char buffer[256];
          memset(&buffer, 0, 256);
          sprintf(buffer, "Could not reduce clockspeed, current cpu=%d and bus=%d", cpuClock, busClock);
          msgShowError(buffer, false, false);
        }
*/      
      }

      l_previousBrightness = kernelDisplayGetBrightness();

      if (configUsbSaveScreenBacklight == 2)
      {
        // Screen and backlight off
        sceGuDisplay(0);
        kernelDisplaySetBrightness(0);
      }
      else if (configUsbSaveScreenBacklight == 1)
      {
        // Minimum brightness
		  kernelDisplaySetBrightness(l_loweredBrightness);
      }

      l_powerSaveModeEnabled = 1;
    }
  }
  else
  {
    if (l_powerSaveModeEnabled)
    {
      powerSetDefaultClockSpeed();
      
      audioInit();

      sceGuDisplay(1);
      kernelDisplaySetBrightness(l_previousBrightness);
      l_powerSaveModeEnabled = false;
    }
    g_idleCounter = 0;
  }
}



/*
  powerStartIdleThread
  ---------------------------------------------------
  Start the idle thread, enabling power saving.
  ---------------------------------------------------
*/

void powerStartIdleThread()
{
  DEBUG_PRINTF("entering powerStartIdleThread()\n");

  l_runPowerSaveThread = true;

  l_powerSaveThreadId = sceKernelCreateThread("powerSaveThread", (SceKernelThreadEntry)powerIdleThread, 0x18, 0x10000, PSP_THREAD_ATTR_USER, 0);    
  sceKernelStartThread(l_powerSaveThreadId, 0, 0);

  DEBUG_PRINTF("exiting powerStartIdleThread()\n");
}




/*
  powerTermIdleThread
  ---------------------------------------------------
  Stop the idle thread, disabling power saving.
  ---------------------------------------------------
*/
void powerTermIdleThread()
{
  DEBUG_PRINTF("entering powerTermIdleThread()\n");

  l_runPowerSaveThread = false;

  sceKernelWaitThreadEnd(l_powerSaveThreadId, NULL);
  sceKernelDeleteThread(l_powerSaveThreadId);

  DEBUG_PRINTF("exiting powerTermIdleThread()\n");
}



/*
  powerIdleThread
  ---------------------------------------------------
  Thread for entering power saving mode on idle.
  ---------------------------------------------------
*/
void powerIdleThread(SceSize args, void *argp)
{
  g_idleCounter = 0;

  while (l_runPowerSaveThread)
  {
    // No power saving in the menu
    if (g_menuActive)
      g_idleCounter = 0;

    if ((g_comMode == COM_MODE_USB) && (configUsbSave))
    {
      if (g_idleCounter == configUsbSaveTimeout)
      {
        // Idle timer reached, go into pwer save mode
        powerEnablePowerSaveMode(true);

        g_idleCounter = configUsbSaveTimeout + 1;
      }
      else if (g_idleCounter > configUsbSaveTimeout)
      {
        // In power save mode, watch for a keypress to
        SceCtrlData controlData;
        sceCtrlPeekBufferPositive(&controlData, 1);

        if (controlData.Buttons)
        {
          powerEnablePowerSaveMode(false);
          g_idleCounter = 0;
        }
      }
    }

    // Hack to catch synchronization problems
    if (g_idleCounter > 1000)
      g_idleCounter = 0;

    g_idleCounter++;

    // Wait a second
    sceKernelDelayThread(1000 * 1000);
  }
}

