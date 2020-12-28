/*
  ***************************************************
  PSPdisp (c) 2008 - 2009 Jochen Schleu

  config.c - loading the config file

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "config.h"


#define CONFIG_NAME "pspdisp.cfg"


/*
  configLoadInteger
  ---------------------------------------------------
  Read a value from the config file.
  ---------------------------------------------------
*/
void configLoadInteger(FILE* file, char* name, int* value, int defaultValue, int minValue, int maxValue)
{
  *value = defaultValue;

  if (file != NULL)
  {
    int found = 0;
    int foundValue = 0;
    char buffer[256];
    char scanName[256];
    fseek(file, 0, SEEK_SET);

    strcpy(scanName, name);
    strcat(scanName, " %d ");

    while (!found) 
    {
      if (fgets(buffer, 255, file) != NULL)
      {
        if (strlen(buffer) < 3) 
          continue;

        found = sscanf(buffer, scanName, &foundValue);
      }
      else
        break;
    }
    
    if (found) 
    {
      if ((foundValue >= minValue) && (foundValue <= maxValue))
        *value = foundValue;
    }
  }

  DEBUG_PRINTF("configLoadInteger: name=%s, value=%d, defaultValue=%d, minValue=%d, maxValue=%d\n", name, *value, defaultValue, minValue, maxValue)
}





/*
  configLoadString
  ---------------------------------------------------
  Read a string from the config file.
  ---------------------------------------------------
*/
void configLoadString(FILE* file, char* name, char* value, char* defaultValue, int minLength, int maxLength)
{
  strcpy(value, defaultValue);

  if (file != NULL)
  {
    int found = 0;
    char foundValue[256];
    char buffer[256];
    char scanName[256];
    fseek(file, 0, SEEK_SET);

    strcpy(scanName, name);
    strcat(scanName, " \"%s\" ");

    while (!found) 
    {
      if (fgets(buffer, 255, file) != NULL)
      {
        if (strlen(buffer) < 3) 
          continue;

        found = sscanf(buffer, scanName, &foundValue);
      }
      else
        break;
    }
    
    if (found) 
    {
      if ((strlen(foundValue) >= minLength) && (strlen(foundValue) <= maxLength + 1))
      {
        // omit trailing "
        foundValue[strlen(foundValue) - 1] = 0;
        strcpy(value, foundValue);
      }
    }
  }
  
  DEBUG_PRINTF("configLoadInteger: name=%s, value=%s, defaultValue=%s, minLength=%d, maxLength=%d\n", name, value, defaultValue, minLength, maxLength)
}






/*
  configLoadConfiguration
  ---------------------------------------------------
  Read the configuration.
  ---------------------------------------------------
*/
void configLoadConfiguration()
{
  FILE *file;
  file = fopen(CONFIG_NAME, "rt");

  DEBUG_PRINTF("configLoadConfiguration: fileFound=%d", (file != NULL))

  configLoadInteger(file, "usbSaveTimeout", &configUsbSaveTimeout, 5, 2, 3600);
  configLoadInteger(file, "usbSave", &configUsbSave, 1, 0, 1);
  configLoadInteger(file, "usbSaveScreenBacklight", &configUsbSaveScreenBacklight, 1, 0, 2);
  configLoadInteger(file, "usbSaveClearScreen", &configUsbSaveClearScreen, 0, 0, 1);
  configLoadInteger(file, "usbSaveDownClock", &configUsbSaveDownClock, 0, 0, 1);
  configLoadInteger(file, "usbClockSpeed", &configUsbClockSpeed, 0, 0, 5);
  configLoadInteger(file, "usbMode", &configUsbMode, 1, 0, 1);

  configLoadInteger(file, "wlanClockSpeed", &configWlanClockSpeed, 0, 0, 5);
  configLoadInteger(file, "wlanLedEnabled", &configWlanLedEnabled, 1, 0, 1);

  configLoadInteger(file, "autoMode", &configAutoMode, 0, 0, 2);
  configLoadInteger(file, "autoTimeout", &configAutoTimeout, 10, 0, 3600);

  configLoadInteger(file, "wlanIPIndex", &configIpAddressIndex, 1, 1, 5);
  configLoadString(file, "wlanPassword", configWlanPassword, "", 0, 32);

  configLoadInteger(file, "wlanPortOffset", &configWlanPortOffset, 0, 0, 9);

  configLoadInteger(file, "wlanEnableWakeOnLan", &configWlanWakeOnLanEnabled, 0, 0, 1);
  configLoadInteger(file, "wlanEnableWakeOnLanBroadcasting", &configWlanWakeOnLanBroadcasting, 0, 0, 1);

  // initialize the address array here
  configLoadString(file, "wlanIPAddress1", configIpAddresses[0], "192.168.0.1", 0, 32);
  configLoadString(file, "wlanIPAddress2", configIpAddresses[1], "192.168.0.1", 0, 32);
  configLoadString(file, "wlanIPAddress3", configIpAddresses[2], "192.168.0.1", 0, 32);
  configLoadString(file, "wlanIPAddress4", configIpAddresses[3], "192.168.0.1", 0, 32);
  configLoadString(file, "wlanIPAddress5", configIpAddresses[4], "192.168.0.1", 0, 32);

  configLoadString(file, "wlanMACAddress1", configMacAddresses[0], "00:00:00:00:00:00", 0, 17);
  configLoadString(file, "wlanMACAddress2", configMacAddresses[1], "00:00:00:00:00:00", 0, 17);
  configLoadString(file, "wlanMACAddress3", configMacAddresses[2], "00:00:00:00:00:00", 0, 17);
  configLoadString(file, "wlanMACAddress4", configMacAddresses[3], "00:00:00:00:00:00", 0, 17);
  configLoadString(file, "wlanMACAddress5", configMacAddresses[4], "00:00:00:00:00:00", 0, 17);

  configLoadInteger(file, "brightness", &configBrightness, 100, 4, 100);
  configLoadInteger(file, "setBrightness", &configAutosetBrightness, 0, 0, 1);
  configLoadInteger(file, "buttonSwap", &configXOButtons, 0, 0, 2);
  configLoadInteger(file, "timeFormat", &configTimeFormat, 0, 0, 2);
  configLoadInteger(file, "verboseBattery", &configVerboseBattery, 1, 0, 1);
  configLoadInteger(file, "transparentMenu", &configTransparentMenu, 1, 0, 1);
  
  if (file != NULL)
    fclose(file);
}



/*
  configSaveInteger
  ---------------------------------------------------
  Write an int value to the config file.
  ---------------------------------------------------
*/
void configSaveInteger(FILE* file, char* name, int* value)
{
  fprintf(file, "%s %d\n", name, *value);
}



/*
  configSaveString
  ---------------------------------------------------
  Write a string to the config file.
  ---------------------------------------------------
*/
void configSaveString(FILE* file, char* name, char* value)
{
  fprintf(file, "%s \"%s\"\n", name, value);
}




/*
  configSaveConfiguration
  ---------------------------------------------------
  Write the config fie.
  ---------------------------------------------------
*/
void configSaveConfiguration()
{
  FILE *file;
  file = fopen(CONFIG_NAME, "wt");

  DEBUG_PRINTF("configSaveConfiguration: fileOpened=%d", (file != NULL))

  configSaveInteger(file, "usbSaveTimeout", &configUsbSaveTimeout);
  configSaveInteger(file, "usbSave", &configUsbSave);
  configSaveInteger(file, "usbSaveScreenBacklight", &configUsbSaveScreenBacklight);
  configSaveInteger(file, "usbSaveClearScreen", &configUsbSaveClearScreen);
  configSaveInteger(file, "usbSaveDownClock", &configUsbSaveDownClock);
  configSaveInteger(file, "usbClockSpeed", &configUsbClockSpeed);
  configSaveInteger(file, "usbMode", &configUsbMode);

  configSaveInteger(file, "wlanClockSpeed", &configWlanClockSpeed);
  configSaveInteger(file, "wlanLedEnabled", &configWlanLedEnabled);

  configSaveInteger(file, "autoMode", &configAutoMode);
  configSaveInteger(file, "autoTimeout", &configAutoTimeout);

  configSaveInteger(file, "wlanIPIndex", &configIpAddressIndex);
  configSaveString(file, "wlanPassword", configWlanPassword);
  configSaveInteger(file, "wlanEnableWakeOnLan", &configWlanWakeOnLanEnabled);
  configSaveInteger(file, "wlanEnableWakeOnLanBroadcasting", &configWlanWakeOnLanBroadcasting);

  configSaveInteger(file, "wlanPortOffset", &configWlanPortOffset);

  // initialize the address array here
  configSaveString(file, "wlanIPAddress1", configIpAddresses[0]);
  configSaveString(file, "wlanIPAddress2", configIpAddresses[1]);
  configSaveString(file, "wlanIPAddress3", configIpAddresses[2]);
  configSaveString(file, "wlanIPAddress4", configIpAddresses[3]);
  configSaveString(file, "wlanIPAddress5", configIpAddresses[4]);

  configSaveString(file, "wlanMACAddress1", configMacAddresses[0]);
  configSaveString(file, "wlanMACAddress2", configMacAddresses[1]);
  configSaveString(file, "wlanMACAddress3", configMacAddresses[2]);
  configSaveString(file, "wlanMACAddress4", configMacAddresses[3]);
  configSaveString(file, "wlanMACAddress5", configMacAddresses[4]);

  configSaveInteger(file, "brightness", &configBrightness);
  configSaveInteger(file, "setBrightness", &configAutosetBrightness);
  configSaveInteger(file, "buttonSwap", &configXOButtons);
  configSaveInteger(file, "timeFormat", &configTimeFormat);
  configSaveInteger(file, "verboseBattery", &configVerboseBattery);
  configSaveInteger(file, "transparentMenu", &configTransparentMenu);

  fclose(file);
}
