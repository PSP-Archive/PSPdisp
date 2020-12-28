/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  com.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef COM_H
#define COM_H

#include <pspsdk.h>
#include <pspthreadman.h>
#include "common.h"
#include "debug.h"
#include "wlan.h"
#include "usbhostfs/usbasync.h"
#include "audio.h"
#include "shared.h"


#define COM_ALIGNED_BUFFER_SIZE 100
#define COM_IMAGE_BUFFER_SIZE (400 * 1024)
#define COM_AUDIO_BUFFER_SIZE (20 * 2688)

#define COM_MODE_NONE -1
#define COM_MODE_USB 0
#define COM_MODE_WLAN 1


int g_comMode;
unsigned char g_comImageReceiveBuffer[COM_IMAGE_BUFFER_SIZE];
unsigned char g_comAudioReceiveBuffer[COM_AUDIO_BUFFER_SIZE];

int g_comAudioWritePosition;

bool g_comRunning;

bool g_comUpdateSettings;

bool g_comForceScreenUpdate;


bool readFrameInformation(comFrameHeader* frameHeader);
void comLoop();
int readData(void* buffer, int length);
bool readDataLarge(void* buffer, int length);
bool readFrame(comFrameHeader* frameHeader, unsigned char* soundData, comSettingsReceived* settingsData);
bool sendResponse();


#endif