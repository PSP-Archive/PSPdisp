unit main;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, XPMan, StdCtrls, FileCtrl, ComCtrls;

type
  TMainForm = class(TForm)
    Label1: TLabel;
    Label2: TLabel;
    ChangePath: TButton;
    Detect: TButton;
    Copy: TButton;
    Cancel: TButton;
    XPManifest1: TXPManifest;
    PathComboBox: TComboBox;
    Label3: TLabel;
    Label4: TLabel;
    Label5: TLabel;
    Label6: TLabel;
    ProgressBar: TProgressBar;
    procedure FormCreate(Sender: TObject);
    procedure DetectClick(Sender: TObject);
    procedure ChangePathClick(Sender: TObject);
    procedure CopyClick(Sender: TObject);
    procedure CancelClick(Sender: TObject);
  private
    procedure Autodetect();
    procedure OnDeviceChangeMessage(var Msg: TMessage); message WM_DEVICECHANGE;
    procedure CopyTheFile(Source: String; Target: String);
    procedure CreateDirectory(Directory: String);
    procedure CheckForCustomFirmware();
  public
  end;

var
  MainForm: TMainForm;
  DoAutoUpdate: Boolean;

implementation

{$R *.dfm}



procedure TMainForm.OnDeviceChangeMessage(var Msg: TMessage);
begin
  if DoAutoUpdate then
    Autodetect();
end;





procedure TMainForm.Autodetect();
var
  Drive: Char;
  SearchRec: TSearchRec;
  lpMaximumComponentLength : DWORD;
  lpFileSystemFlags: DWORD;
  GameFolderIndex: Integer;
begin
  GameFolderIndex := 0;
  PathComboBox.Clear;
  Copy.Enabled := False;

  for Drive := 'A' to 'Z' do
  begin
    try
      if GetVolumeInformation(PChar(Drive + ':\'), nil, 0, nil, lpMaximumComponentLength, lpFileSystemFlags, nil, 0) then
      begin
        if (GetDriveType(PChar(Drive + ':\')) = DRIVE_REMOVABLE) then
        begin
          if DirectoryExists(Drive + ':\PSP') then
          begin
            if (FindFirst(Drive + ':\PSP\GAME*', (faDirectory), SearchRec) = 0) then
            begin
              if UpperCase(SearchRec.Name) <> 'GAME150' then
                PathComboBox.Items.Add(Drive + ':\PSP\' + SearchRec.Name);
              if UpperCase(SearchRec.Name) = 'GAME' then
                GameFolderIndex := PathComboBox.Items.Count - 1;

              while (FindNext(SearchRec) = 0) do
              begin
                if UpperCase(SearchRec.Name) <> 'GAME150' then
                  PathComboBox.Items.Add(Drive + ':\PSP\' + SearchRec.Name);
                if UpperCase(SearchRec.Name) = 'GAME' then
                  GameFolderIndex := PathComboBox.Items.Count - 1;
              end;
            end;
            FindClose(SearchRec);
          end;
        end;
      end;
    except

    end;
  end;

  if (PathComboBox.Items.Count > 0) then
  begin
    Copy.Enabled := True;
    PathComboBox.ItemIndex := GameFolderIndex;
  end;
end;


procedure TMainForm.CancelClick(Sender: TObject);
begin
  Close;
end;

procedure TMainForm.ChangePathClick(Sender: TObject);
var
  Directory : String;
begin
  if (SelectDirectory('Select the path to the PSP game folder.', '', Directory)) then
  begin
    if (DirectoryExists(Directory)) then
    begin
      PathComboBox.Items.Add(Directory);
      PathComboBox.ItemIndex := PathComboBox.Items.Count - 1;
    end;

    Copy.Enabled := True; 
  end;
end;

procedure TMainForm.CopyClick(Sender: TObject);
begin
  DoAutoUpdate := False;

  // Check for other custom firmware folders on the memory stick
  // and warn if the PSP seems to run official firmware
  //CheckForCustomFirmware();

  try
    ProgressBar.Step := 1;
    Progressbar.Max := 18;
    ProgressBar.Min := 0;
    Cancel.Visible := False;
    Copy.Visible := False;
    PathComboBox.Enabled := False;
    ChangePath.Enabled := False;
    ProgressBar.Visible := True;
    Application.ProcessMessages;

    CreateDirectory('\PSPdisp');
    CreateDirectory('\PSPdisp\graphics');

    CopyTheFile('..\psp\PSP\GAME\PSPdisp\EBOOT.PBP', '\PSPdisp\EBOOT.PBP');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\usbhostfs.prx', '\PSPdisp\usbhostfs.prx');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\kernel.prx', '\PSPdisp\kernel.prx');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\pspdisp.cfg', '\PSPdisp\pspdisp.cfg');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\keys.png', '\PSPdisp\graphics\keys.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\keys_c.png', '\PSPdisp\graphics\keys_c.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\keys_c_t.png', '\PSPdisp\graphics\keys_c_t.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\keys_s.png', '\PSPdisp\graphics\keys_s.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\keys_s_c.png', '\PSPdisp\graphics\keys_s_c.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\keys_t.png', '\PSPdisp\graphics\keys_t.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\nums.png', '\PSPdisp\graphics\nums.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\nums_c.png', '\PSPdisp\graphics\nums_c.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\nums_c_t.png', '\PSPdisp\graphics\nums_c_t.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\nums_s.png', '\PSPdisp\graphics\nums_s.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\nums_s_c.png', '\PSPdisp\graphics\nums_s_c.png');
    CopyTheFile('..\psp\PSP\GAME\PSPdisp\graphics\nums_t.png', '\PSPdisp\graphics\nums_t.png');

  except
    MessageDlg('An error occurred while copying files to "' + PathComboBox.Text + '\PSPdisp".', mtError, [mbOk], -1);
    Close;
  end;
  MessageDlg('Files successfully copied to "' + PathComboBox.Text + '\PSPdisp".', mtInformation, [mbOk], -1);
  Close;
end;

procedure TMainForm.DetectClick(Sender: TObject);
begin
  Autodetect();
end;

procedure TMainForm.FormCreate(Sender: TObject);
begin
  DoAutoUpdate := True;
  Autodetect();
end;

procedure TMainForm.CopyTheFile(Source: String; Target: String);
begin
  if not CopyFile(PAnsiChar(Source), PAnsiChar(PathComboBox.Text + Target), False) then
    raise Exception.Create('Cannot copy "' + Source + '" to "' + PathComboBox.Text + Target + '".');

  ProgressBar.StepIt();
end;

procedure TMainForm.CreateDirectory(Directory: String);
begin
    if not ForceDirectories(PathComboBox.Text + Directory) then
      raise Exception.Create('Cannot create directory "' + PathComboBox.Text + Directory + '".');

  ProgressBar.StepIt();
end;


procedure TMainForm.CheckForCustomFirmware();
var
  SEPLUGINS_Directory: String;
  ISO_Directory: String;
begin
  SEPLUGINS_Directory := ExtractFileDrive(PathCombobox.Text) + '\SEPLUGINS';
  ISO_Directory := ExtractFileDrive(PathCombobox.Text) + '\ISO';

  if not (DirectoryExists(SEPLUGINS_Directory) or DirectoryExists(ISO_Directory)) then
    MessageDlg('The drive you selected does not seem to be formatted for use by a PSP Custom Firmware. Make sure you are running a CFW or Homebrew Enabler on your PSP otherwise PSPdisp will not work. You may receive "The game could not be started" errors.', mtWarning, [mbok], -1);
end;

end.
