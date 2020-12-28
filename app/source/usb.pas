{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  usb.pas - functions dealing with usb transfer

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}

unit usb;

interface

uses
  Windows, SysUtils, Classes, Messages, LibUsb;


procedure UsbInit();
procedure UsbTerm();
function UsbCheckForConnection(DoOpen: Boolean): Boolean;
function UsbCheckForLibusbDevice(DoOpen: Boolean): Boolean;
function UsbCheckForWinUsbDevice(DoOpen: Boolean): Boolean;
procedure UsbCloseConnection();
function UsbInitializeAsync(): Boolean;
function UsbSendBuffer(DataBuffer: Pointer; DataSize: Cardinal): Boolean;
function UsbReceiveData(Buffer: Pointer; BufferSize: LongWord): Boolean;
procedure UsbOnDeviceChange(var Msg: TMessage);
procedure UsbForceDetection();

function WUsb_GetDevicePath(guidString: PChar; devicePath: PChar): Integer; cdecl; external 'wusb.dll' name 'WUsb_GetDevicePath';
function WUsb_Initialize(DeviceHandle: THandle; var InterfaceHandle: THandle): Boolean; cdecl; external 'wusb.dll' name 'WUsb_Initialize';
function WUsb_Free(InterfaceHandle: THandle): Boolean; cdecl; external 'wusb.dll' name 'WUsb_Free';
function WUsb_SetPipePolicy(InterfaceHandle: THandle; PipeID: Byte; PolicyType: Cardinal; ValueLength: Cardinal; Value: Pointer): Boolean; cdecl; external 'wusb.dll' name 'WUsb_SetPipePolicy';
function WUsb_ReadPipe(InterfaceHandle: THandle; PipeID: Byte; Buffer: Pointer; BufferLength: Cardinal; var LengthTransferred: Cardinal; Overlapped: Pointer): Boolean; cdecl; external 'wusb.dll' name 'WUsb_ReadPipe';
function WUsb_WritePipe(InterfaceHandle: THandle; PipeID: Byte; Buffer: Pointer; BufferLength: Cardinal; var LengthTransferred: Cardinal; Overlapped: Pointer): Boolean; cdecl; external 'wusb.dll' name 'WUsb_WritePipe';




implementation

const
  // Sony vendor and product ID
  VendorId = $54C;
  ProductId_Type_B = $1C9;
  ProductId_Type_C = $1CA;

  FrameHeaderSize = 16;
  FrameHeaderMinusMagicSize = 12;

var
  ScrapBuffer: Array[0..4096] of Char;

  LibusbDeviceHandle: pusb_dev_handle;

  WinUsbInterfaceHandle: Cardinal;
  WinUsbDeviceHandle: THandle;

  DeviceEventOccured: Boolean;



{
  UsbInit
  ---------------------------------------------------
  Start the USB functions.
  ---------------------------------------------------
}
procedure UsbInit();
begin
  usb_init();
  UsbForceDetection();
end;




{
  UsbTerm
  ---------------------------------------------------
  Shut down the USB functions.
  ---------------------------------------------------
}
procedure UsbTerm();
begin
end;




{
  UsbForceDetection
  ---------------------------------------------------
  Search for the USB devices even if no device
  was recently plugged in.
  ---------------------------------------------------
}
procedure UsbForceDetection();
begin
  DeviceEventOccured := True;
end;





{
  UsbOnDeviceChange
  ---------------------------------------------------
  Receives Windows device change messages.
  ---------------------------------------------------
}
procedure UsbOnDeviceChange(var Msg: TMessage);
const
  DBT_DEVNODES_CHANGED: Integer = 7;
begin
  if (Msg.WParam = DBT_DEVNODES_CHANGED) then
    DeviceEventOccured := True;
end;





{
  UsbCheckForConnection
  ---------------------------------------------------
  Look for the PSP device.
  ---------------------------------------------------
  Returns True if the PSP was found.
}
function UsbCheckForConnection(DoOpen: Boolean): Boolean;
begin
  Result := (DeviceEventOccured and (UsbCheckForLibusbDevice(DoOpen) or UsbCheckForWinUsbDevice(DoOpen)));
  DeviceEventOccured := False;
//  Sleep(1000);
end;




{
  usbCheckForLibusbDevice
  ---------------------------------------------------
  Look for the PSP libusb device in the device tree.
  ---------------------------------------------------
  Returns True if the PSP was found.
}
function UsbCheckForLibusbDevice(DoOpen: Boolean): Boolean;
var
  Device: pusb_device;
  Bus:    pusb_bus;

begin
  usb_find_busses();
  usb_find_devices();

  Bus := usb_get_busses();

  while Assigned(Bus) do
  begin
    Device := Bus^.devices;
    while Assigned(Device) do
    begin
      if ((Device^.descriptor.idVendor = VendorId) and
          (Device^.descriptor.idProduct = ProductId_Type_B)) then
      begin
        // Device found
        if (DoOpen) then
        begin
          LibusbDeviceHandle := usb_open(Device);
          usb_set_configuration(LibusbDeviceHandle, 1);
          usb_claim_interface(LibusbDeviceHandle, 0);
          usb_set_altinterface(LibusbDeviceHandle, 0);
          UsbInitializeAsync();
        end;

        Result := True;
        Exit;
      end;
      Device := Device^.next;
    end;
    Bus := Bus^.next;
  end;

  LibUsbDeviceHandle := nil;
  Result := False;
end;





{
  usbCheckForWinusbDevice
  ---------------------------------------------------
  Look for the PSP WinUSB device in the device tree.
  ---------------------------------------------------
  Returns True if the PSP was found.
}
function usbCheckForWinusbDevice(DoOpen: Boolean): Boolean;
var
  PathBuffer: Array[0..255] of AnsiChar;
  GuidBuffer: Array[0..255] of Ansichar;
  Timeout: Integer;

begin
  GuidBuffer := '{9ef8fcc2-f024-4fbd-ad39-50fcdbb81a95}';
  PathBuffer := '';

  if (WUsb_GetDevicePath(@GuidBuffer, @PathBuffer) > 0) then
  begin
    if (DoOpen) then
    begin
      WinUsbDeviceHandle := CreateFile(PathBuffer, GENERIC_WRITE or GENERIC_READ, FILE_SHARE_READ or FILE_SHARE_WRITE, nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL or FILE_FLAG_OVERLAPPED, 0);

      WUsb_Initialize(WinUsbDeviceHandle, WinUsbInterfaceHandle);

      UsbInitializeAsync();

      Timeout := 1000;
      WUsb_SetPipePolicy(WinUsbInterfaceHandle, $02, 3, 4, @Timeout);
      WUsb_SetPipePolicy(WinUsbInterfaceHandle, $81, 3, 4, @Timeout);
    end;

    Result := True;
    Exit;
  end;

  WinUsbInterfaceHandle := 0;
  WinUsbDeviceHandle := 0;
  Result := False;
end;





{
  UsbCloseConnection
  ---------------------------------------------------
  Shutdown the usb mode.
  ---------------------------------------------------
}
procedure UsbCloseConnection();
begin
  if (WinUsbInterfaceHandle <> 0) then
  begin
    WUsb_Free(WinUsbInterfaceHandle);
    WinUsbInterfaceHandle := 0;
    CloseHandle(WinUsbDeviceHandle);
  end;

  if (Assigned(LibusbDeviceHandle)) then
  begin
    usb_release_interface(LibusbDeviceHandle, 0);
    usb_close(LibusbDeviceHandle);
    LibusbDeviceHandle := nil;
  end;
end;






{
  UsbInitializeAsync
  ---------------------------------------------------
  Do initialization procedure for usbhostfs.
  Not sure what kind of data is transmitted,
  this was found out by spying on the usb
  transfer of PSPLINK.
  ---------------------------------------------------
  Returns True if successfull.
}
function UsbInitializeAsync(): Boolean;
var
  LengthTransferred: Cardinal;
  Timeout: Cardinal;

begin
  ScrapBuffer := #$12#$08#$2F#$78; // HOSTFS_MAGIC

  Result := False;

  if (WinUsbInterfaceHandle <> 0) then
  begin
    Timeout := 500;
    WUsb_SetPipePolicy(WinUsbInterfaceHandle, $02, 3, 4, @Timeout);
    WUsb_SetPipePolicy(WinUsbInterfaceHandle, $81, 3, 4, @Timeout);

    WUsb_WritePipe(WinUsbInterfaceHandle, $02, @ScrapBuffer, 4, LengthTransferred, nil);
    if (LengthTransferred <> 4) then
      Exit;

    // Without the wait, the next write may "succeed" even though it
    // really doesn't
    Sleep(500);

    WUsb_WritePipe(WinUsbInterfaceHandle, $02, @ScrapBuffer, FrameHeaderMinusMagicSize, LengthTransferred, nil);
    if (LengthTransferred = FrameHeaderMinusMagicSize) then
      Exit;

    WUsb_ReadPipe(WinUsbInterfaceHandle, $81, @ScrapBuffer, 12, LengthTransferred, nil);
    if (LengthTransferred <> 12) then
      Exit;

    WUsb_WritePipe(WinUsbInterfaceHandle, $02, @ScrapBuffer, 12, LengthTransferred, nil);
    if (LengthTransferred <> 12) then
      Exit;
  end
  else if (Assigned(LibusbDeviceHandle)) then
  begin
    // Send magic
    LengthTransferred := usb_bulk_write(LibusbDeviceHandle, $02, ScrapBuffer, 4, 500);
    if (LengthTransferred <> 4) then
      Exit;

    // Try to send another 12 bytes to complete the frameHeader
    // If the PSP is already initialized, this will succeed. Then there
    // is no need to complete the rest of the initialization. The function
    // readFrameInformation() on the PSP will discard this data because of
    // a wrong magic.
    LengthTransferred := usb_bulk_write(LibusbDeviceHandle, $02, ScrapBuffer, FrameHeaderMinusMagicSize, 500);
    if (LengthTransferred = FrameHeaderMinusMagicSize) then
      Exit;

    // Receive hello response
    LengthTransferred := usb_bulk_read(LibusbDeviceHandle, $81, ScrapBuffer, 12, 500);
    if (LengthTransferred <> 12) then
      Exit;

    LengthTransferred := usb_bulk_write(LibusbDeviceHandle, $02, ScrapBuffer, 12, 500);
    if (LengthTransferred <> 12) then
      Exit;

  end;

  Result := True;
end;





{
  UsbSendBuffer
  ---------------------------------------------------
  Send a buffer over usb.
  ---------------------------------------------------
  Returns TRUE on success.
}
function UsbSendBuffer(DataBuffer: Pointer; DataSize: Cardinal): Boolean;
var
  Length: Cardinal;
  SendLength: Cardinal;
  Position: Cardinal;
  DataPointer: PByte;
  Success: Boolean;

begin
  Success := False;
  Position := 0;
  DataPointer := DataBuffer;

  while (Position < DataSize) do
  begin
    Length := DataSize - Position;

    if (WinUsbInterfaceHandle <> 0) then
    begin
      Success := WUsb_WritePipe(WinUsbInterfaceHandle, $02, DataPointer, Length, SendLength, nil);
    end
    else if Assigned(LibusbDeviceHandle) then
    begin
      SendLength := usb_bulk_write(LibusbDeviceHandle, $02, DataPointer^, Length, 10000);
      if (Integer(SendLength) < 0) then
      begin
        SendLength := 0;
        Success := False;
      end
      else
        Success := True;
    end;

    if not Success then
    begin
      Result := False;
      Exit;
    end;

    Inc(DataPointer, SendLength);
    Position := Position + SendLength;
  end;

  Result := True;
end;




{
  UsbReceiveData
  ---------------------------------------------------
  Read data from the usb port and store it into
  the buffer. The function only returns after
  bufferSize Bytes are read or the usb function
  times out.
  ---------------------------------------------------
  Returns the number of bytes read.
}
function UsbReceiveData(Buffer: Pointer; BufferSize: LongWord): Boolean;
var
  LengthTransferred: Cardinal;

begin
  if (WinUsbInterfaceHandle <> 0) then
  begin
    WUsb_ReadPipe(WinUsbInterfaceHandle, $81, Buffer, BufferSize, LengthTransferred, nil);
    Result := (LengthTransferred = BufferSize);
    Exit;
  end;

  if (Assigned(LibusbDeviceHandle)) then
  begin
    Result := (usb_bulk_read(LibusbDeviceHandle, $81, Buffer^, BufferSize, 300) = BufferSize);
    Exit;
  end;

  Result := False;
end;



end.