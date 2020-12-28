object MainForm: TMainForm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'Copy PSPdisp game files to the PSP'
  ClientHeight = 279
  ClientWidth = 431
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 24
    Top = 94
    Width = 359
    Height = 26
    Caption = 
      '3. The path to your game folder will be autodetected. If there a' +
      're multiple folders they will all appear in the list below.'
    WordWrap = True
  end
  object Label2: TLabel
    Left = 24
    Top = 25
    Width = 361
    Height = 17
    AutoSize = False
    Caption = 
      'This program will try to copy the PSPdisp game files to your PSP' +
      '.'
    WordWrap = True
  end
  object Label3: TLabel
    Left = 24
    Top = 127
    Width = 389
    Height = 13
    Caption = 
      '4. Choose the non-1.5-kernel path, eg: P:\PSP\GAME5XX and press ' +
      '"Copy Files".'
    WordWrap = True
  end
  object Label4: TLabel
    Left = 24
    Top = 56
    Width = 224
    Height = 13
    Caption = '1. Connect your PSP with the PC through USB.'
  end
  object Label5: TLabel
    Left = 24
    Top = 75
    Width = 324
    Height = 13
    Caption = 
      '2. If not automatically done, choose "USB connection" on your PS' +
      'P.'
  end
  object Label6: TLabel
    Left = 24
    Top = 146
    Width = 372
    Height = 26
    Caption = 
      '5. If you don'#39't want to copy the files now, you can do it later ' +
      'through a start menu link.'
    WordWrap = True
  end
  object ChangePath: TButton
    Left = 330
    Top = 197
    Width = 71
    Height = 21
    Caption = 'Browse ...'
    TabOrder = 1
    OnClick = ChangePathClick
  end
  object Detect: TButton
    Left = 24
    Top = 263
    Width = 145
    Height = 25
    Caption = 'Try to autodetect again'
    TabOrder = 2
    Visible = False
    OnClick = DetectClick
  end
  object Copy: TButton
    Left = 256
    Top = 240
    Width = 145
    Height = 25
    Caption = 'Copy files'
    Default = True
    TabOrder = 3
    OnClick = CopyClick
  end
  object Cancel: TButton
    Left = 24
    Top = 240
    Width = 145
    Height = 25
    Caption = 'Cancel'
    TabOrder = 4
    OnClick = CancelClick
  end
  object PathComboBox: TComboBox
    Left = 24
    Top = 197
    Width = 300
    Height = 21
    Style = csDropDownList
    ItemHeight = 13
    TabOrder = 0
  end
  object ProgressBar: TProgressBar
    Left = 24
    Top = 241
    Width = 377
    Height = 16
    TabOrder = 5
    Visible = False
  end
  object XPManifest1: TXPManifest
    Left = 384
    Top = 16
  end
end
