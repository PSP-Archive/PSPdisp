/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  usb.c - functions for usb connectivity

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "usb.h"

#include "com.h"
#include "msgdlg.h"


int l_currentUsbMode; // Currently used usb mode
SceUID l_usbhostfsModuleId = 0; // ID of the usbhostfs prx



/*
  usbEnterLoop
  ---------------------------------------------------
  Initialize the usb connection.
  ---------------------------------------------------
*/
void usbEnterLoop()
{
  // Load modules, start driver
  int handle = usbInit();

  if (handle < 0) 
  {
    // Usb initializaton failed, draw error message and return to the main menu
    char errorMessage[256];
    sprintf(errorMessage, "Could not start the USB driver\n(error 0x%08lX, code %d).\n\nPlease disable all USB related plugins and try again.", (long unsigned int)l_usbhostfsModuleId, handle);
    msgShowError(errorMessage, false, false);
  }
  else
  {
	  usbDrawWaitingForDataMessage(false, false, false);

	  // Start the power save idle thread
	  powerStartIdleThread();

	  // Go into the communication loop
	  comLoop();

	  // Stop the power save idle thread
	  powerTermIdleThread();
  }

  DEBUG_PRINTF("usbEnterLoop: now invoking usbTerm()\n");

  // Shutdown driver, unload modules
  // But only if the error was not caused by PSPLink
  if (handle != -5)
    usbTerm();
}




/*
  StartUSB
  ---------------------------------------------------
  Load and initialize the usb driver.
  ---------------------------------------------------
  Returns 0 on success, error code otherwise.
*/
int usbInit()
{
  if (l_usbhostfsModuleId == 0)
    l_usbhostfsModuleId = utilsLoadStartModule("usbhostfs.prx");
 
  DEBUG_PRINTF("usbInit: usbhostfsModuleId = 0x%08lX\n", (long)l_usbhostfsModuleId);

  if (l_usbhostfsModuleId < 0)
    return -1;

  if (sceUsbStart(PSP_USBBUS_DRIVERNAME, 0, 0) != 0)
    return -2;

  if (sceUsbStart(USB_HOSTFSDRIVER_NAME, 0, 0) != 0)
    return -3;

  l_currentUsbMode = (configUsbMode == 0) ? USB_HOSTFSDRIVER_PID_TYPE_B : USB_HOSTFSDRIVER_PID_TYPE_C;
  if (sceUsbActivate(l_currentUsbMode) != 0) 
    return -4;

  struct AsyncEndpoint endp;
  usbAsyncRegister(4, &endp);

  return 0;
}




/*
  usbTerm
  ---------------------------------------------------
  Stop and unload the usb driver.
  ---------------------------------------------------
*/
void usbTerm() 
{
  int result = 0;

  usbAsyncUnregister(4);

  DEBUG_PRINTF("usbTerm: enter\n");
  result = sceUsbDeactivate(l_currentUsbMode);

  DEBUG_PRINTF("usbTerm: sceUsbDeactive done 0x%08lX\n", (long unsigned int)result);

  result = sceUsbStop(USB_HOSTFSDRIVER_NAME, 0, 0);

  DEBUG_PRINTF("usbTerm: sceUsbStop 1 done 0x%08lX\n", (long unsigned int)result);

  result = sceUsbStop(PSP_USBBUS_DRIVERNAME, 0, 0);

  DEBUG_PRINTF("usbTerm: sceUsbStop 2 done 0x%08lX\n", (long unsigned int)result);

  result = utilsStopUnloadModule(l_usbhostfsModuleId);

  DEBUG_PRINTF("usbTerm: exit 0x%08lX\n", (long unsigned int)result);

  // Unloading is broken on 6.20 TN-C
  if (result != 0x80020136)
    l_usbhostfsModuleId = 0;
}




/*
  usbDeactivate
  ---------------------------------------------------
  Deactivate the usb driver, used to force a
  blocking usb transfer to return.
  ---------------------------------------------------
*/
void usbDeactivate()
{
  sceUsbDeactivate(l_currentUsbMode);
}




/*
  usbDrawWaitingForDataMessage
  ---------------------------------------------------
  Display the "Waiting for data..." overlay.
  ---------------------------------------------------
*/
void usbDrawWaitingForDataMessage(bool enteringPowerSaveMode, bool clearScreen, bool forceDrawing)
{
  if (g_menuActive && !forceDrawing)
    return;

  if (clearScreen)
    graphicClearScreen(0);
  else
    graphicRedrawLastFrame();

  graphicStartDrawing();

  intraFontSetStyle(sansSerifLarge, 1.0f, COLOR_WHITE, COLOR_BLACK, INTRAFONT_ALIGN_CENTER);
  intraFontPrint(sansSerifLarge, 240, 125, "Waiting for data...");

  if (enteringPowerSaveMode)
  {
    intraFontSetStyle(sansSerifSmall, 1.0f, COLOR_WHITE, COLOR_BLACK, INTRAFONT_ALIGN_CENTER);
    intraFontPrint(sansSerifSmall, 240, 200, "Entering power save mode in 10 seconds.");
  }

  graphicFinishDrawing(true);
}
