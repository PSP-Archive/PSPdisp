/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  audio.c - audio related functions

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "audio.h"


int l_lastPlaybackPosition; // Position in the playback buffer audio got played from
bool l_runAudioThread; // true while the audiothread should be executet
SceUID l_playbackThreadId; // Id of the playback thread
bool l_lastFrameContainedAudio; // Got new audio data with the last frame?
unsigned int l_lastFrameSampleRate;
unsigned int l_currentSampleRate = 0; // Initially 0, will be set to the default rate for the mode
unsigned int l_currentChunkSize = AUDIO_CHUNK_SIZE; // Will be set on audio reset



/*
  audioInit
  ---------------------------------------------------
  Initialize audio system.
  ---------------------------------------------------
*/
void audioInit()
{
  if (l_currentSampleRate == 0)
    l_currentSampleRate = ((g_comMode == COM_MODE_USB) ? AUDIO_SAMPLE_RATE_USB : AUDIO_SAMPLE_RATE_WLAN);

  if ((l_currentChunkSize != 2240) && (l_currentChunkSize != 2688))
  {
    l_currentChunkSize = 2688;
    g_audioCurrentFrameSize = l_currentChunkSize * 2;
    g_audioCurrentBufferSize = g_audioCurrentFrameSize * 10;
  }
    
  audioResetPlaybackBuffer();

  sceAudioSRCChReserve(l_currentChunkSize, l_currentSampleRate, 2);

  l_runAudioThread = true;

  l_playbackThreadId = sceKernelCreateThread("playbackThread", (SceKernelThreadEntry)audioPlaybackThread, 0x18, 0x10000, PSP_THREAD_ATTR_USER, 0);    
  sceKernelStartThread(l_playbackThreadId, 0, 0);
}




/*
  audioTerm
  ---------------------------------------------------
  Shut down audio system.
  ---------------------------------------------------
*/
void audioTerm()
{
  l_runAudioThread = false;
  sceKernelWaitThreadEnd(l_playbackThreadId, NULL);
  sceKernelDeleteThread(l_playbackThreadId);

  // Wait till playback has finished
  while (sceAudioOutput2GetRestSample() > 0)
    sceKernelDelayThread(1000);

  sceAudioSRCChRelease();
}





/*
  audioResetPlaybackBuffer
  ---------------------------------------------------
  Clear the playback buffer.
  ---------------------------------------------------
*/
void audioResetPlaybackBuffer()
{
//  DEBUG_PRINTF("audioResetPlaybackBuffer called\n")

  memset(g_comAudioReceiveBuffer, 0, COM_AUDIO_BUFFER_SIZE);
  g_comAudioWritePosition = (g_audioCurrentBufferSize / 2);
  g_audioPlaybackPosition = 0;
}





/*
  audioCheckIfResetIsNeeded
  ---------------------------------------------------
  Check if the playing position advanced over the 
  filling position, mute sound if so
  ---------------------------------------------------
*/
void audioCheckIfResetIsNeeded(comFrameHeader* header)
{
  if (header->flags & COM_FLAGS_CONTAINS_AUDIO_DATA)
  {
    l_lastFrameContainedAudio = true;
  }
  else
  {
    if (l_lastFrameContainedAudio)
      audioResetPlaybackBuffer();

    l_lastFrameContainedAudio = false;
  }

  if (l_lastFrameSampleRate != (header->flags & COM_FLAGS_AUDIO_SAMPLE_RATE_MASK))
  {
    if (header->flags & COM_FLAGS_AUDIO_11025_HZ)
      l_currentSampleRate = 11025;
    else if (header->flags & COM_FLAGS_AUDIO_22050_HZ)
      l_currentSampleRate = 22050;
    else if (header->flags & COM_FLAGS_AUDIO_44100_HZ)
      l_currentSampleRate = 44100;

    if (header->flags & COM_FLAGS_AUDIO_CHUNK_2240)
      l_currentChunkSize = 2240;
    else if (header->flags & COM_FLAGS_AUDIO_CHUNK_2688)
      l_currentChunkSize = 2688;

    g_audioCurrentFrameSize = 2 * l_currentChunkSize;
    g_audioCurrentBufferSize = 10 * g_audioCurrentFrameSize;

    audioTerm();
    audioInit();
    l_lastFrameSampleRate = (header->flags & COM_FLAGS_AUDIO_SAMPLE_RATE_MASK);
  }
}





/*
  audioThread
  ---------------------------------------------------
  Thread for filling the audio buffer.
  ---------------------------------------------------
*/
void audioPlaybackThread(SceSize args, void *argp)
{
  l_lastPlaybackPosition = 0;

  while (l_runAudioThread)
  { 
    // Check if the playing position advanced over the filling position, mute sound if so
    if ((l_lastPlaybackPosition <= g_comAudioWritePosition) && (g_audioPlaybackPosition >= g_comAudioWritePosition))
      audioResetPlaybackBuffer();

    if (g_audioPlaybackPosition + (g_audioCurrentFrameSize * 2) > g_audioCurrentBufferSize)
      g_audioPlaybackPosition = 0;

    sceAudioSRCOutputBlocking(PSP_AUDIO_VOLUME_MAX, &(g_comAudioReceiveBuffer[g_audioPlaybackPosition]));

    l_lastPlaybackPosition = g_audioPlaybackPosition;
    g_audioPlaybackPosition += (g_audioCurrentFrameSize * 2);
  }
}
