{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  wlan.pas - functions dealing with wlan transfer

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}

unit wlan;

interface

uses
  SysUtils, Classes, ExtCtrls, Dialogs, WinSock, Windows;




function WlanWaitForClient() : Boolean;
function WlanShutdownServer() : Boolean;
function WlanStartServer() : Boolean;
function WlanCloseClient() : Boolean;
function WlanSendBuffer(Buffer: Pointer; DataSize: Integer): Boolean;
function WlanReceiveData(Buffer: Pointer; bufferSize: Integer): Boolean;
function GETAllIPAddresses(): string;

implementation

uses
  Main;

var
  SocketHandle : Integer;
  ClientHandle : Integer;
  SockAddr : TSockAddrIn;
  WSADataStruct : TWSAData;



{
  WlanStartServer
  ---------------------------------------------------
  Initialize the server.
  ---------------------------------------------------
  Returns TRUE on success.
}
function WlanStartServer() : Boolean;
var
  arg: Integer;
  IPs: String;
begin
  Result := False;

  WSAStartup($101, WSADataStruct);

  // Create socket
  SocketHandle := Winsock.socket(PF_INET, SOCK_STREAM, 0);

  if (SocketHandle = INVALID_SOCKET) then
  begin
    MessageDlg('socket() failed!', mtError, [mbOK], -1);
    Exit;
  end;

  // Bind socket
  SockAddr.sin_family := AF_INET;
  SockAddr.sin_addr.S_addr := INADDR_ANY;
  SockAddr.sin_port := htons(17584 + InstanceNumber);

  if (bind(SocketHandle, SockAddr, sizeof(SockAddr)) = SOCKET_ERROR) then
  begin
    MessageDlg('The port used by the PSPdisp server is in use. Probably another instance is already running.', mtError, [mbOK], -1);
    //MessageDlg('bind() failed!', mtError, [mbOK], -1);
    Exit;
  end;

  // Start listening, only accept 1 client (0 clients in backlog)
  if (listen(SocketHandle, 1) = SOCKET_ERROR) then
  begin
    MessageDlg('listen() failed!', mtError, [mbOK], -1);
    Exit;
  end;

  arg := 1000;
  setsockopt(SocketHandle, SOL_SOCKET, SO_RCVTIMEO, @arg, sizeof(arg));
  setsockopt(SocketHandle, SOL_SOCKET, SO_SNDTIMEO, @arg, sizeof(arg));

  // Show "Server started" message
  IPs := GETAllIPAddresses();
  MainForm.OptionsWlanAddress.Caption := Ips;

  if MainForm.OptionsWlanNotifyStart.Checked then
    MainForm.ShowBalloonHint('PSPdisp: Server started', 'Please connect your PSP to the following IP address:' + #10 + Ips, bfInfo);

  Result := True;
end;



{
  GETAllIPAddresses
  ---------------------------------------------------
  Retrieves the IP addresses of all local network
  adapters.
  ---------------------------------------------------
  Returns a string containing the IP addresses.
}
function GETAllIPAddresses(): string;
type
  TaPInAddr = array[0..10] of PInAddr;
  PaPInAddr = ^TaPInAddr;
var
  phe: PHostEnt;
  pptr: PaPInAddr;
  Buffer: array[0..63] of Char;
  I: Integer;
  GInitData: TWSAData;
  tempString: string;
begin
  Result := '';

  GetHostName(Buffer, SizeOf(Buffer));
  phe := GetHostByName(buffer);
  if phe = nil then Exit;
  pPtr := PaPInAddr(phe^.h_addr_list);
  I    := 0;
  while pPtr^[I] <> nil do
  begin
    tempString := inet_ntoa(pptr^[I]^);
    if Result = '' then
      Result := tempString
    else
      Result := Result + ' or' + #10 + tempString;
    Inc(I);
  end;

  if (Result = '127.0.0.1') then
    Result := 'No local IP found. You may not be able to use the Wireless LAN mode.';
end;





{
  WlanShutdownServer
  ---------------------------------------------------
  Close the server handle.
  ---------------------------------------------------
  Returns TRUE.
}
function WlanShutdownServer() : Boolean;
begin
  shutdown(SocketHandle, SD_BOTH);
  closesocket(SocketHandle);
  WSACleanup;
  Result := True;
end;





{
  WlanCloseClient
  ---------------------------------------------------
  Close a client handle.
  ---------------------------------------------------
  Returns TRUE.
}
function WlanCloseClient() : Boolean;
begin
  shutdown(ClientHandle, SD_BOTH);
  closesocket(ClientHandle);
  Result := True;
end;



{
  WlanReceiveData
  ---------------------------------------------------
  Read data from the network.
  ---------------------------------------------------
  Returns TRUE on success.
}
function WlanReceiveData(Buffer: Pointer; bufferSize: Integer): Boolean;
var
  Bytessent: Integer;
  Retries: Integer;
begin
  for Retries := 0 to 2 do
  begin
    Bytessent := recv(ClientHandle, Buffer^, bufferSize, 0);
    if (Bytessent <> SOCKET_ERROR) then
    begin
      Result := (Bytessent = bufferSize);
      Exit;
    end;
  end;
  Result := False;
end;




{
  WlanSendBuffer
  ---------------------------------------------------
  Send a buffer over Wlan.
  ---------------------------------------------------
  Returns TRUE on success.
}
function WlanSendBuffer(Buffer: Pointer; DataSize: Integer): Boolean;
var
  AmountSent: Integer;
  Position : Integer;
  BytesLeftToWrite: Integer;
  BytesToWrite: Integer;
  DataPointer: PByte;

begin
  DataPointer := Buffer;
  Position := 0;
  BytesToWrite := 0;

  while (Position < DataSize) do
  begin
    BytesLeftToWrite := DataSize - Position;
    if (BytesLeftToWrite > 0) then
    begin
      if BytesLeftToWrite > 2048 then
        BytesToWrite := 2048
      else
        BytesToWrite := BytesLeftToWrite;
    end;

    AmountSent := send(ClientHandle, DataPointer^, BytesToWrite, 0);

    if (AmountSent = SOCKET_ERROR) then
    begin
      Result := False;
      Exit;
    end;

    Inc(DataPointer, AmountSent);
    Position := Position + AmountSent;
  end;

  Result := True;
end;





{
  WlanWaitForClient
  ---------------------------------------------------
  Wait for a client connection.
  ---------------------------------------------------
  Returns TRUE on successfull connection.
}
function WlanWaitForClient() : Boolean;
var
  AddressLength : Integer;
  fds : Tfdset;
  timeout : timeval;
  arg: Integer;
  Buffer: Array[0..32] of Byte;

begin
  Result := False;

  AddressLength := sizeof(SockAddr);

  FD_ZERO(fds);
  FD_SET(SocketHandle, fds);
  timeout.tv_usec := 0;
  timeout.tv_sec := 2;

  if (select(SocketHandle + 1, @fds, nil, nil, @timeout) = SOCKET_ERROR) then
  begin
    MessageDlg('select() failed!', mtError, [mbOK], -1);
    Exit;
  end;

  if (FD_ISSET(SocketHandle, fds)) then
  begin
    ClientHandle := accept(SocketHandle, @SockAddr, @AddressLength);
    if (ClientHandle = SOCKET_ERROR) then
    begin
      MessageDlg('accept() failed!', mtError, [mbOK], -1);
      Exit;
    end;

    arg := 1000;
    setsockopt(SocketHandle, SOL_SOCKET, SO_RCVTIMEO, @arg, sizeof(arg));
    setsockopt(SocketHandle, SOL_SOCKET, SO_SNDTIMEO, @arg, sizeof(arg));

    // Check "password"
    ZeroMemory(@Buffer, 33);
    if WlanReceiveData(@Buffer, 32) then
    begin
      if (StrComp(PAnsiChar(@Buffer), PAnsiChar(MainForm.OptionsWlanPassword.Text)) = 0) then
      begin
        Result := True;
        if MainForm.OptionsWlanNotifyConnect.Checked then
          MainForm.ShowBalloonHint('PSPdisp: New client connected', 'A PSP client has connected to the PSPdisp server.', bfInfo);
      end
      else begin
        // Show failed connection regardless of notification setting
        MainForm.ShowBalloonHint('PSPdisp: Warning', 'A connection attempt occured. It was rejected because of an incorrect password.', bfWarning);
        WlanCloseClient();
        Sleep(5000);
      end;
    end;
  end;
end;


end.