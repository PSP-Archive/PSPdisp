/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  com.c - all data transmission functions

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "com.h"

#include "compress.h"

#include "menu.h"
extern menuEntry pcSettingsDisplay[4];
extern menuEntry pcSettingsMisc[2];


#define CHECK_WLAN_RESULT { if ((g_comMode == COM_MODE_WLAN) && !g_netConnected) { DEBUG_PRINTF("comLoop: CHECK_WLAN_RESULT reports disconnect\n"); return; } }


char l_alignedBuffer[COM_ALIGNED_BUFFER_SIZE] __attribute__((aligned(64)));
comSettingsResponse l_response __attribute__((aligned(64)));


/*
  readData
  ---------------------------------------------------
  Read data from the active communication mode.
  ---------------------------------------------------
  Returns the number of bytes read, < 0 on error.
*/
int readData(void* buffer, int length)
{
  if (g_comMode == COM_MODE_USB)
  {
    int result = usbReadRawBulkData(buffer, length);
//    DEBUG_PRINTF("result = %d\n", result);

    // Pause a bit on error
    if (result < 0)
      sceKernelDelayThread(1000 * 200);

    return result;
  }
  else // if (g_comMode == COM_MODE_WLAN)
  {
    if (!g_netConnected)
      return -1;
      
    SceUID alarmId = sceKernelSetAlarm(NET_SOCKET_TIMEOUT, (SceKernelAlarmHandler)netSocketTimeoutAlarm, NULL);
    int result = sceNetInetRecv(g_netSocket, buffer, length, 0);
    sceKernelCancelAlarm(alarmId);

    if (result <= 0)
      g_netConnected = false;

    return result;
  }
}



/*
  readDataLarge
  ---------------------------------------------------
  Read data from the active communication mode
  until all requested data is received.
  ---------------------------------------------------
  Returns true on success.
*/
bool readDataLarge(void* buffer, int length)
{
  if (length == 0)
    return true;

  int bufferPosition = 0;
  int bytesToRead = 0;
  int len;
  unsigned char* charBuffer = buffer;

  while (bufferPosition < length) 
  {
    bytesToRead = (length - bufferPosition);
    if (bytesToRead > 0)
    {
      len = readData(&charBuffer[bufferPosition], bytesToRead);
//      DEBUG_PRINTF("bytesToRead = %d, len = %d\n", bytesToRead, len);

      if (len < 0)
      {
        // Reading error
        DEBUG_PRINTF("## readDataLarge: reading error\n");
        return false;
      }

      bufferPosition += len;
    }
  }
  
  return true;
}




/*
  writeData
  ---------------------------------------------------
  Write data to the selected output.
  ---------------------------------------------------
  Returns bytes written or an error code.
*/
int writeData(void* buffer, int length)
{
  if (g_comMode == COM_MODE_USB)
  {
    // Check for 64 byte alignment
    if (((unsigned int)buffer & 0x3F) != 0)
    {
      memcpy(l_alignedBuffer, buffer, length);
      return usbWriteRawBulkData(l_alignedBuffer, length);
    }
    else
      return usbWriteRawBulkData(buffer, length);
  }
  else // if (g_comMode == COM_MODE_WLAN)
  {
    if (!g_netConnected)
      return -1;

    SceUID alarmId = sceKernelSetAlarm(NET_SOCKET_TIMEOUT, (SceKernelAlarmHandler)netSocketTimeoutAlarm, NULL);
    int result = sceNetInetSend(g_netSocket, buffer, length, 0);     
    sceKernelCancelAlarm(alarmId);

    if (result < 0)
      g_netConnected = false;

    return result;
  }
}





/*
  waitForDataSize
  ---------------------------------------------------
  Waits till the PC sends the size of the next
  Jpeg frame.
  ---------------------------------------------------
  Returns true if information was received.
*/
bool readFrameInformation(comFrameHeader* frameHeader)
{
  memset(frameHeader, 0, sizeof(comFrameHeader));
  int len = readData(frameHeader, sizeof(comFrameHeader));

  if (len == sizeof(comFrameHeader)) 
  {
    if (frameHeader->magic == COM_HEADER_MAGIC) 
    {
      return true;
    }
  }

  sceKernelDelayThread(200 * 1000);
  return false;
}




/*
  readFrame
  ---------------------------------------------------
  Reads from the input till the whole frame
  is transmitted.
  ---------------------------------------------------
  Returns true on success.
*/
bool readFrame(comFrameHeader* frameHeader, unsigned char* soundData, comSettingsReceived* settingsData)
{
  unsigned int audioFrameSize = ((frameHeader->flags & COM_FLAGS_AUDIO_CHUNK_2240) ? (2240 * 2) : (2688 * 2));

  unsigned int bytesToRead = frameHeader->imageSize
    + ((frameHeader->flags & COM_FLAGS_CONTAINS_AUDIO_DATA) ? audioFrameSize : 0)
    + frameHeader->settingsSize;

  if (!readDataLarge(g_comImageReceiveBuffer, bytesToRead))
    return false;

  if (frameHeader->flags & COM_FLAGS_CONTAINS_AUDIO_DATA)
  {
    memcpy(soundData, &g_comImageReceiveBuffer[frameHeader->imageSize], audioFrameSize);
  }

  if (frameHeader->flags & COM_FLAGS_CONTAINS_SETTINGS_DATA)
  {
    int bufferPosition = frameHeader->imageSize + ((frameHeader->flags & COM_FLAGS_CONTAINS_AUDIO_DATA) ? audioFrameSize : 0);

    // Copy the settings structure
    memcpy(&settingsData->settings, &g_comImageReceiveBuffer[bufferPosition], sizeof(comSettings));

    // Fill the control, preset and device array
    memset(&settingsData->devices, 0, 32 * COM_MAX_PC_DEVICES);
    memset(&settingsData->controls, 0, 32 * COM_MAX_PC_CONTROLS);
    memset(&settingsData->presets, 0, 32 * COM_MAX_PC_PRESETS);

    bufferPosition += sizeof(comSettings);
    memcpy(settingsData->devices, &g_comImageReceiveBuffer[bufferPosition], settingsData->settings.displayDeviceCount * 32);
    bufferPosition += settingsData->settings.displayDeviceCount * 32;
    memcpy(settingsData->controls, &g_comImageReceiveBuffer[bufferPosition], settingsData->settings.controlCount * 32);
    bufferPosition += settingsData->settings.controlCount * 32;
    memcpy(settingsData->presets, &g_comImageReceiveBuffer[bufferPosition], settingsData->settings.presetCount * 32);

    pcSettingsDisplay[1].maximum = settingsData->settings.displayDeviceCount - 1;
    pcSettingsMisc[2].maximum = settingsData->settings.controlCount - 1;
    pcSettingsMisc[3].maximum = settingsData->settings.presetCount - 1;
  }

  return true;
}





/*
  sendResponse
  ---------------------------------------------------
  Send the control data and setting changes to the PC.
  ---------------------------------------------------
*/
bool sendResponse()
{
  // Read controls
  SceCtrlData controlData;

  sceCtrlPeekBufferPositive(&controlData, 1);

  if (g_menuActive)
  {
    controlData.Buttons = 0;
    controlData.Lx = 127;
    controlData.Ly = 127;
  }

  oskCheckDanzeffState(&controlData);

  // Omit buttons that are still pressed after exiting the menu
  if (g_menuButtonsOnExit > 0)
  {
    if (g_menuButtonsOnExit == controlData.Buttons)
      controlData.Buttons &= ~g_menuButtonsOnExit;
    else
      g_menuButtonsOnExit = 0;
  }

  bool result = false;

//  DEBUG_PRINTF("writing response\n");

  l_response.response.magic = COM_HEADER_MAGIC;
  l_response.response.flags = (g_comUpdateSettings ? COM_FLAGS_CONTAINS_SETTINGS_DATA : 0);
  if (g_comForceScreenUpdate)
    l_response.response.flags = l_response.response.flags | COM_FLAGS_FORCE_UPDATE;
  l_response.response.buttons = controlData.Buttons;
  l_response.response.analogX = controlData.Lx;
  l_response.response.analogY = controlData.Ly;

  sceKernelDcacheWritebackInvalidateAll();

  if (g_comUpdateSettings || (g_comMode == COM_MODE_USB))
  {
    memcpy(&l_response.settings, &configPCSettings, sizeof(comSettings));

    result = (writeData(&l_response, sizeof(comSettingsResponse)) == sizeof(comSettingsResponse));

    if (g_comUpdateSettings)
    {
      g_comForceScreenUpdate = false;

      if (result)
        g_comUpdateSettings = false;
    }
  }
  else
  {
    result = (writeData(&l_response, sizeof(comResponseHeader)) == sizeof(comResponseHeader));
  }

  return result;
}





/*
  comLoop
  ---------------------------------------------------
  Main loop for sending and receiving.
  ---------------------------------------------------
*/
void comLoop()
{
  DEBUG_PRINTF("comLoop: enter\n")

  comFrameHeader frameHeader;
  memset(&frameHeader, 0, sizeof(comFrameHeader));

  g_comRunning = true;
  g_comAudioWritePosition = 0;

  while (g_comRunning) 
  {
//    DEBUG_PRINTF("comLoop: readFrameInformation\n")
    // Wait for the PC to send the size of the next compressed frame.
    if (!readFrameInformation(&frameHeader))
    {
//      DEBUG_PRINTF("readError %d\n", g_comRunning);

      // Break if connection lost in Wlan mode
      CHECK_WLAN_RESULT
    }
    else
    {
      // Frame information received
//      DEBUG_PRINTF("comLoop: size=%d, flags=0x%02lX, settings size=%d\n", frameHeader.imageSize, (long)frameHeader.flags, frameHeader.settingsSize)

      // Disable power save mode
      if (g_comMode == COM_MODE_USB)
        powerEnablePowerSaveMode(false);

//      DEBUG_PRINTF("comLoop: readframe starting\n")

      // Got data size, read in data.
      if (g_comAudioWritePosition >= g_audioCurrentBufferSize)
        g_comAudioWritePosition = 0;

      readFrame(&frameHeader, &(g_comAudioReceiveBuffer[g_comAudioWritePosition]), &configPCSettings);
      CHECK_WLAN_RESULT

      g_comAudioWritePosition += g_audioCurrentFrameSize;

      audioCheckIfResetIsNeeded(&frameHeader);

	    // Start the thread again
//      sceKernelWaitThreadEnd(g_compressThreadId, NULL);
      if ((frameHeader.flags & COM_FLAGS_CONTAINS_IMAGE_DATA) || (frameHeader.flags & COM_FLAGS_IMAGE_CLEAR_SCREEN))
        sceKernelStartThread(g_compressThreadId, sizeof(comFrameHeader), &frameHeader);

      // Read and transmit the PSP controls
      sendResponse();
      CHECK_WLAN_RESULT
    }

    sceDisplayWaitVblankStart();
  }
  DEBUG_PRINTF("comLoop: exit\n")
}
