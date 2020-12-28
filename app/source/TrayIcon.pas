unit TrayIcon;

interface

uses
  Windows, ShellAPI, ExtCtrls, Forms;

type
  TPSPdispTrayIcon = class(TTrayIcon)
  public
    function GetHandle: HWND;
  end;

implementation

function TPSPdispTrayIcon.GetHandle : HWND;
begin
  Result := Data.Wnd;
end;


end.
