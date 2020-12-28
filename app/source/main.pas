{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  main.pas - general program logic

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}

unit main;

interface

uses
  Windows, Messages, SysUtils, Classes, Controls, Graphics, Forms, Dialogs,
  StdCtrls, ExtCtrls, Menus, ShellApi, ImgList, IniFiles, SHFolder,
  LibUsb, ComCtrls, XPMan, MainLoop, trayicon;

const
  // For some reason not defined in Windows.pas
  CAPTUREBLT = $40000000;
  // Where the display device list starts
  MenuDisplayDeviceFirstIndex = 12;


type
  TMainForm = class(TForm)
    PageControl: TPageControl;
    TabSheet1: TTabSheet;
    GroupBox1: TGroupBox;
    Label1: TLabel;
    AboutFps: TLabel;
    Popup: TPopupMenu;
    MenuAbout: TMenuItem;
    N6: TMenuItem;
    MenuControl: TMenuItem;
    MenuControlEnabled: TMenuItem;
    N9: TMenuItem;
    MenuOptions: TMenuItem;
    MenuUseCaptureBlt: TMenuItem;
    MenuUseUSB: TMenuItem;
    MenuUseWirelessLAN: TMenuItem;
    MenuUpdate: TMenuItem;
    MenuUpdateContinuous: TMenuItem;
    MenuUpdateSleep1: TMenuItem;
    MenuUpdateSleep2: TMenuItem;
    MenuUpdateSleep3: TMenuItem;
    MenuQuality: TMenuItem;
    MenuQualityHighest: TMenuItem;
    MenuQualityHigh: TMenuItem;
    MenuQualityMedium: TMenuItem;
    MenuQualityLow: TMenuItem;
    MenuQualityLowest: TMenuItem;
    MenuDesktopProperties: TMenuItem;
    MenuEnableDisplay: TMenuItem;
    MenuScreen: TMenuItem;
    MenuFitScreen: TMenuItem;
    MenuFitScreen16to9: TMenuItem;
    MenuFollowMouse: TMenuItem;
    MenuFollowMouse2: TMenuItem;
    MenuStaticViewport: TMenuItem;
    N1: TMenuItem;
    MenuViewportSize: TMenuItem;
    MenuRotation: TMenuItem;
    MenuRotationOff: TMenuItem;
    MenuRotation90: TMenuItem;
    MenuRotation180: TMenuItem;
    MenuRotation270: TMenuItem;
    MenuSetOrigin: TMenuItem;
    MenuResetOrigin: TMenuItem;
    MenuDisplay: TMenuItem;
    N3: TMenuItem;
    MenuEnabled: TMenuItem;
    N2: TMenuItem;
    MenuExit: TMenuItem;
    ImageList: TImageList;
    Timer: TTimer;
    FpsTimer: TTimer;
    N11: TMenuItem;
    N12: TMenuItem;
    Label15: TLabel;
    Label16: TLabel;
    Label12: TLabel;
    MenuEnableOnStartup: TMenuItem;
    MenuResolution: TMenuItem;
    MenuResolutionRotated: TMenuItem;
    MenuUpdateSleep4: TMenuItem;
    MenuUpdateSleep5: TMenuItem;
    MenuControlOff: TMenuItem;
    MenuHelp: TMenuItem;
    MenuUseSideshow: TMenuItem;
    MenuSideshowProperties: TMenuItem;
    N4: TMenuItem;
    MenuReduceCpuLoad: TMenuItem;
    WlanTab: TTabSheet;
    GroupBox2: TGroupBox;
    OptionsWlanNotifyStart: TCheckBox;
    OptionsWlanNotifyConnect: TCheckBox;
    OptionsWlanNotifyDisconnect: TCheckBox;
    N8: TMenuItem;
    MenuMoreOptions: TMenuItem;
    QualityTab: TTabSheet;
    DelayTab: TTabSheet;
    N13: TMenuItem;
    MenuDelayCustomize: TMenuItem;
    N14: TMenuItem;
    MenuQualityCustomize: TMenuItem;
    GroupBox3: TGroupBox;
    GroupBox4: TGroupBox;
    XPManifest1: TXPManifest;
    Label7: TLabel;
    Label8: TLabel;
    Label9: TLabel;
    AboutFrameSize: TLabel;
    Label5: TLabel;
    Label10: TLabel;
    N15: TMenuItem;
    N320x2001: TMenuItem;
    N320x2401: TMenuItem;
    N640x4001: TMenuItem;
    N640x4801: TMenuItem;
    N800x6001: TMenuItem;
    TabSheet2: TTabSheet;
    GroupBox5: TGroupBox;
    OptionsScancode: TEdit;
    Label11: TLabel;
    GroupBox6: TGroupBox;
    OptionsOpenControlFileFolder: TButton;
    N16: TMenuItem;
    MenuControlCustomize: TMenuItem;
    Label13: TLabel;
    Label14: TLabel;
    Label17: TLabel;
    Label18: TLabel;
    Label19: TLabel;
    Label20: TLabel;
    AboutTotalFrames: TLabel;
    Label22: TLabel;
    AboutTotalBytesSent: TLabel;
    MenuFollowMouseAcrossScreens: TMenuItem;
    MenuQualitySideShow: TMenuItem;
    Sound1: TMenuItem;
    MenuSoundEnabled: TMenuItem;
    N18: TMenuItem;
    MenuOpenMixer: TMenuItem;
    TabSheet3: TTabSheet;
    GroupBox8: TGroupBox;
    Label4: TLabel;
    OptionsAudioDevice: TComboBox;
    Chooseinputdevice1: TMenuItem;
    GroupBox7: TGroupBox;
    Label21: TLabel;
    Button1: TButton;
    OptionsSoundRefresh: TButton;
    TabSheet4: TTabSheet;
    GroupBox9: TGroupBox;
    GroupBox10: TGroupBox;
    Label24: TLabel;
    Label25: TLabel;
    OptionsSideShowInterval: TEdit;
    Label26: TLabel;
    MenuCycleSideShowGadgets: TMenuItem;
    OptionsSideShowCycleDirection: TComboBox;
    MenuQualityUncompressed: TMenuItem;
    N20: TMenuItem;
    GroupBox11: TGroupBox;
    OptionsWlanAddress: TLabel;
    ConfigTimer: TTimer;
    MenuHideCurser: TMenuItem;
    Label27: TLabel;
    Label28: TLabel;
    Label29: TLabel;
    Label30: TLabel;
    Label32: TLabel;
    Label34: TLabel;
    OptionsQualityHighest: TEdit;
    OptionsQualityHigh: TEdit;
    OptionsQualityMedium: TEdit;
    OptionsQualityLow: TEdit;
    OptionsQualityLowest: TEdit;
    OptionsQualityReset: TButton;
    Label36: TLabel;
    Label37: TLabel;
    Label38: TLabel;
    Label39: TLabel;
    Label40: TLabel;
    Label46: TLabel;
    Label47: TLabel;
    Label48: TLabel;
    Label49: TLabel;
    Label50: TLabel;
    OptionsDelayShortest: TEdit;
    OptionsDelayShort: TEdit;
    OptionsDelayMedium: TEdit;
    OptionsDelayLong: TEdit;
    OptionsDelayLongest: TEdit;
    Label41: TLabel;
    Label42: TLabel;
    Label43: TLabel;
    Label44: TLabel;
    Label45: TLabel;
    Label51: TLabel;
    Label52: TLabel;
    Label53: TLabel;
    Label54: TLabel;
    Label55: TLabel;
    OptionsDelayReset: TButton;
    OptionsQualitySideShow: TEdit;
    Label23: TLabel;
    Label33: TLabel;
    Label56: TLabel;
    Panel1: TPanel;
    Label2: TLabel;
    Label3: TLabel;
    Label6: TLabel;
    OptionsVersionNumber: TLabel;
    AboutVisitHomepage: TLabel;
    OptionsRefreshControlFileList: TButton;
    OptionsReloadControlFile: TButton;
    N21: TMenuItem;
    N240x1361: TMenuItem;
    N120x681: TMenuItem;
    N60x341: TMenuItem;
    N22: TMenuItem;
    TabSheet5: TTabSheet;
    DisplayPaintBox: TPaintBox;
    MenuConnection: TMenuItem;
    MenuUseBoth: TMenuItem;
    N5: TMenuItem;
    N7: TMenuItem;
    MenuScaleCursor: TMenuItem;
    MenuFastImageStretching: TMenuItem;
    GroupBox12: TGroupBox;
    Label58: TLabel;
    OptionsWlanPassword: TEdit;
    MenuEnableImageStreaming: TMenuItem;
    MenuHigherPriority: TMenuItem;
    menuSoundWlan: TMenuItem;
    menuSoundUsb: TMenuItem;
    N10: TMenuItem;
    MenuSoundWlan11: TMenuItem;
    MenuSoundWlan22: TMenuItem;
    MenuSoundWlan44: TMenuItem;
    MenuSoundUsb11: TMenuItem;
    MenuSoundUsb22: TMenuItem;
    MenuSoundUsb44: TMenuItem;
    SettingsClose: TButton;
    OptionsWlanNoPassword: TButton;
    MenuStreaming: TMenuItem;
    GroupBox13: TGroupBox;
    Label59: TLabel;
    Label31: TLabel;
    OptionsDisplayPositionX: TEdit;
    OptionsDisplayPositionY: TEdit;
    Label57: TLabel;
    OptionsDisplayApplySettings: TButton;
    MenuDisplayChangePosition: TMenuItem;
    MenuPresets: TMenuItem;
    MenuEditPresets: TMenuItem;
    N17: TMenuItem;
    TabSheet6: TTabSheet;
    GroupBox14: TGroupBox;
    Label35: TLabel;
    OptionsPresetsList: TListBox;
    OptionsPresetsSave: TButton;
    OptionsPresetsDelete: TButton;
    OptionsPresetsRefresh: TButton;
    OptionsPresetsName: TEdit;
    SaveDialog: TSaveDialog;
    OptionsPresetsLoad: TButton;
    OptionsPresetsOpenFolder: TButton;
    procedure TimerTimer(Sender: TObject);
    procedure MenuUpdateClick(Sender: TObject);
    procedure MenuQualityClick(Sender: TObject);
    procedure MenuDesktopPropertiesClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure MenuExitClick(Sender: TObject);
    procedure MenuEnabledClick(Sender: TObject);
    procedure MenuSetOriginClick(Sender: TObject);
    procedure MenuResetOriginClick(Sender: TObject);
    procedure MenuEnableDisplayClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FpsTimerTimer(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure MenuViewPortSizeClick(Sender: TObject);
    procedure MenuAboutClick(Sender: TObject);
    procedure GetMonitorParameters();
    procedure MainLoop;
    procedure SetIcon(DeviceConnected: Boolean);
    procedure FlashIcon(IsOn: Boolean);
    procedure CreateDisplayList;
    procedure MenuSetDisplay(Sender: TObject);
    procedure MenuUseWirelessLANClick(Sender: TObject);
    procedure MenuUseCaptureBltClick(Sender: TObject);
    procedure MenuEnableOnStartupClick(Sender: TObject);
    procedure MenuRotation90Click(Sender: TObject);
    procedure MenuRotationOffClick(Sender: TObject);
    procedure CreateResolutionList();
    procedure SetResolution(Sender: TObject);
    procedure CloseBtnClick(Sender: TObject);
    procedure MenuChangeWlanSettingsClick(Sender: TObject);
    procedure CreateControlFileList();
    procedure SelectControlFile(Sender: TObject);
    procedure SelectPresetFile(Sender: TObject);
    procedure SelectAudioDevice(ChangeDevice: Boolean);
    function GetAppDataFolder(): String;
    procedure CreatePresetFileList();

    procedure OptionsScancodeWindowProc(var Msg: TMessage);
    procedure MenuHelpClick(Sender: TObject);
    procedure MenuUseSideshowClick(Sender: TObject);
    procedure MenuReduceCpuLoadClick(Sender: TObject);
    procedure MenuSideshowPropertiesClick(Sender: TObject);
    procedure SelectMouseControl();    

    function GetWindowsVersion() : Integer;
    procedure MenuMoreOptionsClick(Sender: TObject);
    procedure MenuDelayCustomizeClick(Sender: TObject);
    procedure MenuQualityCustomizeClick(Sender: TObject);
    procedure SetIconIndex(Index: Integer);
    procedure ShowBalloonHint(Title: String; Hint: String; Flags: TBalloonFlags);
    procedure OptionsOpenControlFileFolderClick(Sender: TObject);
    procedure MenuControlCustomizeClick(Sender: TObject);
    function ReadVersionFromFile(): String;
    procedure MenuFollowMouseAcrossScreensClick(Sender: TObject);
    procedure MenuSoundEnabledClick(Sender: TObject);
    procedure MenuOpenMixerClick(Sender: TObject);
    procedure Chooseinputdevice1Click(Sender: TObject);
    procedure CreateAudioDeviceList();
    procedure OptionsSoundRefreshClick(Sender: TObject);
    procedure MenuCycleSideShowGadgetsClick(Sender: TObject);
    procedure MenuScreenClick(Sender: TObject);
    procedure ConfigTimerTimer(Sender: TObject);
    procedure OnModificationForConfig(Sender: TObject);
    procedure AboutVisitHomepageClick(Sender: TObject);
    procedure OptionsQualityOnChange(Sender: TObject);
    procedure OptionsQualityResetClick(Sender: TObject);
    procedure OptionsQualityOnExit(Sender: TObject);
    procedure OptionsDelayOnChange(Sender: TObject);
    procedure OptionsDelayResetClick(Sender: TObject);
    procedure OptionsDelayOnExit(Sender: TObject);
    procedure OptionsRefreshControlFileListClick(Sender: TObject);
    procedure OptionsReloadControlFileClick(Sender: TObject);

    procedure OnDeviceChangeMessage(var Msg: TMessage); message WM_DEVICECHANGE;
    procedure OnDisplayChangeMessage(var Msg: TMessage); message WM_DISPLAYCHANGE;
    procedure OnSystemCommand(var Msg: TMessage); message WM_SYSCOMMAND;

    procedure UpdateDisplayInformations();
    procedure DisplayPaintBoxPaint(Sender: TObject);
    procedure OptionsDisplayApplySettingsClick(Sender: TObject);
    procedure MenuScaleCursorClick(Sender: TObject);
    procedure MenuFastImageStretchingClick(Sender: TObject);
    procedure MenuHigherPriorityClick(Sender: TObject);
    procedure MenuSoundClick(Sender: TObject);
    procedure SettingsCloseClick(Sender: TObject);
    procedure OptionsWlanNoPasswordClick(Sender: TObject);
    procedure MenuEnableImageStreamingClick(Sender: TObject);
    procedure OptionsQualitySideShowChange(Sender: TObject);
    procedure MenuDisplayChangePositionClick(Sender: TObject);
    procedure testClick(Sender: TObject);
    procedure MenuEditPresetsClick(Sender: TObject);
    procedure OptionsPresetsDeleteClick(Sender: TObject);
    procedure OptionsPresetsListClick(Sender: TObject);
    procedure OptionsPresetsRefreshClick(Sender: TObject);
    procedure OptionsPresetsSaveClick(Sender: TObject);
    procedure OptionsPresetsLoadClick(Sender: TObject);
    procedure OptionsPresetsOpenFolderClick(Sender: TObject);
  end;



var
  MainForm       : TMainForm;
  i              : Integer;
  x, y           : Integer;
  TempX, TempY   : Integer;
  LastX, LastY   : Integer;
  FramesRendered : Integer;
  CursorIcon     : TIcon;
  CursorInfo     : TCursorInfo;
  IconInfo       : TIconInfo;
  FrameCount     : Integer;
  lastTime       : TDateTime;

  MonitorLeft    : Integer;
  MonitorTop     : Integer;
  MonitorWidth   : Integer;
  MonitorHeight  : Integer;

  WorkerThread   : MainLoopThread;

  Delay          : Integer;

  TransmissionFinished : Boolean;

  StartX         : Integer;
  StartY         : Integer;
  StartXSetting  : Integer;
  StartYSetting  : Integer;

  lastTempX      : Integer;
  lastTempY      : Integer;

  EqualValuesCount: Integer;

  ConfigFile     : TIniFile;

  StaticViewport : Boolean;

  CurrentSize : TPoint;
  Fps : LongWord;
  framesize : Cardinal;
  lag : Integer;

  ViewPortWidth : Integer;
  ViewPortHeight : Integer;

  lastCursorPosition : TPoint;

  bufferSize : Integer;

  copyMode : LongWord;

  LastConnectionState : Boolean;

  ViewPortIndex : Integer;

  Resolutions : Array of TPoint;

  MainLoopRunning : Boolean;

  ShuttingDown : Boolean;

  CurrentBitmapHandle : HBITMAP;

  TrayIcon : TPSPdispTrayIcon;

  OldWndProc: TWndMethod;

  TotalFrames : LongWord;
  TotalFrameSize : LongWord;

  KeyboardState: LongWord;

  ConfigurationModified: Boolean;

  FirstStart: Boolean;

  InstanceNumber: Integer;

  // Display position settings
  DisplayPosition: Array[0..100] of TPoint;
  DisplaySize: Array[0..100] of TPoint;
  DisplayDeviceName: Array[0..100] of String;
  DisplayDeviceString: Array[0..100] of String;
  NumberOfDisplays: Integer;

  LeftBorder: Integer;
  RightBorder: Integer;
  TopBorder: Integer;
  BottomBorder: Integer;
  TotalWidth: Integer;
  TotalHeight: Integer;
  ScalingFactor: Double;

  PrintPositionX: Integer;
  PrintPositionY: Integer;
  PrintSizeX: Integer;
  PrintSizeY: Integer;

  PSPdispDriverIndex: Integer;
  NumberOfDisplaysInArray:Integer;

  UpdatingDisplayInformation : boolean;


implementation

{$R *.DFM}

uses
  wlan, usb, display, control, compress, graphic, audio, config;



{
  OnSystemCommand
  ---------------------------------------------------
  This event occurs when a system message is sent.
  Used for minimizing the main window when clicking
  the close button.
  ---------------------------------------------------
}
procedure TMainForm.OnSystemCommand(var Msg: TMessage);
begin
  if (Msg.WParam = SC_CLOSE) then
  begin
     Msg.Result := 0;
     MainForm.Hide;
  end
  else
    with Msg do begin
      DefWindowProc(Handle, Msg, WParam, LParam);
    end;
end;


{
  OnDisplayChangeMessage
  ---------------------------------------------------
  This event occurs when display settings change.
  ---------------------------------------------------
}
procedure TMainForm.OnDisplayChangeMessage(var Msg: TMessage);
var
  i: Integer;
  CurrentDisplayIndex: Integer;

begin
  CurrentDisplayIndex := 0;

  for i := MenuDisplayDeviceFirstIndex to MenuDisplay.Count - 1 do
  begin
    if MenuDisplay.Items[i].Checked then
    begin
      CurrentDisplayIndex := i;
      Break;
    end;
  end;

  CreateDisplayList;

  if CurrentDisplayIndex < MenuDisplay.Count then
    MenuDisplay.Items[CurrentDisplayIndex].Click
  else
    MenuDisplay.Items[MenuDisplayDeviceFirstIndex].Click;

//UpdateDisplayInformations();    
end;




{
  OnModificationForConfig
  ---------------------------------------------------
  Gets called every time a setting is changed that
  has to be saved to the config file.
  ---------------------------------------------------
}
procedure TMainForm.OnModificationForConfig(Sender: TObject);
begin
  ConfigurationModified := True;
  SendPCSettings := True;
end;




{
  OptionsOpenControlFileFolderClick
  ---------------------------------------------------
  Open the folder containing the control files.
  ---------------------------------------------------
}
procedure TMainForm.OptionsOpenControlFileFolderClick(Sender: TObject);
begin
  ForceDirectories(MainForm.GetAppDataFolder + '\PSPdisp\control\');
  ShellExecute(MainForm.Handle, 'explore', PChar(GetAppDataFolder + '\PSPdisp\control\'), '', nil, SW_SHOW);
end;



{
  OptionsRefreshControlFileListClick
  ---------------------------------------------------
  Reread the control files.
  ---------------------------------------------------
}
procedure TMainForm.OptionsRefreshControlFileListClick(Sender: TObject);
var
  CurrentControlFile: String;
  i: Integer;
begin
  CurrentControlFile := '___notfound___';

  for i := 2 to MenuControl.Count - 3 do
  begin
    if MenuControl.Items[i].Checked then
    begin
      CurrentControlFile := MenuControl.Items[i].Caption;
      Break;
    end;
  end;

  MenuControl.Items[2].Click; // turn control off

  CreateControlFileList();

  for i := 2 to MenuControl.Count - 3 do
  begin
    if MenuControl.Items[i].Caption = CurrentControlFile then
    begin
      MenuControl.Items[i].Click;
      Exit;
    end;
  end;

  MenuControl.Items[2].Click; // turn control off if no item matches
end;



{
  OptionsReloadControlFileClick
  ---------------------------------------------------
  Reload the current control file, same as clicking the
  currently selected menu entry.
  ---------------------------------------------------
}
procedure TMainForm.OptionsReloadControlFileClick(Sender: TObject);
var
  i: Integer;
begin
  for i := 2 to MenuControl.Count - 3 do
  begin
    if MenuControl.Items[i].Checked then
    begin
      MenuControl.Items[i].Click;
      Break;
    end;
  end;
end;




{
  OptionsReloadControlFileClick
  ---------------------------------------------------
  Reload the current control file, same as clicking the
  currently selected menu entry.
  ---------------------------------------------------
}
procedure TMainForm.SelectMouseControl();
var
  i: Integer;
begin
  for i := 2 to MenuControl.Count - 3 do
  begin
    if MenuControl.Items[i].Caption = 'Mouse emulation' then
    begin
      MenuControl.Items[i].Click;
      Break;
    end;
  end;
end;



{
  ReadVersionFromFile
  ---------------------------------------------------
  Extract the file version from the program exe.
  ---------------------------------------------------
}
function TMainForm.ReadVersionFromFile(): String;
var 
  VerInfoSize: DWORD; 
  VerInfo: Pointer; 
  VerValueSize: DWORD; 
  VerValue: PVSFixedFileInfo; 
  Dummy: DWORD; 
  V1, V2, V3, V4: Word; 
begin 
  VerInfoSize := GetFileVersionInfoSize(PChar(ParamStr(0)), Dummy); 
  GetMem(VerInfo, VerInfoSize); 
  GetFileVersionInfo(PChar(ParamStr(0)), 0, VerInfoSize, VerInfo);
  VerQueryValue(VerInfo, '\', Pointer(VerValue), VerValueSize); 
  with VerValue^ do 
  begin 
    V1 := dwFileVersionMS shr 16; 
    V2 := dwFileVersionMS and $FFFF; 
    V3 := dwFileVersionLS shr 16;
    V4 := dwFileVersionLS and $FFFF;
  end; 
  FreeMem(VerInfo, VerInfoSize);

  Result := IntToStr(V1) + '.' + IntToStr(V2) + '.' + IntToStr(V3) + '.' + IntToStr(V4);
end;



{
  SetIcon
  ---------------------------------------------------
  Change the tray icon.
  ---------------------------------------------------
}
procedure TMainForm.SetIcon(DeviceConnected: Boolean);
var
  IconIndex: Integer;
begin
  // Base index
  if (DeviceConnected) then
    IconIndex := 2
  else if (MainLoopRunning) then
    IconIndex := 1
  else
    IconIndex := 0;

  // Connection specific offset
  if (MenuUseWirelessLAN.Checked) then
    Inc(IconIndex, 3)
  else if (MenuUseBoth.Checked) then
    Inc(IconIndex, 6);

  TrayIcon.IconIndex := IconIndex;
  ImageList.GetIcon(IconIndex, Application.Icon);
end;





{
  SetIconIndex
  ---------------------------------------------------
  Change the program icon.
  ---------------------------------------------------
}
procedure TMainForm.SetIconIndex(Index: Integer);
begin
  MainForm.ImageList.GetIcon(Index, Application.Icon);
  Main.TrayIcon.IconIndex := Index;
end;



{
  ShowBalloonHint
  ---------------------------------------------------
  Open a balloon hint, set all parameters at once.
  ---------------------------------------------------
}
procedure TMainForm.ShowBalloonHint(Title: String; Hint: String; Flags: TBalloonFlags);
begin
  trayIcon.BalloonTimeout := 3000;
  trayicon.BalloonHint := Hint;
  trayIcon.BalloonTitle := Title;
  trayIcon.BalloonFlags := Flags;
  trayIcon.ShowBalloonHint;
end;





procedure TMainForm.testClick(Sender: TObject);
begin
  with PCSettings do
  begin
    Settings.ScreenRotation := 0;
    Settings.ScreenMode := 0;
    Settings.UpdateInterval := 0;
    Settings.Quality := 0;

    Settings.ControlIndex := 0;
    Settings.DisplayDeviceIndex := 0;
    Settings.ScreenViewport := 0;

    Settings.SoundEnabled := 1;

    Settings.SoundSampleRate := 2;

    Settings.DisplayEnable := 0;
    Settings.DisplayAutoselect := 1;
    Settings.ScreenNoCompression := 1;
    Settings.DeviceUseSideShow := 0;

    PCSettingsChanged := True;
  end;
end;

{
  FlashIcon
  ---------------------------------------------------
  Lets the tray icon blink between enabled and
  disabled state icon.
  ---------------------------------------------------
}
procedure TMainForm.FlashIcon(IsOn: Boolean);
var
  IconIndex: Integer;
begin
  // Base index
  if (IsOn) then
    IconIndex := 2
  else
    IconIndex := 1;

  // Connection specific offset
  if (MenuUseWirelessLAN.Checked) then
    Inc(IconIndex, 3)
  else if (MenuUseBoth.Checked) then
    Inc(IconIndex, 6);

  TrayIcon.IconIndex := IconIndex;
  ImageList.GetIcon(IconIndex, Application.Icon);
end;





{
  GetMonitorParameters
  ---------------------------------------------------
  Retrieve information about the PSPdisp drivers
  display position. TScreen cannot be used because
  it is only initialized at startup and doesn't
  reflect later changes.
  ---------------------------------------------------
}
procedure TMainForm.GetMonitorParameters;
begin
  DispGetPosition(CurrentDeviceNumber, MonitorLeft, MonitorTop);
  DispGetInformation(CurrentDeviceNumber, CurrentSize, CurrentDeviceName, CurrentDeviceString);
  MonitorWidth := CurrentSize.X;
  MonitorHeight := CurrentSize.Y;
end;





{
  MainLoop
  ---------------------------------------------------
  Start the worker thread.
  ---------------------------------------------------
}
procedure TMainForm.MainLoop;
begin
  if not MainLoopRunning then
    WorkerThread := MainLoopThread.Create(False);
end;





{
  CreateAudioDeviceList
  ---------------------------------------------------
  Populate the combobox for audio device selection.
  ---------------------------------------------------
}
procedure TMainForm.CreateAudioDeviceList();
var
  i: Integer;
begin
  OptionsAudioDevice.Clear;

  for i := 0 to audioEnumerateDevices() - 1 do
  begin
    OptionsAudioDevice.Items.Add(audioGetDeviceName(i));
  end;
end;




{
  FormClose
  ---------------------------------------------------
  Main form closing event handler.
  ---------------------------------------------------
}
procedure TMainForm.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  SaveConfig;
  Hide;
end;




{
  FormCreate
  ---------------------------------------------------
  Main form creation event handler.
  Initializes all static variables.
  ---------------------------------------------------
}
procedure TMainForm.FormCreate(Sender: TObject);
begin
  OldWndProc := OptionsScancode.WindowProc;
  OptionsScancode.WindowProc := OptionsScancodeWindowProc;

  TrayIcon := TPSPdispTrayIcon.Create(MainForm);
  TrayIcon.Visible := True;
  TrayIcon.Icons := ImageList;
  TrayIcon.PopupMenu := Popup;
  TrayIcon.OnDblClick := MenuEnabledClick;

  // Get the instance from the command line
  if (ParamCount > 0) then
  begin
    InstanceNumber := StrToIntDef(ParamStr(1), 9999);

    if (InstanceNumber = 9999) then
    begin
      MessageDlg('Wrong instance number specified, must be between 0 and 9.', mtError, [mbok], -1);
      InstanceNumber := 0;
    end
    else
      Application.Title := 'PSPdisp (' + IntToStr(InstanceNumber) + ')';
      TrayIcon.Hint := Application.Title; 
  end;


  // init variables
  ShuttingDown := False;
  TransmissionFinished := True;
  MainLoopRunning := False;
  Delay := 0;
  ChangeRotationOnNextFrame := False;
  KeyboardState := 0;
  ForceTransmission := False;
  ConfigurationModified := False;

  OptionsVersionNumber.Caption := OptionsVersionNumber.Caption + ' ' + ReadVersionFromFile;

  CursorIcon := TIcon.Create;
  CursorInfo.cbSize := SizeOf(CursorInfo);

  if not CompressInitLibrary then
  begin
    MessageDlg('File jpeg62.dll not found or cannot be initialized.', mtError, [mbOk], -1);
    Halt;
  end;

  GraphicInitBitmaps;

  // query display modes and available devies
  CreateResolutionList;
  CreateDisplayList;
  MenuDisplay.Items[MenuDisplayDeviceFirstIndex].Click;

  // create audio recording device list
  CreateAudioDeviceList();

  // control files
  CreateControlFileList();

  // preset files
  CreatePresetFileList();

  // load ini-file
  LoadConfig();

  // Autoselect the best audio device on first startus
  SelectAudioDevice(FirstStart);

  // Autoselect 'Mouse control' control file on first start
  if (FirstStart) then
    SelectMouseControl();

  GraphicChangeRotation;

  if (MenuUseCaptureBlt.Checked) then
    copyMode := SRCCOPY or CAPTUREBLT
  else
    copyMode := SRCCOPY;

  // Select WLAN only mode when a second instance and it is the first start
  if (FirstStart and (InstanceNumber > 0)) then
    MenuUseWirelessLAN.Checked := True;  

  if (MenuEnableOnStartup.Checked) then
    MenuEnabled.Click();

  SetIcon(False);

  if (GetWindowsVersion() < 6) then
  begin
    MenuUseSideshow.Enabled := False;
    MenuSideshowProperties.Enabled := False;
  end;

  ConfigTimer.Enabled := True;

  UpdateDisplayInformations();  
end;




{
  FormShow
  ---------------------------------------------------
  Enable the fps timer when the options window
  gets shown.
  ---------------------------------------------------
}
procedure TMainForm.FormShow(Sender: TObject);
begin
  MainForm.FpsTimer.Enabled := true;
end;



{
  FpsTimerTimer
  ---------------------------------------------------
  Gets called once a second to update the fps.
  ---------------------------------------------------
}
procedure TMainForm.FpsTimerTimer(Sender: TObject);
begin
  AboutFps.Caption := IntToStr(fps);
  if (fps > 0) then
    AboutFrameSize.Caption := IntToStr(frameSize div fps div 1024) + ' KiB'
  else
    AboutFrameSize.Caption := '0 KiB';

  TotalFrames := TotalFrames + Fps;
  TotalFrameSize := TotalFrameSize + FrameSize;

  Fps := 0;
  FrameSize := 0;

  if (TotalFrameSize > 1024 * 1024) then
    AboutTotalBytesSent.Caption := IntToStr(TotalFrameSize div (1024 * 1024)) + ' MiB'
  else
    AboutTotalBytesSent.Caption := IntToStr(TotalFrameSize div 1024) + ' KiB';

  AboutTotalFrames.Caption := IntToStr(TotalFrames);

  Application.ProcessMessages;
end;




{
  MenuEnabledClick
  ---------------------------------------------------
  Event for clicking the "Enabled" entry in the
  tray menu. Enters the main loop.
  ---------------------------------------------------
}
procedure TMainForm.MenuEnabledClick(Sender: TObject);
begin
  if not MenuEnabled.Enabled then
    Exit;

  if MainLoopRunning and not MenuEnabled.Checked then
    Exit;

  MenuEnabled.Checked := not MenuEnabled.Checked;
  MenuSetOrigin.Enabled := MenuEnabled.Checked;

  MainForm.SetIcon(False);

  if MenuEnabled.Checked then
    MainLoop;

  MenuUseUSB.Enabled := not MenuEnabled.Checked;
  MenuUseWirelessLAN.Enabled := not MenuEnabled.Checked;
  MenuUseBoth.Enabled := not MenuEnabled.Checked;
end;





{
  MenuEnableDisplayClick
  ---------------------------------------------------
  Event for clicking the "Enable display" entry in the
  tray menu. Sets the display driver enable state.
  ---------------------------------------------------
}
procedure TMainForm.MenuEnableDisplayClick(Sender: TObject);
var
  deviceNumber: Integer;
begin
  deviceNumber := DispGetDeviceNumber();
  if (deviceNumber > -1) then
    DispSetDisplayEnabledState(deviceNumber, not MenuEnableDisplay.Checked);
end;





procedure TMainForm.MenuEnableImageStreamingClick(Sender: TObject);
begin
  OnModificationForConfig(Self);
end;

{
  MenuExitClick
  ---------------------------------------------------
  Event for clicking the "Quit" entry in the
  tray menu. Quits program.
  ---------------------------------------------------
}
procedure TMainForm.MenuExitClick(Sender: TObject);
begin
  ShuttingDown := True;
  MenuEnabled.Checked := False;
  while not TransmissionFinished do
  begin
    Sleep(500);
  end;
  UsbCloseConnection;
  MainForm.Close;
end;




{
  MenuFastImageStretchingClick
  ---------------------------------------------------
  Handler for clicks on "Fast menu stretching".
  ---------------------------------------------------
}
procedure TMainForm.MenuFastImageStretchingClick(Sender: TObject);
begin
  ChangeRotationOnNextFrame := True;
  
  OnModificationForConfig(Self);
end;



{
  MenuFollowMouseAcrossScreensClick
  ---------------------------------------------------
  If checked, the PSP screen always displays the
  desktop area containing the mouse cursor.
  ---------------------------------------------------
}
procedure TMainForm.MenuFollowMouseAcrossScreensClick(Sender: TObject);
begin
  MenuFollowMouseAcrossScreens.Checked := not MenuFollowMouseAcrossScreens.Checked; 
  ForceTransmission := True;

  OnModificationForConfig(Self);
end;



{
  MenuHelpClick
  ---------------------------------------------------
  Invoke the HTML help.
  ---------------------------------------------------
}
procedure TMainForm.MenuHelpClick(Sender: TObject);
begin
  ShellExecute(MainForm.Handle, 'open', PChar(ExtractFilePath(Application.ExeName) + '\PSPdisp_help.html'), '', nil, SW_SHOW);
end;




{
  MenuHigherPriorityClick
  ---------------------------------------------------
  Set the process priority depending on the requested
  level.
  ---------------------------------------------------
}
procedure TMainForm.MenuHigherPriorityClick(Sender: TObject);
begin
  if (MenuHigherPriority.Checked) then
    Setpriorityclass(GetCurrentProcess(), $00008000 {ABOVE_NORMAL_PRIORITY_CLASS})
  else
    Setpriorityclass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
end;




{
  MenuMoreOptionsClick
  ---------------------------------------------------
  Open the options dialog.
  ---------------------------------------------------
}
procedure TMainForm.MenuMoreOptionsClick(Sender: TObject);
begin
  MainForm.PageControl.ActivePageIndex := 1;
  MainForm.Show;
end;



{
  OnDeviceChangeMessage
  ---------------------------------------------------
  Occurs when a hardware change occurs, e.g.
  plugging or unplugging of the psp.
  ---------------------------------------------------
}
procedure TMainForm.OnDeviceChangeMessage(var Msg: TMessage);
begin
  UsbOnDeviceChange(Msg);
end;



{
  MenuDesktopPropertiesClick
  ---------------------------------------------------
  Event for clicking the "Display properties" entry in the
  tray menu. Invokes the display settings dialog.
  ---------------------------------------------------
}
procedure TMainForm.MenuDesktopPropertiesClick(Sender: TObject);
begin
  ShellExecute(MainForm.Handle, nil, 'rundll32.exe', 'shell32.dll,Control_RunDLL desk.cpl,,4', nil, SW_SHOW);
end;




{
  MenuDisplayChangePositionClick
  ---------------------------------------------------
  Open the settings window and switch to the
  "Display" tab.
  ---------------------------------------------------
}
procedure TMainForm.MenuDisplayChangePositionClick(Sender: TObject);
begin
  MainForm.PageControl.ActivePageIndex := 7;
  MainForm.Show;
end;




{
  MenuQualityClick
  ---------------------------------------------------
  Event for clicking on of the "quality" entries in the
  tray menu.
  ---------------------------------------------------
}
procedure TMainForm.MenuQualityClick(Sender: TObject);
begin
  // The Jpeg "quality" is stored in the "Tag" attribute of
  // the menu entries.
  (Sender as TMenuItem).Checked := True;

  JpegQuality := (Sender as TMenuItem).Tag;

  OnModificationForConfig(Self);
end;




{
  MenuQualityCustomizeClick
  ---------------------------------------------------
  Event for clicking on the "Custmize" entry of the
  quality menu.
  ---------------------------------------------------
}
procedure TMainForm.MenuQualityCustomizeClick(Sender: TObject);
begin
  PageControl.ActivePageIndex := 2;
  MainForm.Show;
end;



{
  MenuResetOriginClick
  ---------------------------------------------------
  Event for clicking the "Reset origin" entry in the
  tray menu. Sets the viewport to default.
  ---------------------------------------------------
}
procedure TMainForm.MenuResetOriginClick(Sender: TObject);
begin
  StartXSetting := 0;
  StartYSetting := 0;
end;




{
  MenuRotationOffClick
  ---------------------------------------------------
  Event for setting the rotation to "off" or "180".
  ---------------------------------------------------
}
procedure TMainForm.MenuRotationOffClick(Sender: TObject);
begin
  (Sender as TMenuItem).Checked := True;
  ChangeRotationOnNextFrame := True;

  OnModificationForConfig(Self);
end;




{
  MenuRotation90Click
  ---------------------------------------------------
  Event for setting the rotation to "90" or "270".
  ---------------------------------------------------
}
procedure TMainForm.MenuRotation90Click(Sender: TObject);
begin
  (Sender as TMenuItem).Checked := True;
  ChangeRotationOnNextFrame := True;
end;



{
  MenuSetOriginClick
  ---------------------------------------------------
  Event for clicking the "Set origin" entry in the
  tray menu. Sets the viewport to current cursor
  position.
  ---------------------------------------------------
}
procedure TMainForm.MenuSetOriginClick(Sender: TObject);
begin
  LastX := 0;
  LastY := 0;
  EqualValuesCount := 0;

  StaticViewport := MenuStaticViewPort.Checked;

  if StaticViewPort then
    MenuFollowMouse2.Checked := True;

  Timer.Enabled := True;
end;




{
  MenuSideshowPropertiesClick
  ---------------------------------------------------
  Event for clicking on the "SideShow Properties"
  entry of the display device menu.
  ---------------------------------------------------
}
procedure TMainForm.MenuSideshowPropertiesClick(Sender: TObject);
begin
  ShellExecute(MainForm.Handle, nil, 'control.exe', '/name Microsoft.WindowsSideShow', nil, SW_SHOW);
end;




{
  MenuSoundEnabledClick
  ---------------------------------------------------
  Event for clicking on "Enable sound".
  ---------------------------------------------------
}
procedure TMainForm.MenuSoundEnabledClick(Sender: TObject);
begin
  MenuSoundEnabled.Checked := not MenuSoundEnabled.Checked;

  OnModificationForConfig(Self);
end;




{
  MenuSoundClick
  ---------------------------------------------------
  Handler for changes to the audio sample rate.
  ---------------------------------------------------
}
procedure TMainForm.MenuSoundClick(Sender: TObject);
begin
  (Sender as TMenuItem).Checked := True;
  RestartAudio := True;

  OnModificationForConfig(Self);  
end;




{
  MenuUpdateClick
  ---------------------------------------------------
  Event for clicking one of the "update" entries in the
  tray menu.
  ---------------------------------------------------
}
procedure TMainForm.MenuUpdateClick(Sender: TObject);
begin
  // The time delay after sending a frame is stored in the
  // MenuItems "Tag" attribute.
  Delay := (Sender as TMenuItem).Tag;
  (Sender as TMenuItem).Checked := True;

  OnModificationForConfig(Self);
end;




{
  MenuEnableoOnConnectionClick
  ---------------------------------------------------
  Event for clicking on the auto enable menu item.
  ---------------------------------------------------
}
procedure TMainForm.MenuEnableOnStartupClick(Sender: TObject);
begin
  MenuEnableOnStartup.Checked := not MenuEnableOnStartup.Checked;

  OnModificationForConfig(Self);
end;




{
  MenuUseWirelessLANClick
  ---------------------------------------------------
  Enable Wlan mode.
  ---------------------------------------------------
}
procedure TMainForm.MenuUseWirelessLANClick(Sender: TObject);
begin
  if not (Sender as TMenuItem).Checked then
    (Sender as TMenuItem).Checked := True;

  MenuEnabled.Enabled := true;
  SetIcon(False);

  OnModificationForConfig(Self);
end;




{
  MenuChangeWlanSettingsClick
  ---------------------------------------------------
  Open the mainform and switch to the wlan tab.
  ---------------------------------------------------
}
procedure TMainForm.MenuChangeWlanSettingsClick(Sender: TObject);
begin
  PageControl.ActivePageIndex := 1;
  Show;
end;




{
  MenuControlCustomizeClick
  ---------------------------------------------------
  Clicking on the "Customize" option for the
  control menu.
  ---------------------------------------------------
}
procedure TMainForm.MenuControlCustomizeClick(Sender: TObject);
begin
  PageControl.ActivePageIndex := 4;
  MainForm.Show;
end;



{
  MenuCycleSideShowGadgetsClick
  ---------------------------------------------------
  Enabling and disabling SideShow cycling.
  ---------------------------------------------------
}
procedure TMainForm.MenuCycleSideShowGadgetsClick(Sender: TObject);
begin
  MenuCycleSideShowGadgets.Checked := not MenuCycleSideShowGadgets.Checked;
  LastSideShowCyclingTime := Time();

  OnModificationForConfig(Self);
end;




{
  MenuViewPortSizeClick
  ---------------------------------------------------
  Event for clicking on an entry in the viewport
  menu. Sets the new viewport depending on
  the menuitem and the rotation setting.
  ---------------------------------------------------
}
procedure TMainForm.MenuViewPortSizeClick(Sender: TObject);
var
  TempString: String;
begin
  TempString := (Sender as TMenuItem).Caption;
  TempString := StringReplace(TempString, '&', '', [rfReplaceAll]);

  TempViewPortWidth := StrToInt(Copy(TempString, 0, 3));
  TempViewPortHeight := StrToInt(Copy(TempString, 5, 3));

  ViewPortIndex := (Sender as TMenuItem).MenuIndex;

  (Sender as TMenuItem).Checked := True;

  ChangeRotationOnNextFrame := True;

  OnModificationForConfig(Self);
end;




{
  TimerTimer
  ---------------------------------------------------
  Gets invoked on timer events. Used for setting
  the viewport.
  ---------------------------------------------------
}
procedure TMainForm.TimerTimer(Sender: TObject);
var
  TempX: Integer;
  TempY: Integer;
  x: Integer;
  y: Integer;
begin
  try
    TempX := Mouse.CursorPos.x - MonitorLeft;
    TempY := Mouse.CursorPos.y - MonitorTop;
  except
    // An exception occurs here under Vista if the "elevation" dialog
    // pops up or the display style is changed (e.g. Aero -> Vista Basic)
    TempX := lastTempX;
    TempY := lastTempY;
  end;
  lastTempX := TempX;
  lastTempY := TempY;

  if ((TempX >= 0) and (TempX <= MonitorWidth) and (TempY >= 0) and (tempY <= MonitorHeight)) then
  begin
    // cursor is on the psp monitor
    if ((TempX = LastX) and (TempY = LastY)) then
    begin
      Inc(EqualValuesCount);
      FlashIcon(Odd(EqualValuesCount));
    end
    else begin
      LastX := TempX;
      LastY := TempY;
      EqualValuesCount := 0;
      SetIcon(True);
    end;

    if EqualValuesCount > 10 then
    begin
      Timer.Enabled := False;

      x := tempX - ViewPortWidth div 2;
      y := tempY - ViewPortHeight div 2;

      if (tempX < ViewPortWidth div 2) then
        x := 0;
      if (tempX > MonitorWidth - ViewPortWidth div 2) then
        x := MonitorWidth - ViewPortWidth;
      if (tempY < ViewPortHeight div 2) then
        y := 0;
      if (tempY > MonitorHeight - ViewPortHeight div 2) then
        y := MonitorHeight - ViewPortHeight;

      StartXSetting := x;
      StartYSetting := y;

      EqualValuesCount := 0;

      if StaticViewport then
      begin
        MenuStaticViewport.Checked := True;
      end;
    end;
  end;

end;




{
  MenuAboutClick
  ---------------------------------------------------
  Opening the about dialog at the correct tab.
  ---------------------------------------------------
}
procedure TMainForm.MenuAboutClick(Sender: TObject);
begin
  MainForm.Show;
  Mainform.PageControl.ActivePageIndex := 0;
end;




{
  CloseBtnClick
  ---------------------------------------------------
  Closing (hiding) the main form.
  ---------------------------------------------------
}
procedure TMainForm.CloseBtnClick(Sender: TObject);
begin
  MainForm.Hide;
end;




{
  ConfigTimer
  ---------------------------------------------------
  Check for modified settings and save them to the
  config file if necessary.
  ---------------------------------------------------
}
procedure TMainForm.ConfigTimerTimer(Sender: TObject);
begin
  if ConfigurationModified then
  begin
    SaveConfig();
    ConfigurationModified := False;
  end;
end;



{
  SetResolution
  ---------------------------------------------------
  Change resolution of the PSPdisp display adapter.
  ---------------------------------------------------
}
procedure TMainForm.SetResolution(Sender: TObject);
var
  index: Integer;
begin
  index := (Sender as TMenuItem).Tag;
  DispChangeResolution(DispGetDeviceNumber(), Resolutions[index].X, Resolutions[index].Y);
end;




{
  SettingsCloseClick
  ---------------------------------------------------
  Handler for closing the settings dialog. Just
  minimizes the window.
  ---------------------------------------------------
}
procedure TMainForm.SettingsCloseClick(Sender: TObject);
begin
  MainForm.Hide;
end;





{
  CreateResolutionList
  ---------------------------------------------------
  Retrieve all display modes from the PSPdisp
  adapter and populate the corresponding menus.
  ---------------------------------------------------
}
procedure TMainForm.CreateResolutionList;
var
  deviceNumber: Integer;
  modeNumber: Integer;
  width: Integer;
  height: Integer;
  Item: TMenuItem;
  NewResolution: TPoint;
  ResolutionCount: Integer;
begin
  MenuResolution.Clear;
  MenuResolutionRotated.Clear;
  SetLength(Resolutions, 0);

  deviceNumber := DispGetDeviceNumber();
  if (deviceNumber > -1) then
  begin
    ModeNumber := 0;
    ResolutionCount := 0;

    while DispEnumerateResolution(deviceNumber, ModeNumber, width, height) do
    begin
      NewResolution.X := width;
      NewResolution.Y := height;

      SetLength(Resolutions, Length(Resolutions) + 1);
      Resolutions[Length(Resolutions) - 1] := NewResolution;

      Inc(ResolutionCount);
      Inc(ModeNumber);
    end;

    for ModeNumber := 0 to ResolutionCount - 1 do
    begin
      if (ModeNumber < ResolutionCount div 2) then
      begin
        Item := TMenuItem.Create(MenuResolution);
        Item.Caption := IntToStr(Resolutions[ModeNumber].X) + 'x' + IntToStr(Resolutions[ModeNumber].Y);
        Item.Tag := ModeNumber;
        Item.OnClick := SetResolution;
        MenuResolution.Add(Item);
      end
      else begin
        Item := TMenuItem.Create(MenuResolutionRotated);
        Item.Caption := IntToStr(Resolutions[ModeNumber].X) + 'x' + IntToStr(Resolutions[ModeNumber].Y);
        Item.Tag := ModeNumber;
        Item.OnClick := SetResolution;
        MenuResolutionRotated.Add(Item);
      end;
    end;
  end;
end;







{
  MenuDelayCustomizeClick
  ---------------------------------------------------
  Clicking the "Customize" entry for the update
  interval menu.
  ---------------------------------------------------
}
procedure TMainForm.MenuDelayCustomizeClick(Sender: TObject);
begin
  PageControl.ActivePageIndex := 3;
  MainForm.Show;
end;




{
  MenuOpenMixerClick
  ---------------------------------------------------
  Show the mixer control panel.
  ---------------------------------------------------
}
procedure TMainForm.MenuOpenMixerClick(Sender: TObject);
begin
  if GetWindowsVersion() >= 6 then
    ShellExecute(MainForm.Handle, nil, 'control.exe', '/name Microsoft.AudioDevicesAndSoundThemes /page 1', nil, SW_SHOW)
  else
    ShellExecute(MainForm.Handle, nil, 'sndvol32.exe', PAnsiChar('/R /D' + IntToStr(audioGetMixerID(OptionsAudioDevice.ItemIndex))), nil, SW_SHOW);
end;




{
  CreateDisplayList
  ---------------------------------------------------
  Get all display adapters that are attached to
  the desktop and build the menu from them.
  ---------------------------------------------------
}
procedure TMainForm.CreateDisplayList;
var
  i: Integer;
  Item: TMenuItem;
  Size: TPoint;
  Name: String;
  DevString: String;
  deviceNumber: Integer;
begin
  // enable / disable options for PSPdisp driver
  deviceNumber := DispGetDeviceNumber();

  MenuEnableDisplay.OnClick := nil;
  if (deviceNumber = -1) then
  begin
    MenuEnableDisplay.Checked := False;
    MenuEnableDisplay.Enabled := False;
    MenuResolution.Enabled := False;
    MenuResolutionRotated.Enabled := False;
    MenuDisplayChangePosition.Enabled := False;
  end
  else begin
    MenuEnableDisplay.Checked := DispGetDisplayEnabledState(deviceNumber);
    MenuEnableDisplay.Enabled := True;
    MenuResolution.Enabled := True;
    MenuResolutionRotated.Enabled := True;
    MenuDisplayChangePosition.Enabled := True;
  end;
  MenuEnableDisplay.OnClick := MenuEnableDisplayClick;



  // clear list
  for i := MenuDisplay.Count - 1 downto MenuDisplayDeviceFirstIndex - 1 do
    MenuDisplay.Delete(i);


  Item := TMenuItem.Create(MenuDisplay);
  Item.Caption := '-';
  Item.OnClick := MenuSetDisplay;
  Item.RadioItem := True;

  MenuDisplay.Add(Item);

  for i := 0 to DispGetNumberOfDevices - 1 do
  begin
    DispGetInformation(i, Size, Name, DevString);

    if DispGetDisplayEnabledState(i) and (Size.X > 0) and (Size.Y > 0) then
    begin
      Item := TMenuItem.Create(MenuDisplay);
      Item.Caption := Name + ' (' + IntToStr(Size.X) + 'x' + IntToStr(Size.Y) + ')';
      Item.Tag := i;
      Item.OnClick := MenuSetDisplay;
      Item.RadioItem := True;

      MenuDisplay.Add(Item);
    end;
  end;

  // No device found, add a dummy device
  if (DispGetNumberOfDevices() = 0) then
  begin
    Item := TMenuItem.Create(MenuDisplay);
    Item.Caption := 'Unknown display';
    Item.Tag := i;
    Item.OnClick := MenuSetDisplay;
    Item.RadioItem := True;

    MenuDisplay.Add(Item);
  end;

  // click the last used display device
  if (MenuDisplay.Count > MenuDisplayDeviceFirstIndex + deviceNumber) then
    MenuDisplay[MenuDisplayDeviceFirstIndex + deviceNumber].Click();
end;





{
  MenuSetDisplay
  ---------------------------------------------------
  Set display mode.
  ---------------------------------------------------
}
procedure TMainForm.MenuSetDisplay(Sender: TObject);
begin
  CurrentDeviceNumber := (Sender as TMenuItem).Tag;
  (Sender as TMenuItem).Checked := True;
  GetMonitorParameters;
  ForceTransmission := True;
  OnModificationForConfig(Self);
end;




{
  MenuUseCaptureBltClick
  ---------------------------------------------------
  Event for clicking on the capture transparent
  windows menu item.
  Will cause cursur flicker if checked.
  ---------------------------------------------------
}
procedure TMainForm.MenuUseCaptureBltClick(Sender: TObject);
begin
  MenuUseCaptureBlt.Checked := not MenuUseCaptureBlt.Checked;

  if (MenuUseCaptureBlt.Checked) then
    copyMode := SRCCOPY or CAPTUREBLT
  else
    copyMode := SRCCOPY;

  ChangeRotationOnNextFrame := True;

  OnModificationForConfig(Self);
end;




{
  MenuUseSideshowClick
  ---------------------------------------------------
  Event for clicking on "Use SideShow device".
  ---------------------------------------------------
}
procedure TMainForm.MenuUseSideshowClick(Sender: TObject);
begin
  MenuUseSideshow.Checked := not MenuUseSideShow.Checked;
  LastSideShowCyclingTime := Time();
  ForceTransmission := True;

  OnModificationForConfig(Self);
end;




{
  CreateControlFileList
  ---------------------------------------------------
  Build the menu of available control fiels.
  ---------------------------------------------------
}
procedure TMainForm.CreateControlFileList;
var
  i : Integer;
  ControlFiles : TControlFileArray;
  NewMenuItem : TMenuItem;
begin
  for i := MenuControl.Count - 1 - 2 downto 3 do
  begin
    MenuControl.Delete(i);
  end;

  ControlFiles := CtrlEnumControlFiles();

  for i := 0 to Length(ControlFiles) - 1 do
  begin
    NewMenuItem := TMenuItem.Create(MenuControl);
    NewMenuItem.Caption := ControlFiles[i].Name;
    NewMenuItem.Tag := i;
    NewMenuItem.RadioItem := True;
    NewMenuItem.GroupIndex := 101;
    NewMenuItem.OnClick := SelectControlFile;
    MenuControl.Insert(MenuControl.Count - 2, NewMenuItem);
  end;
end;





{
  SelectControlFile
  ---------------------------------------------------
  Event for clicking in the control file menu.
  ---------------------------------------------------
}
procedure TMainForm.SelectControlFile(Sender: TObject);
begin
  (Sender as TMenuItem).Checked := True;
  if (Sender as TMenuItem).Tag = -1 then
  begin
    MenuControlEnabled.Checked := False;
  end
  else
  begin
    CtrlLoadConfiguration((Sender as TMenuItem).Tag);
    MenuControlEnabled.Checked := True;
  end;

  OnModificationForConfig(Self);
end;




{
  GetLocalAppDataFolder
  ---------------------------------------------------
  Retrive the application data path.
  ---------------------------------------------------
  Returns the path.
}
function TMainForm.GetAppDataFolder(): String;
var
  Path: Array[0..MAX_PATH] of Char;
begin
  SHGetFolderPath( 0, CSIDL_APPDATA, 1, 0, @Path[0]);
  Result := Path;
end;





{
  MenuReduceCpuLoadClick
  ---------------------------------------------------
  OnClick event for MenuReduceCpuLoad.
  ---------------------------------------------------
}
procedure TMainForm.MenuReduceCpuLoadClick(Sender: TObject);
begin
  MenuReduceCpuLoad.Checked := not MenuReduceCpuLoad.Checked;

  OnModificationForConfig(Self);
end;




{
  GetWindowsVersion
  ---------------------------------------------------
  Retrieves the major Windows version.
  ---------------------------------------------------
}
function TMainForm.GetWindowsVersion() : Integer;
var
   VersionInfo: TOSVersionInfo;
begin
   VersionInfo.dwOSVersionInfoSize := SizeOf(TOSVersionInfo) ;
   if GetVersionEx(VersionInfo) then
   begin
     Result := VersionInfo.dwMajorVersion;
   end
   else
     Result := 0;
end;



{
  OptionsScancodeWindowProc
  ---------------------------------------------------
  Determines the entered scancode in the options.
  ---------------------------------------------------
}
procedure TMainForm.OptionsScancodeWindowProc(var Msg: TMessage);
var
  scancode : Integer;
begin
  if Msg.Msg = WM_KEYDOWN then
  begin
      begin
        if (Msg.LParam and $01000000) > 0 then
          scancode := $E000
        else
          scancode := 0;

        scancode := scancode + (Msg.lparam and $00ff0000) div (256 * 256);
      end;

      OptionsScancode.Text := IntToHex(scancode , 4);
  end
  else if Msg.Msg = WM_KEYUP then
  begin

  end
  else
    OldWndProc(Msg);
end;



{
  OptionsSoundRefreshClick
  ---------------------------------------------------
  Rebuild the audio device list.
  ---------------------------------------------------
}
procedure TMainForm.OptionsSoundRefreshClick(Sender: TObject);
var
  ItemIndex: Integer;
begin
  ItemIndex := OptionsAudioDevice.ItemIndex;
  CreateAudioDeviceList();
  if ItemIndex < OptionsAudioDevice.Items.Count then
    OptionsAudioDevice.ItemIndex := ItemIndex
  else
    OptionsAudioDevice.ItemIndex := 0;

  SelectAudioDevice(True);
end;




{
  OptionsWlanNoPasswordClick
  ---------------------------------------------------
  Handler for click on "No Password" button.
  ---------------------------------------------------
}
procedure TMainForm.OptionsWlanNoPasswordClick(Sender: TObject);
begin
  OptionsWlanPassword.Text := '';
end;




{
  SelectAudioDevice
  ---------------------------------------------------
  Choose the best device for recording sounds.
  ---------------------------------------------------
}
procedure TMainForm.SelectAudioDevice(ChangeDevice: Boolean);
var
  i: Integer;
  CompareString: String;
  LastIndex: Integer;
  StereoIndex: Integer;
  MonoIndex: Integer;
  SoftwareIndex: Integer;
begin
  LastIndex := OptionsAudioDevice.ItemIndex;
  if GetWindowsVersion() > 5 then
  begin
    StereoIndex := -1;
    MonoIndex := -1;
    SoftwareIndex := -1;

    for i := 0 to OptionsAudioDevice.Items.Count - 1 do
    begin
      if OptionsAudioDevice.Items[i][1] = '*' then
        OptionsAudioDevice.Items[i] := Copy(OptionsAudioDevice.Items[i], 1, 256);

      CompareString := LowerCase(OptionsAudioDevice.Items[i]);

      if AnsiPos('stereomix', CompareString) > 0 then
        StereoIndex := i;

      if AnsiPos('stereo mix', CompareString) > 0 then
        StereoIndex := i;

      if AnsiPos('mixed output', CompareString) > 0 then
        StereoIndex := i;

      if AnsiPos('waveout mix', CompareString) > 0 then
        StereoIndex := i;

      if AnsiPos('waveoutmix', CompareString) > 0 then
        StereoIndex := i;

      if AnsiPos('what you hear', CompareString) > 0 then
        StereoIndex := i;

      if AnsiPos('monomix', CompareString) > 0 then
        MonoIndex := i;

      if AnsiPos('mono mix', CompareString) > 0 then
        MonoIndex := i;

      if AnsiPos('software loopback device', CompareString) > 0 then
        SoftwareIndex := i;
    end;

    if StereoIndex > -1 then
      i := StereoIndex
    else if MonoIndex > -1 then
      i := MonoIndex
    else
      i := SoftwareIndex;         

    OptionsAudioDevice.Items[i] := '*' + OptionsAudioDevice.Items[i];
    if ChangeDevice then
      OptionsAudioDevice.ItemIndex := i
    else
      OptionsAudioDevice.ItemIndex := LastIndex;
//    MessageDlg(CompareString, mtInformation, [mbok], -1);
  end
  else
  begin
    OptionsAudioDevice.Items[0] := '*' + OptionsAudioDevice.Items[0];
    OptionsAudioDevice.ItemIndex := LastIndex;
  end;
end;




{
  MenuSclaeCursorClick
  ---------------------------------------------------
  OnClick event for "Scale mouse cursor"
  ---------------------------------------------------
}
procedure TMainForm.MenuScaleCursorClick(Sender: TObject);
begin
  ConfigurationModified := True;
end;




{
  MenuScreenClick
  ---------------------------------------------------
  OnClick event for the screen menu.
  ---------------------------------------------------
}
procedure TMainForm.MenuScreenClick(Sender: TObject);
begin
  (Sender as TMenuItem).Checked := True;

  OnModificationForConfig(Self);
end;




{
  AboutVisitHomepageClick
  ---------------------------------------------------
  OnClick event for clicking the homepage link in the
  about dialog.
  ---------------------------------------------------
}
procedure TMainForm.AboutVisitHomepageClick(Sender: TObject);
begin
  ShellExecute(Handle, 'open', 'http://www.jjs.at/redirect/pspdisp/v0.6', nil, nil, SW_SHOWNORMAL);
end;



{
  Chooseinputdevice1Click
  ---------------------------------------------------
  OnClick event for choosing the input device in
  the sound menu.
  ---------------------------------------------------
}
procedure TMainForm.Chooseinputdevice1Click(Sender: TObject);
begin
  PageControl.ActivePageIndex := 5;
  MainForm.Show;
end;




{
  OptionsQualityOnChange
  ---------------------------------------------------
  Verifies the input and colors wrong data in red.
  ---------------------------------------------------
}
procedure TMainForm.OptionsQualityOnChange(Sender: TObject);
var
  CurrentValue: Integer;
begin
  CurrentValue := StrToIntDef((Sender as TEdit).Text, -1);

  if (CurrentValue < 0) or (CurrentValue > 100) then
    (Sender as TEdit).Font.Color := clRed
  else
  begin
    (Sender as TEdit).Font.Color := clWindowText;
    if Sender = OptionsQualityHighest then
    begin
      MenuQualityHighest.Tag := CurrentValue;
      if MenuQualityHighest.Checked then
        JpegQuality := CurrentValue;
    end
    else if Sender = OptionsQualityHigh then
    begin
      MenuQualityHigh.Tag := CurrentValue;
      if MenuQualityHigh.Checked then
        JpegQuality := CurrentValue;
    end
    else if Sender = OptionsQualityMedium then
    begin
      MenuQualityMedium.Tag := CurrentValue;
      if MenuQualityMedium.Checked then
        JpegQuality := CurrentValue;
    end
    else if Sender = OptionsQualityLow then
    begin
      MenuQualityLow.Tag := CurrentValue;
      if MenuQualityLow.Checked then
        JpegQuality := CurrentValue;
    end
    else if Sender = OptionsQualityLowest then
    begin
      MenuQualityLowest.Tag := CurrentValue;
      if MenuQualityLowest.Checked then
        JpegQuality := CurrentValue;
    end;

  end;
end;




{
  OptionsQualityOnExit
  ---------------------------------------------------
  Verifies the input and resets wrong input to the
  default value.
  ---------------------------------------------------
}
procedure TMainForm.OptionsQualityOnExit(Sender: TObject);
var
  CurrentValue: Integer;
begin
  CurrentValue := StrToIntDef((Sender as TEdit).Text, -1);

  if (CurrentValue < 0) or (CurrentValue > 100) then
    (Sender as TEdit).Text := IntToStr((Sender as TEdit).Tag);
end;





{
  OptionsQualityResetClick
  ---------------------------------------------------
  Reset the quality values to the defaults.
  ---------------------------------------------------
}
procedure TMainForm.OptionsQualityResetClick(Sender: TObject);
begin
  OptionsQualityHighest.Text := IntToStr(OptionsQualityHighest.Tag);
  OptionsQualityHigh.Text := IntToStr(OptionsQualityHigh.Tag);
  OptionsQualityMedium.Text := IntToStr(OptionsQualityMedium.Tag);
  OptionsQualityLow.Text := IntToStr(OptionsQualityLow.Tag);
  OptionsQualityLowest.Text := IntToStr(OptionsQualityLowest.Tag);
end;




{
  OptionsQualitySideShowChange
  ---------------------------------------------------
  Set the menu items tag when the quality setting
  is changed.
  ---------------------------------------------------
}
procedure TMainForm.OptionsQualitySideShowChange(Sender: TObject);
begin
  MenuQualitySideShow.Tag := StrToIntDef(OptionsQualitySideShow.Text, 95);
end;






{
  OptionsDelayOnChangeO
  ---------------------------------------------------
  Verifies the input and colors wrong data in red.
  ---------------------------------------------------
}
procedure TMainForm.OptionsDelayOnChange(Sender: TObject);
var
  CurrentValue: Integer;
begin
  CurrentValue := StrToIntDef((Sender as TEdit).Text, -1);

  if (CurrentValue < 0) or (CurrentValue > 1000000) then
    (Sender as TEdit).Font.Color := clRed
  else
  begin
    (Sender as TEdit).Font.Color := clWindowText;
    if Sender = OptionsDelayLongest then
    begin
      if MenuUpdateSleep5.Checked then
        Delay := CurrentValue;
      MenuUpdateSleep5.Caption := 'Min. ' + IntToStr(CurrentValue) + ' ms per frame';
      MenuUpdateSleep5.Tag := CurrentValue;
    end
    else if Sender = OptionsDelayLong then
    begin
      if MenuUpdateSleep4.Checked then
        Delay := CurrentValue;
      MenuUpdateSleep4.Caption := 'Min. ' + IntToStr(CurrentValue) + ' ms per frame';
      MenuUpdateSleep4.Tag := CurrentValue;
    end
    else if Sender = OptionsDelayMedium then
    begin
      if MenuUpdateSleep3.Checked then
        Delay := CurrentValue;
      MenuUpdateSleep3.Caption := 'Min. ' + IntToStr(CurrentValue) + ' ms per frame';
      MenuUpdateSleep3.Tag := CurrentValue;
    end
    else if Sender = OptionsDelayShort then
    begin
      if MenuUpdateSleep2.Checked then
        Delay := CurrentValue;
      MenuUpdateSleep2.Caption := 'Min. ' + IntToStr(CurrentValue) + ' ms per frame';
      MenuUpdateSleep2.Tag := CurrentValue;
    end
    else if Sender = OptionsDelayShortest then
    begin
      if MenuUpdateSleep1.Checked then
        Delay := CurrentValue;
      MenuUpdateSleep1.Caption := 'Min. ' + IntToStr(CurrentValue) + ' ms per frame';
      MenuUpdateSleep1.Tag := CurrentValue;
    end;
  end;
end;




{
  OptionsDelayOnExit
  ---------------------------------------------------
  Verifies the input and resets wrong data to the
  default value.
  ---------------------------------------------------
}
procedure TMainForm.OptionsDelayOnExit(Sender: TObject);
var
  CurrentValue: Integer;
begin
  CurrentValue := StrToIntDef((Sender as TEdit).Text, -1);

  if (CurrentValue < 0) or (CurrentValue > 1000000) then
    (Sender as TEdit).Text := IntToStr((Sender as TEdit).Tag);
end;



{
  OptionsDelayResetClick
  ---------------------------------------------------
  Reset the delay values to the defaults.
  ---------------------------------------------------
}
procedure TMainForm.OptionsDelayResetClick(Sender: TObject);
begin
  OptionsDelayLongest.Text := IntToStr(OptionsDelayLongest.Tag);
  OptionsDelayLong.Text := IntToStr(OptionsDelayLong.Tag);
  OptionsDelayMedium.Text := IntToStr(OptionsDelayMedium.Tag);
  OptionsDelayShort.Text := IntToStr(OptionsDelayShort.Tag);
  OptionsDelayShortest.Text := IntToStr(OptionsDelayShortest.Tag);
end;












{
 *************************************************
   Display Visualizer
 *************************************************
}



procedure TMainForm.OptionsDisplayApplySettingsClick(Sender: TObject);
begin
  DispSetPosition(DispGetDeviceNumber(), StrToIntDef(OptionsDisplayPositionX.text, 0), StrToIntDef(OptionsDisplayPositionY.text, 0));
end;



procedure TMainForm.UpdateDisplayInformations();
var
  j: Integer;
  i: Integer;
begin
  UpdatingDisplayInformation := True;

  // Get all displays
  NumberOfDisplays := DispGetNumberOfDevices();

  PSPdispDriverIndex := -1;
  LeftBorder := 0;
  RightBorder := 0;
  TopBorder := 0;
  BottomBorder := 0;
  NumberOfDisplaysInArray := 0;
  j := 0;

  for i := 0 to NumberOfDisplays - 1 do
  begin
    if (DispGetDisplayEnabledState(i)) then
    begin
      DispGetInformation(i, DisplaySize[j], DisplayDeviceString[j], DisplayDeviceName[j]);
      DispGetPosition(i, DisplayPosition[j].X, DisplayPosition[j].Y);

      Inc(j);
    end;
  end;

  NumberOfDisplaysInArray := j;

  UpdatingDisplayInformation := False;
end;



procedure TMainForm.DisplayPaintBoxPaint(Sender: TObject);
var
  i: integer;
  j: integer;
begin
  UpdateDisplayInformations();

  DisplayPaintBox.Canvas.Brush.Color := clBlack;
  DisplayPaintBox.Canvas.FillRect(DisplayPaintBox.Canvas.ClipRect);


  // Get all displays
  PSPdispDriverIndex := -1;
  LeftBorder := 0;
  RightBorder := 0;
  TopBorder := 0;
  BottomBorder := 0;
  j := NumberOfDisplaysInArray;

  for i := 0 to j - 1 do
  begin
    if (DisplayPosition[i].X < LeftBorder) then
      LeftBorder := DisplayPosition[i].X;

    if (DisplayPosition[i].Y < TopBorder) then
      TopBorder := DisplayPosition[i].Y;

    if ((DisplayPosition[i].X + DisplaySize[i].X) > RightBorder) then
      RightBorder := (DisplayPosition[i].X + DisplaySize[i].X);

    if ((DisplayPosition[i].Y + DisplaySize[i].Y) > BottomBorder) then
      BottomBorder := (DisplayPosition[i].Y + DisplaySize[i].Y);

    if (StrPos(PAnsiChar(DisplayDeviceString[i]), 'PSPdisp Display Driver') <> nil) then
      PSPdispDriverIndex := i;
  end;

  if (PSPdispDriverIndex > -1) then
  begin
    OptionsDisplayPositionX.Text := IntToStr(DisplayPosition[PSPdispDriverIndex].X);
    OptionsDisplayPositionY.Text := IntToStr(DisplayPosition[PSPdispDriverIndex].Y);
  end
  else
  begin
    OptionsDisplayPositionX.Text := 'n/a';
    OptionsDisplayPositionY.Text := 'n/a';
  end;



  TotalWidth := Abs(LeftBorder) + Abs(RightBorder);
  TotalHeight := Abs(TopBorder) + Abs(BottomBorder);

  if (DisplayPaintBox.Width / DisplayPaintBox.Height) >= (TotalWidth / TotalHeight) then
    ScalingFactor := DisplayPaintBox.Height / TotalHeight / 2
  else
    ScalingFactor := DisplayPaintBox.Width / TotalWidth / 2;


  for i := 0 to j - 1 do
  begin
    if (i = PSPdispDriverIndex) then
      DisplayPaintBox.Canvas.Brush.Color := clWhite
    else
      DisplayPaintBox.Canvas.Brush.Color := clGray;

    PrintPositionX := Round((DisplayPaintBox.Width / 2) + (DisplayPosition[i].X * ScalingFactor));
    PrintPositionY := Round(((DisplayPosition[i].Y + TotalHeight / 2) * ScalingFactor));
    PrintSizeX := Round(DisplaySize[i].X * ScalingFactor);
    PrintSizeY := Round(DisplaySize[i].Y * ScalingFactor);

    DisplayPaintBox.Canvas.Rectangle(PrintPositionX, PrintPositionY, PrintPositionX + PrintSizeX, PrintPositionY + PrintSizeY);
  end;
end;







{
 *************************************************
   Presets
 *************************************************
}         


procedure TMainForm.CreatePresetFileList;
var
  i : Integer;
  PresetFiles : TPresetFileArray;
  NewMenuItem : TMenuItem;
begin
  if (MenuPresets.Count > 2) then
  begin
    for i := MenuPresets.Count - 3 downto 0 do
    begin
      MenuPresets.Delete(i);
    end;
  end;

  OptionsPresetsList.Clear;
  OptionsPresetsList.Items.Add('<new preset>');

  PresetFiles := ConfigEnumPresetFiles();

  for i := 0 to Length(PresetFiles) - 1 do
  begin
    NewMenuItem := TMenuItem.Create(MenuPresets);
    NewMenuItem.Caption := PresetFiles[i].Name;
    NewMenuItem.Tag := i;
    NewMenuItem.OnClick := SelectPresetFile;
    MenuPresets.Insert(MenuPresets.Count - 2, NewMenuItem);
    OptionsPresetsList.Items.Add(PresetFiles[i].Name + ' (' + PresetFiles[i].Filename + ')');
  end;

  if (OptionsPresetsList.ItemIndex = -1) then
    OptionsPresetsList.ItemIndex := 0;
  OptionsPresetsList.OnClick(OptionsPresetsList);
end;


procedure TMainForm.SelectPresetFile(Sender: TObject);
begin
  LoadPreset((Sender as TMenuItem).MenuIndex);
end;


procedure TMainForm.OptionsPresetsDeleteClick(Sender: TObject);
begin
  DeleteFile(GetAppDataFolder + '\PSPdisp\preset\' + PresetFiles[OptionsPresetsList.ItemIndex - 1].Filename);
  CreatePresetFileList();
  OnModificationForConfig(Self);
end;


procedure TMainForm.OptionsPresetsListClick(Sender: TObject);
begin
  OptionsPresetsDelete.Enabled := (OptionsPresetsList.ItemIndex > 0);
  OptionsPresetsLoad.Enabled := (OptionsPresetsList.ItemIndex > 0);
  if (OptionsPresetsList.ItemIndex = 0) then
    OptionsPresetsName.Text := 'Please enter a name'
  else
    OptionsPresetsName.Text := PresetFiles[OptionsPresetsList.ItemIndex - 1].Name;
end;


procedure TMainForm.OptionsPresetsRefreshClick(Sender: TObject);
begin
  CreatePresetFileList();
  OnModificationForConfig(Self);
end;


procedure TMainForm.OptionsPresetsSaveClick(Sender: TObject);
var
  Path: String;
  Filename: String;
begin
  if (OptionsPresetsList.ItemIndex = 0) then
  begin
    Path := GetAppDataFolder + '\PSPdisp\preset\';
    ForceDirectories(Path);
    SaveDialog.InitialDir := Path;
    SaveDialog.FileName := Path + 'new.preset';
    if (SaveDialog.Execute) then
    begin
      Filename := ExtractFilename(SaveDialog.Filename);
      if (ExtractFileExt(Filename) <> '.preset') then
        Filename := Filename + '.preset';
      SavePreset(OptionsPresetsList.ItemIndex, OptionsPresetsName.Text, Filename);
    end;
  end
  else begin
    SavePreset(OptionsPresetsList.ItemIndex, OptionsPresetsName.Text, PresetFiles[OptionsPresetsList.ItemIndex - 1].Filename);
  end;

  CreatePresetFileList();
  OnModificationForConfig(Self);  
end;


procedure TMainForm.OptionsPresetsLoadClick(Sender: TObject);
begin
  LoadPreset(OptionsPresetsList.ItemIndex - 1);
end;


procedure TMainForm.OptionsPresetsOpenFolderClick(Sender: TObject);
begin
  ForceDirectories(MainForm.GetAppDataFolder + '\PSPdisp\preset\');
  ShellExecute(MainForm.Handle, 'explore', PChar(GetAppDataFolder + '\PSPdisp\preset\'), '', nil, SW_SHOW);
end;


procedure TMainForm.MenuEditPresetsClick(Sender: TObject);
begin
  PageControl.ActivePageIndex := 8;
  MainForm.Show;
end;


end.



