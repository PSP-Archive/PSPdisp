/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * shared.old.h - Current desktop and PSP protocol   *
 *                                                   *
 *  This file shall not define symbols.              *
 *                                                   *
 * Copyright © 2008-2015 Jochen Schleu               *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <stdint.h>

// All the types of this file are packed and little-endian whatever the host, so the PSP and the desktop share the same layout.
// Both pragmas are reset at the end of this file, so they don't leak into the files that include it.
#pragma pack(push, 1)
#pragma scalar_storage_order little-endian

/*
 * Communication protocol between the desktop and the PSP, over USB or WLAN.
 *
 * For each frame, the desktop sends a `PSPdispComFrameHeader` followed by its data (see its description), and the PSP answers with a `PSPdispComResponseHeader` or a `PSPdispComSettingsResponse`.
 * The settings are the desktop options that the PSP shows and changes on its "PC Settings" menu.
 *
 * DEPRECATED: every definition of this file is going to be replaced soon by a simpler protocol, easier to maintain, so don't build anything new on them.
 * Many of them are likely to go, e.g. `PSPDISP_COM_FLAG_IMAGE_IS_PNG` (the desktop never sends PNG images) or the `PSPDISP_COM_FLAG_IMAGE_IS_ROTATED_*` flags (the desktop never rotates the images, the display settings of the computer rotate the virtual display).
 */

#define PSPDISP_COM_HEADER_MAGIC 0xAFFE0600 // First value of every frame and response header, to recognize them

#define PSPDISP_COM_MAX_DISPLAY_DEVICES 10 // Max display device names the PSP keeps (see `PSPdispComSettingsReceived.devices`)
#define PSPDISP_COM_MAX_CONTROLS        20 // Max control file names the PSP keeps (see `PSPdispComSettingsReceived.controls`)
#define PSPDISP_COM_MAX_PRESETS         20 // Max preset names the PSP keeps (see `PSPdispComSettingsReceived.presets`)
#define PSPDISP_COM_NAME_SIZE           32 // Size in bytes of each display device, control file and preset name, null-terminated or padded with zeros

/**
 * @brief Flags of a frame header (desktop to PSP) or of a response header (PSP to desktop), combined with bitwise OR.
 *
 * Each flag tells which data follows the header, or how to handle it.
 * Only `PSPDISP_COM_FLAG_CONTAINS_SETTINGS_DATA` and `PSPDISP_COM_FLAG_FORCE_UPDATE` are used in a response header.
 */
typedef enum: uint32_t {
    PSPDISP_COM_FLAG_CONTAINS_IMAGE_DATA      = 0b00000000000000000000000000000001, // Frame: an image follows the header (`image_size` bytes)
    PSPDISP_COM_FLAG_CONTAINS_AUDIO_DATA      = 0b00000000000000000000000000000010, // Frame: an audio chunk follows the image (see `PSPDISP_COM_FLAG_AUDIO_CHUNK_2240` and `PSPDISP_COM_FLAG_AUDIO_CHUNK_2688`)
    PSPDISP_COM_FLAG_CONTAINS_SETTINGS_DATA   = 0b00000000000000000000000000000100, // Frame: the settings follow the audio (`settings_size` bytes), response: it's a `PSPdispComSettingsResponse` with settings changed on the PSP
    PSPDISP_COM_FLAG_ENABLE_OSK               = 0b00000000000000000000000000001000, // Frame: the PSP may open its on-screen keyboard (Danzeff) with the Select button
    PSPDISP_COM_FLAG_IMAGE_IS_JPEG            = 0b00000000000000000000000000010000, // Frame: the image is a JPEG
    PSPDISP_COM_FLAG_IMAGE_IS_PNG             = 0b00000000000000000000000000100000, // Frame: the image is a PNG (the PSP decodes it, but the desktop never sends one)
    PSPDISP_COM_FLAG_IMAGE_IS_UNCOMPRESSED    = 0b00000000000000000000000001000000, // Frame: the image is uncompressed, 24 bpp in B,G,R order with the rows bottom-up (a Windows bitmap without its headers)
    PSPDISP_COM_FLAG_IMAGE_IS_ROTATED_90_DEG  = 0b00000000000000000000000100000000, // Frame: the image is 272x480 and the PSP draws it rotated by 90 degrees
    PSPDISP_COM_FLAG_IMAGE_IS_ROTATED_180_DEG = 0b00000000000000000000001000000000, // Frame: the image is 480x272 and the PSP draws it rotated by 180 degrees
    PSPDISP_COM_FLAG_IMAGE_IS_ROTATED_270_DEG = 0b00000000000000000000010000000000, // Frame: the image is 272x480 and the PSP draws it rotated by 270 degrees
    PSPDISP_COM_FLAG_IMAGE_ROTATION_MASK      = 0b00000000000000000000011100000000, // Mask of the 3 rotation flags, without any the image is 480x272 and not rotated
    PSPDISP_COM_FLAG_AUDIO_11025_HZ           = 0b00000000000000000001000000000000, // Frame: the audio sample rate is 11025 Hz
    PSPDISP_COM_FLAG_AUDIO_22050_HZ           = 0b00000000000000000010000000000000, // Frame: the audio sample rate is 22050 Hz
    PSPDISP_COM_FLAG_AUDIO_44100_HZ           = 0b00000000000000000100000000000000, // Frame: the audio sample rate is 44100 Hz
    PSPDISP_COM_FLAG_AUDIO_CHUNK_2240         = 0b00000000000000010000000000000000, // Frame: the audio chunk is 2240 samples of 16 bits, stereo (4480 bytes, used at 11025 and 22050 Hz)
    PSPDISP_COM_FLAG_AUDIO_CHUNK_2688         = 0b00000000000000100000000000000000, // Frame: the audio chunk is 2688 samples of 16 bits, stereo (5376 bytes, used at 44100 Hz)
    PSPDISP_COM_FLAG_AUDIO_SAMPLE_RATE_MASK   = 0b00000000000000110111000000000000, // Mask of the 3 sample rate flags and the 2 chunk flags, the PSP restarts its audio when they change
    PSPDISP_COM_FLAG_IMAGE_CLEAR_SCREEN       = 0b00000000000100000000000000000000, // Frame: no image, the PSP clears its screen (the display streaming is disabled)
    PSPDISP_COM_FLAG_FORCE_UPDATE             = 0b00000001000000000000000000000000, // Response: the PSP asks for a new image even if the desktop didn't change (e.g. after closing its menu)
} PSPdispComFlags;

/**
 * @brief How the desktop picks the area of its display that is sent to the PSP (`PSPdispComSettings.screen_mode`).
 */
typedef enum: uint32_t {
    PSPDISP_COM_SCREEN_MODE_FIT_SCREEN      = 0, // The whole display, scaled to the PSP screen
    PSPDISP_COM_SCREEN_MODE_FIT_SCREEN_16_9 = 1, // The whole display cropped to 16:9 (centered vertically), scaled to the PSP screen
    PSPDISP_COM_SCREEN_MODE_FOLLOW_MOUSE_1  = 2, // A viewport that moves across the display in proportion to the mouse position
    PSPDISP_COM_SCREEN_MODE_FOLLOW_MOUSE_2  = 3, // A viewport centered on the mouse, stopping at the display edges
    PSPDISP_COM_SCREEN_MODE_STATIC_VIEWPORT = 4, // A viewport at a fixed origin, set on the desktop ("Set screen origin")
} PSPdispComScreenMode;

/**
 * @brief Rotation of the image sent to the PSP (`PSPdispComSettings.screen_rotation`), see the `PSPDISP_COM_FLAG_IMAGE_IS_ROTATED_*` flags.
 */
typedef enum: uint32_t {
    PSPDISP_COM_ROTATION_0   = 0, // Not rotated
    PSPDISP_COM_ROTATION_90  = 1, // Rotated by 90 degrees
    PSPDISP_COM_ROTATION_180 = 2, // Rotated by 180 degrees
    PSPDISP_COM_ROTATION_270 = 3, // Rotated by 270 degrees
} PSPdispComRotation;

/**
 * @brief Size in desktop pixels of the viewport sent to the PSP (`PSPdispComSettings.screen_viewport`), scaled to the PSP screen.
 *
 * Only used by the viewport screen modes (`PSPDISP_COM_SCREEN_MODE_FOLLOW_MOUSE_1`, `_2` and `PSPDISP_COM_SCREEN_MODE_STATIC_VIEWPORT`).
 * The first sizes are 16:9 like the PSP, the next ones are classic desktop sizes, and the last ones zoom in (1/8, 1/4 and 1/2 of the PSP screen).
 */
typedef enum: uint32_t {
    PSPDISP_COM_VIEWPORT_480X272 = 0,  // 480x272, the PSP screen size (not scaled)
    PSPDISP_COM_VIEWPORT_560X315 = 1,  // 560x315
    PSPDISP_COM_VIEWPORT_640X360 = 2,  // 640x360
    PSPDISP_COM_VIEWPORT_720X405 = 3,  // 720x405
    PSPDISP_COM_VIEWPORT_800X450 = 4,  // 800x450
    PSPDISP_COM_VIEWPORT_880X495 = 5,  // 880x495
    PSPDISP_COM_VIEWPORT_960X544 = 6,  // 960x544, twice the PSP screen size
    PSPDISP_COM_VIEWPORT_320X200 = 7,  // 320x200
    PSPDISP_COM_VIEWPORT_320X240 = 8,  // 320x240
    PSPDISP_COM_VIEWPORT_640X400 = 9,  // 640x400
    PSPDISP_COM_VIEWPORT_640X480 = 10, // 640x480
    PSPDISP_COM_VIEWPORT_800X600 = 11, // 800x600
    PSPDISP_COM_VIEWPORT_60X34   = 12, // 60x34, 1/8 of the PSP screen size
    PSPDISP_COM_VIEWPORT_120X68  = 13, // 120x68, 1/4 of the PSP screen size
    PSPDISP_COM_VIEWPORT_240X136 = 14, // 240x136, 1/2 of the PSP screen size
} PSPdispComViewport;

/**
 * @brief JPEG quality of the images sent to the PSP (`PSPdispComSettings.quality`), the higher the bigger the frames.
 */
typedef enum: uint32_t {
    PSPDISP_COM_QUALITY_LOWEST  = 0, // JPEG quality 10
    PSPDISP_COM_QUALITY_LOW     = 1, // JPEG quality 20
    PSPDISP_COM_QUALITY_MEDIUM  = 2, // JPEG quality 30
    PSPDISP_COM_QUALITY_HIGH    = 3, // JPEG quality 50
    PSPDISP_COM_QUALITY_HIGHEST = 4, // JPEG quality 80
} PSPdispComQuality;

/**
 * @brief Pause of the desktop after sending each frame (`PSPdispComSettings.update_interval`), to lower the frame rate and the load.
 */
typedef enum: uint32_t {
    PSPDISP_COM_UPDATE_INTERVAL_CONTINUOUS = 0, // No pause, the next frame is sent right away
    PSPDISP_COM_UPDATE_INTERVAL_SHORTEST   = 1, // 30 ms
    PSPDISP_COM_UPDATE_INTERVAL_SHORT      = 2, // 40 ms
    PSPDISP_COM_UPDATE_INTERVAL_MEDIUM     = 3, // 50 ms
    PSPDISP_COM_UPDATE_INTERVAL_LONG       = 4, // 100 ms
    PSPDISP_COM_UPDATE_INTERVAL_LONGEST    = 5, // 200 ms
} PSPdispComUpdateInterval;

/**
 * @brief Sample rate of the sound streamed to the PSP (`PSPdispComSettings.sound_sample_rate`), see the `PSPDISP_COM_FLAG_AUDIO_*_HZ` flags.
 */
typedef enum: uint32_t {
    PSPDISP_COM_SAMPLE_RATE_11025 = 0, // 11025 Hz
    PSPDISP_COM_SAMPLE_RATE_22050 = 1, // 22050 Hz
    PSPDISP_COM_SAMPLE_RATE_44100 = 2, // 44100 Hz
} PSPdispComSampleRate;

/**
 * @brief Desktop settings shown and changed on the "PC Settings" menu of the PSP.
 *
 * The desktop sends them in a frame, after the audio (see `PSPdispComFrameHeader`), and the PSP sends them back changed in a `PSPdispComSettingsResponse`.
 * The flags are 0 (off) or 1 (on), the `*_index` fields select an entry of the names in `PSPdispComSettingsReceived`, and the `*_count` fields tell how many there are.
 */
typedef struct {
    uint32_t device_index;                    // Unused, always 0
    uint32_t device_use_sideshow;             // Flag: send the Windows SideShow gadgets instead of the display ("SideShow mode")
    PSPdispComScreenMode screen_mode;         // Area of the display sent to the PSP
    PSPdispComRotation screen_rotation;       // Rotation of the image
    PSPdispComViewport screen_viewport;       // Size of the viewport, for the viewport screen modes
    PSPdispComQuality quality;                // JPEG quality of the image
    uint32_t screen_no_compression;           // Flag: send the images uncompressed (`PSPDISP_COM_FLAG_IMAGE_IS_UNCOMPRESSED`) instead of JPEG
    PSPdispComUpdateInterval update_interval; // Pause after each frame
    uint32_t sound_enabled;                   // Flag: stream the desktop sound
    PSPdispComSampleRate sound_sample_rate;   // Sample rate of the sound
    uint32_t control_index;                   // Control file in use (how the PSP buttons drive the desktop), in `PSPdispComSettingsReceived.controls`
    uint32_t control_count;                   // Number of control files, at most `PSPDISP_COM_MAX_CONTROLS`
    uint32_t display_device_index;            // Display device (monitor) streamed, in `PSPdispComSettingsReceived.devices`
    uint32_t display_device_count;            // Number of display devices, at most `PSPDISP_COM_MAX_DISPLAY_DEVICES`
    uint32_t display_enable;                  // Flag: stream the display, otherwise the frames only clear the PSP screen (`PSPDISP_COM_FLAG_IMAGE_CLEAR_SCREEN`)
    uint32_t display_autoselect;              // Flag: stream the display device the mouse is on, switching when it moves to another one
    uint32_t preset_index;                    // Preset (a saved set of these settings), in `PSPdispComSettingsReceived.presets`
    uint32_t preset_count;                    // Number of presets, at most `PSPDISP_COM_MAX_PRESETS`
    uint32_t preset_do_set;                   // Flag: the PSP asks the desktop to apply the preset `preset_index`
} PSPdispComSettings;

static_assert(sizeof(PSPdispComSettings) == 76, "The settings must be 19 values of 4 bytes");

/**
 * @brief Header of a frame, sent by the desktop to the PSP before its data.
 *
 * The data follows it in this order, each part only with its flag in `flags`:
 *   1. The image, `image_size` bytes (`PSPDISP_COM_FLAG_CONTAINS_IMAGE_DATA`), in the format of the `PSPDISP_COM_FLAG_IMAGE_IS_*` flags.
 *   2. The audio chunk, 4480 or 5376 bytes (`PSPDISP_COM_FLAG_CONTAINS_AUDIO_DATA`), as told by the `PSPDISP_COM_FLAG_AUDIO_CHUNK_*` flags.
 *   3. The settings, `settings_size` bytes (`PSPDISP_COM_FLAG_CONTAINS_SETTINGS_DATA`): a `PSPdispComSettings`, then the display device names, the control file names and the preset names, `PSPDISP_COM_NAME_SIZE` bytes each and as many as their `*_count` fields.
 */
typedef struct {
    uint32_t magic;         // Always `PSPDISP_COM_HEADER_MAGIC`
    uint32_t flags;         // Combination of `PSPdispComFlags`
    uint32_t image_size;    // Size in bytes of the image
    uint32_t settings_size; // Size in bytes of the settings, with their names
} PSPdispComFrameHeader;

static_assert(sizeof(PSPdispComFrameHeader) == 16, "A frame header must be 16 bytes");

/**
 * @brief Header of a response, sent by the PSP to the desktop after each frame, with the state of its controls.
 *
 * It's sent alone, or as part of a `PSPdispComSettingsResponse`.
 */
typedef struct {
    uint32_t magic;   // Always `PSPDISP_COM_HEADER_MAGIC`
    uint32_t flags;   // Combination of `PSPdispComFlags`: `PSPDISP_COM_FLAG_CONTAINS_SETTINGS_DATA` and `PSPDISP_COM_FLAG_FORCE_UPDATE`
    uint32_t buttons; // Pressed buttons, the `PSP_CTRL_*` bits of `SceCtrlData.Buttons` from `pspctrl.h` (none while the PSP menu is open), or only the character typed on its on-screen keyboard in the highest 8 bits while it's open (255 for none)
    uint8_t analog_x; // Analog stick position from the left (0) to the right (255), `SceCtrlData.Lx` (127 while the PSP menu is open)
    uint8_t analog_y; // Analog stick position from the top (0) to the bottom (255), `SceCtrlData.Ly` (127 while the PSP menu is open)
} PSPdispComResponseHeader;

static_assert(sizeof(PSPdispComResponseHeader) == 14, "A response header must be 14 bytes");

/**
 * @brief Response with the settings of the PSP, instead of a plain `PSPdispComResponseHeader`.
 *
 * In USB mode it's sent after every frame, in WLAN mode only when the settings changed on the PSP.
 * The settings changed when `PSPDISP_COM_FLAG_CONTAINS_SETTINGS_DATA` is in the flags of the response.
 */
typedef struct {
    PSPdispComResponseHeader response; // The response header
    PSPdispComSettings settings;       // The settings, as on the PSP
} PSPdispComSettingsResponse;

static_assert(sizeof(PSPdispComSettingsResponse) == 14 + 76, "A settings response must be a response header and the settings");

/**
 * @brief Settings received by the PSP, with the names of their entries.
 *
 * It's how the PSP keeps them, not how they are sent: in a frame, each names array only holds as many names as its `*_count` field (see `PSPdispComFrameHeader`).
 */
typedef struct {
    PSPdispComSettings settings;                                          // The settings
    char devices[PSPDISP_COM_MAX_DISPLAY_DEVICES][PSPDISP_COM_NAME_SIZE]; // Names of the display devices, `settings.display_device_count` of them
    char controls[PSPDISP_COM_MAX_CONTROLS][PSPDISP_COM_NAME_SIZE];       // Names of the control files, `settings.control_count` of them
    char presets[PSPDISP_COM_MAX_PRESETS][PSPDISP_COM_NAME_SIZE];         // Names of the presets, `settings.preset_count` of them
} PSPdispComSettingsReceived;

static_assert(sizeof(PSPdispComSettingsReceived) == 76 + (10 + 20 + 20) * 32, "The received settings must be the settings and all their names");

#pragma scalar_storage_order default
#pragma pack(pop)
