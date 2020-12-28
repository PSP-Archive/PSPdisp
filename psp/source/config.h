/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  config.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "shared.h"

char configIpAddresses[5][36]; // individual entries must be 4 byte aligned
char configMacAddresses[5][20];
int configIpAddressIndex;
int configAutoMode;
int configAutoTimeout;
int configUsbSave;
int configUsbSaveScreenBacklight;
int configUsbSaveClearScreen;
int configUsbSaveTimeout;
int configUsbSaveDownClock;
int configWlanClockSpeed;
int configWlanLedEnabled;
int configWlanWakeOnLanEnabled;
int configWlanWakeOnLanBroadcasting;
int configWlanPortOffset;
int configUsbClockSpeed;
int configUsbMode;
int configBrightness;
int configAutosetBrightness;
int configXOButtons;
int configMenuButton;
int configTimeFormat;
int configVerboseBattery;
int configTransparentMenu;
char configWlanPassword[32];

comSettingsReceived configPCSettings;

void configLoadConfiguration();
void configLoadInteger(FILE* file, char* name, int* value, int defaultValue, int minValue, int maxValue);
void configLoadString(FILE* file, char* name, char* value, char* defaultValue, int minLength, int maxLength);
void configSaveConfiguration();


#endif