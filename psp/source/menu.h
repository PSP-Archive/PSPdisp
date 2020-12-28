/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  menu.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef MENU_H
#define MENU_H

#include <pspsdk.h>
#include <sys/types.h>
#include <psprtc.h>
#include <math.h>
#include "common.h"
#include "utils.h"
#include "wlan.h"
#include "config.h"
#include "osk.h"
#include "power.h"



// How many options are display at once
#define MENU_MAX_OPTIONS_ON_SCREEN 6

// Where to start printing the menu options and spacing inbetween
#define MENU_OFFSET_OPTION_X 230
#define MENU_OFFSET_OPTION_Y 70
#define MENU_OFFSET_OPTION_INCREMENT 24

// Available input methods for the menu option
#define MENU_INPUT_LR_SELECT 1
#define MENU_INPUT_OSK 2
#define MENU_INPUT_CLICK 4

// Menu IDs
#define MENU_MAIN 0
#define MENU_WLAN 1
#define MENU_USB 2
#define MENU_USB_POWER_SAVING 3
#define MENU_AUTOSTART 4
#define MENU_WLAN_IP 5
#define MENU_PC_MISC 6
#define MENU_PC_DISPLAY 7
#define MENU_PC_SCREEN 8
#define MENU_INGAME 9
#define MENU_GENERAL 10
#define MENU_WLAN_MAC 11

#define MENU_PROHIBIT_CHANGE 0
#define MENU_STAY 1000
#define MENU_QUIT 2000
#define MENU_NONE 3000

// Button glyphs
#define MENU_GLYPH_CROSS 0
#define MENU_GLYPH_CIRCLE 1
#define MENU_GLYPH_TRIANGLE 2
#define MENU_GLYPH_SQUARE 3
#define MENU_GLYPH_LEFT_RIGHT 4
#define MENU_GLYPH_UP_DOWN 5
#define MENU_GLYPH_SHOULDER 6
#define MENU_GLYPH_BATTERY0 7
#define MENU_GLYPH_BATTERY1 8
#define MENU_GLYPH_BATTERY2 9
#define MENU_GLYPH_BATTERY3 10


// What kind of value the menu option represents
#define TYPE_INTEGER 0
#define TYPE_BOOL 1
#define TYPE_STRING 2
#define TYPE_INTEGER_NAMED 3
#define TYPE_INTEGER_WITH_UNIT 4
#define TYPE_BUTTON 5
#define TYPE_STRING_SELECTION 6

// Callback reason
#define CALLBACK_BEFORE_VALUE_CHANGE 0
#define CALLBACK_VALUE_CHANGED 1
#define CALLBACK_BUTTON_PRESSED 2
#define CALLBACK_RESTORE_DEFAULT 3
#define CALLBACK_DRAW 4


typedef int(*menuOptionCallback)(int, void*);  


typedef struct
{
  int menuId;
  char* caption;
  int* value;
  int defaultValue;
  int minimum;
  int maximum;
  int increment;
  int settingType;
  char** valueArray;
  int inputMethods;
  menuOptionCallback callback;
} menuEntry;


typedef struct
{
  int menuId;
  char* caption;
  char* pageNumber;
  menuEntry* menuEntries;
  int entryCount;
  int previousMenuId;
  int nextMenuId;
} menuProperties;


typedef struct
{
  int x;
  int y;
  menuEntry* menuOption;
  bool processed;
} menuCallbackDrawStruct;


bool g_menuActive; // Is the menu being displayed?
bool g_menuForceRedraw;
unsigned int g_menuButtonsOnExit;


void drawMenu(menuProperties* menuProperty, int selectionIndex);
int menuLoop(int startMenuId);
menuProperties* getMenuProperty(int menuId);
void drawCaption(menuProperties* menuProperty, int selectionIndex);
void drawOption(menuProperties* menuProperty, int optionIndex, int selectionIndex);
void drawStatus(menuProperties* menuProperty, int selectionIndex);
bool increaseValue(menuEntry* menuOption);
bool decreaseValue(menuEntry* menuOption);
int buttonGotPressed(unsigned int buttonCode, unsigned int currentButtons, unsigned int previousButtons);
void drawGlyph(int glyph, int x, int y);
void menuDrawPleaseWaitScreen();
int menuDrawAutostartScreen();

int menuBrightnessCallback(int reason, void* data);
int menuStartWlanModeCallback(int reason, void* data);
int menuStartUsbModeCallback(int reason, void* data);
int menuResumeCallback(int reason, void* data);
int menuSettingsCallback(int reason, void* data);
int menuQuitGameCallback(int reason, void* data);
int menuIpAddressCallback(int reason, void* data);
int menuMacAddressCallback(int reason, void* data);
int menuPcSettingsCallback(int reason, void* data);
int menuButtonConfigurationCallback(int reason, void* data);
int menuChangeCpu(int reason, void* data);
int menuToggleWlanLed(int reason, void* data);
int menuChangeButtonSwap(int reason, void* data);
int menuChangeTimeFormat(int reason, void* data);
int menuKeyboardCallback(int reason, void* data);
int menuPasswordCallback(int reason, void* data);
int menuPcSettingsChangeCallback(int reason, void* data);
int menuPcSettingsDeviceCallback(int reason, void* data);
int menuPcSettingsControlCallback(int reason, void* data);
int menuPcSettingsPresetCallback(int reason, void* data);

#endif