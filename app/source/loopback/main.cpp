/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  main.c - loopback recording for Vista and Win 7

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/


#include <math.h>
#include <stdio.h>
#include <Windows.h>
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

// Speex 1.2 rc1 includes
#include "speex/speex_resampler.h"
#include "speex/speex_types.h"


#define DllExport __declspec( dllexport )


// Error codes
#define ERROR_DEVICE_INVALIDATED 1
#define ERROR_RECORDING_TIMEOUT 2


//
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);


// Definition for the callback function
typedef void (*bufferCallbackProc)(void* pointer, unsigned int dataSize);

// Input and output buffers
float inputData[500 * 1000 * sizeof(float)];
float resampledData[500 * 1000 * sizeof(float)];
short outputData[500 * 1000 * sizeof(char)];

//
BOOL recordingFinished = TRUE;
BOOL playbackFinished = TRUE;
BOOL doPlayback = FALSE;
BOOL doRecord = FALSE;
BOOL errorOccurred = FALSE;

//
IMMDeviceEnumerator *deviceEnumerator = NULL;
IMMDevice *playbackDevice = NULL;
IAudioClient *playbackAudioClient = NULL;
IAudioCaptureClient *captureClient = NULL;
WAVEFORMATEX *playbackWaveFormat = NULL;

//
unsigned int globalRecordingSize;
unsigned int globalSampleRate;
bufferCallbackProc globalCallback;
unsigned int outputBufferBase;
unsigned int writeBufferBase;
BOOL signalBufferFull;

//
HANDLE signalingThreadHandle = NULL;
HANDLE recordingThreadHandle = NULL;
HANDLE playbackThreadHandle = NULL;







/*
  cleanUp
  ---------------------------------------------------
  Free all globally allocated resources.
  ---------------------------------------------------
*/
void cleanUp()
{
  CoTaskMemFree(playbackWaveFormat);

  if (deviceEnumerator != NULL)
    deviceEnumerator->Release();

  if (playbackDevice != NULL)
    playbackDevice->Release();
  
  if (playbackAudioClient != NULL)
    playbackAudioClient->Release();

  if (captureClient != NULL)
    captureClient->Release();

  deviceEnumerator = NULL;
  playbackDevice = NULL;
  playbackAudioClient = NULL;
  captureClient = NULL;

  CoUninitialize();
}




/*
  stopLoopbackRecording
  ---------------------------------------------------
  Signals the recording thread to stop, which in turn
  shuts off the playback thread.
  ---------------------------------------------------
*/
DllExport int StopLoopbackRecording()
{
  int retryCount = 0;

  // Stop recording
  doRecord = FALSE;
  while (!recordingFinished)
  {
    Sleep(100);
    retryCount++;
    if (retryCount > 10)
    {
      // Force the thread to quit
      TerminateThread(recordingThreadHandle, 0);
      recordingFinished = TRUE;
    }
  }

  retryCount = 0;

  // Stop playback
  doPlayback = FALSE;
  while (!playbackFinished)
  {
    Sleep(100);
    retryCount++;
    if (retryCount > 10)
    {
      // Force the thread to quit
      TerminateThread(playbackThreadHandle, 0);
      playbackFinished = TRUE;
    }
  }

  cleanUp();
  return 1;
}





/*
  signalingThread
  ---------------------------------------------------
  Runs the callback function, in its own thread so
  that copying the data doesn't introduce stuttering.
  ---------------------------------------------------
*/
DWORD WINAPI signalingThread(LPVOID lpParameter)
{
  while (doRecord)
  {
    if (signalBufferFull)
    {
      signalBufferFull = FALSE;
      globalCallback(&(outputData[outputBufferBase]), globalRecordingSize);
    }
    else if (errorOccurred)
    {
      errorOccurred = FALSE;
      signalBufferFull = FALSE;
      globalCallback(NULL, globalRecordingSize);
    }

    Sleep(1);
  }
  return 0;
}




/*
  PlayAudioStream
  ---------------------------------------------------
  This code comes right from the SDK sample.
  ---------------------------------------------------
*/
#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

HRESULT PlayAudioStream()
{
    HRESULT  hr;
    REFERENCE_TIME  hnsRequestedDuration = 10000000;
    REFERENCE_TIME  hnsActualDuration;
    IMMDeviceEnumerator  *pEnumerator = NULL;
    IMMDevice  *pDevice = NULL;
    IAudioClient  *pAudioClient = NULL;
    IAudioRenderClient  *pRenderClient = NULL;
    WAVEFORMATEX  *pwfx = NULL;
    UINT32  bufferFrameCount;
    UINT32  numFramesAvailable;
    UINT32  numFramesPadding;
    BYTE  *pData;
    DWORD  flags = 0;

    CoInitialize(NULL);

    hr = CoCreateInstance(
           CLSID_MMDeviceEnumerator, NULL,
           CLSCTX_ALL, IID_IMMDeviceEnumerator,
           (void**)&pEnumerator);
    EXIT_ON_ERROR(hr)

    hr = pEnumerator->GetDefaultAudioEndpoint(
                        eRender, eConsole, &pDevice);
    EXIT_ON_ERROR(hr)

    hr = pDevice->Activate(
                    IID_IAudioClient, CLSCTX_ALL,
                    NULL, (void**)&pAudioClient);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->GetMixFormat(&pwfx);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->Initialize(
                         AUDCLNT_SHAREMODE_SHARED,
                         0,
                         hnsRequestedDuration,
                         0,
                         pwfx,
                         NULL);
    EXIT_ON_ERROR(hr)

    // Get the actual size of the allocated buffer.
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->GetService(
                         IID_IAudioRenderClient,
                         (void**)&pRenderClient);
    EXIT_ON_ERROR(hr)

    // Grab the entire buffer for the initial fill operation.
    hr = pRenderClient->GetBuffer(bufferFrameCount, &pData);
    EXIT_ON_ERROR(hr)

    hr = pRenderClient->ReleaseBuffer(bufferFrameCount, flags);
    EXIT_ON_ERROR(hr)

    // Calculate the actual duration of the allocated buffer.
    hnsActualDuration = (double)10000000 *
                        bufferFrameCount / pwfx->nSamplesPerSec;

    hr = pAudioClient->Start();  // Start playing.
    EXIT_ON_ERROR(hr)

    // Each loop fills about half of the shared buffer.
    while (doPlayback)
    {
        // Sleep for half the buffer duration.
        Sleep((DWORD)(hnsActualDuration/10000/2));

        // See how much buffer space is available.
        hr = pAudioClient->GetCurrentPadding(&numFramesPadding);
        EXIT_ON_ERROR(hr)

        numFramesAvailable = bufferFrameCount - numFramesPadding;

        // Grab all the available space in the shared buffer.
        hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
        EXIT_ON_ERROR(hr)

        hr = pRenderClient->ReleaseBuffer(numFramesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
        EXIT_ON_ERROR(hr)
    }

    // Wait for last data in buffer to play before stopping.
    Sleep((DWORD)(hnsActualDuration/10000/2));

    hr = pAudioClient->Stop();  // Stop playing.
    EXIT_ON_ERROR(hr)

Exit:
    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pEnumerator)
    SAFE_RELEASE(pDevice)
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(pRenderClient)
    return hr;

ErrorExit:
    errorOccurred = TRUE;
    goto Exit;
}




/*
  playbackThread
  ---------------------------------------------------
  Thread for dummy playback. Ensures that recording
  doesn't stop when no other application is playing
  a sound. This also fixes the sound stuttering bug.
  ---------------------------------------------------
*/
DWORD WINAPI playbackThread(LPVOID lpParameter)
{
  doRecord = TRUE;
  PlayAudioStream();
  playbackFinished = TRUE;
  return 0;
}




/*
  recordingThread
  ---------------------------------------------------
  Thread for recording and signal a full buffer.
  ---------------------------------------------------
*/
DWORD WINAPI recordingThread(LPVOID lpParameter)
{
  float* inputDataPointer;
  unsigned int outputSamplecount = 2 * 1000 * 1000;
  int i;
  bool sameInOutFormat = false;
  short* convertOutputPointer;
  float* convertInputPointer;

  unsigned int ratio_num;
  unsigned int ratio_den;

  // Try to adjust for fractional sample rate conversion so
  // that the output sample count is always the same
  switch (playbackWaveFormat->nSamplesPerSec)
  {
	case 8000:
	case 16000:
	case 32000:
  case 48000:
  case 96000:
  case 192000:
      ratio_num = playbackWaveFormat->nSamplesPerSec / 100;
      switch (globalSampleRate)
      {
        case 11025: 
          ratio_den = 112;
          break;
        case 22050:
          ratio_den = 224;
          break;
        case 44100:
          ratio_den = 448;
          break;
      }
      break;

	case 11025:
	case 22050:
  case 44100:
    default:
      ratio_num = playbackWaveFormat->nSamplesPerSec;
      ratio_den = globalSampleRate;
  }

  sameInOutFormat = ((ratio_num / ratio_den) == 1);

  if (playbackAudioClient->Start() == S_OK)
  {
	  // Initialize resampler
    int error;
    SpeexResamplerState* resamplerState = speex_resampler_init_frac(2, ratio_num, ratio_den, playbackWaveFormat->nSamplesPerSec, globalSampleRate, SPEEX_RESAMPLER_QUALITY_DESKTOP, &error);
	  speex_resampler_set_input_stride(resamplerState, 2);
	  speex_resampler_set_output_stride(resamplerState, 2);

    // Positions in the outputBuffer and resampledBuffer
    unsigned int packetLength = 0;
    DWORD flags = 0;
    unsigned int numFramesAvailable = 0;
    int base = 0;
    writeBufferBase = 0;
    outputBufferBase = 0;

	  int loopCounter = 0;

    while (doRecord)
    {
      Sleep(1);

      loopCounter++;

      // Get next audio packet
      HRESULT result = captureClient->GetNextPacketSize(&packetLength);
      if ((loopCounter > 1000) || (result == AUDCLNT_E_DEVICE_INVALIDATED))
      {
        errorOccurred = TRUE;
        recordingFinished = TRUE;
        return 0;
      }
      
      while (packetLength != 0)
      {
	      loopCounter = 0;

        if (captureClient->GetBuffer((BYTE**)&inputDataPointer, &numFramesAvailable, &flags, NULL, NULL) == S_OK)
        {
          // Resample if necessary
          if (sameInOutFormat)
          {
            outputSamplecount = numFramesAvailable;

            // Convert float to short
            convertOutputPointer = &outputData[0] + base + writeBufferBase;
            convertInputPointer = &inputDataPointer[0];

		        for (i = 0; i < outputSamplecount * 2; i++)
            {
              *convertOutputPointer = (*convertInputPointer * 32768.0f);
              convertOutputPointer++;
              convertInputPointer++;
            }
          }
          else
          {
            outputSamplecount = 2 * 1000 * 1000;
		        speex_resampler_process_interleaved_float(resamplerState, inputDataPointer, &numFramesAvailable, resampledData, &outputSamplecount); 

		        // Convert float to short
		        for (i = 0; i < outputSamplecount * 2; i++)
			        outputData[i + base + writeBufferBase] = floor(resampledData[i] * 32768.0f);
          }

		      base += outputSamplecount * 2;
		      if (base * 2 >= globalRecordingSize)
		      {
			      outputBufferBase = writeBufferBase;
			      writeBufferBase = (writeBufferBase == 0) ? globalRecordingSize : 0;
			      base = 0;
			      signalBufferFull = TRUE;
		      }

          captureClient->ReleaseBuffer(numFramesAvailable);
          captureClient->GetNextPacketSize(&packetLength);
        }
      }
    }
    playbackAudioClient->Stop();
  }

  recordingFinished = TRUE;

  return 0;
}




/*
  initializeLoopbackRecording
  ---------------------------------------------------
  Set up everything to be ready for recording.
  ---------------------------------------------------
*/
DllExport int InitializeLoopbackRecording(unsigned int sampleRate, unsigned int recordingSize, bufferCallbackProc callback)
{
  BOOL success = FALSE;

  // Init COM
  if (CoInitialize(NULL) == S_OK)
  {
    // Create class instance
    if (CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&deviceEnumerator) == S_OK)
    {
      // Get default audio adapter for playback
      if (deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &playbackDevice) == S_OK)
      {
        // Activate playback device
        if (playbackDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&playbackAudioClient) == S_OK)
        {
          // Get the audio format
          if (playbackAudioClient->GetMixFormat(&playbackWaveFormat) == S_OK)
          {
            // Initialize the audio client
            if (playbackAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 10 * 1000 * 1000, 0, playbackWaveFormat, NULL) == S_OK)
            {
              // Get capture service
              if (playbackAudioClient->GetService(IID_IAudioCaptureClient, (void**)&captureClient) == S_OK)
              {
                success = TRUE;
              }
            }
          }
        }
      }
    }
  }

  if (success)
  {
    globalRecordingSize = recordingSize;
    globalSampleRate = sampleRate;
    globalCallback = callback;

    signalBufferFull = 0;

    doPlayback = TRUE;
    doRecord = TRUE;

    recordingFinished = FALSE;

    // All done, now create the threads
    signalingThreadHandle = CreateThread(NULL, 0, signalingThread, NULL, 0, NULL);
    playbackThreadHandle = CreateThread(NULL, 0, playbackThread, NULL, 0, NULL);
    recordingThreadHandle = CreateThread(NULL, 0, recordingThread, NULL, 0, NULL);

    return TRUE;
  }
  else
  {
    cleanUp();
    return FALSE;
  }
}



