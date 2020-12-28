object MainForm: TMainForm
  Left = 457
  Top = 150
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsDialog
  Caption = 'PSPdisp'
  ClientHeight = 407
  ClientWidth = 496
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnClose = FormClose
  OnCreate = FormCreate
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Label15: TLabel
    Left = 28
    Top = 120
    Width = 26
    Height = 13
    Caption = 'Cross'
  end
  object Label16: TLabel
    Left = 28
    Top = 120
    Width = 26
    Height = 13
    Caption = 'Cross'
  end
  object Label12: TLabel
    Left = 28
    Top = 147
    Width = 26
    Height = 13
    Caption = 'Cross'
  end
  object Label7: TLabel
    Left = 20
    Top = 21
    Width = 90
    Height = 13
    Caption = 'Frames per second'
  end
  object Label8: TLabel
    Left = 146
    Top = 21
    Width = 6
    Height = 13
    Caption = '0'
  end
  object Label14: TLabel
    Left = 28
    Top = 29
    Width = 90
    Height = 13
    Caption = 'Frames per second'
  end
  object Label17: TLabel
    Left = 158
    Top = 21
    Width = 6
    Height = 13
    Caption = '0'
  end
  object Label18: TLabel
    Left = 20
    Top = 40
    Width = 90
    Height = 13
    Caption = 'Average frame size'
  end
  object Label19: TLabel
    Left = 158
    Top = 40
    Width = 6
    Height = 13
    Caption = '0'
  end
  object Label46: TLabel
    Left = 100
    Top = 123
    Width = 39
    Height = 13
    Caption = 'Shortest'
  end
  object Label47: TLabel
    Left = 125
    Top = 150
    Width = 25
    Height = 13
    Caption = 'Short'
  end
  object Label48: TLabel
    Left = 100
    Top = 177
    Width = 37
    Height = 13
    Caption = 'Medium'
  end
  object Label49: TLabel
    Left = 100
    Top = 204
    Width = 24
    Height = 13
    Caption = 'Long'
  end
  object Label50: TLabel
    Left = 100
    Top = 231
    Width = 38
    Height = 13
    Caption = 'Longest'
  end
  object PageControl: TPageControl
    Left = 8
    Top = 8
    Width = 480
    Height = 361
    ActivePage = WlanTab
    TabOrder = 0
    object TabSheet1: TTabSheet
      Caption = 'About'
      object GroupBox1: TGroupBox
        Left = 118
        Top = 191
        Width = 242
        Height = 105
        Caption = 'Status'
        TabOrder = 0
        object Label1: TLabel
          Left = 20
          Top = 21
          Width = 90
          Height = 13
          Caption = 'Frames per second'
        end
        object AboutFps: TLabel
          Left = 158
          Top = 21
          Width = 6
          Height = 13
          Caption = '0'
        end
        object Label9: TLabel
          Left = 20
          Top = 40
          Width = 90
          Height = 13
          Caption = 'Average frame size'
        end
        object AboutFrameSize: TLabel
          Left = 158
          Top = 40
          Width = 25
          Height = 13
          Caption = '0 KiB'
        end
        object Label20: TLabel
          Left = 20
          Top = 59
          Width = 81
          Height = 13
          Caption = 'Total frames sent'
        end
        object AboutTotalFrames: TLabel
          Left = 158
          Top = 59
          Width = 6
          Height = 13
          Caption = '0'
        end
        object Label22: TLabel
          Left = 20
          Top = 78
          Width = 85
          Height = 13
          Caption = 'Total amount sent'
        end
        object AboutTotalBytesSent: TLabel
          Left = 158
          Top = 78
          Width = 25
          Height = 13
          Caption = '0 KiB'
        end
      end
      object Panel1: TPanel
        Left = 16
        Top = 16
        Width = 441
        Height = 137
        BevelOuter = bvLowered
        TabOrder = 1
        object Label2: TLabel
          Left = 26
          Top = 40
          Width = 3
          Height = 13
        end
        object Label3: TLabel
          Left = 23
          Top = 24
          Width = 105
          Height = 32
          Caption = 'PSPdisp'
          Color = clBtnFace
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -27
          Font.Name = 'Verdana'
          Font.Style = []
          ParentColor = False
          ParentFont = False
        end
        object Label6: TLabel
          Left = 134
          Top = 38
          Width = 125
          Height = 16
          Caption = #169' 2008 - 2015 JJS'
          Color = clBtnFace
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -13
          Font.Name = 'Verdana'
          Font.Style = []
          ParentColor = False
          ParentFont = False
        end
        object OptionsVersionNumber: TLabel
          Left = 23
          Top = 62
          Width = 47
          Height = 16
          Caption = 'Version'
          Color = clBtnFace
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -13
          Font.Name = 'Verdana'
          Font.Style = []
          ParentColor = False
          ParentFont = False
        end
        object AboutVisitHomepage: TLabel
          Left = 23
          Top = 102
          Width = 206
          Height = 13
          Cursor = crHandPoint
          Caption = 'Visit the PSPdisp homepage @ jjs.at'
          Color = clBtnFace
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'Verdana'
          Font.Style = []
          ParentColor = False
          ParentFont = False
          OnClick = AboutVisitHomepageClick
        end
      end
    end
    object WlanTab: TTabSheet
      Caption = 'WLAN'
      ImageIndex = 1
      object GroupBox2: TGroupBox
        Left = 16
        Top = 8
        Width = 441
        Height = 85
        Caption = 'Show notification on these events'
        TabOrder = 0
        object OptionsWlanNotifyStart: TCheckBox
          Left = 24
          Top = 26
          Width = 137
          Height = 17
          Caption = 'Server has been started'
          Checked = True
          State = cbChecked
          TabOrder = 0
          OnClick = OnModificationForConfig
        end
        object OptionsWlanNotifyConnect: TCheckBox
          Left = 208
          Top = 26
          Width = 113
          Height = 17
          Caption = 'Client connected'
          Checked = True
          State = cbChecked
          TabOrder = 1
          OnClick = OnModificationForConfig
        end
        object OptionsWlanNotifyDisconnect: TCheckBox
          Left = 24
          Top = 49
          Width = 113
          Height = 17
          Caption = 'Client disconnected'
          Checked = True
          State = cbChecked
          TabOrder = 2
          OnClick = OnModificationForConfig
        end
      end
      object GroupBox11: TGroupBox
        Left = 16
        Top = 172
        Width = 441
        Height = 149
        Caption = 'IP address'
        TabOrder = 1
        object OptionsWlanAddress: TLabel
          Left = 24
          Top = 63
          Width = 297
          Height = 79
          AutoSize = False
        end
        object Label34: TLabel
          Left = 24
          Top = 24
          Width = 393
          Height = 33
          AutoSize = False
          Caption = 
            'When WLAN mode is chosen and PSPdisp enabled, all IP addresses b' +
            'ound to this PC are shown here.'
          WordWrap = True
        end
      end
      object GroupBox12: TGroupBox
        Left = 16
        Top = 99
        Width = 441
        Height = 59
        Caption = 'Password'
        TabOrder = 2
        object Label58: TLabel
          Left = 24
          Top = 28
          Width = 118
          Height = 13
          Caption = 'Password (max 32 chars)'
        end
        object OptionsWlanPassword: TEdit
          Left = 160
          Top = 24
          Width = 145
          Height = 21
          MaxLength = 32
          PasswordChar = '*'
          TabOrder = 0
        end
        object OptionsWlanNoPassword: TButton
          Left = 328
          Top = 22
          Width = 91
          Height = 25
          Caption = 'No password'
          TabOrder = 1
          OnClick = OptionsWlanNoPasswordClick
        end
      end
    end
    object QualityTab: TTabSheet
      Caption = 'Quality'
      ImageIndex = 2
      object GroupBox3: TGroupBox
        Left = 16
        Top = 8
        Width = 441
        Height = 313
        Caption = 'JPEG compression quality'
        TabOrder = 0
        object Label5: TLabel
          Left = 13
          Top = 27
          Width = 404
          Height = 34
          AutoSize = False
          Caption = 
            'Quality can be set from 0 to 100. Higher values produce larger f' +
            'rames which can reduce the frame rate.'
          WordWrap = True
        end
        object Label36: TLabel
          Left = 54
          Top = 145
          Width = 36
          Height = 13
          Caption = 'Highest'
        end
        object Label37: TLabel
          Left = 54
          Top = 172
          Width = 22
          Height = 13
          Caption = 'High'
        end
        object Label38: TLabel
          Left = 54
          Top = 199
          Width = 37
          Height = 13
          Caption = 'Medium'
        end
        object Label39: TLabel
          Left = 54
          Top = 226
          Width = 20
          Height = 13
          Caption = 'Low'
        end
        object Label40: TLabel
          Left = 54
          Top = 253
          Width = 34
          Height = 13
          Caption = 'Lowest'
        end
        object Label56: TLabel
          Left = 13
          Top = 67
          Width = 412
          Height = 34
          AutoSize = False
          Caption = 
            'Values higher than 95 are usually not beneficial for the image q' +
            'uality and only unnecessarily increase the frame size.'
          WordWrap = True
        end
        object OptionsQualityHighest: TEdit
          Tag = 80
          Left = 132
          Top = 142
          Width = 60
          Height = 21
          TabOrder = 0
          Text = '80'
          OnChange = OptionsQualityOnChange
          OnExit = OptionsQualityOnExit
        end
        object OptionsQualityHigh: TEdit
          Tag = 50
          Left = 132
          Top = 169
          Width = 60
          Height = 21
          TabOrder = 1
          Text = '50'
          OnChange = OptionsQualityOnChange
          OnExit = OptionsQualityOnExit
        end
        object OptionsQualityMedium: TEdit
          Tag = 30
          Left = 132
          Top = 196
          Width = 60
          Height = 21
          TabOrder = 2
          Text = '30'
          OnChange = OptionsQualityOnChange
          OnExit = OptionsQualityOnExit
        end
        object OptionsQualityLow: TEdit
          Tag = 20
          Left = 132
          Top = 223
          Width = 60
          Height = 21
          TabOrder = 3
          Text = '20'
          OnChange = OptionsQualityOnChange
          OnExit = OptionsQualityOnExit
        end
        object OptionsQualityLowest: TEdit
          Tag = 10
          Left = 132
          Top = 250
          Width = 60
          Height = 21
          TabOrder = 4
          Text = '10'
          OnChange = OptionsQualityOnChange
          OnExit = OptionsQualityOnExit
        end
        object OptionsQualityReset: TButton
          Left = 264
          Top = 194
          Width = 144
          Height = 25
          Caption = 'Reset to defaults'
          TabOrder = 5
          OnClick = OptionsQualityResetClick
        end
      end
    end
    object DelayTab: TTabSheet
      Caption = 'Update interval'
      ImageIndex = 3
      object GroupBox4: TGroupBox
        Left = 16
        Top = 8
        Width = 441
        Height = 313
        Caption = 'Delay between frames'
        TabOrder = 0
        object Label10: TLabel
          Left = 13
          Top = 27
          Width = 404
          Height = 34
          AutoSize = False
          Caption = 
            'If chosen in the '#39'Update Interval'#39' menu, PSPdisp will delay the ' +
            'display of the next frame. This limits the maximum framerate.'
          WordWrap = True
        end
        object Label32: TLabel
          Left = 13
          Top = 67
          Width = 404
          Height = 53
          AutoSize = False
          Caption = 
            'Higher delays lead to choppier display with lower fps. Lower del' +
            'ays will produce a smoother display, but utilize more CPU cycles' +
            '.'
          WordWrap = True
        end
        object Label41: TLabel
          Left = 54
          Top = 145
          Width = 39
          Height = 13
          Caption = 'Shortest'
        end
        object Label42: TLabel
          Left = 54
          Top = 172
          Width = 25
          Height = 13
          Caption = 'Short'
        end
        object Label43: TLabel
          Left = 54
          Top = 199
          Width = 37
          Height = 13
          Caption = 'Medium'
        end
        object Label44: TLabel
          Left = 54
          Top = 226
          Width = 24
          Height = 13
          Caption = 'Long'
        end
        object Label45: TLabel
          Left = 54
          Top = 253
          Width = 38
          Height = 13
          Caption = 'Longest'
        end
        object Label51: TLabel
          Left = 198
          Top = 145
          Width = 13
          Height = 13
          Caption = 'ms'
        end
        object Label52: TLabel
          Left = 198
          Top = 172
          Width = 13
          Height = 13
          Caption = 'ms'
        end
        object Label53: TLabel
          Left = 198
          Top = 199
          Width = 13
          Height = 13
          Caption = 'ms'
        end
        object Label54: TLabel
          Left = 198
          Top = 226
          Width = 13
          Height = 13
          Caption = 'ms'
        end
        object Label55: TLabel
          Left = 198
          Top = 253
          Width = 13
          Height = 13
          Caption = 'ms'
        end
        object OptionsDelayShortest: TEdit
          Tag = 30
          Left = 132
          Top = 142
          Width = 60
          Height = 21
          TabOrder = 0
          Text = '30'
          OnChange = OptionsDelayOnChange
          OnExit = OptionsDelayOnExit
        end
        object OptionsDelayShort: TEdit
          Tag = 40
          Left = 132
          Top = 169
          Width = 60
          Height = 21
          TabOrder = 1
          Text = '40'
          OnChange = OptionsDelayOnChange
          OnExit = OptionsDelayOnExit
        end
        object OptionsDelayMedium: TEdit
          Tag = 50
          Left = 132
          Top = 196
          Width = 60
          Height = 21
          TabOrder = 2
          Text = '50'
          OnChange = OptionsDelayOnChange
          OnExit = OptionsDelayOnExit
        end
        object OptionsDelayLong: TEdit
          Tag = 100
          Left = 132
          Top = 223
          Width = 60
          Height = 21
          TabOrder = 3
          Text = '100'
          OnChange = OptionsDelayOnChange
          OnExit = OptionsDelayOnExit
        end
        object OptionsDelayLongest: TEdit
          Tag = 200
          Left = 132
          Top = 250
          Width = 60
          Height = 21
          TabOrder = 4
          Text = '200'
          OnChange = OptionsDelayOnChange
          OnExit = OptionsDelayOnExit
        end
        object OptionsDelayReset: TButton
          Left = 264
          Top = 194
          Width = 144
          Height = 25
          Caption = 'Reset to defaults'
          TabOrder = 5
          OnClick = OptionsDelayResetClick
        end
      end
    end
    object TabSheet2: TTabSheet
      Caption = 'Control'
      ImageIndex = 4
      object GroupBox5: TGroupBox
        Left = 16
        Top = 208
        Width = 441
        Height = 113
        Caption = 'Scancode'
        TabOrder = 0
        object Label11: TLabel
          Left = 24
          Top = 28
          Width = 401
          Height = 46
          AutoSize = False
          Caption = 
            'Click into the edit field and press a key to view its scancode. ' +
            'Some keys do not show their scancode here, please refer to the l' +
            'isting in the help file instead.'
          WordWrap = True
        end
        object OptionsScancode: TEdit
          Left = 166
          Top = 72
          Width = 121
          Height = 21
          ReadOnly = True
          TabOrder = 0
        end
      end
      object GroupBox6: TGroupBox
        Left = 16
        Top = 8
        Width = 441
        Height = 185
        Caption = 'Edit files'
        TabOrder = 1
        object Label13: TLabel
          Left = 24
          Top = 68
          Width = 401
          Height = 45
          AutoSize = False
          Caption = 
            'To create a new control file, copy over an existing one (like '#39'm' +
            'ouse.control'#39') and edit this. You can also modify an existing co' +
            'ntrol scheme.'
          WordWrap = True
        end
        object OptionsOpenControlFileFolder: TButton
          Left = 72
          Top = 19
          Width = 297
          Height = 25
          Caption = 'Open control file folder to modify / add control files'
          TabOrder = 0
          OnClick = OptionsOpenControlFileFolderClick
        end
        object OptionsRefreshControlFileList: TButton
          Left = 72
          Top = 119
          Width = 297
          Height = 25
          Caption = 'Refresh control file list'
          TabOrder = 1
          OnClick = OptionsRefreshControlFileListClick
        end
        object OptionsReloadControlFile: TButton
          Left = 72
          Top = 150
          Width = 297
          Height = 25
          Caption = 'Reload current control file'
          TabOrder = 2
          OnClick = OptionsReloadControlFileClick
        end
      end
    end
    object TabSheet3: TTabSheet
      Caption = 'Sound'
      ImageIndex = 5
      object GroupBox8: TGroupBox
        Left = 16
        Top = 8
        Width = 441
        Height = 101
        Caption = 'Audio input device'
        TabOrder = 0
        object Label4: TLabel
          Left = 24
          Top = 20
          Width = 393
          Height = 27
          AutoSize = False
          Caption = 
            'Choose the audio recording device that should be used for captur' +
            'ing sounds.'
          WordWrap = True
        end
        object Label27: TLabel
          Left = 24
          Top = 77
          Width = 117
          Height = 13
          Caption = '* = recommended setting'
        end
        object OptionsAudioDevice: TComboBox
          Left = 24
          Top = 50
          Width = 313
          Height = 21
          Style = csDropDownList
          ItemHeight = 13
          TabOrder = 0
        end
        object OptionsSoundRefresh: TButton
          Left = 352
          Top = 50
          Width = 65
          Height = 21
          Caption = 'Refresh'
          TabOrder = 1
          OnClick = OptionsSoundRefreshClick
        end
      end
      object GroupBox7: TGroupBox
        Left = 16
        Top = 115
        Width = 441
        Height = 206
        Caption = 'Instructions'
        TabOrder = 1
        object Label21: TLabel
          Left = 24
          Top = 43
          Width = 401
          Height = 30
          AutoSize = False
          Caption = 
            'Choose your main sound card from the list above, then enable '#39'St' +
            'ereo Mix'#39' or '#39'What you hear'#39' in the recording mixer.'
          WordWrap = True
        end
        object Label28: TLabel
          Left = 27
          Top = 106
          Width = 382
          Height = 62
          AutoSize = False
          Caption = 
            'If possible, enable '#39'Stereo Mix'#39' in the recording settings. Then' +
            ' refresh the list above and choose the newly enabled input. Othe' +
            'rwise choose '#39'Software Loopback Device'#39' which (almost) always wo' +
            'rks.'
          WordWrap = True
        end
        object Label29: TLabel
          Left = 24
          Top = 24
          Width = 96
          Height = 13
          Caption = 'Windows 2000 / XP'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
        object Label30: TLabel
          Left = 27
          Top = 87
          Width = 87
          Height = 13
          Caption = 'Windows Vista / 7'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
        object Button1: TButton
          Left = 72
          Top = 166
          Width = 297
          Height = 25
          Caption = 'Open recording mixer / recording settings'
          TabOrder = 0
          OnClick = MenuOpenMixerClick
        end
      end
    end
    object TabSheet4: TTabSheet
      Caption = 'SideShow'
      ImageIndex = 6
      object GroupBox9: TGroupBox
        Left = 16
        Top = 172
        Width = 441
        Height = 129
        Caption = 'JPEG compression quality'
        TabOrder = 0
        object Label23: TLabel
          Left = 41
          Top = 79
          Width = 32
          Height = 13
          Caption = 'Quality'
        end
        object Label33: TLabel
          Left = 24
          Top = 27
          Width = 393
          Height = 34
          AutoSize = False
          Caption = 
            'SideShow content is mostly static. Therefore a high quality can ' +
            'be chosen without affecting performance (default = 95).'
          WordWrap = True
        end
        object OptionsQualitySideShow: TEdit
          Left = 92
          Top = 76
          Width = 60
          Height = 21
          TabOrder = 0
          Text = '90'
          OnChange = OptionsQualitySideShowChange
        end
      end
      object GroupBox10: TGroupBox
        Left = 16
        Top = 8
        Width = 441
        Height = 150
        Caption = 'Gadget cycling'
        TabOrder = 1
        object Label24: TLabel
          Left = 24
          Top = 28
          Width = 393
          Height = 43
          AutoSize = False
          Caption = 
            'Interval in which the gadgets are automatically cycled. You can ' +
            'also set the direction in which the next gadget to display is ch' +
            'osen.'
          WordWrap = True
        end
        object Label25: TLabel
          Left = 31
          Top = 77
          Width = 35
          Height = 13
          Caption = 'Interval'
        end
        object Label26: TLabel
          Left = 31
          Top = 104
          Width = 42
          Height = 13
          Caption = 'Direction'
        end
        object OptionsSideShowInterval: TEdit
          Left = 97
          Top = 74
          Width = 79
          Height = 21
          TabOrder = 0
          Text = '30'
          OnChange = OnModificationForConfig
        end
        object OptionsSideShowCycleDirection: TComboBox
          Left = 97
          Top = 101
          Width = 103
          Height = 21
          Style = csDropDownList
          ItemHeight = 13
          ItemIndex = 1
          TabOrder = 1
          Text = 'Right'
          OnChange = OnModificationForConfig
          Items.Strings = (
            'Left'
            'Right')
        end
      end
    end
    object TabSheet5: TTabSheet
      Caption = 'Display'
      ImageIndex = 7
      object DisplayPaintBox: TPaintBox
        Left = 16
        Top = 12
        Width = 441
        Height = 178
        Color = clBlack
        ParentColor = False
        OnPaint = DisplayPaintBoxPaint
      end
      object GroupBox13: TGroupBox
        Left = 16
        Top = 199
        Width = 441
        Height = 128
        Caption = 'PSPdisp extended display position'
        TabOrder = 0
        object Label59: TLabel
          Left = 24
          Top = 24
          Width = 401
          Height = 49
          AutoSize = False
          Caption = 
            'In Vista and 7 the position of the extended screen can only be c' +
            'hanged here, not in the regular display properties. In the graph' +
            'ic above, the white box represents the PSPdisp display and the g' +
            'rey boxes your other displays.'
          WordWrap = True
        end
        object Label31: TLabel
          Left = 24
          Top = 92
          Width = 7
          Height = 13
          Caption = 'X'
        end
        object Label57: TLabel
          Left = 144
          Top = 92
          Width = 7
          Height = 13
          Caption = 'Y'
        end
        object OptionsDisplayPositionX: TEdit
          Left = 47
          Top = 89
          Width = 73
          Height = 21
          TabOrder = 0
          Text = '0'
        end
        object OptionsDisplayPositionY: TEdit
          Left = 167
          Top = 89
          Width = 73
          Height = 21
          TabOrder = 1
          Text = '0'
        end
        object OptionsDisplayApplySettings: TButton
          Left = 281
          Top = 86
          Width = 129
          Height = 25
          Caption = 'Apply'
          TabOrder = 2
          OnClick = OptionsDisplayApplySettingsClick
        end
      end
    end
    object TabSheet6: TTabSheet
      Caption = 'Presets'
      ImageIndex = 8
      object GroupBox14: TGroupBox
        Left = 16
        Top = 8
        Width = 441
        Height = 313
        Caption = 'Preset files'
        TabOrder = 0
        object Label35: TLabel
          Left = 13
          Top = 27
          Width = 404
          Height = 66
          AutoSize = False
          Caption = 
            'Various capturing settings can be saved to preset files to make ' +
            'switching between different use cases easier. You could e.g. hav' +
            'e a preset for desktop remote control and another one for high q' +
            'uality audio streaming.'
          WordWrap = True
        end
        object OptionsPresetsList: TListBox
          Left = 13
          Top = 110
          Width = 265
          Height = 187
          ExtendedSelect = False
          ItemHeight = 13
          TabOrder = 0
          OnClick = OptionsPresetsListClick
        end
        object OptionsPresetsSave: TButton
          Left = 294
          Top = 81
          Width = 127
          Height = 25
          Caption = 'Save preset'
          TabOrder = 1
          OnClick = OptionsPresetsSaveClick
        end
        object OptionsPresetsDelete: TButton
          Left = 294
          Top = 271
          Width = 127
          Height = 25
          Caption = 'Delete preset'
          Enabled = False
          TabOrder = 2
          OnClick = OptionsPresetsDeleteClick
        end
        object OptionsPresetsRefresh: TButton
          Left = 294
          Top = 179
          Width = 127
          Height = 25
          Caption = 'Refresh list'
          TabOrder = 3
          OnClick = OptionsPresetsRefreshClick
        end
        object OptionsPresetsName: TEdit
          Left = 15
          Top = 83
          Width = 263
          Height = 21
          TabOrder = 4
        end
        object OptionsPresetsLoad: TButton
          Left = 294
          Top = 125
          Width = 127
          Height = 25
          Caption = 'Load preset'
          Enabled = False
          TabOrder = 5
          OnClick = OptionsPresetsLoadClick
        end
        object OptionsPresetsOpenFolder: TButton
          Left = 294
          Top = 210
          Width = 127
          Height = 25
          Caption = 'Open preset folder'
          TabOrder = 6
          OnClick = OptionsPresetsOpenFolderClick
        end
      end
    end
  end
  object SettingsClose: TButton
    Left = 409
    Top = 375
    Width = 75
    Height = 25
    Caption = 'Close'
    TabOrder = 1
    OnClick = SettingsCloseClick
  end
  object Popup: TPopupMenu
    Left = 138
    Top = 374
    object MenuAbout: TMenuItem
      AutoLineReduction = maManual
      Caption = 'About PSPdisp...'
      OnClick = MenuAboutClick
    end
    object MenuHelp: TMenuItem
      Caption = 'Help file...'
      OnClick = MenuHelpClick
    end
    object N6: TMenuItem
      AutoLineReduction = maManual
      Caption = '-'
    end
    object MenuPresets: TMenuItem
      AutoHotkeys = maManual
      AutoLineReduction = maManual
      Caption = 'Presets'
      GroupIndex = 20
      object N17: TMenuItem
        Caption = '-'
      end
      object MenuEditPresets: TMenuItem
        Caption = 'Edit presets...'
        OnClick = MenuEditPresetsClick
      end
    end
    object MenuOptions: TMenuItem
      AutoLineReduction = maManual
      Caption = 'Options'
      GroupIndex = 20
      object MenuUseCaptureBlt: TMenuItem
        Caption = 'Grab transparent windows'
        GroupIndex = 20
        OnClick = MenuUseCaptureBltClick
      end
      object MenuFastImageStretching: TMenuItem
        AutoCheck = True
        Caption = 'Fast image stretching'
        GroupIndex = 20
        OnClick = MenuFastImageStretchingClick
      end
      object MenuReduceCpuLoad: TMenuItem
        Caption = 'Reduce CPU load on idle'
        GroupIndex = 20
        OnClick = MenuReduceCpuLoadClick
      end
      object MenuCycleSideShowGadgets: TMenuItem
        Caption = 'Cycle SideShow gadgets'
        GroupIndex = 20
        OnClick = MenuCycleSideShowGadgetsClick
      end
      object MenuHigherPriority: TMenuItem
        AutoCheck = True
        Caption = 'Higher process priority'
        GroupIndex = 20
        OnClick = MenuHigherPriorityClick
      end
      object N7: TMenuItem
        Caption = '-'
        GroupIndex = 20
      end
      object MenuHideCurser: TMenuItem
        AutoCheck = True
        Caption = 'Automatically hide mouse cursor'
        GroupIndex = 20
        OnClick = OnModificationForConfig
      end
      object MenuScaleCursor: TMenuItem
        AutoCheck = True
        Caption = 'Scale mouse cursor'
        GroupIndex = 20
        OnClick = MenuScaleCursorClick
      end
      object N8: TMenuItem
        Caption = '-'
        GroupIndex = 20
      end
      object MenuMoreOptions: TMenuItem
        Caption = 'Set server message options...'
        GroupIndex = 20
        OnClick = MenuMoreOptionsClick
      end
    end
    object MenuControl: TMenuItem
      AutoHotkeys = maManual
      AutoLineReduction = maManual
      Caption = 'Control'
      GroupIndex = 20
      object MenuControlEnabled: TMenuItem
        AutoCheck = True
        Caption = 'Enabled'
        Visible = False
      end
      object N9: TMenuItem
        Caption = '-'
        Visible = False
      end
      object MenuControlOff: TMenuItem
        Tag = -1
        Caption = 'Off'
        Checked = True
        GroupIndex = 101
        RadioItem = True
        OnClick = SelectControlFile
      end
      object N16: TMenuItem
        Caption = '-'
        GroupIndex = 101
      end
      object MenuControlCustomize: TMenuItem
        Caption = 'Customize...'
        GroupIndex = 101
        OnClick = MenuControlCustomizeClick
      end
    end
    object Sound1: TMenuItem
      Caption = 'Sound'
      GroupIndex = 20
      object N18: TMenuItem
        Caption = '-'
      end
      object menuSoundWlan: TMenuItem
        Caption = 'Sample rate in WLAN mode '
        object MenuSoundWlan11: TMenuItem
          Caption = '11025 Hz'
          Checked = True
          GroupIndex = 111
          RadioItem = True
          OnClick = MenuSoundClick
        end
        object MenuSoundWlan22: TMenuItem
          Caption = '22050 Hz'
          GroupIndex = 111
          RadioItem = True
          OnClick = MenuSoundClick
        end
        object MenuSoundWlan44: TMenuItem
          Caption = '44100 Hz'
          GroupIndex = 111
          RadioItem = True
          OnClick = MenuSoundClick
        end
      end
      object menuSoundUsb: TMenuItem
        Caption = 'Sample rate in USB mode'
        object MenuSoundUsb11: TMenuItem
          Caption = '11025 Hz'
          GroupIndex = 222
          RadioItem = True
          OnClick = MenuSoundClick
        end
        object MenuSoundUsb22: TMenuItem
          Caption = '22050 Hz'
          Checked = True
          GroupIndex = 222
          RadioItem = True
          OnClick = MenuSoundClick
        end
        object MenuSoundUsb44: TMenuItem
          Caption = '44100 Hz'
          GroupIndex = 222
          RadioItem = True
          OnClick = MenuSoundClick
        end
      end
      object N10: TMenuItem
        Caption = '-'
      end
      object MenuOpenMixer: TMenuItem
        Caption = 'Mixer control panel...'
        OnClick = MenuOpenMixerClick
      end
      object Chooseinputdevice1: TMenuItem
        Caption = 'Choose input device...'
        OnClick = Chooseinputdevice1Click
      end
    end
    object MenuConnection: TMenuItem
      Caption = 'Connection'
      GroupIndex = 20
      object MenuUseUSB: TMenuItem
        AutoCheck = True
        Caption = 'Use only USB'
        GroupIndex = 20
        RadioItem = True
        OnClick = MenuUseWirelessLANClick
      end
      object MenuUseWirelessLAN: TMenuItem
        AutoCheck = True
        Caption = 'Use only Wireless LAN'
        GroupIndex = 20
        RadioItem = True
        OnClick = MenuUseWirelessLANClick
      end
      object MenuUseBoth: TMenuItem
        AutoCheck = True
        Caption = 'Use USB and Wireless LAN'
        Checked = True
        GroupIndex = 20
        RadioItem = True
        OnClick = MenuUseWirelessLANClick
      end
      object N5: TMenuItem
        Caption = '-'
        GroupIndex = 20
      end
      object MenuEnableOnStartup: TMenuItem
        Caption = 'Enable on startup'
        Checked = True
        GroupIndex = 21
        OnClick = MenuEnableOnStartupClick
      end
    end
    object MenuStreaming: TMenuItem
      Caption = 'Streaming'
      GroupIndex = 20
      object MenuSoundEnabled: TMenuItem
        Caption = 'Enable sound streaming'
        OnClick = MenuSoundEnabledClick
      end
      object MenuEnableImageStreaming: TMenuItem
        AutoCheck = True
        Caption = 'Enable video streaming'
        Checked = True
        GroupIndex = 20
        OnClick = MenuEnableImageStreamingClick
      end
    end
    object N22: TMenuItem
      Caption = '-'
      GroupIndex = 20
    end
    object MenuUpdate: TMenuItem
      AutoLineReduction = maManual
      Caption = 'Update interval'
      GroupIndex = 20
      object MenuUpdateContinuous: TMenuItem
        Caption = 'Continuous'
        Checked = True
        GroupIndex = 3
        RadioItem = True
        OnClick = MenuUpdateClick
      end
      object MenuUpdateSleep1: TMenuItem
        Tag = 30
        AutoCheck = True
        Caption = 'Sleep 30 ms'
        GroupIndex = 3
        RadioItem = True
        OnClick = MenuUpdateClick
      end
      object MenuUpdateSleep2: TMenuItem
        Tag = 40
        AutoCheck = True
        Caption = 'Sleep 40 ms'
        GroupIndex = 3
        RadioItem = True
        OnClick = MenuUpdateClick
      end
      object MenuUpdateSleep3: TMenuItem
        Tag = 50
        AutoCheck = True
        Caption = 'Sleep 50 ms'
        GroupIndex = 3
        RadioItem = True
        OnClick = MenuUpdateClick
      end
      object MenuUpdateSleep4: TMenuItem
        Tag = 100
        AutoCheck = True
        Caption = 'Sleep 100 ms'
        GroupIndex = 3
        RadioItem = True
        OnClick = MenuUpdateClick
      end
      object MenuUpdateSleep5: TMenuItem
        Tag = 200
        AutoCheck = True
        Caption = 'Sleep 200 ms'
        GroupIndex = 3
        RadioItem = True
        OnClick = MenuUpdateClick
      end
      object N13: TMenuItem
        Caption = '-'
        GroupIndex = 3
      end
      object MenuDelayCustomize: TMenuItem
        Caption = 'Customize...'
        GroupIndex = 3
        OnClick = MenuDelayCustomizeClick
      end
    end
    object MenuQuality: TMenuItem
      AutoLineReduction = maManual
      Caption = 'Quality'
      GroupIndex = 20
      object MenuQualityUncompressed: TMenuItem
        AutoCheck = True
        Caption = 'Uncompressed'
        Checked = True
        GroupIndex = 2
        OnClick = OnModificationForConfig
      end
      object N20: TMenuItem
        Caption = '-'
        GroupIndex = 2
      end
      object MenuQualityHighest: TMenuItem
        Tag = 80
        Caption = 'Highest'
        GroupIndex = 2
        RadioItem = True
        OnClick = MenuQualityClick
      end
      object MenuQualityHigh: TMenuItem
        Tag = 50
        Caption = 'High'
        GroupIndex = 2
        RadioItem = True
        OnClick = MenuQualityClick
      end
      object MenuQualityMedium: TMenuItem
        Tag = 30
        Caption = 'Medium'
        Checked = True
        GroupIndex = 2
        RadioItem = True
        OnClick = MenuQualityClick
      end
      object MenuQualityLow: TMenuItem
        Tag = 20
        Caption = 'Low'
        GroupIndex = 2
        RadioItem = True
        OnClick = MenuQualityClick
      end
      object MenuQualityLowest: TMenuItem
        Tag = 10
        Caption = 'Lowest'
        GroupIndex = 2
        RadioItem = True
        OnClick = MenuQualityClick
      end
      object MenuQualitySideShow: TMenuItem
        Tag = 95
        Caption = 'SideShow'
        GroupIndex = 2
        Visible = False
      end
      object N14: TMenuItem
        Caption = '-'
        GroupIndex = 2
      end
      object MenuQualityCustomize: TMenuItem
        Caption = 'Customize...'
        GroupIndex = 2
        OnClick = MenuQualityCustomizeClick
      end
    end
    object MenuRotation: TMenuItem
      Caption = 'Rotation'
      GroupIndex = 20
      object MenuRotationOff: TMenuItem
        Caption = 'Off'
        GroupIndex = 201
        RadioItem = True
        OnClick = MenuRotationOffClick
      end
      object MenuRotation90: TMenuItem
        Caption = '90'#176
        GroupIndex = 201
        RadioItem = True
        OnClick = MenuRotation90Click
      end
      object MenuRotation180: TMenuItem
        Caption = '180'#176
        GroupIndex = 201
        RadioItem = True
        OnClick = MenuRotationOffClick
      end
      object MenuRotation270: TMenuItem
        Caption = '270'#176
        GroupIndex = 201
        RadioItem = True
        OnClick = MenuRotation90Click
      end
    end
    object MenuViewportSize: TMenuItem
      Caption = 'Viewport size'
      GroupIndex = 20
      object N480x2721: TMenuItem
        Caption = '480x272'
        Checked = True
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N560x1: TMenuItem
        Caption = '560x315'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N640x3601: TMenuItem
        Caption = '640x360'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N720x4051: TMenuItem
        Caption = '720x405'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N800x1: TMenuItem
        Caption = '800x450'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N8801: TMenuItem
        Caption = '880x495'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N960x5441: TMenuItem
        Caption = '960x544'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N15: TMenuItem
        Caption = '-'
        GroupIndex = 100
      end
      object N320x2001: TMenuItem
        Caption = '320x200'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N320x2401: TMenuItem
        Caption = '320x240'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N640x4001: TMenuItem
        Caption = '640x400'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N640x4801: TMenuItem
        Caption = '640x480'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N800x6001: TMenuItem
        Caption = '800x600'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N21: TMenuItem
        Caption = '-'
        GroupIndex = 100
      end
      object N60x341: TMenuItem
        Caption = '060x034'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N120x681: TMenuItem
        Caption = '120x068'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
      object N240x1361: TMenuItem
        Caption = '240x136'
        GroupIndex = 100
        RadioItem = True
        OnClick = MenuViewPortSizeClick
      end
    end
    object MenuScreen: TMenuItem
      AutoLineReduction = maManual
      Caption = 'Screen mode'
      GroupIndex = 20
      object MenuFitScreen: TMenuItem
        Caption = 'Fit screen'
        GroupIndex = 5
        RadioItem = True
        OnClick = MenuScreenClick
      end
      object MenuFitScreen16to9: TMenuItem
        Caption = 'Fit screen (crop to 16:9)'
        GroupIndex = 5
        RadioItem = True
        OnClick = MenuScreenClick
      end
      object MenuFollowMouse: TMenuItem
        Caption = 'Follow mouse (mode 1)'
        GroupIndex = 5
        RadioItem = True
        OnClick = MenuScreenClick
      end
      object MenuFollowMouse2: TMenuItem
        Caption = 'Follow mouse (mode 2)'
        GroupIndex = 5
        RadioItem = True
        OnClick = MenuScreenClick
      end
      object MenuStaticViewport: TMenuItem
        Caption = 'Static viewport'
        GroupIndex = 5
        RadioItem = True
        OnClick = MenuScreenClick
      end
      object N1: TMenuItem
        Caption = '-'
        GroupIndex = 5
      end
      object MenuSetOrigin: TMenuItem
        Caption = 'Set screen origin'
        GroupIndex = 5
        OnClick = MenuSetOriginClick
      end
      object MenuResetOrigin: TMenuItem
        Caption = 'Reset origin to (0,0)'
        GroupIndex = 5
        OnClick = MenuResetOriginClick
      end
    end
    object MenuDisplay: TMenuItem
      AutoLineReduction = maManual
      Caption = 'Display device'
      GroupIndex = 20
      object MenuDesktopProperties: TMenuItem
        AutoLineReduction = maManual
        Caption = 'Display properties...'
        GroupIndex = 20
        OnClick = MenuDesktopPropertiesClick
      end
      object N11: TMenuItem
        AutoLineReduction = maManual
        Caption = '-'
        GroupIndex = 20
      end
      object MenuEnableDisplay: TMenuItem
        AutoLineReduction = maManual
        Caption = 'Enable PSPdisp display driver'
        GroupIndex = 20
        OnClick = MenuEnableDisplayClick
      end
      object MenuResolution: TMenuItem
        Caption = 'Change resolution (landscape)'
        GroupIndex = 20
      end
      object MenuResolutionRotated: TMenuItem
        Caption = 'Change resolution (portrait)'
        GroupIndex = 20
      end
      object MenuDisplayChangePosition: TMenuItem
        Caption = 'Change position...'
        GroupIndex = 20
        OnClick = MenuDisplayChangePositionClick
      end
      object N12: TMenuItem
        AutoLineReduction = maManual
        Caption = '-'
        GroupIndex = 20
      end
      object MenuUseSideshow: TMenuItem
        Caption = 'Use SideShow device'
        GroupIndex = 20
        OnClick = MenuUseSideshowClick
      end
      object MenuSideshowProperties: TMenuItem
        Caption = 'SideShow properties...'
        GroupIndex = 20
        OnClick = MenuSideshowPropertiesClick
      end
      object N4: TMenuItem
        Caption = '-'
        GroupIndex = 20
      end
      object MenuFollowMouseAcrossScreens: TMenuItem
        Caption = 'Autoselect display with mouse cursor'
        GroupIndex = 20
        OnClick = MenuFollowMouseAcrossScreensClick
      end
    end
    object N3: TMenuItem
      AutoLineReduction = maManual
      Caption = '-'
      GroupIndex = 20
    end
    object MenuEnabled: TMenuItem
      AutoLineReduction = maManual
      Caption = 'Enabled'
      Default = True
      GroupIndex = 20
      OnClick = MenuEnabledClick
    end
    object N2: TMenuItem
      AutoLineReduction = maManual
      Caption = '-'
      GroupIndex = 20
    end
    object MenuExit: TMenuItem
      AutoLineReduction = maManual
      Caption = 'Exit'
      GroupIndex = 20
      OnClick = MenuExitClick
    end
  end
  object ImageList: TImageList
    Left = 106
    Top = 374
    Bitmap = {
      494C010109000E00040010001000FFFFFFFFFF10FFFFFFFFFFFFFFFF424D3600
      0000000000003600000028000000400000004000000001002000000000000040
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000003939
      3900393939003939390000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000FF854800FF9D6C00FF9D6C00FFB59000FFB59000FFB59000FFB59000FFB5
      9000000000000000000039393900000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000393939000000
      0000FF9D6C00FF9D6C00FFB59000FFB59000F0CAA600F0CAA600F0CAA600F0CA
      A600000000003939390000000000393939000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000039393900000000003939
      3900FF9D6C00FFB59000FFB59000F0CAA600F0CAA600F0CAA600F0CAA600FFCD
      B400000000000000000000000000393939000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000393939000000
      0000FFB59000FFB59000F0CAA600F0CAA600F0CAA600F0CAA600FFCDB400FFCD
      B400000000003939390000000000393939000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000FFB59000F0CAA600F0CAA600F0CAA600F0CAA600FFCDB400FFCDB400FFCD
      B400000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000393939000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000003939390000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000043F18C00159C1200096E
      3400096E34000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000000000000000000043F1
      8C00096E3400096E340043F18C00000000000000000043F18C0043F18C00159C
      1200096E34000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000043F18C0043F18C000000000000000000000000000000000043F18C00159C
      1200000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000159C1200000000000000
      000000000000000000000000000000000000159C1200000000000000000043F1
      8C00159C12000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000043F18C00159C1200159C
      1200096E3400096E3400159C1200159C120043F18C00000000000000000043F1
      8C00159C12000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000043F18C0043F1
      8C0043F18C0043F18C0043F18C0043F18C000000000000000000000000000000
      000043F18C00159C120000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000043F18C00159C1200000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000043F18C00159C12000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000003939
      3900393939003939390000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000003939
      3900393939003939390000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000003939
      3900393939003939390000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000003939
      3900393939003939390000000000000000000000000000000000000000000000
      00005F5F5F006C6C6C006C6C6C00787878007878780078787800787878007878
      7800000000000000000039393900000000000000000000000000000000000000
      00005F5F5F006C6C6C006C6C6C00787878007878780078787800787878007878
      7800000000000000000039393900000000000000000000000000000000000000
      00005F5F5F006C6C6C006C6C6C00787878007878780078787800787878007878
      7800000000000000000039393900000000000000000000000000000000000000
      0000FF854800FF9D6C00FF9D6C00FFB59000FFB59000FFB59000FFB59000FFB5
      9000000000000000000039393900000000000000000000000000393939000000
      00006C6C6C006C6C6C0085858500858585009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      00006C6C6C006C6C6C0085858500858585009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      00006C6C6C006C6C6C0085858500858585009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      0000FF9D6C00FF9D6C00FFB59000FFB59000F0CAA600F0CAA600F0CAA600F0CA
      A600000000003939390000000000393939000000000039393900000000003939
      39006C6C6C008585850092929200929292009292920092929200929292009292
      9200000000000000000000000000393939000000000039393900000000003939
      39006C6C6C008585850092929200929292009292920092929200929292009292
      9200000000000000000000000000393939000000000039393900000000003939
      39006C6C6C008585850092929200929292009292920092929200929292009292
      9200000000000000000000000000393939000000000039393900000000003939
      3900FF9D6C00FFB59000FFB59000F0CAA600F0CAA600F0CAA600F0CAA600FFCD
      B400000000000000000000000000393939000000000000000000393939000000
      0000787878008585850092929200929292009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      0000787878008585850092929200929292009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      0000787878008585850092929200929292009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      0000FFB59000FFB59000F0CAA600F0CAA600F0CAA600F0CAA600FFCDB400FFCD
      B400000000003939390000000000393939000000000000000000000000000000
      0000787878009292920092929200929292009292920092929200929292009292
      9200000000000000000000000000000000000000000000000000000000000000
      0000787878009292920092929200929292009292920092929200929292009292
      9200000000000000000000000000000000000000000000000000000000000000
      0000787878009292920092929200929292009292920092929200929292009292
      9200000000000000000000000000000000000000000000000000000000000000
      0000FFB59000F0CAA600F0CAA600F0CAA600F0CAA600FFCDB400FFCDB400FFCD
      B400000000000000000000000000000000000000000000000000393939000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000003939390000000000000000000000000000000000393939000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000003939390000000000000000000000000000000000393939000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000003939390000000000000000000000000000000000393939000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000003939390000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000D2D7D700999999006464
      6400646464000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000043F18C00159C1200096E
      3400096E34000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000000000000D2D7
      D7006464640064646400D2D7D7000000000000000000D2D7D700D2D7D7009999
      99006464640000000000000000000000000000000000000000000000000043F1
      8C00096E3400096E340043F18C00000000000000000043F18C0043F18C00159C
      1200096E34000000000000000000000000000000000000000000000000000000
      0000000000000000000043F18C00096E3400096E340043F18C00000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000043F18C00096E3400096E340043F18C00000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000D2D7D700D2D7D70000000000000000000000000000000000D2D7D7009999
      9900000000000000000000000000000000000000000000000000000000000000
      000043F18C0043F18C000000000000000000000000000000000043F18C00159C
      1200000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000043F18C0043F18C0000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000043F18C0043F18C0000000000000000000000
      0000000000000000000000000000000000000000000064646400000000000000
      000000000000000000000000000000000000646464000000000000000000D2D7
      D7009999990000000000000000000000000000000000159C1200000000000000
      000000000000000000000000000000000000159C1200000000000000000043F1
      8C00159C12000000000000000000000000000000000000000000000000000000
      0000159C1200000000000000000000000000000000000000000000000000159C
      1200000000000000000000000000000000000000000000000000000000000000
      0000159C1200000000000000000000000000000000000000000000000000159C
      12000000000000000000000000000000000000000000D2D7D700646464006464
      640058646300586463006464640064646400D2D7D7000000000000000000D2D7
      D700999999000000000000000000000000000000000043F18C00159C1200159C
      1200096E3400096E3400159C1200159C120043F18C00000000000000000043F1
      8C00159C12000000000000000000000000000000000000000000000000000000
      000043F18C00159C1200159C1200096E3400096E3400159C1200159C120043F1
      8C00000000000000000000000000000000000000000000000000000000000000
      000043F18C00159C1200159C1200096E3400096E3400159C1200159C120043F1
      8C00000000000000000000000000000000000000000000000000D2D7D700D2D7
      D7009999990099999900D2D7D700D2D7D7000000000000000000000000000000
      0000D2D7D700999999000000000000000000000000000000000043F18C0043F1
      8C0043F18C0043F18C0043F18C0043F18C000000000000000000000000000000
      000043F18C00159C120000000000000000000000000000000000000000000000
      00000000000043F18C0043F18C0043F18C0043F18C0043F18C0043F18C000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000043F18C0043F18C0043F18C0043F18C0043F18C0043F18C000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000D2D7D70099999900000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000043F18C00159C1200000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000D2D7D700999999000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000043F18C00159C12000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000003939
      3900393939003939390000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000003939
      3900393939003939390000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000003939
      3900393939003939390000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00005F5F5F006C6C6C006C6C6C00787878007878780078787800787878007878
      7800000000000000000039393900000000000000000000000000000000000000
      00005F5F5F006C6C6C006C6C6C00787878007878780078787800787878007878
      7800000000000000000039393900000000000000000000000000000000000000
      0000FF854800FF9D6C00FF9D6C00FFB59000FFB59000FFB59000FFB59000FFB5
      9000000000000000000039393900000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000003939
      3900393939003939390000000000000000000000000000000000393939000000
      00006C6C6C006C6C6C0085858500858585009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      00006C6C6C006C6C6C0085858500858585009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      0000FF9D6C00FF9D6C00FFB59000FFB59000F0CAA600F0CAA600F0CAA600F0CA
      A600000000003939390000000000393939000000000000000000000000000000
      00005F5F5F006C6C6C006C6C6C00787878007878780078787800787878007878
      7800000000000000000039393900000000000000000039393900000000003939
      39006C6C6C008585850092929200929292009292920092929200929292009292
      9200000000000000000000000000393939000000000039393900000000003939
      39006C6C6C008585850092929200929292009292920092929200929292009292
      9200000000000000000000000000393939000000000039393900000000003939
      3900FF9D6C00FFB59000FFB59000F0CAA600F0CAA600F0CAA600F0CAA600FFCD
      B400000000000000000000000000393939000000000000000000393939000000
      00006C6C6C006C6C6C0085858500858585009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      0000787878008585850092929200929292009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      0000787878008585850092929200929292009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      0000FFB59000FFB59000F0CAA600F0CAA600F0CAA600F0CAA600FFCDB400FFCD
      B400000000003939390000000000393939000000000039393900000000003939
      39006C6C6C008585850092929200929292009292920092929200929292009292
      9200000000000000000000000000393939000000000000000000000000000000
      0000787878009292920092929200929292009292920092929200929292009292
      9200000000000000000000000000000000000000000000000000000000000000
      0000787878009292920092929200929292009292920092929200929292009292
      9200000000000000000000000000000000000000000000000000000000000000
      0000FFB59000F0CAA600F0CAA600F0CAA600F0CAA600FFCDB400FFCDB400FFCD
      B400000000000000000000000000000000000000000000000000393939000000
      0000787878008585850092929200929292009292920092929200929292009292
      9200000000003939390000000000393939000000000000000000393939000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000003939390000000000000000000000000000000000393939000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000003939390000000000000000000000000000000000393939000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000003939390000000000000000000000000000000000000000000000
      0000787878009292920092929200929292009292920092929200929292009292
      9200000000000000000000000000000000000000000000000000000000000000
      00000000000000000000D2D7D700999999006464640064646400000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000043F18C00159C1200096E3400096E3400000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000043F18C00159C1200096E3400096E3400000000000000
      0000000000000000000000000000000000000000000000000000393939000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000003939390000000000000000000000000000000000000000000000
      00000000000000000000D2D7D700D2D7D7009999990064646400000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000043F18C0043F18C00159C1200096E3400000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000043F18C0043F18C00159C1200096E3400000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000D2D7D7009999990000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000043F18C00159C120000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000043F18C00159C120000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000D2D7D7005864630058646300D2D7D700000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000D2D7D70099999900000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000043F18C00159C1200000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000043F18C00159C1200000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000D2D7D700D2D7D70000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000D2D7D70099999900000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000043F18C00159C1200000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000043F18C00159C1200000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00009BA7A6000000000000000000000000000000000000000000000000009BA7
      A600000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000D2D7D700999999000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000043F18C00159C12000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000043F18C00159C12000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000D2D7D7009BA7A6009BA7A60058646300586463009BA7A6009BA7A600D2D7
      D700000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000D2D7D7009999
      9900000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000043F18C00159C
      1200000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000043F18C00159C
      1200000000000000000000000000000000000000000000000000000000000000
      000000000000D2D7D700D2D7D700D2D7D700D2D7D700D2D7D700D2D7D7000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000000000000D2D7
      D700999999000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000000000000000000043F1
      8C00159C12000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000000000000000000043F1
      8C00159C12000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000424D3E000000000000003E000000
      2800000040000000400000000100010000000000000200000000000000000000
      000000000000000000000000FFFFFF0000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000FFFF000000000000C003000000000000
      8001000000000000000000000000000000000000000000000000000000000000
      8001000000000000C003000000000000FF87000000000000E187000000000000
      F3CF000000000000BF670000000000008067000000000000C0F3000000000000
      FFF9000000000000FFFC000000000000FFFFFFFFFFFFFFFFFFFFFFFFC003C003
      C003C00380018001800180010000000000000000000000000000000000000000
      000000008001800180018001C003C003C003C003FF87FF87FFFFFFFFE187E187
      FC3FFC3FF3CFF3CFFE7FFE7FBF67BF67F7EFF7EF80678067F00FF00FC0F3C0F3
      F81FF81FFFF9FFF9FFFFFFFFFFFCFFFCFFFFFFFFFFFFFFFFC003C003C003FFFF
      800180018001C003000000000000800100000000000000000000000000000000
      8001800180010000C003C003C0038001FC3FFC3FFC3FC003FC3FFC3FFC3FFFFF
      FE7FFE7FFE7FFC3FFF3FFF3FFF3FFE7FFF3FFF3FFF3FF7EFFF9FFF9FFF9FF00F
      FFCFFFCFFFCFF81FFFE7FFE7FFE7FFFF00000000000000000000000000000000
      000000000000}
  end
  object Timer: TTimer
    Enabled = False
    Interval = 200
    OnTimer = TimerTimer
    Left = 74
    Top = 374
  end
  object FpsTimer: TTimer
    Enabled = False
    OnTimer = FpsTimerTimer
    Left = 42
    Top = 374
  end
  object XPManifest1: TXPManifest
    Left = 10
    Top = 374
  end
  object ConfigTimer: TTimer
    Enabled = False
    Interval = 60000
    OnTimer = ConfigTimerTimer
    Left = 170
    Top = 374
  end
  object SaveDialog: TSaveDialog
    DefaultExt = 'preset'
    Filter = 'PSPdisp presets|*.preset'
    Options = [ofOverwritePrompt, ofHideReadOnly, ofNoChangeDir, ofPathMustExist, ofNoReadOnlyReturn, ofNoNetworkButton, ofEnableSizing, ofDontAddToRecent]
    Left = 203
    Top = 374
  end
end
