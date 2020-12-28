{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  config.pas - config and preset file handling

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}

unit config;

interface

uses Windows, Sysutils, IniFiles;

type
  TPresetFile = record
    Name: String;
    Filename: String;
  end;

  TPresetFileArray = Array of TPresetFile;

var
  PresetFiles : TPresetFileArray;

procedure LoadConfig();
procedure SaveConfig();
procedure LoadConfigFile(Filename: String; IsPreset: Boolean);
procedure SaveConfigFile(Filename: String; IsPreset: Boolean; PresetName: String);
function ConfigEnumPresetFiles(): TPresetFileArray;
procedure LoadPreset(Index: Integer);
procedure SavePreset(Index: Integer; Name: String; Filename: String);


implementation

uses main, compress, graphic;




{
  CtrlEnumControlFiles
  ---------------------------------------------------
  Searches for all available control files
  ---------------------------------------------------
  Returns an array of control files.
}
function ConfigEnumPresetFiles() : TPresetFileArray;
var
  SearchRec : TSearchRec;
  ControlFile : TIniFile;
  NewControlFile : TPresetFile;
  Path : String;
begin
  SetLength(PresetFiles, 0);

  Path := MainForm.GetAppDataFolder + '\PSPdisp\preset\';

  if (FindFirst(Path + '*.preset', faAnyFile, SearchRec) = 0) then
  begin
    repeat
      ControlFile := TIniFile.Create(Path + SearchRec.Name);
      NewControlFile.Name := ControlFile.ReadString('General', 'Name', '');
      if NewControlFile.Name <> '' then
      begin
        NewControlFile.FileName := SearchRec.Name;
        SetLength(PresetFiles, Length(PresetFiles) + 1);
        PresetFiles[Length(PresetFiles) - 1] := NewControlFile;
      end;
      ControlFile.Free;
    until
      FindNext(SearchRec) <> 0;
  end;

  Result := PresetFiles;
end;



procedure LoadPreset(Index: Integer);
begin
  LoadConfigFile(PresetFiles[Index].Filename, True);
end;


procedure SavePreset(Index: Integer; Name: String; Filename: String);
begin
  SaveConfigFile(Filename, True, Name);
end;


procedure LoadConfig();
begin
  if (InstanceNumber = 0) then
    LoadConfigFile('PSPdisp.ini', False)
  else
    LoadConfigFile('PSPdisp_' + IntToStr(InstanceNumber) + '.ini', False);
end;


procedure SaveConfig();
begin
  if (InstanceNumber = 0) then
    SaveConfigFile('PSPdisp.ini', False, '')
  else
    SaveConfigFile('PSPdisp_' + IntToStr(InstanceNumber) + '.ini', False, '');
end;


{
  LoadConfig
  ---------------------------------------------------
  Get settings from the ini-file.
  ---------------------------------------------------
}
procedure LoadConfigFile(Filename: String; IsPreset: Boolean);
var
  DeviceIndex : Integer;
  ControlFileIndex : Integer;
  Path : String;
  AudioDeviceIndex: Integer;
begin
  with (MainForm) do
  begin
    if (IsPreset) then
    begin
      Path := GetAppDataFolder + '\PSPdisp\preset\';
      ForceDirectories(Path);
    end
    else begin
      Path := GetAppDataFolder + '\PSPdisp\';
      ForceDirectories(Path);
    end;

    if (not IsPreset) then
      FirstStart := not FileExists(Path + Filename);
    ConfigFile := TIniFile.Create(Path + Filename);

    if (not IsPreset) then
    begin
      MenuQualityHighest.Tag := ConfigFile.ReadInteger( 'Quality Options', 'Highest', MenuQualityHighest.Tag);
      MenuQualityHigh.Tag := ConfigFile.ReadInteger( 'Quality Options', 'High', MenuQualityHigh.Tag);
      MenuQualityMedium.Tag := ConfigFile.ReadInteger( 'Quality Options', 'Medium', MenuQualityMedium.Tag);
      MenuQualityLow.Tag := ConfigFile.ReadInteger( 'Quality Options', 'Low', MenuQualityLow.Tag);
      MenuQualityLowest.Tag := ConfigFile.ReadInteger( 'Quality Options', 'Lowest', MenuQualityLowest.Tag);
      MenuQualitySideShow.Tag := ConfigFile.ReadInteger( 'Quality Options', 'SideShow', MenuQualitySideShow.Tag);

      MenuUpdateSleep1.Tag := ConfigFile.ReadInteger('Delay Options', 'Shortest', MenuUpdateSleep1.Tag);
      MenuUpdateSleep2.Tag := ConfigFile.ReadInteger('Delay Options', 'Short', MenuUpdateSleep2.Tag);
      MenuUpdateSleep3.Tag := ConfigFile.ReadInteger('Delay Options', 'Medium', MenuUpdateSleep3.Tag);
      MenuUpdateSleep4.Tag := ConfigFile.ReadInteger('Delay Options', 'Long', MenuUpdateSleep4.Tag);
      MenuUpdateSleep5.Tag := ConfigFile.ReadInteger('Delay Options', 'Longest', MenuUpdateSleep5.Tag);
      MenuUpdateSleep1.Caption := 'Min. ' + IntToStr(MenuUpdateSleep1.Tag) + ' ms per frame';
      MenuUpdateSleep2.Caption := 'Min. ' + IntToStr(MenuUpdateSleep2.Tag) + ' ms per frame';
      MenuUpdateSleep3.Caption := 'Min. ' + IntToStr(MenuUpdateSleep3.Tag) + ' ms per frame';
      MenuUpdateSleep4.Caption := 'Min. ' + IntToStr(MenuUpdateSleep4.Tag) + ' ms per frame';
      MenuUpdateSleep5.Caption := 'Min. ' + IntToStr(MenuUpdateSleep5.Tag) + ' ms per frame';

      OptionsQualityHighest.Text := IntToStr(MenuQualityHighest.Tag);
      OptionsQualityHigh.Text := IntToStr(MenuQualityHigh.Tag);
      OptionsQualityMedium.Text := IntToStr(MenuQualityMedium.Tag);
      OptionsQualityLow.Text := IntToStr(MenuQualityLow.Tag);
      OptionsQualityLowest.Text := IntToStr(MenuQualityLowest.Tag);
      OptionsQualitySideShow.Text := IntToStr(MenuQualitySideShow.Tag);

      OptionsDelayLongest.Text := IntToStr(MenuUpdateSleep5.Tag);
      OptionsDelayLong.Text := IntToStr(MenuUpdateSleep4.Tag);
      OptionsDelayMedium.Text := IntToStr(MenuUpdateSleep3.Tag);
      OptionsDelayShort.Text := IntToStr(MenuUpdateSleep2.Tag);
      OptionsDelayShortest.Text := IntToStr(MenuUpdateSleep1.Tag);

      MenuConnection[ConfigFile.ReadInteger('User Settings', 'ConnectionMode', 2)].Click;

      MenuEnableOnStartup.Checked := ConfigFile.ReadBool('User Settings', 'EnableOnStartup', True);

      OptionsWlanNotifyStart.Checked := ConfigFile.ReadBool('User Settings', 'WlanEventStart', True);
      OptionsWlanNotifyConnect.Checked := ConfigFile.ReadBool('User Settings', 'WlanEventConnect', True);
      OptionsWlanNotifyDisconnect.Checked := ConfigFile.ReadBool('User Settings', 'WlanEventDisconnect', True);

      AudioDeviceIndex := ConfigFile.ReadInteger('User Settings', 'SoundDeviceIndex', 0);
      if (AudioDeviceIndex > OptionsAudioDevice.Items.Count - 1) then
        AudioDeviceindex := 0;
      OptionsAudioDevice.ItemIndex := AudioDeviceIndex;
    end;

    MenuScreen[ConfigFile.ReadInteger('User Settings', 'Mode', 3)].Checked := True;
    StartXSetting := ConfigFile.ReadInteger('User Settings', 'X', 0);
    StartYSetting := ConfigFile.ReadInteger('User Settings', 'Y', 0);

    MenuViewportSize[ConfigFile.ReadInteger('User Settings', 'Viewport Size', 0)].Click;
    MenuRotation[ConfigFile.ReadInteger('User Settings', 'Rotation', 0)].Click;

    MenuQuality[2 + ConfigFile.ReadInteger('User Settings', 'Quality', 2)].Checked := True;
    JpegQuality := MenuQuality[2 + ConfigFile.ReadInteger('User Settings', 'Quality', 2)].Tag;
    MenuUpdate[ConfigFile.ReadInteger( 'User Settings', 'Sleep', 0)].Checked := True;
    Delay := MenuUpdate[ConfigFile.ReadInteger( 'User Settings', 'Sleep', 0)].Tag;

    MenuSoundWlan[ConfigFile.ReadInteger('User Settings', 'SampleRateWlan', 0)].Checked := True;
    MenuSoundUsb[ConfigFile.ReadInteger('User Settings', 'SampleRateUsb', 1)].Checked := True;

    MenuUseCaptureBlt.Checked := ConfigFile.ReadBool('User Settings', 'GrabTransparentWindows', False);

    MenuReduceCpuLoad.Checked := ConfigFile.ReadBool('User Settings', 'ReduceCpuLoad', True);

    MenuFollowMouseAcrossScreens.Checked := ConfigFile.ReadBool('User Settings', 'FollowMouseAcrossScreens', False);

    MenuHideCurser.Checked := ConfigFile.ReadBool('User Settings', 'HideCursor', False);

    MenuUseSideshow.Checked := ConfigFile.ReadBool('User Settings', 'UseSideShow', False);

    MenuEnableImageStreaming.Checked := ConfigFile.ReadBool('User Settings', 'EnableImageStreaming', True);

    MenuHigherPriority.Checked := ConfigFile.ReadBool('User Settings', 'HigherPriority', True);
    MenuHigherPriority.OnClick(MainForm);

    MenuCycleSideShowGadgets.Checked := ConfigFile.ReadBool('User Settings', 'CycleSideShowGadgets', False);
    OptionsSideShowInterval.Text := IntToStr(ConfigFile.ReadInteger('User Settings', 'SideShowCycleInterval', 30));
    OptionsSideShowCycleDirection.ItemIndex := ConfigFile.ReadInteger('User Settings', 'SideShowCycleDirection', 1);

    MenuSoundEnabled.Checked := ConfigFile.ReadBool('User Settings', 'SoundEnabled', False);

    OptionsWlanPassword.Text := ConfigFile.ReadString('User Settings', 'WlanPassword', '');

    DeviceIndex := ConfigFile.ReadInteger('User Settings', 'Device', 0);
    if MenuDisplayDeviceFirstIndex + DeviceIndex < MenuDisplay.Count then
      MenuDisplay[MenuDisplayDeviceFirstIndex + DeviceIndex].Click
    else
      MenuDisplay[MenuDisplayDeviceFirstIndex].Click;

    ControlFileIndex := ConfigFile.ReadInteger('User Settings', 'ControlFileIndex', 0);
    if 2 + ControlFileIndex < MenuControl.Count then
      MenuControl[2 + ControlFileIndex].Click
    else
      MenuControl[2].Click;

    MenuQualityUncompressed.Checked := ConfigFile.ReadBool('User Settings', 'NoCompression', False);

    ConfigFile.Free;
  end;
end;



{
  SaveConfigFile
  ---------------------------------------------------
  Save settings to the ini-file.
  ---------------------------------------------------
}
procedure SaveConfigFile(Filename: String; IsPreset: Boolean; PresetName: String);
var
  i: Integer;
  Path : String;
begin
  with (MainForm) do
  begin
    if (IsPreset) then
    begin
      Path := GetAppDataFolder + '\PSPdisp\preset\';
      ForceDirectories(Path);
    end
    else begin
      Path := GetAppDataFolder + '\PSPdisp\';
      ForceDirectories(Path);
    end;
    ConfigFile := TIniFile.Create(Path + Filename);

    try
      if (isPreset) then
      begin
        ConfigFile.WriteString('General', 'Name', PresetName);
      end;

      for i := 2 to MenuControl.Count - 1 do
      begin
        if MenuControl[i].Checked then
        begin
          ConfigFile.WriteInteger('User Settings', 'ControlFileIndex', i - 2);
          Break;
        end;
      end;

      for i := MenuDisplayDeviceFirstIndex to MenuDisplay.Count - 1 do
      begin
        if MenuDisplay[i].Checked then
        begin
          ConfigFile.WriteInteger('User Settings', 'Device', i - MenuDisplayDeviceFirstIndex);
          Break;
        end;
      end;

      for i := 0 to 4 do
      begin
        if MenuScreen[i].Checked then
        begin
          ConfigFile.WriteInteger('User Settings', 'Mode', i);
          Break;
        end;
      end;

      for i := 0 to MenuViewportSize.Count - 1 do
      begin
        if MenuViewportSize[i].Checked then
        begin
          ConfigFile.WriteInteger('User Settings', 'Viewport', i);
          Break;
        end;
      end;

      for i := 0 to MenuRotation.Count - 1 do
      begin
        if MenuRotation[i].Checked then
        begin
          ConfigFile.WriteInteger('User Settings', 'Rotation', i);
          Break;
        end;
      end;

      for i := 2 to MenuQuality.Count - 1 do
      begin
        if MenuQuality[i].Checked then
        begin
          ConfigFile.WriteInteger('User Settings', 'Quality', i - 2);
          Break;
        end;
      end;

      for i := 0 to MenuUpdate.Count - 1 do
      begin
        if MenuUpdate[i].Checked then
        begin
          ConfigFile.WriteInteger('User Settings', 'Sleep', i);
          Break;
        end;
      end;

      for i := 0 to MenuViewPortSize.Count - 1 do
      begin
        if MenuViewPortSize[i].Checked then
        begin
          ConfigFile.WriteInteger('User Settings', 'Viewport Size', i);
          Break;
        end;
      end;

      for i := 0 to MenuSoundWlan.Count - 1 do
      begin
        if menuSoundWlan[i].Checked then
        begin
          ConfigFile.WriteInteger('User Settings', 'SampleRateWlan', i);
          Break;
        end;
      end;

      for i := 0 to MenuSoundUsb.Count - 1 do
      begin
        if MenuSoundUsb[i].Checked then
        begin
          ConfigFile.WriteInteger('User Settings', 'SampleRateUsb', i);
          Break;
        end;
      end;

      ConfigFile.WriteBool('User Settings', 'GrabTransparentWindows', MenuUseCaptureBlt.Checked);

      ConfigFile.WriteBool('User Settings', 'ReduceCpuLoad', MenuReduceCpuLoad.Checked);

      ConfigFile.WriteBool('User Settings', 'HigherPriority', MenuHigherPriority.Checked);

      ConfigFile.WriteBool('User Settings', 'FollowMouseAcrossScreens', MenuFollowMouseAcrossScreens.Checked);

      ConfigFile.WriteBool('User Settings', 'EnableImageStreaming', MenuEnableImageStreaming.Checked);

      ConfigFile.WriteBool('User Settings', 'HideCursor', MenuHideCurser.Checked);

      ConfigFile.WriteBool('User Settings', 'UseSideShow', MenuUseSideShow.Checked);

      ConfigFile.WriteBool('User Settings', 'CycleSideShowGadgets', MenuCycleSideShowGadgets.Checked);
      ConfigFile.WriteInteger('User Settings', 'SideShowCycleInterval', StrToIntDef(OptionsSideShowInterval.Text, 30));
      ConfigFile.WriteInteger('User Settings', 'SideShowCycleDirection', OptionsSideShowCycleDirection.ItemIndex);

      ConfigFile.WriteBool('User Settings', 'SoundEnabled', MenuSoundEnabled.Checked);

      ConfigFile.WriteInteger('User Settings', 'X', StartXSetting);
      ConfigFile.WriteInteger('User Settings', 'Y', StartYSetting);

      ConfigFile.WriteBool('User Settings', 'NoCompression', MenuQualityUncompressed.Checked);

      if (not IsPreset) then
      begin
        for i := 0 to MenuConnection.Count - 1 do
        begin
          if MenuConnection[i].Checked then
          begin
            ConfigFile.WriteInteger('User Settings', 'ConnectionMode', i);
            Break;
          end;
        end;

        ConfigFile.WriteBool('User Settings', 'EnableOnStartup', MenuEnableOnStartup.Checked);

        ConfigFile.WriteBool('User Settings', 'WlanEventStart', OptionsWlanNotifyStart.Checked);
        ConfigFile.WriteBool('User Settings', 'WlanEventConnect', OptionsWlanNotifyConnect.Checked);
        ConfigFile.WriteBool('User Settings', 'WlanEventDisconnect', OptionsWlanNotifyDisconnect.Checked);

        ConfigFile.WriteString('User Settings', 'WlanPassword', OptionsWlanPassword.Text);

        ConfigFile.WriteInteger( 'Quality Options', 'Highest', MenuQualityHighest.Tag);
        ConfigFile.WriteInteger( 'Quality Options', 'High', MenuQualityHigh.Tag);
        ConfigFile.WriteInteger( 'Quality Options', 'Medium', MenuQualityMedium.Tag);
        ConfigFile.WriteInteger( 'Quality Options', 'Low', MenuQualityLow.Tag);
        ConfigFile.WriteInteger( 'Quality Options', 'Lowest', MenuQualityLowest.Tag);
        ConfigFile.WriteInteger( 'Quality Options', 'SideShow', MenuQualitySideShow.Tag);

        ConfigFile.WriteInteger('Delay Options', 'Shortest', MenuUpdateSleep1.Tag);
        ConfigFile.WriteInteger('Delay Options', 'Short', MenuUpdateSleep2.Tag);
        ConfigFile.WriteInteger('Delay Options', 'Medium', MenuUpdateSleep3.Tag);
        ConfigFile.WriteInteger('Delay Options', 'Long', MenuUpdateSleep4.Tag);
        ConfigFile.WriteInteger('Delay Options', 'Longest', MenuUpdateSleep5.Tag);

        ConfigFile.WriteInteger('User Settings', 'SoundDeviceIndex', OptionsAudioDevice.ItemIndex);
      end;

      ConfigFile.Free;

    except
      // Writing settings not possible
    end;
  end;
end;


end.