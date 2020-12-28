{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  display.pas - dealing with the display API

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}

unit display;

interface

uses
  Windows, Dialogs, SysUtils;

var
  CurrentDeviceNumber: Integer;
  CurrentDeviceName: String;
  CurrentDeviceString: String;

  PSPdispDriverPosition: TPoint;
  PSPdispDriverSize: TPoint;

function DispGetNumberOfDevices: Integer;
function DispChangeResolution(DeviceNumber: Integer; Width: Integer; Height: Integer): Boolean;
function DispSetDisplayEnabledState(DeviceNumber: Integer; Enable: Boolean): Boolean;
function DispGetDisplayEnabledState(DeviceNumber: Integer): Boolean;
function DispSetPosition(DeviceNumber: Integer; PositionX: Integer; PositionY: Integer): Boolean;
function DispGetPosition(DeviceNumber: Integer; var PositionX: Integer; var PositionY: Integer): Boolean;
procedure DispGetInformation(DeviceNumber: Integer; var Size: TPoint; var DeviceString: String; var DeviceName: String);
function DispGetDeviceNumber: Integer;
function DispEnumerateResolution(DeviceNumber: Integer; ModeNumber: Integer; var Width: Integer; var Height: Integer): Boolean;
function DispHasScreenChanged: Boolean;
function DispGetDeviceContainingPoint(Point: TPoint): Integer;
procedure DispSwitchDesktopCompositionState(Enable: Boolean);



implementation

const
  // For some reason those are not defined in Turbo Delphi 2006
  ENUM_CURRENT_SETTINGS = DWORD(-1);
  ENUM_REGISTRY_SETTINGS = DWORD(-2);
  DISPLAY_DEVICE_ATTACHED_TO_DESKTOP = 1;

  // This has to match the device name in the display driver
  DRIVER_NAME = 'PSPdisp Display Driver';
  DRIVER_NAME_VISTA = 'PSPdisp Display Driver (Vista)';

  // Custom escape command for checking if the screen content
  // has changed, must match the definition in the display driver
  ESCAPE_IS_DIRTY = ($10000 + 100);

type
  // TDevMode declaration in windows.pas
  // is only for printer devices
  TDevModeMonitor = packed record
    dmDeviceName: array[0..CCHDEVICENAME - 1] of AnsiChar;
    dmSpecVersion: Word;
    dmDriverVersion: Word;
    dmSize: Word;
    dmDriverExtra: Word;
    dmFields: DWORD;

    // this is different from the windows.pas declaration
    dmPosition: TPoint;
    dmDisplayOrientation: DWORD;
    dmDisplayFixedOutput: DWORD;

    dmColor: SHORT;
    dmDuplex: SHORT;
    dmYResolution: SHORT;
    dmTTOption: SHORT;
    dmCollate: SHORT;
    dmFormName: array[0..CCHFORMNAME - 1] of AnsiChar;
    dmLogPixels: Word;
    dmBitsPerPel: DWORD;
    dmPelsWidth: DWORD;
    dmPelsHeight: DWORD;
    dmDisplayFlags: DWORD;
    dmDisplayFrequency: DWORD;
    dmICMMethod: DWORD;
    dmICMIntent: DWORD;
    dmMediaType: DWORD;
    dmDitherType: DWORD;
    dmICCManufacturer: DWORD;
    dmICCModel: DWORD;
    dmPanningWidth: DWORD;
    dmPanningHeight: DWORD;
  end;

type
  DWMAPI_DwmEnableComposition = function(uCompositionAction: DWORD): HRESULT; stdcall;

var
  DwmEnableComposition: DWMAPI_DwmEnableComposition;




{
  DispSwitchDesktopCompositionState
  ---------------------------------------------------
  Enable or disable the Aero desktop composition.
  ---------------------------------------------------
}
procedure DispSwitchDesktopCompositionState(Enable: Boolean);
const
  DWM_EC_DISABLECOMPOSITION = 0;
  DWM_EC_ENABLECOMPOSITION = 1;
var
  DWMLibrary: THandle;
begin
  DWMlibrary := LoadLibrary('DWMAPI.dll');
  DwmEnableComposition := GetProcAddress(DWMLibrary, 'DwmEnableComposition');
  if DWMlibrary <> 0 then
    begin
    if @DwmEnableComposition <> nil then
    begin
      if Enable then
        DwmEnableComposition(DWM_EC_ENABLECOMPOSITION)
      else
        DwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
    end;
  end;
  FreeLibrary(DWMLibrary);
  DwmEnableComposition := nil;
end;


  

{
  DispGetDeviceContainingPoint
  ---------------------------------------------------
  Retrieve the graphics device on which the given
  point lies.
  ---------------------------------------------------
  Returns device index.
}
function DispGetDeviceContainingPoint(Point: TPoint): Integer;
var
  i: Integer;
  DeviceName: String;
  DeviceString: String;
  Size: TPoint;
  PositionX: Integer;
  PositionY: Integer;

begin
  for i := 0 to DispGetNumberOfDevices - 1 do
  begin
    DispGetInformation(i, Size, DeviceString, DeviceName);

    if DispGetDisplayEnabledState(i) and (Size.X > 0) and (Size.Y > 0) then
    begin
      DispGetPosition(i, PositionX, PositionY);
      if (Point.X >= PositionX) and (Point.X < PositionX + Size.X) and (Point.Y >= PositionY) and (Point.Y < PositionY + Size.Y) then
      begin
        // Point lies on the devices screen
        Result := i;
        Exit;
      end;
    end;
  end;
  Result := -1;
end;




{
  DispHasScreenChanged
  ---------------------------------------------------
  Check if the screen content has changed.
  ---------------------------------------------------
  Returns False if the PSPdisp display screen
  has not changed, in any other case it returns True.
}
function DispHasScreenChanged: Boolean;
var
  ReturnValue: Integer;
  Handle: HDC;
begin
  if ((CurrentDeviceName = DRIVER_NAME) or (CurrentDeviceName = DRIVER_NAME_VISTA)) then
  begin
    ReturnValue := 99;

    Handle := CreateDC('DISPLAY', PAnsiChar(CurrentDeviceString), nil, nil);

    if (Handle > 0) then
    begin
      ExtEscape(Handle, ESCAPE_IS_DIRTY, 0, nil, 4, PAnsiChar(@ReturnValue));
      DeleteDC(Handle);
    end;
    
    Result := (ReturnValue <> 0);
  end
  else
    Result := True;
end;




{
  DispGetNumberOfDevices
  ---------------------------------------------------
  Get the number of display devices.
  ---------------------------------------------------
  Returns the number of devices.
}
function DispGetNumberOfDevices: Integer;
var
  DevNum: Cardinal;
  DisplayDevice: TDisplayDevice;

begin
  DevNum := 0;

  ZeroMemory(@DisplayDevice, sizeof(DisplayDevice));
  DisplayDevice.cb := sizeof(DisplayDevice);

  // search for the display
  while EnumDisplayDevices(nil, DevNum, DisplayDevice, 0) do
    Inc(DevNum);

  Result := DevNum;
end;





{
  DispSetPosition
  ---------------------------------------------------
  Change the position of the specified display.
  ---------------------------------------------------
  Returns True on success.
}
function DispSetPosition(DeviceNumber: Integer; PositionX: Integer; PositionY: Integer): Boolean;
var
  DevNum: Integer;
  DisplayDevice: TDisplayDevice;
  DevMode: TDevModeMonitor;

begin
  DevNum := DispGetDeviceNumber;

  if DevNum > -1 then
  begin
    ZeroMemory(@DisplayDevice, sizeof(DisplayDevice));
    DisplayDevice.cb := sizeof(DisplayDevice);

    EnumDisplayDevices(nil, DevNum, DisplayDevice, 0);

    // device is present, get position info
    ZeroMemory(@DevMode, sizeof(TDevModeMonitor));
    DevMode.dmSize := sizeof(TDevModeMonitor);
    EnumDisplaySettings(DisplayDevice.DeviceName, ENUM_REGISTRY_SETTINGS, TDevMode(DevMode));

    DevMode.dmPosition.X := PositionX;
    DevMode.dmPosition.Y := PositionY;

    DevMode.dmFields := (DM_POSITION);

    ChangeDisplaySettingsEx(DisplayDevice.DeviceName, TDevMode(DevMode), 0, CDS_UPDATEREGISTRY, nil);

    Result := True;
  end
  else begin
    Result := False;
  end;
end;



{
  DispSetDisplayEnabledState
  ---------------------------------------------------
  Enable or disable the display driver as well as
  attaching to and detach it from the desktop.
  Look here for C code to attach / detach a display:
  MS Knowledge Base article 306399
  ---------------------------------------------------
  Return False if the driver is not installed.
}
function DispSetDisplayEnabledState(DeviceNumber: Integer; Enable: Boolean): Boolean;
var
  DevNum: Integer;
  DisplayDevice: TDisplayDevice;
  DevMode: TDevModeMonitor;
  Res: Integer;

begin
  DevNum := DispGetDeviceNumber;

  if DevNum > -1 then
  begin
    DispSwitchDesktopCompositionState(not Enable);

    ZeroMemory(@DisplayDevice, sizeof(DisplayDevice));
    DisplayDevice.cb := sizeof(DisplayDevice);

    EnumDisplayDevices(nil, DevNum, DisplayDevice, 0);

    // device is present, get position info
    ZeroMemory(@DevMode, sizeof(TDevModeMonitor));
    DevMode.dmSize := sizeof(TDevModeMonitor);
    EnumDisplaySettings(DisplayDevice.DeviceName, ENUM_REGISTRY_SETTINGS, TDevMode(DevMode));

    // enabling a display device:
    // - set width and height to a valid value or
    //   just use the one stored in DevMode
    // - position doesn't need to be set, it's last value is retrieved
    //   from the registry

    // disabling a display device:
    // - set width and height to 0

    if Enable then
    begin
      if ((DevMode.dmPelsWidth = 0) or (DevMode.dmPelsHeight = 0)) then
      begin
        DevMode.dmPelsWidth := 480;
        DevMode.dmPelsHeight := 272;
      end;
    end
    else begin
      DevMode.dmPelsWidth := 0;
      DevMode.dmPelsHeight := 0;
    end;

    DevMode.dmFields := (DM_PELSWIDTH or DM_PELSHEIGHT or DM_POSITION);

    Res := ChangeDisplaySettingsEx(DisplayDevice.DeviceName, TDevMode(DevMode), 0, CDS_UPDATEREGISTRY, nil);

    Result := (Res = DISP_CHANGE_SUCCESSFUL);
  end
  else begin
    Result := False;
  end;
end;




{
  DispGetDisplayEnabledState
  ---------------------------------------------------
  Determine wether the display is currently
  attached to the desktop.
  ---------------------------------------------------
  Return True if it is and False if not.
}
function DispGetDisplayEnabledState(DeviceNumber: Integer): Boolean;
var
  DevNum: Integer;
  DisplayDevice: _DISPLAY_DEVICEA;
  DevMode: TDevModeMonitor;

begin
  DevNum := DeviceNumber;

  if DevNum > -1 then
  begin
    ZeroMemory(@DisplayDevice, sizeof(DisplayDevice));
    DisplayDevice.cb := sizeof(DisplayDevice);

    EnumDisplayDevices(nil, DevNum, DisplayDevice, 0);

    ZeroMemory(@DevMode, sizeof(TDevModeMonitor));
    DevMode.dmSize := sizeof(TDevModeMonitor);
    EnumDisplaySettings(DisplayDevice.DeviceName, ENUM_REGISTRY_SETTINGS, TDevMode(DevMode));

    // Report mirror drivers as not attached
    Result := (DisplayDevice.StateFlags and DISPLAY_DEVICE_ATTACHED_TO_DESKTOP > 0)
      and not (DisplayDevice.StateFlags and DISPLAY_DEVICE_MIRRORING_DRIVER > 0);
  end
  else begin
    Result := False;
  end;
end;







{
  DispGetDeviceNumber
  ---------------------------------------------------
  Get the index of the PSPdisp display device.
  ---------------------------------------------------
  Returns the device index.
}
function DispGetDeviceNumber: Integer;
var
  DevNum: Cardinal;
  DisplayDevice: TDisplayDevice;

begin
  DevNum := 0;

  ZeroMemory(@DisplayDevice, sizeof(DisplayDevice));
  DisplayDevice.cb := sizeof(DisplayDevice);

  // search for the display
  while EnumDisplayDevices(nil, DevNum, DisplayDevice, 0) do
  begin
    if (((DisplayDevice.DeviceString = DRIVER_NAME)
       or (DisplayDevice.DeviceString = DRIVER_NAME_VISTA))
      and not (DisplayDevice.StateFlags and DISPLAY_DEVICE_MIRRORING_DRIVER > 0)) then
    begin
      Result := DevNum;
      Exit;
    end;

    Inc(DevNum);
  end;

  Result := -1;
end;





{
  DispGetPosition
  ---------------------------------------------------
  Get the desktop position of the attached display.
  ---------------------------------------------------
  Return False if the driver is not installed.
}
function DispGetPosition(DeviceNumber: Integer; var PositionX: Integer; var PositionY: Integer): Boolean;
var
  DevNum: Integer;
  DisplayDevice: TDisplayDevice;
  DevMode: TDevModeMonitor;

begin
  DevNum := DeviceNumber;

  if DevNum > -1 then
  begin
    ZeroMemory(@DisplayDevice, sizeof(DisplayDevice));
    DisplayDevice.cb := sizeof(DisplayDevice);

    EnumDisplayDevices(nil, DevNum, DisplayDevice, 0);

    ZeroMemory(@DevMode, sizeof(TDevModeMonitor));
    DevMode.dmSize := sizeof(TDevModeMonitor);
    EnumDisplaySettings(DisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, TDevMode(DevMode));

    PositionX := DevMode.dmPosition.x;
    PositionY := DevMode.dmPosition.y;

    Result := True;
  end
  else begin
    PositionX := 0;
    PositionY := 0;
    Result := False;
  end;
end;




{
  DispGetInformation
  ---------------------------------------------------
  Retrieves the current display mode and
  display adapter name.
  ---------------------------------------------------
}
procedure DispGetInformation(DeviceNumber: Integer; var Size: TPoint; var DeviceString: String; var DeviceName: String);
var
  DevNum: Integer;
  DisplayDevice: TDisplayDevice;
  DevMode: TDevModeMonitor;

begin
  DevNum := DeviceNumber;

  if DevNum > -1 then
  begin
    ZeroMemory(@DisplayDevice, sizeof(DisplayDevice));
    DisplayDevice.cb := sizeof(DisplayDevice);

    EnumDisplayDevices(nil, DevNum, DisplayDevice, 0);

    ZeroMemory(@DevMode, sizeof(TDevModeMonitor));
    DevMode.dmSize := sizeof(TDevModeMonitor);
    // For some reason ENUM_CURRENT_SETTINGS works on Windows 7 RC
    // while ENUM_REGISTRY_SETTINGS seems to always report (0,0) for size
    // on 64 bit systems and at the first start on 32 bit systems 
    EnumDisplaySettings(DisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, TDevMode(DevMode));

    Size.X := DevMode.dmPelsWidth;
    Size.Y := DevMode.dmPelsHeight;
    DeviceName := DisplayDevice.DeviceName;
    DeviceString := DisplayDevice.DeviceString;
  end
  else begin
    Size.X := 0;
    Size.Y := 0;
    DeviceName := '';
    DeviceString := '';
  end;
end;




{
  DispChangeResolution
  ---------------------------------------------------
  Changes the display mode.
  ---------------------------------------------------
  Returns True on success.
}
function DispChangeResolution(DeviceNumber: Integer; Width: Integer; Height: Integer): Boolean;
var
  DevNum: Integer;
  DisplayDevice: TDisplayDevice;
  DevMode: TDevModeMonitor;
  ReturnedValue: Integer;

begin
  DevNum := DeviceNumber;

  if DevNum > -1 then
  begin
    ZeroMemory(@DisplayDevice, sizeof(DisplayDevice));
    DisplayDevice.cb := sizeof(DisplayDevice);

    EnumDisplayDevices(nil, DevNum, DisplayDevice, 0);

    ZeroMemory(@DevMode, sizeof(TDevModeMonitor));
    DevMode.dmSize := sizeof(TDevModeMonitor);
    EnumDisplaySettings(DisplayDevice.DeviceName, ENUM_REGISTRY_SETTINGS, TDevMode(DevMode));

    DevMode.dmPelsWidth := Width;
    DevMode.dmPelsHeight := Height;
    DevMode.dmFields := DM_PELSWIDTH or DM_PELSHEIGHT;

    ReturnedValue := ChangeDisplaySettingsEx(DisplayDevice.DeviceName, TDevMode(DevMode), 0, CDS_UPDATEREGISTRY, nil);

    Result := (ReturnedValue = DISP_CHANGE_SUCCESSFUL);
  end
  else begin
    Result := False;
  end;
end;





{
  DispEnumerateResolution
  ---------------------------------------------------
  Retrieves information about the specified
  display mode.
  ---------------------------------------------------
  Returns True on success.
}
function DispEnumerateResolution(DeviceNumber: Integer; ModeNumber: Integer; var Width: Integer; var Height: Integer): Boolean;
var
  DevNum: Integer;
  DisplayDevice: TDisplayDevice;
  DevMode: TDevModeMonitor;
  ReturnedValue: Boolean;

begin
  DevNum := DeviceNumber;

  if DevNum > -1 then
  begin
    ZeroMemory(@DisplayDevice, sizeof(DisplayDevice));
    DisplayDevice.cb := sizeof(DisplayDevice);

    EnumDisplayDevices(nil, DevNum, DisplayDevice, 0);

    ZeroMemory(@DevMode, sizeof(TDevModeMonitor));
    DevMode.dmSize := sizeof(TDevModeMonitor);

    DevMode.dmFields := DM_PELSWIDTH or DM_PELSHEIGHT;

    ReturnedValue := EnumDisplaySettings(DisplayDevice.DeviceName, ModeNumber, TDevMode(DevMode));

    if (ReturnedValue) then
    begin
      Width := DevMode.dmPelsWidth;
      Height := DevMode.dmPelsHeight;
    end
    else begin
      Width := 0;
      Height := 0;
    end;
    Result := ReturnedValue;
  end
  else begin
    Result := False;
  end;
end;




end.
