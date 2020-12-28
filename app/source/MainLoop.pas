{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  MainLoop.pas - worker thread

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}

unit mainloop;

interface

uses
  Classes, Windows, SysUtils, Forms, Controls, Graphics, TrayIcon, ExtCtrls, DateUtils;

type
  MainLoopThread = class(TThread)
  private
    InputFileMapping: THandle;
    OutputFileMapping: THandle;
    InputMemory : Pointer;
    OutputMemory : Pointer;
    InputImageCounter : LongWord;
    InputImageLastCount : LongWord;
    OutputButtonCode : Integer;
    OutputButtonCounter : Integer;

    SideShowAvailable : Boolean;

  protected
    procedure Execute; override;
    function GetCurrentFrameNumber: LongWord;
    procedure AcquireFrame;
    procedure CycleSideShowGadgets();
    function SendAndReceive(): Boolean;
    function ReceiveBuffer(DataBuffer: Pointer; BufferSize: Cardinal): Boolean;
    function Sendbuffer(DataBuffer: Pointer; BufferSize: Cardinal): Boolean;
    function CheckForConnection(): Boolean;
    procedure InitializeAudio();
    procedure ApplyPcSettings();
    procedure FormatPCSettingsForSending();
  end;


procedure AudioOnRecord(Data: Pointer; DataSize: LongWord);
procedure AudioOnRecordCdecl(Data: Pointer; DataSize: LongWord); cdecl;

const
  HeaderMagic: Cardinal = $AFFE0600;
  ModeNone: Cardinal = 0;
  ModeUsb: Cardinal = 1;
  ModeWlan: Cardinal = 2;

  COM_FLAGS_CONTAINS_IMAGE_DATA: Cardinal = $00000001;
  COM_FLAGS_CONTAINS_AUDIO_DATA: Cardinal = $00000002;
  COM_FLAGS_CONTAINS_SETTINGS_DATA: Cardinal = $00000004;

  COM_FLAGS_ENABLE_OSK: Cardinal = $00000008;

  COM_FLAGS_IMAGE_IS_JPEG: Cardinal = $00000010;
  COM_FLAGS_IMAGE_IS_PNG: Cardinal = $00000020;
  COM_FLAGS_IMAGE_IS_UNCOMPRESSED: Cardinal = $00000040;

  COM_FLAGS_IMAGE_IS_ROTATED_90_DEG: Cardinal = $00000100;
  COM_FLAGS_IMAGE_IS_ROTATED_180_DEG: Cardinal = $00000200;
  COM_FLAGS_IMAGE_IS_ROTATED_270_DEG: Cardinal = $00000400;

  COM_FLAGS_AUDIO_11025_HZ: Cardinal = $00001000;
  COM_FLAGS_AUDIO_22050_HZ: Cardinal = $00002000;
  COM_FLAGS_AUDIO_44100_HZ: Cardinal = $00004000;

  COM_FLAGS_AUDIO_CHUNK_2240: Cardinal = $00010000;
  COM_FLAGS_AUDIO_CHUNK_2688: Cardinal = $00020000;

  COM_FLAGS_IMAGE_CLEAR_SCREEN: Cardinal = $00100000;

  COM_FLAGS_FORCE_UPDATE: Cardinal = $01000000;

type
  TSettings = packed record
    DeviceIndex: Cardinal;
    DeviceUseSideShow: Cardinal;
    ScreenMode: Cardinal;
    ScreenRotation: Cardinal;
    ScreenViewport: Cardinal;
    Quality: Cardinal;
    ScreenNoCompression: Cardinal;
    UpdateInterval: Cardinal;
    SoundEnabled: Cardinal;
    SoundSampleRate: Cardinal;
    ControlIndex: Cardinal;
    ControlCount: Cardinal;
    DisplayDeviceIndex: Cardinal;
    DisplayDeviceCount: Cardinal;
    DisplayEnable: Cardinal;
    DisplayAutoselect: Cardinal;
    PresetIndex: Cardinal;
    PresetCount: Cardinal;
    PresetDoSet: Cardinal;
  end;

  TSettingsForSending = packed record
    Settings: TSettings;
    ExtraData: Array[0..(32 * 50)] of Byte;
    ExtraDataSize: Cardinal;
  end;


var
  LastSideShowCyclingTime: TDateTime;
  TimeSinceLastMouseMove: LongWord;
  ConnectionMode: Cardinal;
  SendPCSettings: Boolean;
  RestartAudio: Boolean;

  PCSettings: TSettingsForSending;
  PCSettingsChanged: Boolean;
  

implementation

uses
  main, wlan, usb, display, control, graphic, audio, config;


type
  TFrameHeader = packed record
    Magic: Cardinal;
    Flags: Cardinal;
    ImageSize: Cardinal;
    ExtraDataSize: Cardinal;
  end;
  PFrameHeader = ^TFrameHeader;

  TResponse = packed record
    Magic: Cardinal;
    Flags: Cardinal;
    Buttons: Cardinal;
    Analog: Cardinal;
  end;
  PResponse = ^TResponse;

  TResponseSettingsBuffer = packed record
    Response: TResponseBuffer;
    Settings: TSettings;
  end;

var
  AudioBuffer: Array[0..(40 * 1024)] of Byte;
  ResponseBuffer: TResponseSettingsBuffer;
  StartTime: TDateTime;
  EndTime: TDateTime;
  MilliSeconds: Word;
  
  ClientConnected : Boolean;
  DataSize       : LongWord;
  AudioDataSize : LongWord;

  UseSideShow: Boolean;

  NewAudioData: Boolean;
  WritingAudioData: Boolean;

  SideShowCurrentScreen: LongWord;

  AudioAvailable: Boolean;
  AudioRecordingError: Boolean;

  FrameHeader: TFrameHeader;

  WlanModeEnabled: Boolean;
  UsbModeEnabled: Boolean;

  AudioBitRate: Cardinal;
  AudioBufferSize: Cardinal;

  ImageStreamingDisabled: Boolean;
  



{
  ApplyPcSettings
  ---------------------------------------------------
  Make the settings received from the PSP current.
  ---------------------------------------------------
}
procedure MainLoopThread.ApplyPcSettings;
begin
  if (PcSettings.Settings.PresetDoSet = 1) then
  begin
    MainForm.MenuPresets[PcSettings.Settings.PresetIndex].Click();
  end
  else begin
    MainForm.MenuRotation[PcSettings.Settings.ScreenRotation].Click();
    MainForm.MenuScreen[PcSettings.Settings.ScreenMode].Click();
    MainForm.MenuUpdate[PcSettings.Settings.UpdateInterval].Click();
    MainForm.MenuQuality[4 - PcSettings.Settings.Quality + 2].Click();

    MainForm.MenuControl[PcSettings.Settings.ControlIndex + 2].Click();
    MainForm.MenuDisplay[PcSettings.Settings.DisplayDeviceIndex + MenuDisplayDeviceFirstIndex].Click();

    if (PcSettings.Settings.ScreenViewport > 6) then
      Inc(PcSettings.Settings.ScreenViewport);
    if (PcSettings.Settings.ScreenViewport > 12) then
      Inc(PcSettings.Settings.ScreenViewport);
    MainForm.MenuViewportSize[PcSettings.Settings.ScreenViewport].Click();

    MainForm.MenuSoundEnabled.Checked := (PCSettings.Settings.SoundEnabled = 1);

    if (ConnectionMode = ModeWlan) then
      MainForm.MenuSoundWlan[PCSettings.Settings.SoundSampleRate].Click()
    else
      MainForm.MenuSoundUsb[PCSettings.Settings.SoundSampleRate].Click();

    MainForm.MenuEnableImageStreaming.Checked := (PCSettings.Settings.DisplayEnable = 1);
    MainForm.MenuFollowMouseAcrossScreens.Checked := (PCSettings.Settings.DisplayAutoselect = 1);
    MainForm.MenuQualityUncompressed.Checked := (PCSettings.Settings.ScreenNoCompression = 1);
    MainForm.MenuUseSideshow.Checked := MainForm.MenuUseSideshow.Enabled and (PCSettings.Settings.DeviceUseSideShow = 1);
  end;

  ForceTransmission := True;

  PCSettingsChanged := False;
end;




{
  FormatPcSettingsForSending
  ---------------------------------------------------
  Retrieve the current settings and convert them
  in the PSP format.
  ---------------------------------------------------
}
procedure MainLoopThread.FormatPCSettingsForSending();
var
  i: Integer;
  ExtraDataBuffer: ShortString;
begin
  with PCSettings do
  begin
    ExtraDataSize := 0;
    ZeroMemory(@ExtraData[0], 32 * 50);

    for i := 0 to MainForm.MenuSoundWlan.Count - 1 do
    begin
      if ((ConnectionMode = ModeWlan) and MainForm.MenuSoundWlan[i].Checked)
          or ((ConnectionMode = ModeUsb) and MainForm.MenuSoundUsb[i].Checked) then
      begin
        Settings.SoundSampleRate := i;
        Break;
      end;
    end;

    for i := 0 to MainForm.MenuRotation.Count - 1 do
    begin
      if MainForm.MenuRotation[i].Checked then
      begin
        Settings.ScreenRotation := i;
        Break;
      end;
    end;

    for i := 0 to MainForm.MenuScreen.Count - 4 do
    begin
      if MainForm.MenuScreen[i].Checked then
      begin
        Settings.ScreenMode := i;
        Break;
      end;
    end;

    for i := 0 to MainForm.MenuUpdate.Count - 3 do
    begin
      if MainForm.MenuUpdate[i].Checked then
      begin
        Settings.UpdateInterval := i;
        Break;
      end;
    end;

    for i := 2 to MainForm.MenuQuality.Count - 3 do
    begin
      if MainForm.MenuQuality[i].Checked then
      begin
        Settings.Quality := 4 - (i - 2);
        Break;
      end;
    end;

    for i := 0 to MainForm.MenuViewportSize.Count - 1 do
    begin
      if MainForm.MenuViewportSize[i].Checked then
      begin
        Settings.ScreenViewport := i;
        if (Settings.ScreenViewport > 6) then
          Dec(Settings.ScreenViewport);
        if (Settings.ScreenViewport > 12) then
          Dec(Settings.ScreenViewport);
        Break;
      end;
    end;

    for i := MenuDisplayDeviceFirstIndex to MainForm.MenuDisplay.Count - 1 do
    begin
      if MainForm.MenuDisplay[i].Checked then
      begin
        Settings.DisplayDeviceIndex := i - MenuDisplayDeviceFirstIndex;
      end;
      ZeroMemory(@ExtraDataBuffer[1], 32);
      ExtraDataBuffer := StringReplace(MainForm.MenuDisplay[i].Caption, '&', '', [rfReplaceAll]);
      CopyMemory(@ExtraData[ExtraDataSize], @ExtraDataBuffer[1], 31);
      Inc(ExtraDataSize, 32);
    end;
    Settings.DisplayDeviceCount := MainForm.MenuDisplay.Count - MenuDisplayDeviceFirstIndex;

    for i := 2 to MainForm.MenuControl.Count - 3 do
    begin
      if MainForm.MenuControl[i].Checked then
      begin
        Settings.ControlIndex := i - 2;
      end;
      ZeroMemory(@ExtraDataBuffer[1], 32);
      ExtraDataBuffer := StringReplace(MainForm.MenuControl[i].Caption, '&', '', [rfReplaceAll]);
      CopyMemory(@ExtraData[ExtraDataSize], @ExtraDataBuffer[1], 31);
      Inc(ExtraDataSize, 32);
    end;
    Settings.ControlCount := MainForm.MenuControl.Count - 4;

    for i := 0 to Length(PresetFiles) - 1 do
    begin
      ZeroMemory(@ExtraDataBuffer[1], 32);
      ExtraDataBuffer := PresetFiles[i].Name;
      CopyMemory(@ExtraData[ExtraDataSize], @ExtraDataBuffer[1], 31);
      Inc(ExtraDataSize, 32);
    end;
    Settings.PresetCount := Length(PresetFiles);
    Settings.PresetIndex := 0;
    Settings.PresetDoSet := 0;

    if (MainForm.MenuSoundEnabled.Checked) then
      Settings.SoundEnabled := 1
    else
      Settings.SoundEnabled := 0;

    if (MainForm.MenuFollowMouseAcrossScreens.Checked) then
      Settings.DisplayAutoselect := 1
    else
      Settings.DisplayAutoselect := 0;

    if (MainForm.MenuEnableImageStreaming.Checked) then
      Settings.DisplayEnable := 1
    else
      Settings.DisplayEnable := 0;

    if (MainForm.MenuQualityUncompressed.Checked) then
      Settings.ScreenNoCompression := 1
    else
      Settings.ScreenNoCompression := 0;

    if (MainForm.MenuUseSideshow.Checked) then
      Settings.DeviceUseSideShow := 1
    else
      Settings.DeviceUseSideShow := 0;
  end;

end;


{
  GetCurrentFrameNumber
  ---------------------------------------------------
  Function to determine if the screen has changed
  since the last frame. Asks the SideShow driver or
  the PSPdisp display driver for information.
  ---------------------------------------------------
  Returns the next frame number if the content has
  changed and the same number when not.
}
function MainLoopThread.GetCurrentFrameNumber: LongWord;
begin
  Result := InputImageCounter + 1;

  if (UseSideShow) then
  begin
    InputFileMapping := OpenFileMapping(FILE_MAP_READ, False, 'Global\a217007d-6605-4289-bbc6-b4d32ff10bd7');
    OutputFileMapping := OpenFileMapping(FILE_MAP_ALL_ACCESS, False, 'Global\45f7a9bc-711e-4012-9d8f-d4d89ec8b00b');

    InputMemory := MapViewOfFile(InputFileMapping, FILE_MAP_READ, 0, 0, 0);
    OutputMemory := MapViewOfFile(OutputFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    SideShowAvailable := (InputMemory <> nil) and (OutputMemory <> nil);
    if (SideShowAvailable) then
    begin
      CopyMemory(@Result, InputMemory, 4);
      SideShowCurrentScreen := Result SHR 24;

      if ForceTransmission then
        Result := InputImageCounter + 1
      else
        Result := Result and $00FFFFFF;

      if (SideShowCurrentScreen = 255) then
      begin
        MainForm.MenuUseSideshowClick(MainForm);
        Inc(OutputButtonCounter);
        TSideshowOutput(OutputMemory^).Counter := OutputButtonCounter;
        TSideshowOutput(OutputMemory^).Button := SIDESHOW_FORCE_REFRESH;
      end;
    end;
  end
  else if (MainForm.MenuReduceCpuLoad.Checked) then
  begin
    if (ForceTransmission or DispHasScreenChanged) then
      Result := InputImageCounter + 1
    else
      Result := InputImageCounter;
  end;

  ForceTransmission := False;
end;




{
  CycleSideShowGadgets
  ---------------------------------------------------
  Commands the SideShow driver to switch to the
  next main menu entry.
  ---------------------------------------------------
}
procedure MainLoopThread.CycleSideShowGadgets();
begin
  if MainForm.MenuCycleSideShowGadgets.Checked then
  begin
    if (SideShowCurrentScreen = 0) then
    begin
      if SecondsBetween(Time(), LastSideShowCyclingTime) > StrToIntDef(MainForm.OptionsSideShowInterval.Text, 30) then
      begin
        LastSideShowCyclingTime := Time();
        Inc(OutputButtonCounter);
        TSideshowOutput(OutputMemory^).Counter := OutputButtonCounter;

        if (MainForm.OptionsSideShowCycleDirection.ItemIndex = 0) then
          TSideshowOutput(OutputMemory^).Button := SIDESHOW_BUTTON_PREVIOUS
        else if (MainForm.OptionsSideShowCycleDirection.ItemIndex = 1) then
          TSideshowOutput(OutputMemory^).Button := SIDESHOW_BUTTON_NEXT;
      end;
    end;
  end;
end;





{
  SendBuffer
  ---------------------------------------------------
  Send data with the current connection mode.
  ---------------------------------------------------
  Return true on success.
}
function MainLoopThread.Sendbuffer(DataBuffer: Pointer; BufferSize: Cardinal): Boolean;
begin
  Result := False;

  if (ConnectionMode = ModeUsb) then
  begin
    Result := UsbSendBuffer(DataBuffer, BufferSize);
    Exit;
  end;

  if (ConnectionMode = ModeWlan) then
  begin
    Result := (ClientConnected and (WlanSendBuffer(DataBuffer, BufferSize)));
    if (not Result) then
    begin
      WlanCloseClient();
      ClientConnected := False;
    end;
    Exit;
  end;
end;





{
  ReceiveBuffer
  ---------------------------------------------------
  Receive data with the current connection mode.
  ---------------------------------------------------
  Return true on success.
}
function MainLoopThread.ReceiveBuffer(DataBuffer: Pointer; BufferSize: Cardinal): Boolean;
begin
  if (ConnectionMode = ModeUsb) then
  begin
    Result := UsbReceiveData(DataBuffer, BufferSize);
    Exit;
  end;

  if (ConnectionMode = ModeWlan) then
  begin
    Result := (ClientConnected and (WlanReceiveData(DataBuffer, BufferSize)));
    if (not Result) then
    begin
      WlanCloseClient();
      ClientConnected := False;
    end;
    Exit;
  end;

  Result := False;
end;







{
  SendAndReceive
  ---------------------------------------------------
  All PSP communication is done here.
  ---------------------------------------------------
  Return true on success.
}
function MainLoopThread.SendAndReceive(): Boolean;
var
  Success: Boolean;
begin
  // Send frame header
  Success := SendBuffer(@FrameHeader, sizeof(TFrameHeader));

  if (Success) then
  begin
    // Send frame data
    Success := SendBuffer(@JpegBuffer, DataSize);

    if (Success) then
    begin
      // Receive response
      ZeroMemory(@ResponseBuffer, SizeOf(TResponseBuffer));

      // Send the settings every frame in USB mode
      if (ConnectionMode = ModeUsb) then
        Success := ReceiveBuffer(@ResponseBuffer, sizeof(TResponseSettingsBuffer))
      else
        Success := ReceiveBuffer(@ResponseBuffer, sizeof(TResponseBuffer));

      if (ResponseBuffer.Response.Magic = HeaderMagic) then
      begin
        if (ResponseBuffer.Response.Flags and COM_FLAGS_CONTAINS_SETTINGS_DATA > 0) then
        begin
          if (ConnectionMode = ModeWlan) then
            Success := ReceiveBuffer(@PcSettings, sizeof(TSettings))
          else
            CopyMemory(@PcSettings, @ResponseBuffer.Settings, sizeof(TSettings));
          PCSettingsChanged := True;
        end;

        if (ResponseBuffer.Response.Flags and COM_FLAGS_FORCE_UPDATE > 0) then
          ForceTransmission := True;

        if (UseSideShow) then
        begin
          if (SideShowAvailable) then
          begin
            CtrlProcessSideshowInput(ResponseBuffer.Response.ControlBuffer, OutputButtonCode);
            if (OutputButtonCode > -1) then
            begin
              // Reset SideShow cycling timer
              LastSideShowCyclingTime := Time();

              Inc(OutputButtonCounter);
              TSideshowOutput(OutputMemory^).Counter := OutputButtonCounter;
              TSideshowOutput(OutputMemory^).Button := OutputButtonCode;
            end
            else
              CycleSideShowGadgets();
          end;
        end
        else if MainForm.MenuControlEnabled.Checked then
          CtrlProcessInput(ResponseBuffer.Response.ControlBuffer);
      end;
    end;
  end;

  Result := Success;
end;





{
  AcquireFrame
  ---------------------------------------------------
  Get next frame ready for sending to the PSP.
  ---------------------------------------------------
}
procedure MainLoopThread.AcquireFrame;
var
  FrameNum: LongWord;
begin
  if (not ImageStreamingDisabled) then
  begin
    FrameNum := GetCurrentFrameNumber();
    if (InputImageCounter = FrameNum) then
    begin
      DataSize := 0;
      Exit;
    end
    else
      InputImageCounter := FrameNum;

    if UseSideShow then
    begin
      // SideShow mode
      if SideShowAvailable then
        GraphicSetBitmapBuffer(Pointer(Integer(InputMemory) + 4 + 54))
      else
        GraphicDrawSideShowNotAvailableMessage();

      DataSize := GraphicCompress(480, 272);
    end
    else begin
      // Desktop mirroring
      GraphicDrawScreen;
      DataSize := GraphicCompress(TargetWidth, TargetHeight);
    end;

    if (DataSize = 480 * 272 * 3) then
      FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_IMAGE_IS_UNCOMPRESSED
    else
      FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_IMAGE_IS_JPEG;

    FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_CONTAINS_IMAGE_DATA;

    if (Rotation = ROTATION_90) then
      FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_IMAGE_IS_ROTATED_90_DEG;

    if (Rotation = ROTATION_180) then
      FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_IMAGE_IS_ROTATED_180_DEG;

    if (Rotation = ROTATION_270) then
      FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_IMAGE_IS_ROTATED_270_DEG;
  end
  else
  begin
    DataSize := 0;
    Frameheader.Flags := FrameHeader.Flags or COM_FLAGS_IMAGE_CLEAR_SCREEN;
  end;
end;






{
  AudioOnRecord
  ---------------------------------------------------
  Callback for the audio recording functions.
  ---------------------------------------------------
}
procedure AudioOnRecord(Data: Pointer; DataSize: LongWord);
begin
  WritingAudioData := True;
  if Data = nil then
  begin
    AudioRecordingError := True;
    AudioAvailable := False;
    ZeroMemory(@(AudioBuffer[0]), DataSize);
    if (MainForm.MenuEnabled.Checked) then
      MainForm.ShowBalloonHint('PSPdisp: Error', 'An error occurred while capturing audio. Disable and enable PSPdisp to enable audio streaming again.', bfError);
  end
  else
    CopyMemory(@(AudioBuffer[0]), Data, DataSize);
  NewAudioData := True;
  WritingAudioData := False;
end;




{
  AudioOnRecordCdecl
  ---------------------------------------------------
  Cdecl wrapper for use in the loopback.dll.
  ---------------------------------------------------
}
procedure AudioOnRecordCdecl(Data: Pointer; DataSize: LongWord);
begin
  AudioOnRecord(Data, DataSize);
end;




{
  InitializeAudio
  ---------------------------------------------------
  Prepare the audio recording.
  ---------------------------------------------------
}
procedure MainLoopThread.InitializeAudio();
begin
  if (ConnectionMode = ModeWlan) then
  begin
    if (MainForm.MenuSoundWlan11.Checked) then
    begin
      AudioBitRate := 11025;
      AudioBufferSize := AUDIO_BUFFER_SIZE_2240;
    end
    else if (MainForm.MenuSoundWlan22.Checked) then
    begin
      AudioBitRate := 22050;
      AudioBufferSize := AUDIO_BUFFER_SIZE_2240;
    end
    else if (MainForm.MenuSoundWlan44.Checked) then
    begin
      AudioBitRate := 44100;
      AudioBufferSize := AUDIO_BUFFER_SIZE_2688;
    end
  end
  else begin
    if (MainForm.MenuSoundUsb11.Checked) then
    begin
      AudioBitRate := 11025;
      AudioBufferSize := AUDIO_BUFFER_SIZE_2240;
    end
    else if (MainForm.MenuSoundUsb22.Checked) then
    begin
      AudioBitRate := 22050;
      AudioBufferSize := AUDIO_BUFFER_SIZE_2240;
    end
    else if (MainForm.MenuSoundUsb44.Checked) then
    begin
      AudioBitRate := 44100;
      AudioBufferSize := AUDIO_BUFFER_SIZE_2688;
    end
  end;

  AudioAvailable := audioInit(MainForm.OptionsAudioDevice.ItemIndex, 2, AudioBitRate, 16, AudioBufferSize, AudioOnRecord, AudioOnRecordCdecl);

  if AudioAvailable then
    audioStartRecording();
end;





{
  CheckForConnection
  ---------------------------------------------------
  Look for a connected device.
  ---------------------------------------------------
  Return true on success.
}
function MainLoopThread.CheckForConnection(): Boolean;
begin
  if (UsbModeEnabled and UsbCheckForConnection(True)) then
  begin
    ConnectionMode := ModeUsb;
    Result := True;
  end
  else if (WlanModeEnabled and WlanWaitForClient()) then
  begin
    ConnectionMode := ModeWlan;
    Result := True;
  end
  else
    Result := False;

  // Select() blocks in WLAN mode, wait a bit if not enabled
  if (not WlanModeEnabled) then
    Sleep(1000);

  ClientConnected := True;
end;




{
  Execute
  ---------------------------------------------------
  This thread gets created and executed when the
  communication is set to "enabled". Afterwards
  it gets destroyed.
  Be careful with calling VCL methods from here as
  they are not necesseraly thread-safe.
  ---------------------------------------------------
}
procedure MainLoopThread.Execute;
begin
  ConnectionMode := ModeNone;
  MainLoopRunning := True;
  MainForm.SetIcon(False);
  PCSettingsChanged := False;
  RestartAudio := False;

  // Reset fps counter
  Fps := 0;
  FrameSize := 0;
  TotalFrames := 0;
  TotalFrameSize := 0;

  // Init audio variables
  AudioRecordingError := False;

  // Init memory mapped file for SideShow
  InputImageCounter := 0;
  InputImageLastCount := 10000000;
  OutputButtonCode := 0;
  OutputButtonCounter := 0;
  SideShowCurrentScreen := 0;
  LastSideShowCyclingTime := Time();
  SideShowAvailable := False;

  // Check the requested connection modes
  WlanModeEnabled := (MainForm.MenuUseWirelessLAN.Checked or MainForm.MenuUseBoth.Checked);
  UsbModeEnabled := (MainForm.MenuUseUSB.Checked or MainForm.MenuUseBoth.Checked);

  // Retrieve monitor position and size
  MainForm.GetMonitorParameters;

  // Reset last mouse move time
  TimeSinceLastMouseMove := 0;

  // Initialize USB
  if (UsbModeEnabled) then 
    UsbInit();

  // Initialize WLAN
  if (WlanModeEnabled) then
    WlanStartServer();

  // Loop while enabled
  while MainForm.MenuEnabled.Checked do
  begin
    // Loop until connection is established
    if (MainForm.MenuEnabled.Checked and CheckForConnection()) then
    begin
      // Make "Enabled" clickable again
      MainForm.MenuEnabled.Enabled := True;
      MainForm.SetIcon(True);        

      InitializeAudio();

      SendPCSettings := True;

      while (MainForm.MenuEnabled.Checked and ClientConnected) do
      begin
        if PCSettingsChanged then
          Synchronize(ApplyPcSettings);

        if RestartAudio then
        begin
          // Delete Audio
          while (WritingAudioData) do
            Sleep(10);

          if AudioAvailable or AudioRecordingError then
            audioStopRecording();

          audioFinish();

          InitializeAudio();

          RestartAudio := False;
        end;

        UseSideShow := MainForm.MenuUseSideshow.Checked;
        ImageStreamingDisabled := not MainForm.MenuEnableImageStreaming.Checked;

        // Start timer
        StartTime := Time();

        // Construct header
        FrameHeader.Magic := HeaderMagic;
        FrameHeader.ExtraDataSize := 0;
        FrameHeader.Flags := 0;
        if (EnableKeyboard) then
          FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_ENABLE_OSK;

        // Get frame
        AcquireFrame();

        FrameHeader.ImageSize := DataSize;

        // Wait for the audio frame to complete
        if (AudioAvailable and MainForm.MenuSoundEnabled.Checked) then
        begin
          while (WritingAudioData or not NewAudioData) do
            Sleep(1);

          NewAudioData := False;
          CopyMemory(@(JpegBuffer[DataSize]), @(AudioBuffer[0]), AudioBufferSize);

          FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_CONTAINS_AUDIO_DATA;

          if (AudioBitRate = 11025) then
          begin
            FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_AUDIO_11025_HZ;
            FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_AUDIO_CHUNK_2240;
          end
          else if (AudioBitRate = 22050) then
          begin
            FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_AUDIO_22050_HZ;
            FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_AUDIO_CHUNK_2240;
          end
          else if (AudioBitRate = 44100) then
          begin
            FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_AUDIO_44100_HZ;
            FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_AUDIO_CHUNK_2688;
          end;

          AudioDataSize := AudioBufferSize;
          DataSize := DataSize + AudioDataSize;
        end;

        if (SendPCSettings) then
        begin
          FormatPCSettingsForSending();
          CopyMemory(@(JpegBuffer[DataSize]), @PCSettings, SizeOf(TSettings) + PCSettings.ExtraDataSize);
          DataSize := DataSize + SizeOf(TSettings) + PCSettings.ExtraDataSize;
          SendPCSettings := False;
          FrameHeader.ExtraDataSize := SizeOf(TSettings) + PCSettings.ExtraDataSize;
          FrameHeader.Flags := FrameHeader.Flags or COM_FLAGS_CONTAINS_SETTINGS_DATA;
        end;

        // Indicate that data transimission is in progress
        TransmissionFinished := False;

        // send data
        ClientConnected := SendAndReceive();

        // Unmap files
        if UseSideShow then
        begin
          UnMapViewOfFile(InputMemory);
          UnMapViewOfFile(OutputMemory);

          if SideShowAvailable then
          begin
            CloseHandle(InputFileMapping);
            CloseHandle(OutputFileMapping);
          end;
        end;

        // Increase fps counter
        Inc(Fps);
        frameSize := frameSize + DataSize; // <- This will overflow
        InputImageLastCount := InputImageCounter;

        EndTime := Time();
        MilliSeconds := MilliSecondsBetween(EndTime, StartTime);
        TimeSinceLastMouseMove := TimeSinceLastMouseMove + MilliSeconds;

        if ((not MainForm.MenuSoundEnabled.Checked) and AudioAvailable and (MilliSeconds < Delay)) then
          Sleep(Delay - MilliSeconds);

        TransmissionFinished := True;
      end;

      if ((not ClientConnected) and (ConnectionMode = ModeWlan) and (MainForm.OptionsWlanNotifyConnect.Checked)) then
        MainForm.ShowBalloonHint('PSPdisp: Client connection lost', 'The connection to the PSP client has been lost.', bfInfo);

      MainForm.SetIcon(False);
      MainForm.MenuEnabled.Enabled := False;

      // Delete Audio
      while (WritingAudioData) do
        Sleep(10);

      if AudioAvailable or AudioRecordingError then
        audioStopRecording();

      audioFinish();

      UsbCloseConnection();

      MainForm.MenuEnabled.Enabled := True;
    end;
  end;

  WlanShutdownServer();

  MainForm.MenuEnabled.Checked := False;

  MainForm.ShowBalloonHint('', '', bfNone);

  // Inidicate that the thread has finished
  MainLoopRunning := False;

  MainForm.SetIcon(False);
  MainForm.MenuEnabled.Enabled := True;
end;




end.
