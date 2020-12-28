/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  menu.c - menu system

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "menu.h"
#include "graphic.h"
#include "com.h"
#include "power.h"


// Colors
#define MENU_COLOR_CAPTION RGB_TO_ABGR(0xFFFFFF)
#define MENU_COLOR_CAPTION_HIGHLIGHTED RGB_TO_ABGR(0xFFFFFF)

#define MENU_COLOR_OPTION  RGB_TO_ABGR(0xdddddd)
#define MENU_COLOR_OPTION_HIGHLIGHTED RGB_TO_ABGR(0xffffff)

#define MENU_COLOR_BUTTON  RGB_TO_ABGR(0xdddddd)
#define MENU_COLOR_BUTTON_HIGHLIGHTED RGB_TO_ABGR(0x27f391)

#define MENU_COLOR_VALUE RGB_TO_ABGR(0xCCCCCC)
#define MENU_COLOR_VALUE_HIGHLIGHTED RGB_TO_ABGR(0x27f391)


// Buttonswap
int l_okButton;
int l_cancelButton;

// Time format
int l_timeFormat;

unsigned int l_previousMenuButtons = 0;


// Embedded button glyphs
extern unsigned char glyphCross_start[];
extern unsigned char glyphCircle_start[];
extern unsigned char glyphTriangle_start[];
extern unsigned char glyphSquare_start[];
extern unsigned char glyphLeftRight_start[];
extern unsigned char glyphUpDown_start[];
extern unsigned char glyphLeftRight_start[];
extern unsigned char glyphShoulder_start[];
extern unsigned char glyphBattery0_start[];
extern unsigned char glyphBattery1_start[];
extern unsigned char glyphBattery2_start[];
extern unsigned char glyphBattery3_start[];


// Menuentry options
char* optionsYesNo[] = {"NO", "YES"};
char* optionsOnOff[] = {"OFF", "ON"};
char* optionsUsbMode[] = {"libusb (PSP Type B)", "WinUSB (PSP Type C)"};
char* optionsClockSpeed[] = {"Default", "33 Mhz", "66 Mhz", "111 Mhz", "222 Mhz", "333 Mhz"}; 
char* optionsBrightness[] = {"OFF", "Lower brightness", "Turn off backlight"};
char* optionsAutostart[] = {"OFF", "Use WLAN", "Use USB"};
char* optionsScreenMode[] = {"Fit Screen", "Fit Screen (16:9)", "Follow Mouse (mode 1)", "Follow Mouse (mode 2)", "Static Viewport"};
char* optionsRotation[] = {"OFF", "90 degree", "180 degree", "270 degree"};
char* optionsOkCancelButton[] = {"System default", "X = OK and O = Cancel", "O = OK and X = Cancel"};
char* optionsQuality[] = {"Lowest", "Low", "Medium", "High", "Highest"};
char* optionsInterval[] = {"Continuous", "Shortest", "Short", "Medium", "Long", "Longest"};
char* optionsTimeFormat[] = {"System default", "12 hour", "24 hour"};
char* optionsViewport[] = {"480x272", "560x315", "640x360", "720x405", "800x450", "880x495", "960x544", "320x200",
                           "320x240", "640x400", "640x480", "800x600", "060x034", "120x068", "240x136"};
char* optionsSampleRate[] = {"11025 Hz", "22050 Hz", "44100 Hz"};


// Menuentry additional units
char* unitSeconds[] = {" seconds"};
char* unitPercent[] = {" %"};


// Main menu
menuEntry mainMenuSettings[] =
{
  { MENU_MAIN, "Start using USB mode", NULL, 0, 0, 0, 0, TYPE_BUTTON, NULL, MENU_INPUT_CLICK, menuStartUsbModeCallback },
  { MENU_MAIN, "Start using WLAN mode", NULL, 0, 0, 0, 0, TYPE_BUTTON, NULL, MENU_INPUT_CLICK, menuStartWlanModeCallback },
  { MENU_MAIN, "Change Settings", NULL, 0, 0, 0, 0, TYPE_BUTTON, NULL, MENU_INPUT_CLICK, menuSettingsCallback },
  { MENU_MAIN, "Quit", NULL, 0, 0, 0, 0, TYPE_BUTTON, NULL, MENU_INPUT_CLICK, menuQuitGameCallback }
};


// "Ingame" menu (while a communication mode is selected)
menuEntry inGameMenuSettings[] =
{
  { MENU_INGAME, "Resume", NULL, 0, 0, 0, 0, TYPE_BUTTON, NULL, MENU_INPUT_CLICK, menuResumeCallback },
  { MENU_INGAME, "Toggle Onscreen Keyboard", NULL, 0, 0, 0, 0, TYPE_BUTTON, NULL, MENU_INPUT_CLICK, menuKeyboardCallback },
  { MENU_INGAME, "Change Settings", NULL, 0, 0, 0, 0, TYPE_BUTTON, NULL, MENU_INPUT_CLICK, menuSettingsCallback },
  { MENU_INGAME, "Change PC Settings", NULL, 0, 0, 0, 0, TYPE_BUTTON, NULL, MENU_INPUT_CLICK, menuPcSettingsCallback },
  { MENU_INGAME, "Quit to Main menu", NULL, 0, 0, 0, 0, TYPE_BUTTON, NULL, MENU_INPUT_CLICK, menuQuitGameCallback }
};


// General settings
menuEntry generalSettings[] =
{
  { MENU_GENERAL, "Display brightness", &configBrightness, 100, 4, 100, 4, TYPE_INTEGER_WITH_UNIT, unitPercent, MENU_INPUT_LR_SELECT, menuBrightnessCallback },
  { MENU_GENERAL, "Set brightness on startup", &configAutosetBrightness, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, NULL },
  { MENU_GENERAL, "Button configuration", &configXOButtons, 0, 0, 2, 1, TYPE_INTEGER, NULL, MENU_INPUT_LR_SELECT, menuButtonConfigurationCallback },
  { MENU_GENERAL, "Time format", &configTimeFormat, 0, 0, 2, 1, TYPE_INTEGER_NAMED, optionsTimeFormat, MENU_INPUT_LR_SELECT, menuChangeTimeFormat },
  { MENU_GENERAL, "Extended battery status", &configVerboseBattery, 1, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, NULL },
  { MENU_GENERAL, "Transparent menu", &configTransparentMenu, 1, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, NULL }
};


// USB menu
menuEntry usbSettings[] =
{
  { MENU_USB, "Clock speed", &configUsbClockSpeed, 0, 0, 5, 1, TYPE_INTEGER_NAMED, optionsClockSpeed, MENU_INPUT_LR_SELECT, menuChangeCpu },
  { MENU_USB, "USB driver", &configUsbMode, 1, 0, 1, 1, TYPE_INTEGER_NAMED, optionsUsbMode, MENU_INPUT_LR_SELECT, NULL }
};


// USB Power save menu
menuEntry usbPowerSaveSettings[] =
{
  { MENU_USB_POWER_SAVING, "Enable power saving", &configUsbSave, 1, 0, 1, 1, TYPE_INTEGER_NAMED, optionsYesNo, MENU_INPUT_LR_SELECT, NULL },
  { MENU_USB_POWER_SAVING, "Idle timeout", &configUsbSaveTimeout, 5, 2, 3600, 1, TYPE_INTEGER_WITH_UNIT, unitSeconds, MENU_INPUT_LR_SELECT | MENU_INPUT_OSK, NULL },
  { MENU_USB_POWER_SAVING, "Reduce brightness", &configUsbSaveScreenBacklight, 1, 0, 2, 1, TYPE_INTEGER_NAMED, optionsBrightness, MENU_INPUT_LR_SELECT, NULL },
  { MENU_USB_POWER_SAVING, "Clear screen contents", &configUsbSaveClearScreen, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsYesNo, MENU_INPUT_LR_SELECT, NULL },
  { MENU_USB_POWER_SAVING, "Reduce clock speed", &configUsbSaveDownClock, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsYesNo, MENU_INPUT_LR_SELECT, NULL }
};


// WLAN menu
menuEntry wlanSettings[] = 
{
  { MENU_WLAN, "Clock speed", &configWlanClockSpeed, 0, 0, 5, 1, TYPE_INTEGER_NAMED, optionsClockSpeed, MENU_INPUT_LR_SELECT, menuChangeCpu },
  { MENU_WLAN, "Password", NULL, 0, 0, 30, 0, TYPE_STRING, (char**)configWlanPassword, MENU_INPUT_OSK, menuPasswordCallback },
  { MENU_WLAN, "LED indicator", &configWlanLedEnabled, 1, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, menuToggleWlanLed }
};


// WLAN IP address menu
menuEntry wlanIPSettings[] =
{
  { MENU_WLAN_IP, "Number to use", &configIpAddressIndex, 1, 1, 5, 1, TYPE_INTEGER, NULL, MENU_INPUT_LR_SELECT, NULL },
  { MENU_WLAN_IP, "Host IP address 1", NULL, 0, 0, 32, 0, TYPE_STRING, (char**)&configIpAddresses[0], MENU_INPUT_OSK, menuIpAddressCallback },
  { MENU_WLAN_IP, "Host IP address 2", NULL, 0, 0, 32, 0, TYPE_STRING, (char**)&configIpAddresses[1], MENU_INPUT_OSK, menuIpAddressCallback },
  { MENU_WLAN_IP, "Host IP address 3", NULL, 0, 0, 32, 0, TYPE_STRING, (char**)&configIpAddresses[2], MENU_INPUT_OSK, menuIpAddressCallback },
  { MENU_WLAN_IP, "Host IP address 4", NULL, 0, 0, 32, 0, TYPE_STRING, (char**)&configIpAddresses[3], MENU_INPUT_OSK, menuIpAddressCallback },
  { MENU_WLAN_IP, "Host IP address 5", NULL, 0, 0, 32, 0, TYPE_STRING, (char**)&configIpAddresses[4], MENU_INPUT_OSK, menuIpAddressCallback },
  { MENU_WLAN_IP, "Port offset (instance)", &configWlanPortOffset, 0, 0, 9, 1, TYPE_INTEGER, NULL, MENU_INPUT_LR_SELECT, NULL }
};


// WLAN Wake-On-Lan menu
menuEntry wlanWakeOnLanSettings[] =
{
  { MENU_WLAN_IP, "Use Wake-On-Lan", &configWlanWakeOnLanEnabled, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, NULL },
  { MENU_WLAN_IP, "Host MAC address 1", NULL, 0, 0, 17, 0, TYPE_STRING, (char**)&configMacAddresses[0], MENU_INPUT_OSK, menuMacAddressCallback },
  { MENU_WLAN_IP, "Host MAC address 2", NULL, 0, 0, 17, 0, TYPE_STRING, (char**)&configMacAddresses[1], MENU_INPUT_OSK, menuMacAddressCallback },
  { MENU_WLAN_IP, "Host MAC address 3", NULL, 0, 0, 17, 0, TYPE_STRING, (char**)&configMacAddresses[2], MENU_INPUT_OSK, menuMacAddressCallback },
  { MENU_WLAN_IP, "Host MAC address 4", NULL, 0, 0, 17, 0, TYPE_STRING, (char**)&configMacAddresses[3], MENU_INPUT_OSK, menuMacAddressCallback },
  { MENU_WLAN_IP, "Host MAC address 5", NULL, 0, 0, 17, 0, TYPE_STRING, (char**)&configMacAddresses[4], MENU_INPUT_OSK, menuMacAddressCallback },
  { MENU_WLAN_IP, "Use broadcasting", &configWlanWakeOnLanBroadcasting, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, NULL }
};


// Autostart menu
menuEntry autoStartSettings[] = 
{
  { MENU_AUTOSTART, "Autoselect mode on startup", &configAutoMode, 0, 0, 2, 1, TYPE_INTEGER_NAMED, optionsAutostart, MENU_INPUT_LR_SELECT, NULL },
  { MENU_AUTOSTART, "Timeout before autoselect", &configAutoTimeout, 10, 0, 3600, 1, TYPE_INTEGER_WITH_UNIT, unitSeconds, MENU_INPUT_LR_SELECT | MENU_INPUT_OSK, NULL }
};


// PC settings - Screen
menuEntry pcSettingsScreen[] =
{
  { MENU_PC_SCREEN, "Screen mode", (int*)&configPCSettings.settings.screenMode, 3, 0, 4, 1, TYPE_INTEGER_NAMED, optionsScreenMode, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback },
  { MENU_PC_SCREEN, "Viewport size", (int*)&configPCSettings.settings.screenViewport, 0, 0, 14, 1, TYPE_INTEGER_NAMED, optionsViewport, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback },
  { MENU_PC_SCREEN, "Rotation", (int*)&configPCSettings.settings.screenRotation, 0, 0, 3, 1, TYPE_INTEGER_NAMED, optionsRotation, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback },
  { MENU_PC_SCREEN, "Quality", (int*)&configPCSettings.settings.quality, 2, 0, 4, 1, TYPE_INTEGER_NAMED, optionsQuality, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback },
  { MENU_PC_SCREEN, "No compression", (int*)&configPCSettings.settings.screenNoCompression, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback },
  { MENU_PC_SCREEN, "Update interval", (int*)&configPCSettings.settings.updateInterval, 0, 0, 5, 1, TYPE_INTEGER_NAMED, optionsInterval, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback }
};


// PC settings - Display
menuEntry pcSettingsDisplay[] =
{
  { MENU_PC_DISPLAY, "Display streaming", (int*)&configPCSettings.settings.displayEnable, 1, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback },
  { MENU_PC_DISPLAY, "Display device", (int*)&configPCSettings.settings.displayDeviceIndex, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsViewport, MENU_INPUT_LR_SELECT, menuPcSettingsDeviceCallback },
  { MENU_PC_DISPLAY, "Autoselect display", (int*)&configPCSettings.settings.displayAutoselect, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback },
  { MENU_PC_DISPLAY, "SideShow mode", (int*)&configPCSettings.settings.deviceUseSideShow, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback }
};


// PC settings - Misc
menuEntry pcSettingsMisc[] =
{
  { MENU_PC_MISC, "Sound Streaming", (int*)&configPCSettings.settings.soundEnabled, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsOnOff, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback },
  { MENU_PC_MISC, "Samplerate", (int*)&configPCSettings.settings.soundSampleRate, 0, 0, 2, 1, TYPE_INTEGER_NAMED, optionsSampleRate, MENU_INPUT_LR_SELECT, menuPcSettingsChangeCallback },
  { MENU_PC_MISC, "Control file", (int*)&configPCSettings.settings.controlIndex, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsViewport, MENU_INPUT_LR_SELECT, menuPcSettingsControlCallback },
  { MENU_PC_MISC, "Preset", (int*)&configPCSettings.settings.presetIndex, 0, 0, 1, 1, TYPE_INTEGER_NAMED, optionsViewport, MENU_INPUT_LR_SELECT | MENU_INPUT_CLICK, menuPcSettingsPresetCallback }
};


// Menu defintions
menuProperties mainMenu = { MENU_MAIN, "Main Menu", NULL, mainMenuSettings, 4, MENU_NONE, MENU_NONE };
menuProperties inGameMenu = { MENU_INGAME, "Menu", NULL, inGameMenuSettings, 5, MENU_NONE, MENU_NONE };

menuProperties pcSettingsScreenMenu = { MENU_PC_SCREEN, "PC Settings - Screen", "1/3", pcSettingsScreen, 6, MENU_PC_MISC, MENU_PC_DISPLAY };
menuProperties pcSettingsDisplayMenu = { MENU_PC_DISPLAY, "PC Settings - Display", "2/3", pcSettingsDisplay, 4, MENU_PC_SCREEN, MENU_PC_MISC };
menuProperties pcSettingsMiscMenu = { MENU_PC_MISC, "PC Settings - Misc", "3/3", pcSettingsMisc, 4, MENU_PC_DISPLAY, MENU_PC_SCREEN };

menuProperties generalMenu = { MENU_GENERAL, "General Settings", "1/7", generalSettings, 6, MENU_AUTOSTART, MENU_WLAN_IP };
menuProperties wlanIPMenu = { MENU_WLAN_IP, "WLAN Host IP Settings", "2/7", wlanIPSettings, 7, MENU_GENERAL, MENU_WLAN_MAC };
menuProperties wlanMACMenu = { MENU_WLAN_MAC, "WLAN Wake-On-Lan Settings", "3/7", wlanWakeOnLanSettings, 7, MENU_WLAN_IP, MENU_WLAN };
menuProperties wlanMenu = { MENU_WLAN, "WLAN Settings", "4/7", wlanSettings, 3, MENU_WLAN_MAC, MENU_USB };
menuProperties usbMenu = { MENU_USB, "USB Settings", "5/7", usbSettings, 2, MENU_WLAN, MENU_USB_POWER_SAVING };
menuProperties usbPowerSaveMenu = { MENU_USB_POWER_SAVING, "USB Power Save Settings", "6/7", usbPowerSaveSettings, 5, MENU_USB, MENU_AUTOSTART };
menuProperties autoStartMenu = { MENU_AUTOSTART, "Autostart Settings", "7/7", autoStartSettings, 2, MENU_USB_POWER_SAVING, MENU_GENERAL };





/*
  getMenuProperty
  ---------------------------------------------------
  Get a pointer to the menu property from the
  menu ID.
  ---------------------------------------------------
  Returns the menu property, or in case of error
  the main menu.
*/
menuProperties* getMenuProperty(int menuId)
{
  switch (menuId)
  {
    case MENU_MAIN:
      return &mainMenu;
    case MENU_INGAME:
      return &inGameMenu;
    case MENU_GENERAL:
      return &generalMenu;
    case MENU_WLAN:
      return &wlanMenu;
    case MENU_WLAN_IP:
      return &wlanIPMenu;
	case MENU_WLAN_MAC:
      return &wlanMACMenu;
    case MENU_AUTOSTART:
      return &autoStartMenu;
    case MENU_USB:
      return &usbMenu;
    case MENU_USB_POWER_SAVING:
      return &usbPowerSaveMenu;
    case MENU_PC_MISC:
      return &pcSettingsMiscMenu;
    case MENU_PC_DISPLAY:
      return &pcSettingsDisplayMenu;
    case MENU_PC_SCREEN:
      return &pcSettingsScreenMenu;
  }
  return &mainMenu;
}




/*
  drawMenu
  ---------------------------------------------------
  Main entry point for drawing the menu.
  ---------------------------------------------------
*/
void drawMenu(menuProperties* menuProperty, int selectionIndex)
{
  // Synchronizing the cache fixes a very strange bug.
  // Without this, about every other frame the buffer swap
  // seems to occur AFTER the text is drawn, therefore you can
  // briefly see the text being drawn over the old one and then
  // a blank background after the frame switch.
  // Also, this only occured when changing a value with the 
  // DPAD.
  sceKernelDcacheWritebackAll();

  // Built-in dialogs should override everything
  if (!g_oskActive && !g_netDialogActive)
  {
    graphicDrawBackground(true, true);

    graphicStartDrawing();

    drawCaption(menuProperty, selectionIndex);

    int i; 
    for (i = 0; i < menuProperty->entryCount; i++)
    {
      drawOption(menuProperty, i, selectionIndex);
    }

    drawStatus(menuProperty, selectionIndex);

    graphicFinishDrawing(true);
  }
}



/*
  drawCaption
  ---------------------------------------------------
  Draw the menu title.
  ---------------------------------------------------
*/
void drawCaption(menuProperties* menuProperty, int selectionIndex)
{
  unsigned int color = MENU_COLOR_CAPTION_HIGHLIGHTED;

  if (menuProperty->menuId == MENU_MAIN)
  {
    // Main menu
    intraFontSetStyle(sansSerifLarge, 1.0f, color, COLOR_BLACK, 0);
    intraFontSetStyle(sansSerifSmall, 1.0f, color, COLOR_BLACK, 0);

	intraFontSetEncoding(sansSerifSmall, INTRAFONT_STRING_UTF8);

    intraFontPrint(sansSerifLarge,  10, 20, "PSPdisp");
    intraFontPrint(sansSerifSmall, 100, 20, "v0.6.1 \xC2\xA9 2008 - 2015 JJS");
  }
  else
  {
    float x;
    if (menuProperty->pageNumber)
    {
      intraFontSetStyle(sansSerifSmall, 1.0f, color, COLOR_BLACK, 0);
      x = intraFontPrint(sansSerifSmall, 10, 20, menuProperty->pageNumber);
    }
    else
      x = 0;

    intraFontSetStyle(sansSerifLarge, 1.0f, color, COLOR_BLACK, 0);
    intraFontPrint(sansSerifLarge, 10 + x, 20, menuProperty->caption); 
  }

  intraFontSetStyle(sansSerifSmall, 1.0f, MENU_COLOR_CAPTION, COLOR_BLACK, INTRAFONT_ALIGN_RIGHT);

  int battery = scePowerGetBatteryLifePercent();

  int batteryOffset = (configVerboseBattery && (battery > -1)) ? 33 : 0;

  if (battery > -1)
  {
    // Draw verbose battery status
    if (configVerboseBattery)
    {
      int timeLeft = scePowerGetBatteryLifeTime();

      intraFontSetStyle(sansSerifSmall, 0.7f, MENU_COLOR_CAPTION, COLOR_BLACK, INTRAFONT_ALIGN_CENTER);
      intraFontPrintf(sansSerifSmall, 460, 12, "%d %%", battery);

      if (timeLeft > 0)
        intraFontPrintf(sansSerifSmall, 460, 23, "%d:%.2d", timeLeft / 60, timeLeft % 60);
      else
        intraFontPrintf(sansSerifSmall, 460, 23, "-:--");
    }

    // Draw battery
    if (battery < 25)      
      drawGlyph(MENU_GLYPH_BATTERY0, 460 - batteryOffset, 7);
    else if (battery < 50)
      drawGlyph(MENU_GLYPH_BATTERY1, 460 - batteryOffset, 7);
    else if (battery < 75)
      drawGlyph(MENU_GLYPH_BATTERY2, 460 - batteryOffset, 7);
    else
      drawGlyph(MENU_GLYPH_BATTERY3, 460 - batteryOffset, 7);

    batteryOffset += 15;
  }

  intraFontSetStyle(sansSerifSmall, 1.0f, MENU_COLOR_CAPTION, COLOR_BLACK, INTRAFONT_ALIGN_RIGHT);

  // Draw time
  pspTime time;
  if (sceRtcGetCurrentClockLocalTime(&time) == 0)
  {
    if (l_timeFormat == PSP_SYSTEMPARAM_TIME_FORMAT_12HR)
    {
      // 12 hour clock
	  if (time.hour >= 12)
        intraFontPrintf(sansSerifSmall, 470 - batteryOffset, 20, "%d:%.2d pm", (time.hour == 12) ? 12 : time.hour - 12, time.minutes);
      else
        intraFontPrintf(sansSerifSmall, 470 - batteryOffset, 20, "%d:%.2d am", (time.hour == 0) ? 12 : time.hour, time.minutes);
    }
    else
    {
      // 24 hour clock
      intraFontPrintf(sansSerifSmall, 470 - batteryOffset, 20, "%d:%.2d", time.hour, time.minutes);
    }
  }
}




/*
  drawOption
  ---------------------------------------------------
  Draw a menu option.
  ---------------------------------------------------
*/
void drawOption(menuProperties* menuProperty, int optionIndex, int selectionIndex)
{
  unsigned int color = (selectionIndex == optionIndex) ? MENU_COLOR_OPTION_HIGHLIGHTED : MENU_COLOR_OPTION;

  int offsetY = optionIndex;

  if (selectionIndex > MENU_MAX_OPTIONS_ON_SCREEN)
  {
    // Not everything fits on screen
    if (optionIndex < selectionIndex - MENU_MAX_OPTIONS_ON_SCREEN)
      return;

    if (optionIndex > selectionIndex)
      return;
    
    offsetY = optionIndex - (selectionIndex - MENU_MAX_OPTIONS_ON_SCREEN);
  }
  else if (optionIndex > MENU_MAX_OPTIONS_ON_SCREEN)
    return;

  offsetY *= MENU_OFFSET_OPTION_INCREMENT;

  if (menuProperty->menuId == MENU_MAIN)
    offsetY += MENU_OFFSET_OPTION_INCREMENT;

  // Caption
  if (menuProperty->menuEntries[optionIndex].settingType == TYPE_BUTTON)
  {
    // Buttons
    color = (selectionIndex == optionIndex) ? MENU_COLOR_BUTTON_HIGHLIGHTED : MENU_COLOR_BUTTON;
    intraFontSetStyle(sansSerifSmall, 1.0f, color, COLOR_BLACK, INTRAFONT_ALIGN_CENTER);
    intraFontPrint(sansSerifSmall, 240, MENU_OFFSET_OPTION_Y + offsetY, menuProperty->menuEntries[optionIndex].caption);
    return;
  }
  else
  {
    // Setting caption
    intraFontSetStyle(sansSerifSmall, 1.0f, color, COLOR_BLACK, INTRAFONT_ALIGN_RIGHT);
    intraFontPrint(sansSerifSmall, MENU_OFFSET_OPTION_X, MENU_OFFSET_OPTION_Y + offsetY, menuProperty->menuEntries[optionIndex].caption);
  }

  // Value
  color = (selectionIndex == optionIndex) ? MENU_COLOR_VALUE_HIGHLIGHTED : MENU_COLOR_VALUE;

  intraFontSetStyle(sansSerifSmall, 1.0f, color, COLOR_BLACK, INTRAFONT_ALIGN_LEFT);

  if (menuProperty->menuEntries[optionIndex].callback != NULL)
  {
    // Custom drawing for this item is enabled
    menuCallbackDrawStruct callbackStruct;
    callbackStruct.x = 480 - MENU_OFFSET_OPTION_X;
    callbackStruct.y = MENU_OFFSET_OPTION_Y + offsetY;
    callbackStruct.processed = false;
    callbackStruct.menuOption = &(menuProperty->menuEntries[optionIndex]);
    menuProperty->menuEntries[optionIndex].callback(CALLBACK_DRAW, &callbackStruct);
    if (callbackStruct.processed)
      return;
  }

  // Draw value
  char* value = NULL;
  switch (menuProperty->menuEntries[optionIndex].settingType)
  {
    case TYPE_INTEGER:
      intraFontPrintf(sansSerifSmall, 480 - MENU_OFFSET_OPTION_X, MENU_OFFSET_OPTION_Y + offsetY, "%d", *menuProperty->menuEntries[optionIndex].value);
      break;
    case TYPE_INTEGER_WITH_UNIT:
      value = menuProperty->menuEntries[optionIndex].valueArray[0];
      intraFontPrintf(sansSerifSmall, 480 - MENU_OFFSET_OPTION_X, MENU_OFFSET_OPTION_Y + offsetY, "%d%s", *menuProperty->menuEntries[optionIndex].value, value);
      break;
    case TYPE_INTEGER_NAMED:
      value = menuProperty->menuEntries[optionIndex].valueArray[*menuProperty->menuEntries[optionIndex].value];
      intraFontPrint(sansSerifSmall, 480 - MENU_OFFSET_OPTION_X, MENU_OFFSET_OPTION_Y + offsetY, value);
      break;
    case TYPE_STRING:
      intraFontPrint(sansSerifSmall, 480 - MENU_OFFSET_OPTION_X, MENU_OFFSET_OPTION_Y + offsetY, (const char*)menuProperty->menuEntries[optionIndex].valueArray);
      break;
    case TYPE_STRING_SELECTION:
      intraFontPrint(sansSerifSmall, 480 - MENU_OFFSET_OPTION_X, MENU_OFFSET_OPTION_Y + offsetY, menuProperty->menuEntries[optionIndex].valueArray[*menuProperty->menuEntries[optionIndex].value]);
      break;
  }
}




/*
  drawStatus
  ---------------------------------------------------
  Draw the available buttons in the status line.
  ---------------------------------------------------
*/
void drawStatus(menuProperties* menuProperty, int selectionIndex)
{
  int offsetX = 10;

  intraFontSetStyle(sansSerifSmall, 0.9f, MENU_COLOR_CAPTION, COLOR_BLACK, INTRAFONT_ALIGN_LEFT);

  if ((menuProperty->nextMenuId != MENU_NONE) && (menuProperty->nextMenuId != MENU_NONE))
  {
    drawGlyph(MENU_GLYPH_SHOULDER, offsetX, 247);
    offsetX += 36;
    offsetX = 15 + floor(intraFontPrint(sansSerifSmall, offsetX, 260, "Page"));
  }

  drawGlyph(MENU_GLYPH_UP_DOWN, offsetX, 248);
  offsetX += 20;
  offsetX = 15 + floor(intraFontPrint(sansSerifSmall, offsetX, 260, "Move"));

  if (menuProperty->menuEntries[selectionIndex].inputMethods & MENU_INPUT_LR_SELECT)
  {
    drawGlyph(MENU_GLYPH_LEFT_RIGHT , offsetX, 248);
    offsetX += 20;
    offsetX = 15 + floor(intraFontPrint(sansSerifSmall, offsetX, 260, "Change"));
  }

  if (menuProperty->menuEntries[selectionIndex].inputMethods & MENU_INPUT_OSK)
  {
    if (l_okButton == PSP_CTRL_CROSS)
      drawGlyph(MENU_GLYPH_CROSS, offsetX, 248);
    else
      drawGlyph(MENU_GLYPH_CIRCLE, offsetX, 248);

    offsetX += 20;
    offsetX = 15 + floor(intraFontPrint(sansSerifSmall, offsetX, 260, "Keyboard"));
  }

  if (menuProperty->menuEntries[selectionIndex].inputMethods & MENU_INPUT_CLICK)
  {
    if (l_okButton == PSP_CTRL_CROSS)
      drawGlyph(MENU_GLYPH_CROSS, offsetX, 248);
    else
      drawGlyph(MENU_GLYPH_CIRCLE, offsetX, 248);

    offsetX += 20;
    offsetX = 15 + floor(intraFontPrint(sansSerifSmall, offsetX, 260, "Select"));
  }

  if (!(menuProperty->menuEntries[selectionIndex].inputMethods & MENU_INPUT_CLICK))
  {
    drawGlyph(MENU_GLYPH_TRIANGLE, offsetX, 248);
    offsetX += 20;
    offsetX = 15 + floor(intraFontPrint(sansSerifSmall, offsetX, 260, "Default"));
  }

  if (menuProperty->menuId != MENU_MAIN)
  {
    intraFontSetStyle(sansSerifSmall, 1.0f, MENU_COLOR_CAPTION, COLOR_BLACK, INTRAFONT_ALIGN_RIGHT);
    intraFontPrint(sansSerifSmall, 470, 260, "Back");
    offsetX = intraFontMeasureText(sansSerifSmall, "Back");

    if (l_okButton == PSP_CTRL_CROSS)
      drawGlyph(MENU_GLYPH_CIRCLE, 470 - 20 - offsetX, 248);
    else
      drawGlyph(MENU_GLYPH_CROSS, 470 - 20 - offsetX, 248);    
  }
}



/*
  drawGlyph
  ---------------------------------------------------
  Draw a button icon.
  ---------------------------------------------------
*/
void drawGlyph(int glyph, int x, int y)
{
  switch (glyph)
  {
    case MENU_GLYPH_CROSS:
      graphicDrawTexturedRectangle(&glyphCross_start, x, y, 16, 16);
      break;
    case MENU_GLYPH_CIRCLE:
      graphicDrawTexturedRectangle(&glyphCircle_start, x, y, 16, 16);
      break;
    case MENU_GLYPH_TRIANGLE:
      graphicDrawTexturedRectangle(&glyphTriangle_start, x, y, 16, 16);
      break;
    case MENU_GLYPH_SQUARE:
      break;
    case MENU_GLYPH_LEFT_RIGHT:
      graphicDrawTexturedRectangle(&glyphLeftRight_start, x, y, 16, 16);
      break;
    case MENU_GLYPH_UP_DOWN:
      graphicDrawTexturedRectangle(&glyphUpDown_start, x, y, 16, 16);
      break;
    case MENU_GLYPH_SHOULDER:
      graphicDrawTexturedRectangle(&glyphShoulder_start, x, y, 32, 16);
      break;
    case MENU_GLYPH_BATTERY0:
      graphicDrawTexturedRectangle(&glyphBattery0_start, x, y, 16, 16);
      break;
    case MENU_GLYPH_BATTERY1:
      graphicDrawTexturedRectangle(&glyphBattery1_start, x, y, 16, 16);
      break;
    case MENU_GLYPH_BATTERY2:
      graphicDrawTexturedRectangle(&glyphBattery2_start, x, y, 16, 16);
      break;
    case MENU_GLYPH_BATTERY3:
      graphicDrawTexturedRectangle(&glyphBattery3_start, x, y, 16, 16);
      break;
  }
}




/*
  increaseValue
  ---------------------------------------------------
  Try to increase the value of the menu item.
  ---------------------------------------------------
  Return true on success.
*/
bool increaseValue(menuEntry* menuOption)
{
  if (menuOption->inputMethods & MENU_INPUT_LR_SELECT)
  {
    int tempValue = *menuOption->value + menuOption->increment;

    if (tempValue > menuOption->maximum)
      tempValue = menuOption->minimum;

    *menuOption->value = tempValue;

    return true;
  }

  return false;
}




/*
  decreaseValue
  ---------------------------------------------------
  Try to decrease the value of the menu item.
  ---------------------------------------------------
  Return true on success.
*/
bool decreaseValue(menuEntry* menuOption)
{
  if (menuOption->inputMethods & MENU_INPUT_LR_SELECT)
  {
    int tempValue = *menuOption->value - menuOption->increment;

    if (tempValue < menuOption->minimum)
      tempValue = menuOption->maximum;

    *menuOption->value = tempValue;

    return true;
  }

  return false;
}




/*
  buttonGotPressed
  ---------------------------------------------------
  Was the specified button pressed this frame?
  ---------------------------------------------------
  Return true if it was.
*/
int buttonGotPressed(unsigned int buttonCode, unsigned int currentButtons, unsigned int previousButtons)
{
  return ((currentButtons & buttonCode) && !(previousButtons & buttonCode));
}



/*
  buttonGotReleased
  ---------------------------------------------------
  Was the specified button released this frame?
  ---------------------------------------------------
  Return true if it was.
*/
int buttonGotReleased(unsigned int buttonCode, unsigned int currentButtons, unsigned int previousButtons)
{
  return (!(currentButtons & buttonCode) && (previousButtons & buttonCode));
}



/*
  menuSetOkButton
  ---------------------------------------------------
  Determine the state of the X->O button swap.
  ---------------------------------------------------
*/
void menuSetOkButton()
{
  if (configXOButtons == 1)
  {
    l_okButton = PSP_CTRL_CROSS;
    l_cancelButton = PSP_CTRL_CIRCLE;
  }
  else if (configXOButtons == 2)
  {
    l_okButton = PSP_CTRL_CIRCLE;
    l_cancelButton = PSP_CTRL_CROSS;
  }
  else
  {
    // Get default behaviour
    int buttonSwap;
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &buttonSwap);
    l_okButton = (buttonSwap == PSP_UTILITY_ACCEPT_CROSS) ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;
    l_cancelButton = (buttonSwap == PSP_UTILITY_ACCEPT_CROSS) ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS;
  }
}




/*
  menuSetTimeFormat
  ---------------------------------------------------
  Determine and set the way the clock is displayed.
  ---------------------------------------------------
*/
void menuSetTimeFormat()
{
  if (configTimeFormat == 1)
    l_timeFormat = PSP_SYSTEMPARAM_TIME_FORMAT_12HR;
  else if (configTimeFormat == 2)
    l_timeFormat = PSP_SYSTEMPARAM_TIME_FORMAT_24HR;
  else
  {
    // Get default behaviour
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_TIME_FORMAT, &l_timeFormat);
  }
}



/*
  menuLoop
  ---------------------------------------------------
  Main menu processing loop.
  ---------------------------------------------------
  Return the next menu ID.
*/
int menuLoop(int startMenuId)
{
  // Reading buttons
  SceCtrlData controlData;
  unsigned int previousButtons = 0;

  // Typematic button behaviour
  int repeatCount = 0;
  int doRepeatFast = 0;

  // Force refresh to update the clock
  int idleRepeats = 0;

  // Current menu properties
  int selectionIndex = 0;
  menuProperties* menuProperty = getMenuProperty(startMenuId);
  menuEntry* menuOption = &(menuProperty->menuEntries[0]);

  // Should the menu be redrawn?
  int doUpdate = 1;
  g_menuForceRedraw = false;

  g_menuButtonsOnExit = 0;

  menuSetOkButton();
  menuSetTimeFormat();

  // Get initial button state, so that already pressed buttons
  // can be ignored
  utilsCtrlReadBufferPositive(&controlData, 1);
  l_previousMenuButtons = controlData.Buttons;

  while (1)
  {
    utilsCtrlReadBufferPositive(&controlData, 1);

    // Omit buttons that are still pressed from the previous menu
    if (l_previousMenuButtons > 0)
    {
      if (l_previousMenuButtons == controlData.Buttons)
      {
        controlData.Buttons &= ~l_previousMenuButtons;
        previousButtons &= ~l_previousMenuButtons;
      }
      else
        l_previousMenuButtons = 0;
    }

    if (buttonGotPressed(PSP_CTRL_LTRIGGER, controlData.Buttons, previousButtons))
    {
      // Page change to previous menu
      if (menuProperty->nextMenuId != MENU_NONE)
      {
        menuProperty = getMenuProperty(menuProperty->previousMenuId);
        selectionIndex = 0;
        doUpdate = 1;
      }
    }
    else if (buttonGotPressed(PSP_CTRL_RTRIGGER, controlData.Buttons, previousButtons))
    {
      // Page change to next menu
      if (menuProperty->nextMenuId != MENU_NONE)
      {
        menuProperty = getMenuProperty(menuProperty->nextMenuId);
        selectionIndex = 0;
        doUpdate = 1;
      }
    }
    else if (buttonGotPressed(PSP_CTRL_UP, controlData.Buttons, previousButtons))
    {
      // Select previous option entry
      int tempValue = selectionIndex - 1;
      if (selectionIndex < 1)
        tempValue = menuProperty->entryCount - 1;

      selectionIndex = tempValue;
      doUpdate = 1;
    }
    else if (buttonGotPressed(PSP_CTRL_DOWN, controlData.Buttons, previousButtons))
    {
      // Select next option entry
      int tempValue = selectionIndex + 1;
      if (selectionIndex > menuProperty->entryCount - 2)
        tempValue = 0;

      selectionIndex = tempValue;
      doUpdate = 1;
    }
    else if (buttonGotPressed(PSP_CTRL_RIGHT, controlData.Buttons, previousButtons))
    {
	    bool allowChange = true;

      if (menuOption->callback)
        allowChange = menuOption->callback(CALLBACK_BEFORE_VALUE_CHANGE, (void*)menuOption);

	    if (allowChange)
	    {
        // Menu option value changes to next item
		    doUpdate = increaseValue(menuOption);

		    if (menuOption->callback)
		      menuOption->callback(CALLBACK_VALUE_CHANGED, (void*)menuOption);
	    }
	  }
    else if (buttonGotPressed(PSP_CTRL_LEFT, controlData.Buttons, previousButtons))
    {
	    bool allowChange = true;

      if (menuOption->callback)
        allowChange = menuOption->callback(CALLBACK_BEFORE_VALUE_CHANGE, (void*)menuOption);

	    if (allowChange)
	    {
          // Menu option value changes to previous item
          doUpdate = decreaseValue(menuOption);

          if (menuOption->callback)
            menuOption->callback(CALLBACK_VALUE_CHANGED, (void*)menuOption);
	    }
    }
    else if (buttonGotPressed(l_okButton, controlData.Buttons, previousButtons))
    {
      // "OK" button pressed
      if (menuOption->inputMethods & MENU_INPUT_CLICK)
      {
        // Menu option is a button and got pressed
        if (menuOption->callback)
        {
          int callbackResult = menuOption->callback(CALLBACK_BUTTON_PRESSED, (void*)menuOption);

          // Refresh the button buffer to avoid the "back" button propagating through the 
          // menu hirarchy
          utilsCtrlReadBufferPositive(&controlData, 1);
          previousButtons = controlData.Buttons;
          repeatCount = 0;

          menuSetOkButton();

          if (callbackResult != MENU_STAY)
            return callbackResult;

          doUpdate = 1;
        }
      }
      else if (menuOption->inputMethods & MENU_INPUT_OSK)
      {
        // Open onscreen keyboard
        char tempValue[50];
        if (menuOption->settingType == TYPE_STRING)
        {
          // Option type is a string
          if (0 == oskDrawKeyboard((char*)menuOption->valueArray, menuOption->caption, tempValue, menuOption->maximum, OSK_INPUT_ALL))
            strcpy((char*)menuOption->valueArray, tempValue);
        }
        else
        {
          // Option type is an integer
          sprintf(tempValue, "%d", *menuOption->value);
          if (0 == oskDrawKeyboard(tempValue, menuOption->caption, tempValue, 32, OSK_INPUT_NUMBERS))
          {
            // Make sure the new value is inside the min/max bounds
            int tempNumber = atoi(tempValue);
            if (tempNumber < menuOption->minimum)
              tempNumber = menuOption->minimum;
            else if (tempNumber > menuOption->maximum)
              tempNumber = menuOption->maximum;

            *menuOption->value = tempNumber;
          }
        }
        // Always update when the OSK gets shown
        doUpdate = 1;
      }
    }
    else if (buttonGotPressed(l_cancelButton, controlData.Buttons, previousButtons)
      || buttonGotPressed(PSP_CTRL_HOME, controlData.Buttons, previousButtons))
    {
      // "Cancel" button was pressed
      if (menuOption->menuId != MENU_MAIN)
        return MENU_MAIN;
    }
    else if (buttonGotPressed(PSP_CTRL_TRIANGLE, controlData.Buttons, previousButtons))
    {
      // Restore default value
      if (menuOption->settingType == TYPE_STRING)
      {
        // Strings must be restored in the callback
        if (menuOption->callback)
          menuOption->callback(CALLBACK_RESTORE_DEFAULT, (void*)menuOption);
      }
      else if (menuOption->settingType != TYPE_BUTTON)
      {
	      bool allowChange = true;

        if (menuOption->callback)
          allowChange = menuOption->callback(CALLBACK_BEFORE_VALUE_CHANGE, (void*)menuOption);

        if (allowChange)
        {
          *menuOption->value = menuOption->defaultValue;
          if (menuOption->callback)
            menuOption->callback(CALLBACK_VALUE_CHANGED, (void*)menuOption);
	      }
      }
      doUpdate = 1;
    }

    // Force updating after 100 ms with transparency and after 10 s without
    if (idleRepeats >= (configTransparentMenu ? 6 : 60))
    {
      g_menuForceRedraw = true;
    }

    // Redraw?
    if (g_menuForceRedraw || doUpdate)
    {
      drawMenu(menuProperty, selectionIndex);
      menuOption = &(menuProperty->menuEntries[selectionIndex]);
      doUpdate = 0;
      g_menuForceRedraw = false;
      idleRepeats = 0;
    }
    else
    {
      // Update idle counter
      idleRepeats++;
    }

    // Handling of button press repeating
    if (previousButtons == controlData.Buttons)
      repeatCount++;
    else
    {
      repeatCount = (doRepeatFast) ? 50 : 0;
      doRepeatFast = 0;
    }

    if (repeatCount > 60)
    {
      controlData.Buttons = 0;
      repeatCount = 0;
      doRepeatFast = 50;
    }

    previousButtons = controlData.Buttons;
  }
}





/*
  menuDrawPleaseWaitScreen
  ---------------------------------------------------
  Draw the transition screen from a com mode to the
  main menu.
  ---------------------------------------------------
*/
void menuDrawPleaseWaitScreen()
{
  graphicDrawBackground(true, false);

  graphicStartDrawing();

  intraFontSetStyle(sansSerifSmall, 1.0f, MENU_COLOR_CAPTION, COLOR_BLACK, INTRAFONT_ALIGN_CENTER);
  intraFontPrint(sansSerifSmall, 240, 130, "Please wait...");

  graphicFinishDrawing(true);
}




/*
  menuDrawAutostartScreen
  ---------------------------------------------------
  Draw and handle the auto start screen.
  ---------------------------------------------------
  Returns the selected mode ID.
*/
int menuDrawAutostartScreen()
{
  int i;
  for (i = configAutoTimeout * 10; i > 0; i--)
  {
    graphicDrawBackground(true, false);

    graphicStartDrawing();

    intraFontSetStyle(sansSerifLarge, 1.0f, MENU_COLOR_CAPTION, COLOR_BLACK, INTRAFONT_ALIGN_CENTER);
    intraFontSetStyle(sansSerifSmall, 1.0f, MENU_COLOR_CAPTION, COLOR_BLACK, INTRAFONT_ALIGN_CENTER);

    if (configAutoMode == 1)
      intraFontPrintf(sansSerifSmall, 240, 110, "Starting WLAN mode in %d seconds...", i / 10);
    else if (configAutoMode == 2)
      intraFontPrintf(sansSerifSmall, 240, 110, "Starting USB mode in %d seconds...", i / 10);

    intraFontSetStyle(sansSerifSmall, 1.0f, MENU_COLOR_CAPTION, COLOR_BLACK, INTRAFONT_ALIGN_RIGHT);
    intraFontPrint(sansSerifSmall, 470, 260, "Cancel");
    int offsetX = intraFontMeasureText(sansSerifSmall, "Cancel");

    menuSetOkButton();

    if (l_okButton == PSP_CTRL_CROSS)
      drawGlyph(MENU_GLYPH_CIRCLE, 470 - 20 - offsetX, 248);
    else
      drawGlyph(MENU_GLYPH_CROSS, 470 - 20 - offsetX, 248);    

    graphicFinishDrawing(true);

    SceCtrlData controlData;
    sceCtrlReadBufferPositive(&controlData, 1); 

    if (controlData.Buttons & l_cancelButton)
      return MAIN_STATE_MENU;

    sceKernelDelayThread(1000 * 100);
  }

  return (configAutoMode == 1) ? MAIN_STATE_WLAN : MAIN_STATE_USB;
}







// Callbacks



int menuBrightnessCallback(int reason, void* data)
{
  if (reason == CALLBACK_BEFORE_VALUE_CHANGE)
  {
    if (!g_utilsKernelFunctionsAvailable)
      return MENU_PROHIBIT_CHANGE;
  }
  else if (reason == CALLBACK_VALUE_CHANGED)
  {
    if (g_utilsKernelFunctionsAvailable)
      kernelDisplaySetBrightness(*(((menuEntry*)data)->value));
  }
  else if (reason == CALLBACK_DRAW)
  {
    menuCallbackDrawStruct* drawStruct = (menuCallbackDrawStruct*)data;
	  if (!g_utilsKernelFunctionsAvailable)
    {
      intraFontPrintf(sansSerifSmall, drawStruct->x, drawStruct->y, "(not available)");
	    drawStruct->processed = true;
    }
    else
    {
      drawStruct->processed = false;
    }
  }
  return MENU_STAY;
}



int menuStartWlanModeCallback(int reason, void* data)
{
  if (reason == CALLBACK_BUTTON_PRESSED)
  {
    return MENU_WLAN;
  }
  else
    return MENU_STAY;
}


int menuStartUsbModeCallback(int reason, void* data)
{
  if (reason == CALLBACK_BUTTON_PRESSED)
  {
    return MENU_USB;
  }
  else
    return MENU_STAY;
}


int menuResumeCallback(int reason, void* data)
{
  if (reason == CALLBACK_BUTTON_PRESSED)
  {
    if (g_comMode == COM_MODE_USB)
      return MENU_USB;
    else
      return MENU_WLAN;
  }
  else
    return MENU_STAY;
}


int menuSettingsCallback(int reason, void* data)
{
  if (reason == CALLBACK_BUTTON_PRESSED)
  {
    menuLoop(MENU_GENERAL);
  }

  return MENU_STAY;
}


int menuPcSettingsCallback(int reason, void* data)
{
  if (reason == CALLBACK_BUTTON_PRESSED)
  {
    menuLoop(MENU_PC_SCREEN);
  }

  return MENU_STAY;
}


int menuQuitGameCallback(int reason, void* data)
{
  if (reason == CALLBACK_BUTTON_PRESSED)
  {
    return MENU_QUIT;
  }

  return MENU_STAY;
}


int menuIpAddressCallback(int reason, void* data)
{
  if (reason == CALLBACK_RESTORE_DEFAULT)
  {
    strcpy((char*)(((menuEntry*)data)->valueArray), "192.168.0.1");
  }

  g_netIpChanged = true;

  return MENU_STAY;
}


int menuMacAddressCallback(int reason, void* data)
{
  if (reason == CALLBACK_RESTORE_DEFAULT)
  {
    strcpy((char*)(((menuEntry*)data)->valueArray), "00:00:00:00:00:00");
  }

  return MENU_STAY;
}


int menuButtonConfigurationCallback(int reason, void* data)
{
  if (reason == CALLBACK_DRAW)
  {
    menuCallbackDrawStruct* drawStruct = (menuCallbackDrawStruct*)data;
    if (*drawStruct->menuOption->value == 0)
    {
      intraFontPrintf(sansSerifSmall, drawStruct->x, drawStruct->y, "System default");
    }
    else if (*drawStruct->menuOption->value == 1)
    {
      drawGlyph(MENU_GLYPH_CROSS, drawStruct->x, drawStruct->y - 12);
      int offsetX = 5 + floor(intraFontPrintf(sansSerifSmall, drawStruct->x + 20, drawStruct->y, "for OK"));
      drawGlyph(MENU_GLYPH_CIRCLE, offsetX, drawStruct->y - 12);
      floor(intraFontPrintf(sansSerifSmall, offsetX + 20, drawStruct->y, "for CANCEL"));
    }
    else if (*drawStruct->menuOption->value == 2)
    {
      drawGlyph(MENU_GLYPH_CROSS, drawStruct->x, drawStruct->y - 12);
      int offsetX = 5 + intraFontPrintf(sansSerifSmall, drawStruct->x + 20, drawStruct->y, "for CANCEL");
      drawGlyph(MENU_GLYPH_CIRCLE, offsetX, drawStruct->y - 12);
      floor(intraFontPrintf(sansSerifSmall, offsetX + 20, drawStruct->y, "for OK"));
    }
    drawStruct->processed = true;
  }
  else if (reason == CALLBACK_VALUE_CHANGED)
  {
    menuSetOkButton();
  }
  return MENU_STAY;
}


int menuChangeCpu(int reason, void* data)
{
  if (reason == CALLBACK_VALUE_CHANGED)
  {
    menuEntry* entry = (menuEntry*)data;

    if (entry->menuId == MENU_USB)
    {
      if (g_comMode == COM_MODE_USB)
        powerSetClockSpeed(*entry->value);
    }
    else if (entry->menuId == MENU_WLAN)
    {
      if (g_comMode == COM_MODE_WLAN)
        powerSetClockSpeed(*entry->value);
    }
  }
  return MENU_STAY;
}


int menuToggleWlanLed(int reason, void* data)
{
  if (reason == CALLBACK_BEFORE_VALUE_CHANGE)
  {
    if (!g_utilsKernelFunctionsAvailable)
      return MENU_PROHIBIT_CHANGE;
  }
  else if (reason == CALLBACK_VALUE_CHANGED)
  {
    menuEntry* entry = (menuEntry*)data;

    kernelSetWlanLedState(*entry->value);
  }
  else if (reason == CALLBACK_DRAW)
  {
    menuCallbackDrawStruct* drawStruct = (menuCallbackDrawStruct*)data;
	if (!g_utilsKernelFunctionsAvailable)
    {
      intraFontPrintf(sansSerifSmall, drawStruct->x, drawStruct->y, "(not available)");
	  drawStruct->processed = true;
    }
    else
    {
      drawStruct->processed = false;
    }
  }
  return MENU_STAY;
}


int menuChangeButtonSwap(int reason, void* data)
{
  if (reason == CALLBACK_VALUE_CHANGED)
  {
    menuSetOkButton();
  }
  return MENU_STAY;
}


int menuChangeTimeFormat(int reason, void* data)
{
  if (reason == CALLBACK_VALUE_CHANGED)
  {
    menuSetTimeFormat();
  }
  return MENU_STAY;
}


int menuKeyboardCallback(int reason, void* data)
{
  if (reason == CALLBACK_BUTTON_PRESSED)
  {
    g_oskDanzeffActive = !g_oskDanzeffActive;

    if (g_comMode == COM_MODE_USB)
      return MENU_USB;
    else
      return MENU_WLAN;
  }
  else
    return MENU_STAY;
}


int menuPasswordCallback(int reason, void* data)
{
  if (reason == CALLBACK_RESTORE_DEFAULT)
  {
    strcpy(configWlanPassword, "");
  }
  else if (reason == CALLBACK_DRAW)
  {
    menuCallbackDrawStruct* drawStruct = (menuCallbackDrawStruct*)data;
    intraFontPrintf(sansSerifSmall, drawStruct->x, drawStruct->y, (strlen((char*)drawStruct->menuOption->valueArray) == 0) ? "(no password set)" : "****");
    drawStruct->processed = true;
  }

  return MENU_STAY;
}


int menuPcSettingsChangeCallback(int reason, void* data)
{
  if (reason == CALLBACK_VALUE_CHANGED)
  {
    g_comUpdateSettings = true;
  }

  return MENU_STAY;
}


int menuPcSettingsControlCallback(int reason, void* data)
{
  if (reason == CALLBACK_VALUE_CHANGED)
  {
    g_comUpdateSettings = true;
  }
  else if (reason == CALLBACK_DRAW)
  {
    menuCallbackDrawStruct* drawStruct = (menuCallbackDrawStruct*)data;
    intraFontPrintf(sansSerifSmall, drawStruct->x, drawStruct->y, configPCSettings.controls[*drawStruct->menuOption->value]);
    drawStruct->processed = true;
  }

  return MENU_STAY;
}


int menuPcSettingsPresetCallback(int reason, void* data)
{
  if (reason == CALLBACK_BUTTON_PRESSED)
  {
    g_comUpdateSettings = true;
    configPCSettings.settings.presetDoSet = 1;
  }
  else if (reason == CALLBACK_DRAW)
  {
    menuCallbackDrawStruct* drawStruct = (menuCallbackDrawStruct*)data;
    intraFontPrintf(sansSerifSmall, drawStruct->x, drawStruct->y, configPCSettings.presets[*drawStruct->menuOption->value]);
    drawStruct->processed = true;
  }

  return MENU_STAY;
}


int menuPcSettingsDeviceCallback(int reason, void* data)
{
  if (reason == CALLBACK_VALUE_CHANGED)
  {
    g_comUpdateSettings = true;
  }
  else if (reason == CALLBACK_DRAW)
  {
    menuCallbackDrawStruct* drawStruct = (menuCallbackDrawStruct*)data;
    intraFontPrintf(sansSerifSmall, drawStruct->x, drawStruct->y, configPCSettings.devices[*drawStruct->menuOption->value]);
    drawStruct->processed = true;
  }

  return MENU_STAY;
}
