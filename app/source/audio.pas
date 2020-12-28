{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  audio.pas - streaming audio functions

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}


unit audio;

interface

uses
  Windows, MMSystem;

type
  TDataAcquiredEvent = procedure(Data: Pointer; DataSize: LongWord);
  TDataAcquiredEventStdCall = procedure(Data: Pointer; DataSize: LongWord); cdecl;


function audioInit(DeviceID: Integer; Channels: LongWord; SampleRate: LongWord; BitRate: LongWord; BufferSize: LongWord; Callback: TDataAcquiredEvent; CallbackStdCall: TDataAcquiredEventStdCall): Boolean;
procedure audioCallback(hwi: HWAVEIN; uMsg: LongWord; dwInstance, dwParam1, dwParam2: LongWord); stdcall;
function audioGetNextWaveDataIndex(): Integer;
procedure audioStartRecording();
procedure audioStopRecording();
procedure audioFinish();
procedure addBuffer();

function audioEnumerateDevices(): Integer;
function audioGetDeviceName(Index: Integer): String;
function audioGetMixerID(DeviceID: Integer): Integer;

const
  ERROR_DEVICE_INVALIDATED = 1;
  ERROR_RECORDING_TIMEOUT = 2;
  AUDIO_BUFFER_SIZE_2240: Cardinal = 448 * 10;
  AUDIO_BUFFER_SIZE_2688: Cardinal = 448 * 12;

type
  InitializeLoopbackRecording_Proc = function(sampleRate: LongWord; recordingSize: LongWord; callback: TDataAcquiredEvent): Integer; cdecl;
  StopLoopbackRecording_Proc = function(): Integer; cdecl;

var
  InitializeLoopbackRecording: InitializeLoopbackRecording_Proc;
  StopLoopbackRecording: StopLoopbackRecording_Proc;



implementation

uses main, mainloop;

type
  TWaveDataStorage = record
    Header: TWaveHdr;
    Data: Array [0..(10240 - 1)] of Byte;
  end;

var
  InputDeviceHandle: HWAVEIN;
  WaveData: Array [0..3] of TWaveDataStorage;
  CurrentWaveDataIndex: Integer;
  NewDataEvent: TDataAcquiredEvent;

  DisableAudioStreamingIndex: Integer;

  SoftwareLoopbackDeviceIndex: Integer;
  LoopbackLibraryHandle: THandle;
  DeviceIDToUse: Integer;

const
  // loopback.dll is written in C++
  LoopbackLibraryName = 'loopback.dll';
  InitializeLoopbackRecordingImport = '?InitializeLoopbackRecording@@YAHIIP6AXPAXI@Z@Z';
  StopLoopbackRecordingImport = '?StopLoopbackRecording@@YAHXZ';



{
  audioEnumerateDevices
  ---------------------------------------------------
  Get the number of installed audio devices.
  ---------------------------------------------------
  Returns the number of devices.
}
function audioEnumerateDevices(): Integer;
begin
  if MainForm.GetWindowsVersion > 5 then
  begin
    SoftwareLoopbackDeviceIndex := waveInGetNumDevs();
    DisableAudioStreamingIndex := SoftwareLoopbackDeviceIndex + 1;
    Result := DisableAudioStreamingIndex + 1;
  end
  else
  begin
    SoftwareLoopbackDeviceIndex := -1;
    DisableAudioStreamingIndex := waveInGetNumDevs();
    Result := DisableAudioStreamingIndex + 1;
  end;
end;



{
  audioGetDeviceName
  ---------------------------------------------------
  Retrieve the device name associated to the specified
  device index. Names are limited to 32 characters.
  ---------------------------------------------------
  Returns the device name.
}
function audioGetDeviceName(Index: Integer): String;
var
  Caps: TWaveInCaps;
begin
  if Index = DisableAudioStreamingIndex then
  begin
     Result := 'Disable audio streaming';
  end
  else
  if Index = SoftwareLoopbackDeviceIndex then
  begin
    Result := 'Software Loopback Device';
  end
  else
  begin
    waveInGetDevCaps(Index, @Caps, SizeOf(TWaveInCaps));
    Result := Caps.szPname;
  end;
end;



{
  audioGetMixerID
  ---------------------------------------------------
  Determines the mixer ID from the given device ID.
  ---------------------------------------------------
  Returns the mixer ID.
}
function audioGetMixerID(DeviceID: Integer): Integer;
var
  MixerID: LongWord;
begin
  mixerGetID(DeviceID, MixerID, MIXER_OBJECTF_WAVEIN);
  Result := MixerID;
end;





{
  audioInit
  ---------------------------------------------------
  Initializes the audio system.
  ---------------------------------------------------
  Returns TRUE on success.
}
function audioInit(DeviceID: Integer; Channels: LongWord; SampleRate: LongWord; BitRate: LongWord; BufferSize: LongWord; Callback: TDataAcquiredEvent; CallbackStdCall: TDataAcquiredEventStdCall): Boolean;
var
  Status: LongWord;
  AudioInputFormat: TWaveFormatEx;
  i : Integer;
  tempPointer: Pointer;

begin
  DeviceIDToUse := DeviceID;

  if DeviceIDToUse = DisableAudioStreamingIndex then
  begin
    Result := False;
    Exit;
  end;

  if DeviceIDToUse = SoftwareLoopbackDeviceIndex then
  begin
    NewDataEvent := Callback;

    LoopbackLibraryHandle := LoadLibrary(LoopbackLibraryName);

    tempPointer := GetProcAddress(LoopbackLibraryHandle, InitializeLoopbackRecordingImport);
    if (tempPointer <> nil) then
      InitializeLoopbackRecording := tempPointer;

    tempPointer := GetProcAddress(LoopbackLibraryHandle, StopLoopbackRecordingImport);
    if (tempPointer <> nil) then
      StopLoopbackRecording := tempPointer;

    InitializeLoopbackRecording(SampleRate, BufferSize, @AudioOnRecordCdecl);

    Result := True;
    Exit;
  end
  else
  begin
    NewDataEvent := Callback;

    AudioInputFormat.wFormatTag := WAVE_FORMAT_PCM;
    AudioInputFormat.nChannels := Channels;
    AudioInputFormat.nSamplesPerSec := SampleRate;
    AudioInputFormat.wBitsPerSample := BitRate;

    AudioInputFormat.nBlockAlign := ((Channels * BitRate) div 8);
    AudioInputFormat.nAvgBytesPerSec := (audioInputFormat.nBlockAlign * SampleRate);

    AudioInputFormat.cbSize := 0;

    CurrentWaveDataIndex := 0;

    InputDeviceHandle := 0;

    Status := waveInOpen(@InputDeviceHandle, DeviceID, @AudioInputFormat, LongWord(@audioCallback), 0, CALLBACK_FUNCTION);

    Result := (Status = MMSYSERR_NOERROR);
    if (Status = MMSYSERR_NOERROR) then
    begin
      for i := 0 to Length(WaveData) - 1 do
      begin
        WaveData[i].Header.lpData := @(WaveData[i].Data);
        WaveData[i].Header.dwBufferLength := BufferSize;
        WaveData[i].Header.dwBytesRecorded := 0;
        WaveData[i].Header.dwUser := i;
        WaveData[i].Header.dwFlags := 0;
        WaveData[i].Header.dwLoops := 0;
        WaveData[i].Header.lpNext := nil;
        WaveData[i].Header.reserved := 0;
        waveInPrepareHeader(InputDeviceHandle, @(WaveData[i].Header), SizeOf(TWAVEHDR));
      end;
    end;
  end;
end;




{
  audioStartRecording
  ---------------------------------------------------
  Starts recording to the wave bufers.
  ---------------------------------------------------
}
procedure audioStartRecording();
var
  i: Integer;
begin
  if DeviceIDToUse <> SoftwareLoopbackDeviceIndex then
  begin
    if (waveInStart(InputDeviceHandle) <> MMSYSERR_NOERROR) then
      NewDataEvent(nil, 0);
    for i := 0 to Length(WaveData) - 1 do
      addBuffer();
  end;
end;




{
  audioStopRecording
  ---------------------------------------------------
  Stops recording.
  ---------------------------------------------------
}
procedure audioStopRecording();
begin
  if DeviceIDToUse = SoftwareLoopbackDeviceIndex then
    StopLoopbackRecording()
  else
    waveInStop(InputDeviceHandle);
end;




{
  audioFinish
  ---------------------------------------------------
  Stops all audio functions.
  ---------------------------------------------------
}
procedure audioFinish();
var
  i : Integer;
begin
  if DeviceIDToUse = DisableAudioStreamingIndex then
  begin
    Exit;
  end
  else
  if DeviceIDToUse = SoftwareLoopbackDeviceIndex then
  begin
    InitializeLoopbackRecording := nil;
    StopLoopbackRecording := nil;
    Sleep(1000);
    FreeLibrary(LoopbackLibraryHandle);
  end
  else
  begin
    for i := 0 to Length(WaveData) - 1 do
    begin
      waveInUnprepareHeader(InputDeviceHandle, @(WaveData[i].Header), SizeOf(TWAVEHDR));
    end;
    waveInClose(InputDeviceHandle);
  end;
end;




{
  audioCallback
  ---------------------------------------------------
  Callback function, gets called when a buffer has
  been filled with audio data.
  ---------------------------------------------------
}
procedure audioCallback(hwi: HWAVEIN; uMsg: LongWord; dwInstance, dwParam1, dwParam2: LongWord);
begin
  if (uMsg = WIM_DATA) then
  begin
    NewDataEvent(PWAVEHDR(dwParam1).lpData, PWAVEHDR(dwParam1).dwBytesRecorded);
    addBuffer();
  end;
end;



{
  addBuffer
  ---------------------------------------------------
  Put another buffer to be filled into the queue.
  ---------------------------------------------------
}
procedure addBuffer();
begin
  audioGetNextWaveDataIndex();
  if (waveInAddBuffer(InputDeviceHandle, @(WaveData[CurrentWaveDataIndex].Header), SizeOf(TWAVEHDR)) <> MMSYSERR_NOERROR) then
    NewDataEvent(nil, 0);
end;




{
  audioGetNextWaveDataIndex
  ---------------------------------------------------
  Get the next unused buffer index.
  ---------------------------------------------------
  Returns a buffer index.
}
function audioGetNextWaveDataIndex(): Integer;
begin
  if (CurrentWaveDataIndex = Length(WaveData) - 1) then
    CurrentWaveDataIndex := 0
  else
    Inc(CurrentWaveDataIndex);

  Result := CurrentWaveDataIndex;
end;


end.
