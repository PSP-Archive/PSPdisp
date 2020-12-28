{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  control.pas - handling PSP buttons

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}

unit control;

interface

uses Windows, SysUtils, IniFiles, Controls, Forms, Messages;

var
  EnableKeyboard: Boolean;

type
  TControlBuffer = packed record
    Buttons: Cardinal;
    AnalogX: Byte;
    AnalogY: Byte;
  end;

  TResponseBuffer = packed record
    Magic: Cardinal;
    Flags: Cardinal;
    ControlBuffer: TControlBuffer;
  end;

  TButtonAction = record
    ButtonCode: Integer;
    ScanCode: Array of Integer;
    MouseButton: Integer;
    Down: Boolean;
    DownTime: Word;
    Typematic: Boolean;
    EventType: Integer;
    MoveByPixels: Integer;
    PSPdispAction: Integer;
    NoBreak: Boolean;
  end;

  TKey = record
    Scancode: Integer;
    Name: String;
  end;

  TPSPButton = record
    Code: Integer;
    Name: String;
  end;

  TControlFile = record
    Name: String;
    Filename: String;
  end;

  TControlFileArray = Array of TControlFile;

  TSideshowOutput = record
    Counter: Integer;
    Button: Integer;
  end;

const
  PSP_CTRL_SELECT = $000001;
  PSP_CTRL_START = $000008;
  PSP_CTRL_UP = $000010;
  PSP_CTRL_RIGHT = $000020;
  PSP_CTRL_DOWN = $000040;
  PSP_CTRL_LEFT = $000080;
  PSP_CTRL_LTRIGGER = $000100;
  PSP_CTRL_RTRIGGER = $000200;
  PSP_CTRL_TRIANGLE = $001000;
  PSP_CTRL_CIRCLE = $002000;
  PSP_CTRL_CROSS = $004000;
  PSP_CTRL_SQUARE = $008000;
  PSP_CTRL_HOME = $010000;

  PSP_ANALOG_UP = $100000;
  PSP_ANALOG_RIGHT = $200000;
  PSP_ANALOG_DOWN = $400000;
  PSP_ANALOG_LEFT = $800000;

  KEYEVENTF_EXTENDEDKEY = $0001;
	KEYEVENTF_KEYUP = $0002;
	KEYEVENTF_UNICODE = $0004;
	KEYEVENTF_SCANCODE = $0008;

  MOUSE_LEFT = $0001;
  MOUSE_RIGHT = $0002;
  MOUSE_MIDDLE = $0003;
  MOUSE_DOUBLECLICK = $0004;
  MOUSE_WHEEL_UP = $0010;
  MOUSE_WHEEL_DOWN = $0011;
  MOUSE_MOVE_UP = $0020;
  MOUSE_MOVE_LEFT = $0021;
  MOUSE_MOVE_DOWN = $0022;
  MOUSE_MOVE_RIGHT = $0023;
  MOUSE_MOVE_ANALOG = $0030;

  PSPDISP_OPENMENU = $0001;

  EVENT_MOUSE = $0001;
  EVENT_KEYBOARD = $0002;
  EVENT_PSPDISP = $0010;

  SIDESHOW_BUTTON_PREVIOUS = 0;
  SIDESHOW_BUTTON_NEXT = 1;
  SIDESHOW_BUTTON_UP = 2;
  SIDESHOW_BUTTON_DOWN = 3;
  SIDESHOW_BUTTON_SELECT = 4;
  SIDESHOW_BUTTON_MENU = 5;
  SIDESHOW_BUTTON_BACK = 6;

  SIDESHOW_FORCE_REFRESH = 100;



procedure CtrlProcessInput(var ControlBuffer: TControlBuffer);
procedure CtrlProcessSideshowInput(var ControlBuffer: TControlBuffer; var ButtonCode: Integer);
procedure CtrlSimulateKeyDown(Scancode: Array of Integer);
procedure CtrlSimulateKeyUp(Scancode: Array of Integer);
procedure CtrlMapAnalogToDpad(var ControlBuffer: TControlBuffer);
procedure CtrlLoadConfiguration(Index : Integer);
function CtrlLoadButtonAction(IniFile: TIniFile; Button: String; PSPButton : Integer; var Action: TButtonAction) : Boolean;
procedure CtrlMapAnalog(var ControlBuffer: TControlBuffer);
function CtrlEnumControlFiles() : TControlFileArray;
procedure CtrlSimulateMouseButtonDown(var Action: TButtonAction);
procedure CtrlSimulateMouseButtonUp(var Action: TButtonAction);
procedure CtrlProcessPSPdispEvent(var Action: TButtonAction);
procedure CtrlSimulateKeypress(Code: Byte);


implementation

uses
  Main;

type
  ArrayOfString = Array of String;

var
  LastControlBuffer: TControlBuffer;
  Actions: Array of TButtonAction;

  PSPButtons : Array[0..16] of TPSPButton =
  (
    (Code: PSP_CTRL_SELECT;   Name: 'SELECT'),
    (Code: PSP_CTRL_START;    Name: 'START '),
    (Code: PSP_CTRL_UP;       Name: 'DPAD_UP'),
    (Code: PSP_CTRL_RIGHT;    Name: 'DPAD_RIGHT'),
    (Code: PSP_CTRL_DOWN;     Name: 'DPAD_DOWN'),
    (Code: PSP_CTRL_LEFT;     Name: 'DPAD_LEFT'),
    (Code: PSP_CTRL_LTRIGGER; Name: 'LEFT_TRIGGER'),
    (Code: PSP_CTRL_RTRIGGER; Name: 'RIGHT_TRIGGER'),
    (Code: PSP_CTRL_TRIANGLE; Name: 'TRIANGLE'),
    (Code: PSP_CTRL_CIRCLE;   Name: 'CIRCLE'),
    (Code: PSP_CTRL_CROSS;    Name: 'CROSS'),
    (Code: PSP_CTRL_SQUARE;   Name: 'SQUARE'),
    (Code: PSP_CTRL_HOME;     Name: 'HOME'),
    (Code: PSP_ANALOG_UP;     Name: 'ANALOG_UP'),
    (Code: PSP_ANALOG_RIGHT;  Name: 'ANALOG_RIGHT'),
    (Code: PSP_ANALOG_DOWN;   Name: 'ANALOG_DOWN'),
    (Code: PSP_ANALOG_LEFT;   Name: 'ANALOG_LEFT')
  );

  Keys: Array[0..83] of TKey =
  (
    (Scancode: $001E; Name: 'A ()'),
    (Scancode: $0030; Name: 'B ()'),
    (Scancode: $002E; Name: 'C ()'),
    (Scancode: $0020; Name: 'D ()'),
    (Scancode: $0012; Name: 'E ()'),
    (Scancode: $0021; Name: 'F ()'),
    (Scancode: $0022; Name: 'G ()'),
    (Scancode: $0023; Name: 'H ()'),
    (Scancode: $0017; Name: 'I ()'),
    (Scancode: $0024; Name: 'J ()'),
    (Scancode: $0025; Name: 'K ()'),
    (Scancode: $0026; Name: 'L ()'),
    (Scancode: $0032; Name: 'M ()'),
    (Scancode: $0031; Name: 'N ()'),
    (Scancode: $0018; Name: 'O ()'),
    (Scancode: $0019; Name: 'P ()'),
    (Scancode: $0010; Name: 'Q ()'),
    (Scancode: $0013; Name: 'R ()'),
    (Scancode: $001F; Name: 'S ()'),
    (Scancode: $0014; Name: 'T ()'),
    (Scancode: $0016; Name: 'U ()'),
    (Scancode: $002F; Name: 'V ()'),
    (Scancode: $0011; Name: 'W ()'),
    (Scancode: $002D; Name: 'X ()'),
    (Scancode: $0015; Name: 'Y ()'),
    (Scancode: $002C; Name: 'Z ()'),
    (Scancode: $000B; Name: '0 ()'),
    (Scancode: $0002; Name: '1 ()'),
    (Scancode: $0003; Name: '2 ()'),
    (Scancode: $0004; Name: '3 ()'),
    (Scancode: $0005; Name: '4 ()'),
    (Scancode: $0006; Name: '5 ()'),
    (Scancode: $0007; Name: '6 ()'),
    (Scancode: $0008; Name: '7 ()'),
    (Scancode: $0009; Name: '8 ()'),
    (Scancode: $000A; Name: '9 ()'),
    (Scancode: $0029; Name: '` ()'),
    (Scancode: $000C; Name: '- ()'),
    (Scancode: $002B; Name: '\ ()'),
    (Scancode: $000E; Name: 'Backspace ()'),
    (Scancode: $0039; Name: 'Space ()'),
    (Scancode: $000F; Name: 'Tab ()'),
    (Scancode: $003A; Name: 'Capslock ()'),
    (Scancode: $002A; Name: 'Left Shift ()'),
    (Scancode: $001D; Name: 'Left Ctrl ()'),
    (Scancode: $E05B; Name: 'Left Windows Key ()'),
    (Scancode: $0038; Name: 'Left Alt ()'),
    (Scancode: $0036; Name: 'Right Shift ()'),
    (Scancode: $E01D; Name: 'Right Ctrl ()'),
    (Scancode: $E05C; Name: 'Right Windows Key ()'),
    (Scancode: $E038; Name: 'Right Alt ()'),
    (Scancode: $E05D; Name: 'APPS ()'),
    (Scancode: $001C; Name: 'Enter ()'),
    (Scancode: $0001; Name: 'Esc (110)'),
    (Scancode: $003B; Name: 'F1 ()'),
    (Scancode: $003C; Name: 'F2 ()'),
    (Scancode: $003D; Name: 'F3 ()'),
    (Scancode: $003E; Name: 'F4 ()'),
    (Scancode: $003F; Name: 'F5 ()'),
    (Scancode: $0040; Name: 'F6 ()'),
    (Scancode: $0041; Name: 'F7 ()'),
    (Scancode: $0042; Name: 'F8 ()'),
    (Scancode: $0043; Name: 'F9 ()'),
    (Scancode: $0044; Name: 'F10 ()'),
    (Scancode: $0057; Name: 'F11 ()'),
    (Scancode: $0058; Name: 'F12 ()'),
    (Scancode: $E019; Name: 'Next Track'),
    (Scancode: $E010; Name: 'Previous Track'),
    (Scancode: $E024; Name: 'Stop'),
    (Scancode: $E022; Name: 'Play/Pause'),
    (Scancode: $E020; Name: 'Mute'),
    (Scancode: $E030; Name: 'Volume Up'),
    (Scancode: $E02E; Name: 'Volume Down'),
    (Scancode: $E06D; Name: 'Media Select'),
    (Scancode: $E06C; Name: 'E-Mail'),
    (Scancode: $E021; Name: 'Calculator'),
    (Scancode: $E06B; Name: 'My Computer'),
    (Scancode: $E065; Name: 'WWW Search'),
    (Scancode: $E032; Name: 'WWW Home'),
    (Scancode: $E06A; Name: 'WWW Back'),
    (Scancode: $E069; Name: 'WWW Forward'),
    (Scancode: $E068; Name: 'WWW Stop'),
    (Scancode: $E067; Name: 'WWW Refresh'),
    (Scancode: $E066; Name: 'WWW Favorites')
  );

  MapDPadToAnalog : Boolean;
  AnalogDeadZone : Integer;

  ControlFiles : TControlFileArray;

  AnalogMovement : TPoint;
  AnalogMapping : Array[0..5] of Integer;

  DisableAnalogForThisFrame : Boolean;




{
  Split
  ---------------------------------------------------
  Implementation of an "explode" function from here:
  http://www.delphipraxis.net/topic27196_splitfunktion.html
  ---------------------------------------------------
  Returns an array containing the separated strings.
}
function Split(const fText: String; const fSep: Char; fTrim: Boolean=false; fQuotes: Boolean=false): ArrayOfString;
var vI: Integer; 
    vBuffer: String; 
    vOn: Boolean; 
begin
  SetLength(Result, 0);

  vBuffer:='';
  vOn:=true; 
  for vI:=1 to Length(fText) do 
  begin 
    if (fQuotes and(fText[vI]=fSep)and vOn)or(Not(fQuotes) and (fText[vI]=fSep)) then 
    begin 
      if fTrim then vBuffer:=Trim(vBuffer); 
      if vBuffer='' then vBuffer:=fSep; // !!! sonst läuft z.B. split(',**',',') auf einen Hammer... 
      if vBuffer[1]=fSep then 
        vBuffer:=Copy(vBuffer,2,Length(vBuffer)); 
      SetLength(Result, Length(Result) + 1);
      Result[Length(Result) - 1] := vBuffer;
      vBuffer:='';
    end; 
    if fQuotes then 
    begin 
      if fText[vI]='"' then 
      begin 
        vOn:=Not(vOn); 
        Continue; 
      end; 
      if (fText[vI]<>fSep)or((fText[vI]=fSep)and(vOn=false)) then 
        vBuffer:=vBuffer+fText[vI]; 
    end else 
      if fText[vI]<>fSep then 
        vBuffer:=vBuffer+fText[vI];
  end; 
  if vBuffer<>'' then 
  begin 
    if fTrim then vBuffer:=Trim(vBuffer); 
    SetLength(Result, Length(Result) + 1);
    Result[Length(Result) - 1] := vBuffer;
  end;
end;




{
  CtrlLoadButtonAction
  ---------------------------------------------------
  Loads the presets for a specific button from
  the ini file. If any error occurs it will
  return False.
  ---------------------------------------------------
  Returns True when the button data is valid.
}
function CtrlLoadButtonAction(IniFile: TIniFile; Button: String; PSPButton : Integer; var Action: TButtonAction): Boolean;
var
  EventType : String;
  ReadString : String;
  Converted : Integer;
  i : Integer;
  Scancodes : ArrayOfString;
begin
  Action.ButtonCode := PSPButton;
  Action.Down := False;

  Result := False;

  Action.Typematic := (IniFile.ReadInteger(Button, 'Typematic', 0) > 0);

  EventType := IniFile.ReadString(Button, 'Event', 'None');
  if (EventType = 'Keyboard') then
  begin
    Action.EventType := EVENT_KEYBOARD;

    Action.NoBreak := (IniFile.ReadInteger(Button, 'NoBreak', 0) = 1);

    SetLength(Action.ScanCode, 0);

    ReadString := IniFile.ReadString(Button, 'Scancode', '-1');
    Scancodes := Split(ReadString, '+', True, True);

    for i := 0 to Length(Scancodes) - 1 do
    begin
      Converted := StrToIntDef('$' + Scancodes[i], -1);
      if (Converted = -1) then
      begin
        Result := (i <> 0);
        Exit;                                                     
      end
      else
      begin
        SetLength(Action.ScanCode, Length(Action.ScanCode) + 1);
        Action.ScanCode[Length(Action.ScanCode) - 1] := Converted;
      end;
    end;

    Result := True;
  end
  else if (EventType = 'Mouse') then
  begin
    Action.EventType := EVENT_MOUSE;

    ReadString := IniFile.ReadString(Button, 'Button', '');
    if (ReadString = 'Left') then
      Action.MouseButton := MOUSE_LEFT
    else if (ReadString = 'Right') then
      Action.MouseButton := MOUSE_RIGHT
    else if (ReadString = 'Middle') then
      Action.MouseButton := MOUSE_MIDDLE
    else if (ReadString = 'DoubleClick') then
      Action.MouseButton := MOUSE_DOUBLECLICK
    else if (ReadString = 'WheelUp') then
      Action.MouseButton := MOUSE_WHEEL_UP
    else if (ReadString = 'WheelDown') then
      Action.MouseButton := MOUSE_WHEEL_DOWN
    else if (ReadString = 'MouseMoveAnalog') then
      Action.MouseButton := MOUSE_MOVE_ANALOG
    else if (ReadString = 'MouseMoveUp') then
    begin
      Action.MouseButton := MOUSE_MOVE_UP;
      Action.MoveByPixels := IniFile.ReadInteger(Button, 'MoveByPixels', 1);
    end
    else if (ReadString = 'MouseMoveDown') then
    begin
      Action.MouseButton := MOUSE_MOVE_DOWN;
      Action.MoveByPixels := IniFile.ReadInteger(Button, 'MoveByPixels', 1);
    end
    else if (ReadString = 'MouseMoveLeft') then
    begin
      Action.MouseButton := MOUSE_MOVE_LEFT;
      Action.MoveByPixels := IniFile.ReadInteger(Button, 'MoveByPixels', 1);
    end
    else if (ReadString = 'MouseMoveRight') then
    begin
      Action.MouseButton := MOUSE_MOVE_RIGHT;
      Action.MoveByPixels := IniFile.ReadInteger(Button, 'MoveByPixels', 1);
    end
    else
    begin
      Result := False;
      Exit;
    end;

    Result := True;
  end
  else if (EventType = 'PSPdisp') then
  begin
    Action.EventType := EVENT_PSPDISP;
    ReadString := IniFile.ReadString(Button, 'Action', '');

    if (ReadString = 'OpenMenu') then
    begin
      Action.PSPdispAction := PSPDISP_OPENMENU;
    end
    else
    begin
      Result := False;
      Exit;
    end;

    Result := True;
  end;
end;




{
  CtrlEnumControlFiles
  ---------------------------------------------------
  Searches for all available control files
  ---------------------------------------------------
  Returns an array of control files.
}
function CtrlEnumControlFiles() : TControlFileArray;
var
  SearchRec : TSearchRec;
  ControlFile : TIniFile;
  NewControlFile : TControlFile;
  Path : String;
begin
  SetLength(ControlFiles, 0);

  //Path := ExtractFilePath(Application.ExeName) + '\';
  Path := MainForm.GetAppDataFolder + '\PSPdisp\control\';

  if (FindFirst(Path + '*.control', faAnyFile, SearchRec) = 0) then
  begin
    repeat
      ControlFile := TIniFile.Create(Path + SearchRec.Name);
      NewControlFile.Name := ControlFile.ReadString('General', 'Name', '');
      if NewControlFile.Name <> '' then
      begin
        NewControlFile.FileName := Path + SearchRec.Name;
        SetLength(ControlFiles, Length(ControlFiles) + 1);
        ControlFiles[Length(ControlFiles) - 1] := NewControlFile;
      end;
      ControlFile.Free;
    until
      FindNext(SearchRec) <> 0;
  end;

  Result := ControlFiles;
end;




{
  CtrlLoadConfiguration
  ---------------------------------------------------
  Sets the actions defined in the file speciefied
  by the index number.
  ---------------------------------------------------
}
procedure CtrlLoadConfiguration(Index : Integer);
var
  ControlFile : TIniFile;
  NewAction : TButtonAction;
  i : Integer;
begin
  ControlFile := TIniFile.Create(ControlFiles[Index].Filename);

  if not ControlFile.ValueExists('General', 'Name') then
    Exit;

  MapDPadToAnalog := (1 = ControlFile.ReadInteger('General', 'MapAnalogToDPad', 0));
  AnalogDeadZone := ControlFile.ReadInteger('General', 'AnalogDeadZone', 70);
  if (AnalogDeadZone < 0) or (AnalogDeadZone > 128) then
    AnalogDeadZone := 70;

  EnableKeyboard := (ControlFile.ReadInteger('General', 'EnableOSK', 0) = 1);

  // Load analog stick mapping
  AnalogMapping[0] := ControlFile.ReadInteger('AnalogStickMapping', 'Section1', 1);
  AnalogMapping[1] := ControlFile.ReadInteger('AnalogStickMapping', 'Section2', 2);
  AnalogMapping[2] := ControlFile.ReadInteger('AnalogStickMapping', 'Section3', 3);
  AnalogMapping[3] := ControlFile.ReadInteger('AnalogStickMapping', 'Section4', 4);
  AnalogMapping[4] := ControlFile.ReadInteger('AnalogStickMapping', 'Section5', 5);
  AnalogMapping[5] := ControlFile.ReadInteger('AnalogStickMapping', 'Maximum', 6);

  SetLength(Actions, 0);

  for i := 0 to 16 do
  begin
    if CtrlLoadButtonAction(ControlFile, PSPButtons[i].Name, PSPButtons[i].Code, NewAction) then
    begin
      SetLength(Actions, Length(Actions) + 1);
      Actions[Length(Actions) - 1] := NewAction;
    end;  
  end;

  ControlFile.Free;
end;




{
  CtrlProcessInput
  ---------------------------------------------------
  Examines the raw control data from the PSP.
  ---------------------------------------------------
}
procedure CtrlProcessInput(var ControlBuffer: TControlBuffer);
var
  i: Integer;
begin
  DisableAnalogForThisFrame := False;

  if (ControlBuffer.Buttons and $FF000000) <> 0 then
  begin
    if ((ControlBuffer.Buttons shr 24) > 0) and ((ControlBuffer.Buttons shr 24) < 255) then
      CtrlSimulateKeypress(ControlBuffer.Buttons shr 24);
  end
  else
  begin
    if MapDPadToAnalog then
      CtrlMapAnalogToDpad(ControlBuffer)
    else
      CtrlMapAnalog(ControlBuffer);

    if (ControlBuffer.Buttons xor LastControlBuffer.Buttons) <> 0 then
    begin
      for i := 0 to Length(Actions) - 1 do
      begin
        if (ControlBuffer.Buttons and Actions[i].ButtonCode > 0) and (LastControlBuffer.Buttons and Actions[i].ButtonCode = 0) then
        begin
          if Actions[i].EventType = EVENT_KEYBOARD then
            CtrlSimulateKeyDown(Actions[i].ScanCode)
          else if Actions[i].EventType = EVENT_MOUSE then
            CtrlSimulateMouseButtonDown(Actions[i])
          else if Actions[i].EventType = EVENT_PSPDISP then
            CtrlProcessPSPdispEvent(Actions[i]);

          Actions[i].Down := True;
        end
        else if (ControlBuffer.Buttons and Actions[i].ButtonCode = 0) and (LastControlBuffer.Buttons and Actions[i].ButtonCode > 0) then
        begin
          if Actions[i].EventType = EVENT_KEYBOARD then
            CtrlSimulateKeyUp(Actions[i].ScanCode)
          else
            CtrlSimulateMouseButtonUp(Actions[i]);

          Actions[i].Down := False;
        end;
      end;
    end
    else
    begin
      // typematic buttons
      for i := 0 to Length(Actions) - 1 do
      begin
        if (Actions[i].Typematic and Actions[i].Down) then
        begin
          if Actions[i].EventType = EVENT_KEYBOARD then
            CtrlSimulateKeyDown(Actions[i].ScanCode)
          else if Actions[i].EventType = EVENT_MOUSE then
            CtrlSimulateMouseButtonDown(Actions[i]);
        end;
      end;
    end;


   LastControlBuffer := ControlBuffer;
 end;
end;




{
  CtrlProcessSideshowInput
  ---------------------------------------------------
  Update the button output buffer with new button
  states for SideShow.
  ---------------------------------------------------
}
procedure CtrlProcessSideshowInput(var ControlBuffer: TControlBuffer; var ButtonCode: Integer);
begin
  ButtonCode := -1;

  if (ControlBuffer.Buttons xor LastControlBuffer.Buttons) <> 0 then
  begin
    if (ControlBuffer.Buttons and PSP_CTRL_START = 0) and (LastControlBuffer.Buttons and PSP_CTRL_START > 0) then
      ButtonCode := SIDESHOW_BUTTON_MENU
    else if (ControlBuffer.Buttons and PSP_CTRL_UP = 0) and (LastControlBuffer.Buttons and PSP_CTRL_UP > 0) then
      ButtonCode := SIDESHOW_BUTTON_UP
    else if (ControlBuffer.Buttons and PSP_CTRL_DOWN = 0) and (LastControlBuffer.Buttons and PSP_CTRL_DOWN > 0) then
      ButtonCode := SIDESHOW_BUTTON_DOWN
    else if (ControlBuffer.Buttons and PSP_CTRL_LEFT = 0) and (LastControlBuffer.Buttons and PSP_CTRL_LEFT > 0) then
      ButtonCode := SIDESHOW_BUTTON_PREVIOUS
    else if (ControlBuffer.Buttons and PSP_CTRL_RIGHT = 0) and (LastControlBuffer.Buttons and PSP_CTRL_RIGHT > 0) then
      ButtonCode := SIDESHOW_BUTTON_NEXT
    else if (ControlBuffer.Buttons and PSP_CTRL_CROSS = 0) and (LastControlBuffer.Buttons and PSP_CTRL_CROSS > 0) then
      ButtonCode := SIDESHOW_BUTTON_SELECT
    else if (ControlBuffer.Buttons and PSP_CTRL_CIRCLE = 0) and (LastControlBuffer.Buttons and PSP_CTRL_CIRCLE > 0) then
      ButtonCode := SIDESHOW_BUTTON_BACK
  end;

  LastControlBuffer := ControlBuffer;
end;





{
  CtrlMapAnalogToDpad
  ---------------------------------------------------
  Assign analog movement to the DPad.
  ---------------------------------------------------
}
procedure CtrlMapAnalogToDpad(var ControlBuffer: TControlBuffer);
begin
  if (ControlBuffer.AnalogX > 128 + AnalogDeadZone) then
    ControlBuffer.Buttons := ControlBuffer.Buttons or PSP_CTRL_RIGHT
  else if (ControlBuffer.AnalogX < 128 - AnalogDeadZone) then
    ControlBuffer.Buttons := ControlBuffer.Buttons or PSP_CTRL_LEFT;

  if (ControlBuffer.AnalogY > 128 + AnalogDeadZone) then
    ControlBuffer.Buttons := ControlBuffer.Buttons or PSP_CTRL_DOWN
  else if (ControlBuffer.AnalogY < 128 - AnalogDeadZone) then
    ControlBuffer.Buttons := ControlBuffer.Buttons or PSP_CTRL_UP;
end;





{
  CtrlMapAnalogInputToMouseMovement
  ---------------------------------------------------
  Translate the analog stick position to mouse pixel
  movement according to the defined mapping.
  ---------------------------------------------------
  Returns the value in pixel, the mouse should be moved.
}
function CtrlMapAnalogInputToMouseMovement(AnalogValue: Integer): Integer;
var
  Sign: Integer;
  Range: Integer;
begin
  if AnalogValue < 0 then
    Sign := -1
  else
    Sign := 1;

  AnalogValue := AnalogValue * Sign;

  Range := 128 - AnalogDeadZone;

  if AnalogValue < (Range * 1 / 5) then
    Result := AnalogMapping[0]
  else if AnalogValue < (Range * 2 / 5) then
    Result := AnalogMapping[1]
  else if AnalogValue < (Range * 3 / 5) then
    Result := AnalogMapping[2]
  else if AnalogValue < (Range * 4 / 5) then
    Result := AnalogMapping[3]
  else if AnalogValue < Range then
    Result := AnalogMapping[4]
  else
    Result := AnalogMapping[5];

  Result := Sign * Result;
end;




{
  CtrlMapAnalog
  ---------------------------------------------------
  Turn analog stick movement into button presses.
  ---------------------------------------------------
}
procedure CtrlMapAnalog(var ControlBuffer: TControlBuffer);
begin
  AnalogMovement.X := 0;
  AnalogMovement.Y := 0;

  if (ControlBuffer.AnalogX > 128 + AnalogDeadZone) then begin
    ControlBuffer.Buttons := ControlBuffer.Buttons or PSP_ANALOG_RIGHT;
    AnalogMovement.X := CtrlMapAnalogInputToMouseMovement(ControlBuffer.AnalogX - 127 - AnalogDeadZone);
  end
  else if (ControlBuffer.AnalogX < 128 - AnalogDeadZone) then
  begin
    ControlBuffer.Buttons := ControlBuffer.Buttons or PSP_ANALOG_LEFT;
    AnalogMovement.X := CtrlMapAnalogInputToMouseMovement(ControlBuffer.AnalogX - 128 + AnalogDeadZone);
  end;

  if (ControlBuffer.AnalogY > 128 + AnalogDeadZone) then
  begin
    ControlBuffer.Buttons := ControlBuffer.Buttons or PSP_ANALOG_DOWN;
    AnalogMovement.Y := CtrlMapAnalogInputToMouseMovement(ControlBuffer.AnalogY - 127 - AnalogDeadZone);
  end
  else if (ControlBuffer.AnalogY < 128 - AnalogDeadZone) then
  begin
    ControlBuffer.Buttons := ControlBuffer.Buttons or PSP_ANALOG_UP;
    AnalogMovement.Y := CtrlMapAnalogInputToMouseMovement(ControlBuffer.AnalogY - 128 + AnalogDeadZone);
  end;
end;







{
  CtrlSimulateKeyDown
  ---------------------------------------------------
  Generate a keydown event.
  ---------------------------------------------------
}
procedure CtrlSimulateKeyDown(Scancode: Array of Integer);
var
  Input: Array of TInput;
  i : Integer;
begin
  SetLength(Input, 0);

  for i := 0 to Length(Scancode) - 1 do
  begin
    SetLength(Input, Length(Input) + 1);
    Input[i].Itype := INPUT_KEYBOARD;
    with Input[i].ki do
    begin
      time := 0;
      wVk := 0;
      wScan := Scancode[i];
      if Scancode[i] > 255 then
        dwFlags := KEYEVENTF_SCANCODE or KEYEVENTF_EXTENDEDKEY
      else
        dwFlags := KEYEVENTF_SCANCODE;
      time := 0;
    end;
  end;
  SendInput(Length(Scancode), Input[0], sizeof(TInput));
end;





{
  CtrlSimulateKeyUp
  ---------------------------------------------------
  Generate a keyup event.
  ---------------------------------------------------
}
procedure CtrlSimulateKeyUp(Scancode: Array of Integer);
var
  Input: Array of TInput;
  i : Integer;
begin
  SetLength(Input, 0);

  for i := 0 to Length(Scancode) - 1 do
  begin
    SetLength(Input, Length(Input) + 1);
    Input[i].Itype := INPUT_KEYBOARD;
    with Input[i].ki do
    begin
      time := 0;
      wVk := 0;
      wScan := Scancode[i];
       if Scancode[i] > 255 then
        dwFlags := KEYEVENTF_KEYUP or KEYEVENTF_SCANCODE or KEYEVENTF_EXTENDEDKEY
      else
        dwFlags := KEYEVENTF_KEYUP or KEYEVENTF_SCANCODE;
      time := 0;
    end;
  end;
  SendInput(Length(Scancode), Input[0], sizeof(TInput));
end;






{
  CtrlSimulateMouseButtonDown
  ---------------------------------------------------
  Generate a mouse event.
  ---------------------------------------------------
}
procedure CtrlSimulateMouseButtonDown(var Action: TButtonAction);
var
  Input: TInput;
begin
  Input.Itype := INPUT_MOUSE;
  with Input.mi do
  begin
    time := 0;
    dx := 0;
    dy := 0;
    mouseData := 0;
    dwExtraInfo := 0;
    case Action.MouseButton of
      MOUSE_LEFT: dwFlags := MOUSEEVENTF_LEFTDOWN;
      MOUSE_RIGHT: dwFlags := MOUSEEVENTF_RIGHTDOWN;
      MOUSE_MIDDLE: dwFlags := MOUSEEVENTF_MIDDLEDOWN;
      MOUSE_DOUBLECLICK: begin
        dwFlags := MOUSEEVENTF_LEFTDOWN;
        SendInput(1, Input, sizeof(TInput));
        dwFlags := MOUSEEVENTF_LEFTUP;
        SendInput(1, Input, sizeof(TInput));
        dwFlags := MOUSEEVENTF_LEFTDOWN;
        SendInput(1, Input, sizeof(TInput));
        dwFlags := MOUSEEVENTF_LEFTUP;
        SendInput(1, Input, sizeof(TInput));
        Exit;
      end;
      MOUSE_WHEEL_UP: begin
        dwFlags := MOUSEEVENTF_WHEEL;
        mouseData := WHEEL_DELTA;
      end;
      MOUSE_WHEEL_DOWN: begin
        dwFlags := MOUSEEVENTF_WHEEL;
        mouseData := Cardinal(-1 * WHEEL_DELTA);
      end;
      MOUSE_MOVE_UP: begin
          dwFlags := MOUSEEVENTF_MOVE;
          dx := 0;
          dy := -1 * Action.MoveByPixels;
        end;
      MOUSE_MOVE_DOWN: begin
          dwFlags := MOUSEEVENTF_MOVE;
          dx := 0;
          dy := Action.MoveByPixels;
        end;
      MOUSE_MOVE_LEFT: begin
          dwFlags := MOUSEEVENTF_MOVE;
          dx := -1 * Action.MoveByPixels;
          dy := 0;
        end;
      MOUSE_MOVE_RIGHT: begin
          dwFlags := MOUSEEVENTF_MOVE;
          dx := Action.MoveByPixels;
          dy := 0;
        end;
      MOUSE_MOVE_ANALOG: begin
          if not DisableAnalogForThisFrame then
          begin
            dwFlags := MOUSEEVENTF_MOVE;
            dx := AnalogMovement.X;
            dy := AnalogMovement.Y;
            SendInput(1, Input, sizeof(TInput));
            DisableAnalogForThisFrame := True;
          end;
          Exit;
        end;
    end;
  end;
  SendInput(1, Input, sizeof(TInput));
end;





{
  CtrlSimulateMouseButtonUp
  ---------------------------------------------------
  Generate a mouse button up event.
  ---------------------------------------------------
}
procedure CtrlSimulateMouseButtonUp(var Action: TButtonAction);
var
  Input: TInput;
begin
  if (Action.MouseButton = MOUSE_MOVE_UP) or (Action.MouseButton = MOUSE_MOVE_DOWN)
  or (Action.MouseButton = MOUSE_MOVE_LEFT) or (Action.MouseButton = MOUSE_MOVE_RIGHT)
  or (Action.MouseButton = MOUSE_MOVE_ANALOG)
  or (Action.MouseButton = MOUSE_WHEEL_UP) or (Action.MouseButton = MOUSE_WHEEL_DOWN)
  or (Action.MouseButton = MOUSE_DOUBLECLICK)
  or (Action.EventType = EVENT_PSPDISP) then
    Exit;

  Input.Itype := INPUT_MOUSE;
  with Input.mi do
  begin
    time := 0;
    dx := 0;
    dy := 0;
    mouseData := 0;
    dwExtraInfo := 0;
    case Action.MouseButton of
      MOUSE_LEFT: dwFlags := MOUSEEVENTF_LEFTUP;
      MOUSE_RIGHT: dwFlags := MOUSEEVENTF_RIGHTUP;
      MOUSE_MIDDLE: dwFlags := MOUSEEVENTF_MIDDLEUP;
    end;
  end;
  SendInput(1, Input, sizeof(TInput));
end;





{
  CtrlProcessPSPdispEvent
  ---------------------------------------------------
  Handle PSPdisp special events.
  ---------------------------------------------------
}
procedure CtrlProcessPSPdispEvent(var Action: TButtonAction);
begin
  if Action.PSPdispAction = PSPDISP_OPENMENU then
  begin
    // This is kind of a hack because it sends a right click to
    // the tray icon window.
    PostMessage(trayicon.GetHandle, 1025, 57051, 517);
  end;
end;




{
  CtrlSimulateKeypress
  ---------------------------------------------------
  Send a keyboard event for input through the onscreen
  keyboard. This is done by sending the key as an ANSI
  value (NOT a keycode or scancode). Because of this
  it will work in standard GUI applications, but most
  probably not in many games.
  ---------------------------------------------------
}
procedure CtrlSimulateKeypress(Code: Byte);
var
  Input: TInput;
  VK: Word;
begin
  if (Code = 4) then
    Exit;

  VK := 0;

  if ((Code = 1) or (Code = 2) or (Code = 8) or (Code = 10)) then
  begin
    // Special cases for Backspace, Return, Arrow Left, Arrow Right
    if (Code = 8) then
      VK := VK_BACK
    else if (Code = 10) then
      VK := VK_RETURN
    else if (Code = 1) then
      VK := VK_LEFT
    else if (Code = 2) then
      VK := VK_RIGHT;

    Input.Itype := INPUT_KEYBOARD;
    with Input.ki do
    begin
      time := 0;
      wVk := VK;
      wScan := 0;
      dwFlags := 0;
      time := 0;
    end;
    SendInput(1, Input, sizeof(TInput));

    Input.Itype := INPUT_KEYBOARD;
    with Input.ki do
    begin
      time := 0;
      wVk := VK;
      wScan := 0;
      dwFlags := KEYEVENTF_KEYUP;
      time := 0;
    end;
    SendInput(1, Input, sizeof(TInput));
  end
  else
  begin
    Input.Itype := INPUT_KEYBOARD;
    with Input.ki do
    begin
      time := 0;
      wVk := 0;
      wScan := Code;
      dwFlags := KEYEVENTF_UNICODE;
      time := 0;
    end;
    SendInput(1, Input, sizeof(TInput));

    Input.Itype := INPUT_KEYBOARD;
    with Input.ki do
    begin
      time := 0;
      wVk := 0;
      wScan := Code;
      dwFlags := KEYEVENTF_KEYUP or KEYEVENTF_UNICODE;
      time := 0;
    end;
    SendInput(1, Input, sizeof(TInput));
  end;
end;


end.
















