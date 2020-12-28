{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  PSPdisp.dpr - project file for PSPdisp

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}


program PSPdisp;

{%TogetherDiagram 'ModelSupport_PSPdisp\default.txaPackage'}

uses
  Forms,
  main in 'main.pas' {MainForm},
  usb in 'usb.pas',
  display in 'display.pas',
  control in 'control.pas',
  wlan in 'wlan.pas',
  mainloop in 'mainloop.pas',
  compress in 'compress.pas',
  graphic in 'graphic.pas',
  audio in 'audio.pas',
  config in 'config.pas';

{$R *.RES}

begin
  Application.Initialize;
  Application.Title := 'PSPdisp';
  Application.CreateForm(TMainForm, MainForm);
  Application.ShowMainForm := False;
  Application.Run;
end.
