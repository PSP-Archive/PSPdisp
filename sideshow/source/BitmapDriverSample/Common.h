//-----------------------------------------------------------------------
// <copyright file="Common.h" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
//
// Module:
//      Common.h
//
// Description:
//
//-----------------------------------------------------------------------


#pragma once

#include <windows.h>

#include <atlbase.h>
extern CComModule _Module; // Required by ATLCOM.h
#include <atlcom.h>
#include <atlimage.h>
#include <atltime.h>

// One forward-declare that most people are going to need to know about
class CWSSDevice;
typedef CComObject<CWSSDevice> *WSSDevicePtr;

// The following are custom WPD commands that can be sent to the driver
// Base GUID: 6BDE5134-3988-498A-98BA-841607FECBE7
const PROPERTYKEY CUSTOM_COMMAND_INVERT_COLORS             = {0x6BDE5134, 0x3988, 0x498A, 0x98, 0xBA, 0x84, 0x16, 0x07, 0xFE, 0xCB, 0xE7, 1};
const PROPERTYKEY CUSTOM_COMMAND_INVERT_COLORS_SETVALUE    = {0x6BDE5134, 0x3988, 0x498A, 0x98, 0xBA, 0x84, 0x16, 0x07, 0xFE, 0xCB, 0xE7, 2}; // [VT_UI4]
const PROPERTYKEY CUSTOM_COMMAND_CHANGE_SOMETHING          = {0x6BDE5134, 0x3988, 0x498A, 0x98, 0xBA, 0x84, 0x16, 0x07, 0xFE, 0xCB, 0xE7, 3};
const PROPERTYKEY CUSTOM_COMMAND_CHANGE_SOMETHING_SETVALUE = {0x6BDE5134, 0x3988, 0x498A, 0x98, 0xBA, 0x84, 0x16, 0x07, 0xFE, 0xCB, 0xE7, 4}; // [VT_UI4]

#include <msxml2.h>

#include <gdiplus.h>
using namespace Gdiplus;

#include <strsafe.h>
#include <intsafe.h>

#pragma warning(push)
#pragma warning(disable: 4995)
#include <string>
#include <vector>
#include <list>
#include <map>
#include <stack>
using namespace std;
#pragma warning(pop)

#include <WindowsSideShowClassExtension.h>
#include <WindowsSideShow.h>
#include <WindowsSideShowDriverEvents.h>

//#define _ASSIGNED_USER_MODEL

#include "resource.h"

#include "Util.h"
#include "Icon.h"

#include "Device.h"
#include "Renderer.h"

#include "ScfElement.h"
#include "ContentViewItem.h"
#include "ContentView.h"
#include "Content.h"
#include "Node.h"
#include "Timer.h"
#include "User.h"

#include "ScfParser.h"

#include "BasicDDI.h"
#include "DataManager.h"
#include "DeviceSpecific.h"
#include "WSSDevice.h"

// Global
extern CDataManager *g_pDataManager;
//

//
// using 13 for SCF_BUTTON_HOME
// #9331 Bitmap Renderer: unable to display mixed text content page
//
#define SCF_BUTTON_HOME 13

