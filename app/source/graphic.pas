{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  graphic.pas - screen capturing and drawing

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}


unit graphic;

interface

uses
  main, Windows, Controls;

const
  BitmapColorBytes = 3;
  ROTATION_OFF: Cardinal = $00000000;
  ROTATION_90: Cardinal = $01000000;
  ROTATION_180: Cardinal = $02000000;
  ROTATION_270: Cardinal = $03000000;

var
  JpegBuffer: Array[0..500 * 1024] of Byte;
  JpegQuality: Word;
  ChangeRotationOnNextFrame: Boolean;
  TargetWidth: Integer;
  TargetHeight: Integer;
  Rotation: LongWord;
  TempViewportWidth: Integer;
  TempViewportHeight: Integer;
  ForceTransmission: Boolean;

procedure GraphicInitBitmaps;
procedure GraphicDrawCursor(x: Integer; y: Integer);
procedure GraphicDrawScreen;
function GraphicCompress(Width: LongWord; Height: LongWord): LongWord;
procedure GraphicChangeRotation;
procedure GraphicSetBitmapBuffer(Start: Pointer);
procedure GraphicDrawSideShowNotAvailableMessage;
procedure GetUpdatedMousePosition;

implementation

uses
  compress, display, mainloop;

var
  DesktopDC: HDC;
  CompatibleDC: HDC;
  LandscapeBitmapHandle: HBITMAP;
  PortraitBitmapHandle: HBITMAP;
  CurrentBitmapHandle: HBITMAP;
  BitmapBuffer: Array[0..480 * 272 * BitmapColorBytes] of Byte;

  HideMouseCursor: Boolean;




{
  GraphicInitBitmaps
  ---------------------------------------------------
  Create DCs and bitmaps.
  ---------------------------------------------------
}
procedure GraphicInitBitmaps;
begin
   // These initializations only have to be done once.
  DesktopDC := GetWindowDC(GetDesktopWindow());
  CompatibleDC := CreateCompatibleDC(DesktopDC);

  // Create Bitmaps
  PortraitBitmapHandle := CreateCompatibleBitmap(DesktopDC, 272, 480);
  LandscapeBitmapHandle := CreateCompatibleBitmap(DesktopDC, 480, 272);

  CurrentBitmapHandle := PortraitBitmapHandle;
  SelectObject(compatibleDC, CurrentBitmapHandle);

  Rotation := ROTATION_OFF;
  TempViewportWidth := 480;
  TempViewportHeight := 272;

  // Set mode for downsampling blits to the nicest looking one.
  SetStretchBltMode(CompatibleDC, STRETCH_HALFTONE);
  SetBrushOrgEx(CompatibleDC, 0, 0, nil);
end;





{
  GraphicChangeRotation
  ---------------------------------------------------
  Reinitialise the bitmaps when the orientation
  changed.
  ---------------------------------------------------
}
procedure GraphicChangeRotation;
begin
  ReleaseDC(CurrentBitmapHandle, CompatibleDC);

  if (TempViewportWidth = 480) and (TempViewportHeight = 272) then
  begin
    // Unstretched, enable grabbing of transparent windows
    SetStretchBltMode(CompatibleDC, COLORONCOLOR);
    SetBrushOrgEx(CompatibleDC, 0, 0, nil);
  end
  else
  begin
    // Set mode for downsampling blits to the nicest looking one.
    if MainForm.MenuFastImageStretching.Checked or ((480 div TempViewportWidth) > 0) and ((480 mod TempViewportWidth) = 0) then
      SetStretchBltMode(CompatibleDC, COLORONCOLOR)
    else
      SetStretchBltMode(CompatibleDC, STRETCH_HALFTONE);
    SetBrushOrgEx(CompatibleDC, 0, 0, nil);
  end;

  if MainForm.MenuRotationOff.Checked then
  begin
    CurrentBitmapHandle := LandscapeBitmapHandle;
    TargetWidth := 480;
    TargetHeight := 272;
    ViewportWidth := TempViewportWidth;
    ViewportHeight := TempViewportHeight;
    Rotation := ROTATION_OFF;
  end
  else if MainForm.MenuRotation90.Checked then
  begin
    CurrentBitmapHandle := PortraitBitmapHandle;
    TargetWidth := 272;
    TargetHeight := 480;
    ViewportWidth := TempViewportHeight;
    ViewportHeight := TempViewportWidth;
    Rotation := ROTATION_90;
  end
  else if MainForm.MenuRotation180.Checked then
  begin
    CurrentBitmapHandle := LandscapeBitmapHandle;
    TargetWidth := 480;
    TargetHeight := 272;
    ViewportWidth := TempViewportWidth;
    ViewportHeight := TempViewportHeight;
    Rotation := ROTATION_180;
  end
  else if MainForm.MenuRotation270.Checked then
  begin
    CurrentBitmapHandle := PortraitBitmapHandle;
    TargetWidth := 272;
    TargetHeight := 480;
    ViewportWidth := TempViewportHeight;
    ViewportHeight := TempViewportWidth;
    Rotation := ROTATION_270;
  end;

  SelectObject(compatibleDC, CurrentBitmapHandle);
end;




{
  GraphicGetBitmapBits
  ---------------------------------------------------
  Copy the bitmap from the Windows buffer to a
  local buffer.
  ---------------------------------------------------
}
procedure GraphicGetBitmapBits;
var
  Info : BITMAPINFO;
begin
//  GetBitmapBits(LandscapeBitmapHandle, 480 * 272 * 4, @BitmapBuffer);
  ZeroMemory(@Info, SizeOf(BITMAPINFO));
  Info.bmiHeader.biSize := SizeOf(Info.bmiHeader);
  Info.bmiHeader.biWidth := TargetWidth;
  Info.bmiHeader.biHeight := {-1 *} TargetHeight;
  Info.bmiHeader.biPlanes := 1;
  Info.bmiHeader.biBitCount := BitmapColorBytes * 8;

  if ((ConnectionMode = ModeUsb) and (MainForm.MenuQualityUncompressed.Checked)) then
    GetDIBits(CompatibleDC, CurrentBitmapHandle, 0, TargetHeight, @JpegBuffer, Info, DIB_RGB_COLORS)
  else
    GetDIBits(CompatibleDC, CurrentBitmapHandle, 0, TargetHeight, @BitmapBuffer, Info, DIB_RGB_COLORS);
end;



{
  GraphicDrawCursor
  ---------------------------------------------------
  Draw the mouse cursor onto the bitmap send
  to the psp.
  ---------------------------------------------------
}
procedure GraphicDrawCursor(x: Integer; y: Integer);
var
  CursorInfo: TCursorInfo;
  IconInfo: TIconInfo;
  IconX: Integer;
  IconY: Integer;
  IconScalingX: Integer;
  IconScalingY: Integer;

begin
  CursorInfo.cbSize := SizeOf(TCursorInfo);
  if GetCursorInfo(CursorInfo) then
  begin
    if CursorInfo.Flags = CURSOR_SHOWING then
    begin
      if GetIconInfo(CursorInfo.hCursor, IconInfo) then
      begin
       if MainForm.MenuFitScreen.Checked then
       begin
         IconX := Round((CursorInfo.ptScreenPos.x - MonitorLeft) / (MonitorWidth / TargetWidth)) - Integer(IconInfo.xHotspot);
         IconY := Round((CursorInfo.ptScreenPos.y - MonitorTop) / (MonitorHeight / TargetHeight)) - Integer(IconInfo.yHotspot);
         IconScalingX := Round(32 * TargetHeight / MonitorHeight);
         IconScalingY :=  Round(32 * TargetWidth / MonitorWidth);
       end
       else if MainForm.MenuFitScreen16to9.Checked then
       begin
         IconX := Round((CursorInfo.ptScreenPos.x - MonitorLeft) / (MonitorWidth / TargetWidth)) - Integer(IconInfo.xHotspot);
         IconY := Round((CursorInfo.ptScreenPos.y - MonitorTop - ((MonitorHeight - (MonitorWidth * 9) div 16) div 2)) / (((MonitorWidth * 9) div 16) / TargetHeight)) - Integer(IconInfo.yHotspot);
         IconScalingX := Round(32 * TargetHeight / MonitorHeight);
         IconScalingY :=  Round(32 * TargetWidth / MonitorWidth);
       end
       else begin
         IconX := Round(((CursorInfo.ptScreenPos.x - MonitorLeft) - x - Integer(IconInfo.xHotspot)) * (TargetWidth / ViewPortWidth));
         IconY := Round(((CursorInfo.ptScreenPos.y - MonitorTop) - y - Integer(IconInfo.yHotspot))  * (TargetHeight / ViewPortHeight));
         IconScalingX := Round(32 * TargetHeight / ViewPortHeight);
         IconScalingY :=  Round(32 * TargetWidth / ViewPortWidth);
       end;

       if (MainForm.MenuScaleCursor.Checked) then
         DrawIconEx(CompatibleDC, IconX, IconY, CursorInfo.hCursor, IconScalingX, IconScalingY, 0, 0, DI_NORMAL)
       else
         DrawIcon(CompatibleDC, IconX, IconY, CursorInfo.hCursor);

        DeleteObject(IconInfo.hbmMask);
        DeleteObject(IconInfo.hbmColor);
      end;
    end;
  end;
end;




{
  GetUpdatedMousePosition
  ---------------------------------------------------
  Retrieve the mouse cursor position.
  ---------------------------------------------------
}
procedure GetUpdatedMousePosition;
begin
  try
    tempX := Mouse.CursorPos.x - MonitorLeft;
    tempY := Mouse.CursorPos.y - MonitorTop;
  except
    // An exception occurs here under Vista if the "elevation" dialog
    // pops up or the display style is changed (e.g. Aero -> Vista Basic)
    tempX := lastTempX;
    tempY := lastTempY;
  end;
  lastTempX := tempX;
  lastTempY := tempY;

  // Check if the cursor must be hidden / restored
  if ((lastCursorPosition.x <> tempX) or (lastCursorPosition.y <> tempY)) then
  begin
    TimeSinceLastMouseMove := 0;
    HideMouseCursor := False;
  end
  else if (TimeSinceLastMouseMove > 3000) then
    HideMouseCursor := MainForm.MenuHideCurser.Checked;

  lastCursorPosition.x := tempX;
  lastCursorPosition.y := tempY;
end;



{
  GraphicDrawScreen
  ---------------------------------------------------
  Copy the framebuffer to the Bitmap canvas,
  choose the right viewport and apply stretching
  if necessary.
  ---------------------------------------------------
}
procedure GraphicDrawScreen;
var
  MonitorTop16to9Offset: Integer;
  MonitorHeight16to9: Integer;
  i: Integer;
  NewDeviceNumber: Integer;
  LastCursorPositionAbsolute: TPoint;

begin
  if ChangeRotationOnNextFrame then
  begin
    ChangeRotationOnNextFrame := False;
    GraphicChangeRotation;
  end;

  GetUpdatedMousePosition;

  if MainForm.MenuFollowMouseAcrossScreens.Checked then
  begin
    // Check if the mouse is still on the current screen
    if not ((tempX >= 0) and (tempX < MonitorWidth) and (tempY >= 0) and (tempY < MonitorHeight)) then
    begin
      // Get the screen the mouse is on
      LastCursorPositionAbsolute.x := lastCursorPosition.x + MonitorLeft;
      LastCursorPositionAbsolute.y := lastCursorPosition.y + MonitorTop;

      NewDeviceNumber := DispGetDeviceContainingPoint(LastCursorPositionAbsolute);
      if (NewDeviceNumber > -1) then
      begin
        for i := MenuDisplayDeviceFirstIndex to MainForm.MenuDisplay.Count - 1 do
        begin
          if MainForm.MenuDisplay.Items[i].Tag = NewDeviceNumber then
          begin
            MainForm.MenuDisplay.Items[i].Click;
            GetUpdatedMousePosition;
            ForceTransmission := True;
            Break;
          end;
        end;
      end;
    end;
  end;

  if MainForm.MenuFitScreen.Checked then
  begin
    x := 0;
    y := 0;
    StretchBlt(compatibleDC, 0, 0, TargetWidth, TargetHeight, DesktopDC, MonitorLeft, MonitorTop, MonitorWidth, MonitorHeight, copyMode);
  end
  else if MainForm.MenuFitScreen16to9.Checked then
  begin
    x := 0;
    y := 0;
    MonitorHeight16to9 := (MonitorWidth * 9) div 16;
    MonitorTop16to9Offset := (MonitorHeight - MonitorHeight16to9) div 2;

    StretchBlt(compatibleDC, 0, 0,TargetWidth, TargetHeight, DesktopDC, MonitorLeft, MonitorTop + MonitorTop16to9Offset, MonitorWidth, MonitorHeight16to9, copyMode);
  end
  else begin
    // Check if we are on the virtual screen and if mouse following is enabled.
    if (tempX >= 0) and (tempX < MonitorWidth) and (tempY >= 0) and (tempY < MonitorHeight) then
    begin
      if MainForm.MenuFollowMouse.Checked then
      begin
        x := Round((tempX / MonitorWidth) * (MonitorWidth - ViewPortWidth));
        y := Round((tempY / MonitorHeight) * (MonitorHeight - ViewPortHeight));
      end
      else if MainForm.MenuFollowMouse2.Checked then
      begin
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
      end
      else if MainForm.MenuStaticViewport.Checked then
      begin
        x := StartXSetting;
        y := StartYSetting;
      end;
    end
    else begin
     // not on the selected screen, use static viewport
     x := StartXSetting;
     y := StartYSetting;
    end;

    if (ViewPortWidth <> 480) then
      StretchBlt(compatibleDC, 0, 0, TargetWidth, TargetHeight, DesktopDC, MonitorLeft + x, MonitorTop + y, ViewPortWidth, ViewPortHeight, copyMode)
    else
      BitBlt(compatibleDC, 0, 0, TargetWidth, TargetHeight, DesktopDC, MonitorLeft + x, MonitorTop + y, copyMode);
  end;

  if (not HideMouseCursor) then
    GraphicDrawCursor(x, y);

  GraphicGetBitmapBits;
end;




{
  GraphicSetBitmapBuffer
  ---------------------------------------------------
  Copy the bitmap or jpeg data to the final output
  buffer.
  ---------------------------------------------------
}
procedure GraphicSetBitmapBuffer(Start: Pointer);
begin
  if (not (ConnectionMode = ModeWlan) and (MainForm.MenuQualityUncompressed.Checked)) then
    CopyMemory(@JpegBuffer, Start, 480 * 272 * 3)
  else
    CopyMemory(@BitmapBuffer, Start, 480 * 272 * 3);
end;





{
  GraphicDrawSideShowNotAvailableMessage
  ---------------------------------------------------
  Write a message to the SideShow canvas if the
  driver is not available.
  ---------------------------------------------------
}
procedure GraphicDrawSideShowNotAvailableMessage;
begin
  TargetWidth := 480;
  TargetHeight := 272;

//  SelectObject(compatibleDC, GetStockObject(DC_BRUSH));
  SelectObject(compatibleDC, GetStockObject(BLACK_BRUSH));
  SetTextColor(compatibleDC, RGB(255, 255, 255));
  SetBkColor(compatibleDC, RGB(0, 0, 0));
  Rectangle(compatibleDC, 0, 0, 480, 480);
  TextOut(CompatibleDC, 50, 50, 'SideShow driver not loaded.', 27);

  GraphicGetBitmapBits;

//  ZeroMemory(@BitmapBuffer, 480 * 272 * 3);
end;




{
  GraphicCompress
  ---------------------------------------------------
  Do the compression.
  ---------------------------------------------------
  Returns the size of the output image.
}
function GraphicCompress(Width: LongWord; Height: LongWord): LongWord;
begin
  if ((ConnectionMode = ModeWlan) or (not MainForm.MenuQualityUncompressed.Checked)) then
  begin
    if (MainForm.MenuUseSideshow.Checked) then
      Result := CompressToJPEG(MainForm.MenuQualitySideShow.Tag, BitmapBuffer, JpegBuffer, Width, Height)
    else
      Result := CompressToJPEG(JpegQuality, BitmapBuffer, JpegBuffer, Width, Height);
  end
  else
  begin
//    CopyMemory(@(JpegBuffer[0]), @(BitmapBuffer[0]), 480 * 272 * 3);
    Result := 480 * 272 * 3;
  end;
end;





end.
