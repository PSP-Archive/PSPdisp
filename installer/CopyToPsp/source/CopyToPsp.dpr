program CopyToPsp;

uses
  Forms,
  main in 'main.pas' {MainForm};

{$R *.res}

begin
  Application.Initialize;
  Application.Title := 'Copy PSPdisp game files to the PSP';
  Application.CreateForm(TMainForm, MainForm);
  Application.Run;
end.
