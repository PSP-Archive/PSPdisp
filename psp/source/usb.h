/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  usb.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef USB_H
#define USB_H


#include <pspsdk.h>
#include <pspkernel.h>
#include <pspusb.h>
#include "usbhostfs/usbasync.h"
#include "common.h"
#include "utils.h"
#include "config.h"
#include "graphic.h"
#include "library/intrafont.h"


#define USB_HOSTFSDRIVER_NAME "USBHostFSDriver"
#define USB_HOSTFSDRIVER_PID_TYPE_B  (0x1C9)
#define USB_HOSTFSDRIVER_PID_TYPE_C  (0x1CA)


void usbEnterLoop();
int usbInit();
void usbTerm();
void usbDeactivate();
void usbDrawWaitingForDataMessage(bool enteringPowerSaveMode, bool clearScreen, bool forceDrawing);


#endif