/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  audio.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef AUDIO_H
#define AUDIO_H


#include <pspaudio.h>
#include <pspaudiolib.h>
#include <pspthreadman.h>
#include "com.h"
#include "common.h"
#include "debug.h"



#define AUDIO_CHUNK_SIZE 2240
#define AUDIO_FRAME_SIZE (AUDIO_CHUNK_SIZE * 2)
#define AUDIO_SAMPLE_RATE_USB 22050
#define AUDIO_SAMPLE_RATE_WLAN 11025


int g_audioPlaybackPosition;
SceUID g_audioSyncSema;
unsigned int g_audioCurrentFrameSize;
unsigned int g_audioCurrentBufferSize;


void audioInit();
void audioTerm();
void audioCheckIfResetIsNeeded();
void audioResetPlaybackBuffer();
void audioPlaybackThread(SceSize args, void *argp);



#endif