/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  shared.h - variables that must be known to
             both the PSP and the PC application

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef SHARED_H
#define SHARED_H


#define COM_HEADER_MAGIC 0xAFFE0600

#define COM_FLAGS_CONTAINS_IMAGE_DATA       0x00000001
#define COM_FLAGS_CONTAINS_AUDIO_DATA       0x00000002
#define COM_FLAGS_CONTAINS_SETTINGS_DATA    0x00000004

#define COM_FLAGS_ENABLE_OSK                0x00000008

#define COM_FLAGS_IMAGE_IS_JPEG             0x00000010
#define COM_FLAGS_IMAGE_IS_PNG              0x00000020
#define COM_FLAGS_IMAGE_IS_UNCOMPRESSED     0x00000040

#define COM_FLAGS_IMAGE_IS_ROTATED_90_DEG   0x00000100
#define COM_FLAGS_IMAGE_IS_ROTATED_180_DEG  0x00000200
#define COM_FLAGS_IMAGE_IS_ROTATED_270_DEG  0x00000400

#define COM_FLAGS_IMAGE_ROTATION_MASK       0x00000700

#define COM_FLAGS_AUDIO_11025_HZ            0x00001000
#define COM_FLAGS_AUDIO_22050_HZ            0x00002000
#define COM_FLAGS_AUDIO_44100_HZ            0x00004000

#define COM_FLAGS_AUDIO_SAMPLE_RATE_MASK    0x00037000

#define COM_FLAGS_AUDIO_CHUNK_2240          0x00010000
#define COM_FLAGS_AUDIO_CHUNK_2688          0x00020000

#define COM_FLAGS_IMAGE_CLEAR_SCREEN        0x00100000

#define COM_FLAGS_FORCE_UPDATE              0x01000000

#define COM_MAX_PC_CONTROLS 20
#define COM_MAX_PC_DEVICES 10
#define COM_MAX_PC_PRESETS 20


typedef struct
{
  unsigned int deviceIndex;
  unsigned int deviceUseSideShow;
  unsigned int screenMode;
  unsigned int screenRotation;
  unsigned int screenViewport;
  unsigned int quality;
  unsigned int screenNoCompression;
  unsigned int updateInterval;
  unsigned int soundEnabled;
  unsigned int soundSampleRate;
  unsigned int controlIndex;
  unsigned int controlCount;
  unsigned int displayDeviceIndex;
  unsigned int displayDeviceCount;
  unsigned int displayEnable;
  unsigned int displayAutoselect;
  unsigned int presetIndex;
  unsigned int presetCount;
  unsigned int presetDoSet;
}
__attribute__((__packed__)) comSettings;



typedef struct
{
  unsigned int magic;
  unsigned int flags;
  unsigned int imageSize;
  unsigned int settingsSize;
}
__attribute__((__packed__)) comFrameHeader;



typedef struct
{
  unsigned int magic;
  unsigned int flags;
  unsigned int buttons;
  unsigned char analogX;
  unsigned char analogY;
} 
__attribute__((__packed__)) comResponseHeader;



typedef struct
{
  comResponseHeader response;
  comSettings settings;
}
__attribute__((__packed__)) comSettingsResponse;



typedef struct
{
  comSettings settings;
  char devices[COM_MAX_PC_DEVICES][32];
  char controls[COM_MAX_PC_CONTROLS][32];
  char presets[COM_MAX_PC_PRESETS][32];
}
__attribute__((__packed__)) comSettingsReceived;



#endif