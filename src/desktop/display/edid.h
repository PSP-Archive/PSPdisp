/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * edid.h - Display identification data (EDID) types *
 *                                                   *
 *  This file shall not define symbols.              *
 *                                                   *
 * Copyright © 2026 Net-0                            *
 *****************************************************/

#pragma once

#include <stdint.h>

// All the types of this file are packed, to match the EDID bytes.
// Their bit-fields are declared from bit 7 to bit 0 whatever the host, thanks to the big-endian scalar storage order.
// Both pragmas are reset at the end of this file, so they don't leak into the files that include it.
#pragma pack(push, 1)
#pragma scalar_storage_order big-endian

/**
 * @brief PNP ID: the 3-letter code of a manufacturer (e.g. "SAM"), assigned by the PNP ID registry of the UEFI Forum.
 *
 * Each letter is 5 bits from 'A' = 1 to 'Z' = 26, so `letter = 'A' + value - 1` and `value = letter - 'A' + 1`.
 *
 * It's big-endian as stored in the EDID, whatever the host endianness.
 */
typedef struct {
    uint16_t reserved : 1; // bit 15: Always 0
    uint16_t first    : 5; // bits 14-10: First letter
    uint16_t second   : 5; // bits 9-5: Second letter
    uint16_t third    : 5; // bits 4-0: Third letter
} PSPdispPNPID;

static_assert(sizeof(PSPdispPNPID) == 2, "A PNP ID must be 2 bytes");

/**
 * @brief Creates a `PSPdispPNPID` from its 3 uppercase letters (e.g. `PSPDISP_PNP_ID('S', 'A', 'M')`).
 *
 * Each letter is converted to its 5-bit value ('A' = 1 to 'Z' = 26) and `reserved` is left 0.
 *
 * It's a compound literal, a struct value, so it can be assigned or used to initialize a field.
 */
#define PSPDISP_PNP_ID(letter_1, letter_2, letter_3) ((PSPdispPNPID) { \
    .first  = (letter_1) - 'A' + 1,                                   \
    .second = (letter_2) - 'A' + 1,                                   \
    .third  = (letter_3) - 'A' + 1,                                   \
})

/**
 * @brief EDID manufacture date: the week and year the display was made, or its model year.
 *
 * When `week` is 0xFF, `year` is the model year instead of the year of manufacture.
 */
typedef struct {
    uint8_t week; // byte 0: Week of manufacture (1-54), 0 when unspecified, 0xFF when `year` is the model year
    uint8_t year; // byte 1: Year of manufacture (or model year) - 1990
} PSPdispEDIDManufactureDate;

static_assert(sizeof(PSPdispEDIDManufactureDate) == 2, "An EDID manufacture date must be 2 bytes");

/**
 * @brief EDID version: which revision of the EDID standard the block follows (e.g. 1.4).
 *
 * The standard calls `major` the version number and `minor` the revision number.
 */
typedef struct {
    uint8_t major; // byte 0: Version number, always 1
    uint8_t minor; // byte 1: Revision number (e.g. 4 for EDID 1.4)
} PSPdispEDIDVersion;

static_assert(sizeof(PSPdispEDIDVersion) == 2, "An EDID version must be 2 bytes");

/**
 * @brief EDID colour bit depth of a digital input: the bits per primary colour (`PSPdispEDIDVideoInput.digital.color_depth`).
 *
 * Value 7 is reserved.
 */
typedef enum: uint8_t {
    PSPDISP_EDID_COLOR_DEPTH_UNDEFINED = 0, // Not defined
    PSPDISP_EDID_COLOR_DEPTH_6         = 1, // 6 bits per primary colour
    PSPDISP_EDID_COLOR_DEPTH_8         = 2, // 8 bits per primary colour
    PSPDISP_EDID_COLOR_DEPTH_10        = 3, // 10 bits per primary colour
    PSPDISP_EDID_COLOR_DEPTH_12        = 4, // 12 bits per primary colour
    PSPDISP_EDID_COLOR_DEPTH_14        = 5, // 14 bits per primary colour
    PSPDISP_EDID_COLOR_DEPTH_16        = 6, // 16 bits per primary colour
} PSPdispEDIDColorDepth;

/**
 * @brief EDID interface standard of a digital input (`PSPdispEDIDVideoInput.digital.interface`).
 *
 * Values 6-15 are reserved.
 */
typedef enum: uint8_t {
    PSPDISP_EDID_INTERFACE_UNDEFINED   = 0, // Not defined
    PSPDISP_EDID_INTERFACE_DVI         = 1, // Digital Visual Interface
    PSPDISP_EDID_INTERFACE_HDMI_A      = 2, // HDMI, type A connector
    PSPDISP_EDID_INTERFACE_HDMI_B      = 3, // HDMI, type B connector
    PSPDISP_EDID_INTERFACE_MDDI        = 4, // Mobile Display Digital Interface
    PSPDISP_EDID_INTERFACE_DISPLAYPORT = 5, // DisplayPort
} PSPdispEDIDInterface;

/**
 * @brief EDID signal level standard of an analog input: the peak-to-peak video and sync voltages (`PSPdispEDIDVideoInput.analog.signal_level`).
 *
 * Constant names give the video then the sync voltage (e.g. `0700_0300` for 0.700 V video and 0.300 V sync).
 */
typedef enum: uint8_t {
    PSPDISP_EDID_SIGNAL_LEVEL_0700_0300 = 0, // 0.700 V video, 0.300 V sync, 1.000 V total
    PSPDISP_EDID_SIGNAL_LEVEL_0714_0286 = 1, // 0.714 V video, 0.286 V sync, 1.000 V total
    PSPDISP_EDID_SIGNAL_LEVEL_1000_0400 = 2, // 1.000 V video, 0.400 V sync, 1.400 V total
    PSPDISP_EDID_SIGNAL_LEVEL_0700_0000 = 3, // 0.700 V video, no sync on video, 0.700 V total
} PSPdispEDIDSignalLevel;

/**
 * @brief EDID video input definition: the kind of signal the display takes.
 *
 * Bit 7 tells whether the input is digital or analog, so the byte is read through the `digital` or the `analog` view.
 *
 * Bit 7 can also be read directly as `is_digital`, but initialize through a view (e.g. `{ .digital = { .is_digital = true, ... } }`), since a union is initialized through a single member.
 *
 * The `digital` view follows EDID 1.4, since on EDID 1.3 only bit 0 is defined (DFP 1.x compatible).
 */
typedef union {
    struct {
        bool is_digital : 1; // bit 7: True for digital, false for analog
        uint8_t         : 7; // bits 6-0: Read through the `digital` or the `analog` view
    }; // Bit 7 alone, as `video_input.is_digital`
    struct {
        bool is_digital                   : 1; // bit 7: Always true
        PSPdispEDIDColorDepth color_depth : 3; // bits 6-4: Bits per primary colour, see `PSPdispEDIDColorDepth`
        PSPdispEDIDInterface interface    : 4; // bits 3-0: Interface standard, see `PSPdispEDIDInterface`
    } digital; // When `is_digital` is true
    struct {
        bool is_digital                     : 1; // bit 7: Always false
        PSPdispEDIDSignalLevel signal_level : 2; // bits 6-5: Video and sync voltages, see `PSPdispEDIDSignalLevel`
        bool blank_to_black                 : 1; // bit 4: Blank-to-black setup (pedestal) expected
        bool separate_sync                  : 1; // bit 3: Separate horizontal and vertical sync supported
        bool composite_sync                 : 1; // bit 2: Composite sync on the horizontal sync line supported
        bool sync_on_green                  : 1; // bit 1: Composite sync on the green video line supported
        bool serrations                     : 1; // bit 0: Serrations on the vertical sync supported
    } analog; // When `is_digital` is false
} PSPdispEDIDVideoInput;

static_assert(sizeof(PSPdispEDIDVideoInput) == 1, "An EDID video input definition must be 1 byte");

/**
 * @brief EDID screen size: the physical size of the display in cm, or only its aspect ratio.
 *
 * When `vertical` is 0, `horizontal` is the landscape aspect ratio as `ratio * 100 - 99` (e.g. 79 for 16:9).
 *
 * When `horizontal` is 0, `vertical` is the portrait aspect ratio as `100 / ratio - 99` (e.g. 79 for 9:16).
 *
 * When both are 0, the size is unknown or variable (e.g. a projector).
 *
 * The aspect ratios are only defined since EDID 1.4.
 */
typedef struct {
    uint8_t horizontal; // byte 0: Horizontal size in cm, or an aspect ratio
    uint8_t vertical;   // byte 1: Vertical size in cm, or an aspect ratio
} PSPdispEDIDScreenSize;

static_assert(sizeof(PSPdispEDIDScreenSize) == 2, "An EDID screen size must be 2 bytes");

/**
 * @brief EDID colour type: the colour encodings of a digital input, or the colour type of an analog one (`PSPdispEDIDFeatures.color_type`).
 *
 * The digital constants are for EDID 1.4 digital inputs, the others for analog inputs and EDID 1.3.
 *
 * Both groups share the values 0-3, so use the group that matches `PSPdispEDIDVideoInput.is_digital`.
 */
typedef enum: uint8_t {
    PSPDISP_EDID_COLOR_TYPE_RGB444                   = 0, // Digital: RGB 4:4:4 only
    PSPDISP_EDID_COLOR_TYPE_RGB444_YCRCB444          = 1, // Digital: RGB 4:4:4 and YCrCb 4:4:4
    PSPDISP_EDID_COLOR_TYPE_RGB444_YCRCB422          = 2, // Digital: RGB 4:4:4 and YCrCb 4:2:2
    PSPDISP_EDID_COLOR_TYPE_RGB444_YCRCB444_YCRCB422 = 3, // Digital: RGB 4:4:4, YCrCb 4:4:4 and YCrCb 4:2:2
    PSPDISP_EDID_COLOR_TYPE_MONOCHROME               = 0, // Analog: monochrome or greyscale
    PSPDISP_EDID_COLOR_TYPE_RGB                      = 1, // Analog: RGB colour
    PSPDISP_EDID_COLOR_TYPE_NON_RGB                  = 2, // Analog: non-RGB colour
    PSPDISP_EDID_COLOR_TYPE_UNDEFINED                = 3, // Analog: undefined
} PSPdispEDIDColorType;

/**
 * @brief EDID feature support: the power management, colour type and timing flags of the display.
 *
 * `color_type` depends on `PSPdispEDIDVideoInput.is_digital`.
 *
 * Bits 1-0 changed meaning from EDID 1.3 to 1.4, as told in their comments.
 */
typedef struct {
    bool dpms_standby               : 1; // bit 7: DPMS standby supported
    bool dpms_suspend               : 1; // bit 6: DPMS suspend supported
    bool dpms_active_off            : 1; // bit 5: DPMS active-off supported
    PSPdispEDIDColorType color_type : 2; // bits 4-3: Colour type, see `PSPdispEDIDColorType`
    bool srgb_default               : 1; // bit 2: sRGB is the default colour space (the chromaticity is sRGB)
    bool preferred_timing_native    : 1; // bit 1: The first descriptor is the native mode (EDID 1.4), or holds the preferred timing (EDID 1.3, always 1)
    bool continuous_frequency       : 1; // bit 0: Continuous frequency (EDID 1.4), or default GTF supported (EDID 1.3)
} PSPdispEDIDFeatures;

static_assert(sizeof(PSPdispEDIDFeatures) == 1, "An EDID feature support must be 1 byte");

/**
 * @brief EDID colour point: the high 8 bits of the CIE 1931 x and y coordinates of a colour.
 *
 * The low 2 bits of both coordinates are packed apart, in `PSPdispEDIDChromaticity`.
 */
typedef struct {
    uint8_t x_high; // byte 0: x coordinate, high 8 bits
    uint8_t y_high; // byte 1: y coordinate, high 8 bits
} PSPdispEDIDColorPoint;

static_assert(sizeof(PSPdispEDIDColorPoint) == 2, "An EDID colour point must be 2 bytes");

/**
 * @brief EDID chromaticity: where the red, green and blue primaries and the white point of the display are on the CIE 1931 xy colour map.
 *
 * Each coordinate is 10 bits in units of 1/1024, split in its high 8 bits (in a `PSPdispEDIDColorPoint`) and its low 2 bits (the `*_low` bit-fields).
 *
 * A coordinate is rebuilt as `(high << 2 | low) / 1024.0` (e.g. `(red.x_high << 2 | red_x_low) / 1024.0`).
 */
typedef struct {
    uint8_t red_x_low   : 2;     // byte 0, bits 7-6: Red x, low 2 bits
    uint8_t red_y_low   : 2;     // byte 0, bits 5-4: Red y, low 2 bits
    uint8_t green_x_low : 2;     // byte 0, bits 3-2: Green x, low 2 bits
    uint8_t green_y_low : 2;     // byte 0, bits 1-0: Green y, low 2 bits
    uint8_t blue_x_low  : 2;     // byte 1, bits 7-6: Blue x, low 2 bits
    uint8_t blue_y_low  : 2;     // byte 1, bits 5-4: Blue y, low 2 bits
    uint8_t white_x_low : 2;     // byte 1, bits 3-2: White point x, low 2 bits
    uint8_t white_y_low : 2;     // byte 1, bits 1-0: White point y, low 2 bits
    PSPdispEDIDColorPoint red;   // bytes 2-3: Red primary, high 8 bits
    PSPdispEDIDColorPoint green; // bytes 4-5: Green primary, high 8 bits
    PSPdispEDIDColorPoint blue;  // bytes 6-7: Blue primary, high 8 bits
    PSPdispEDIDColorPoint white; // bytes 8-9: White point, high 8 bits
} PSPdispEDIDChromaticity;

static_assert(sizeof(PSPdispEDIDChromaticity) == 10, "An EDID chromaticity must be 10 bytes");

/**
 * @brief Converts a CIE 1931 x or y coordinate (e.g. 0.640) to its 10-bit EDID value, rounded to the nearest 1/1024.
 *
 * The coordinate must be between 0 and 1, which every visible colour is.
 */
#define PSPDISP_EDID_COORDINATE(coordinate) ((uint16_t) ((coordinate) * 1024 + 0.5))

/**
 * @brief Creates a `PSPdispEDIDChromaticity` from the CIE 1931 xy coordinates of the primaries and the white point (e.g. sRGB is `PSPDISP_EDID_CHROMATICITY(0.640, 0.330, 0.300, 0.600, 0.150, 0.060, 0.3127, 0.3290)`).
 *
 * Each coordinate is converted with `PSPDISP_EDID_COORDINATE`, then split in its high 8 bits and low 2 bits.
 *
 * It's a compound literal, a struct value, so it can be assigned or used to initialize a field.
 *
 * Each argument is used twice, so pass constants or expressions without side effects.
 */
#define PSPDISP_EDID_CHROMATICITY(red_x, red_y, green_x, green_y, blue_x, blue_y, white_x, white_y) ((PSPdispEDIDChromaticity) { \
    .red_x_low   = PSPDISP_EDID_COORDINATE(red_x) & 3,                                                                           \
    .red_y_low   = PSPDISP_EDID_COORDINATE(red_y) & 3,                                                                           \
    .green_x_low = PSPDISP_EDID_COORDINATE(green_x) & 3,                                                                         \
    .green_y_low = PSPDISP_EDID_COORDINATE(green_y) & 3,                                                                         \
    .blue_x_low  = PSPDISP_EDID_COORDINATE(blue_x) & 3,                                                                          \
    .blue_y_low  = PSPDISP_EDID_COORDINATE(blue_y) & 3,                                                                          \
    .white_x_low = PSPDISP_EDID_COORDINATE(white_x) & 3,                                                                         \
    .white_y_low = PSPDISP_EDID_COORDINATE(white_y) & 3,                                                                         \
    .red   = { .x_high = PSPDISP_EDID_COORDINATE(red_x) >> 2,   .y_high = PSPDISP_EDID_COORDINATE(red_y) >> 2 },                 \
    .green = { .x_high = PSPDISP_EDID_COORDINATE(green_x) >> 2, .y_high = PSPDISP_EDID_COORDINATE(green_y) >> 2 },               \
    .blue  = { .x_high = PSPDISP_EDID_COORDINATE(blue_x) >> 2,  .y_high = PSPDISP_EDID_COORDINATE(blue_y) >> 2 },                \
    .white = { .x_high = PSPDISP_EDID_COORDINATE(white_x) >> 2, .y_high = PSPDISP_EDID_COORDINATE(white_y) >> 2 },               \
})

/**
 * @brief Gets the 10-bit x coordinate of a colour of a `PSPdispEDIDChromaticity` (e.g. `PSPDISP_EDID_CHROMATICITY_X(edid.chromaticity, red)`).
 *
 * `color` is one of `red`, `green`, `blue` or `white`.
 *
 * Divide the result by 1024.0 to get the CIE 1931 x coordinate.
 */
#define PSPDISP_EDID_CHROMATICITY_X(chromaticity, color) ((uint16_t) ((chromaticity).color.x_high << 2 | (chromaticity).color##_x_low))

/**
 * @brief Gets the 10-bit y coordinate of a colour of a `PSPdispEDIDChromaticity` (e.g. `PSPDISP_EDID_CHROMATICITY_Y(edid.chromaticity, red)`).
 *
 * `color` is one of `red`, `green`, `blue` or `white`.
 *
 * Divide the result by 1024.0 to get the CIE 1931 y coordinate.
 */
#define PSPDISP_EDID_CHROMATICITY_Y(chromaticity, color) ((uint16_t) ((chromaticity).color.y_high << 2 | (chromaticity).color##_y_low))

/**
 * @brief EDID established timings: which of the classic modes of the VGA era (by IBM, VESA and Apple) the display supports, one bit each.
 *
 * Field names give the resolution and the refresh rate of the mode (e.g. `mode_640x480_60hz`).
 *
 * Field comments tell who defined each mode, after its byte and bit.
 */
typedef struct {
    bool mode_720x400_70hz             : 1; // byte 0, bit 7: IBM VGA (text mode)
    bool mode_720x400_88hz             : 1; // byte 0, bit 6: IBM XGA-2
    bool mode_640x480_60hz             : 1; // byte 0, bit 5: IBM VGA
    bool mode_640x480_67hz             : 1; // byte 0, bit 4: Apple Macintosh II
    bool mode_640x480_72hz             : 1; // byte 0, bit 3: VESA
    bool mode_640x480_75hz             : 1; // byte 0, bit 2: VESA
    bool mode_800x600_56hz             : 1; // byte 0, bit 1: VESA
    bool mode_800x600_60hz             : 1; // byte 0, bit 0: VESA
    bool mode_800x600_72hz             : 1; // byte 1, bit 7: VESA
    bool mode_800x600_75hz             : 1; // byte 1, bit 6: VESA
    bool mode_832x624_75hz             : 1; // byte 1, bit 5: Apple Macintosh II
    bool mode_1024x768_87hz_interlaced : 1; // byte 1, bit 4: IBM 8514/A
    bool mode_1024x768_60hz            : 1; // byte 1, bit 3: VESA
    bool mode_1024x768_70hz            : 1; // byte 1, bit 2: VESA
    bool mode_1024x768_75hz            : 1; // byte 1, bit 1: VESA
    bool mode_1280x1024_75hz           : 1; // byte 1, bit 0: VESA
    bool mode_1152x870_75hz            : 1; // byte 2, bit 7: Apple Macintosh II
    uint8_t manufacturer_timings       : 7; // byte 2, bits 6-0: The manufacturer's own modes, one bit each
} PSPdispEDIDEstablishedTimings;

static_assert(sizeof(PSPdispEDIDEstablishedTimings) == 3, "EDID established timings must be 3 bytes");

/**
 * @brief EDID aspect ratio of a standard timing (`PSPdispEDIDStandardTiming.aspect_ratio`), which gives the height from the width.
 */
typedef enum: uint8_t {
    PSPDISP_EDID_ASPECT_RATIO_16_10 = 0, // 16:10, height = width * 10 / 16 (1:1 before EDID 1.3)
    PSPDISP_EDID_ASPECT_RATIO_4_3   = 1, // 4:3, height = width * 3 / 4
    PSPDISP_EDID_ASPECT_RATIO_5_4   = 2, // 5:4, height = width * 4 / 5
    PSPDISP_EDID_ASPECT_RATIO_16_9  = 3, // 16:9, height = width * 9 / 16
} PSPdispEDIDAspectRatio;

/**
 * @brief EDID standard timing: a display mode given by its width, aspect ratio and refresh rate, whose exact timings come from the VESA DMT table or a VESA formula (GTF or CVT).
 *
 * The height is not stored, it comes from the width and the aspect ratio (e.g. 1920 at 16:9 is 1080).
 *
 * An unused slot has 0x01 in both bytes (see `PSPDISP_EDID_STANDARD_TIMING_UNUSED`).
 */
typedef struct {
    uint8_t horizontal_active;               // byte 0: Width in pixels / 8 - 31 (256-2288 pixels, in steps of 8)
    PSPdispEDIDAspectRatio aspect_ratio : 2; // byte 1, bits 7-6: Aspect ratio, see `PSPdispEDIDAspectRatio`
    uint8_t refresh_rate                : 6; // byte 1, bits 5-0: Refresh rate - 60 Hz (60-123 Hz)
} PSPdispEDIDStandardTiming;

static_assert(sizeof(PSPdispEDIDStandardTiming) == 2, "An EDID standard timing must be 2 bytes");

/**
 * @brief Creates a `PSPdispEDIDStandardTiming` from the width in pixels, the aspect ratio and the refresh rate in Hz (e.g. `PSPDISP_EDID_STANDARD_TIMING(1920, PSPDISP_EDID_ASPECT_RATIO_16_9, 60)`).
 *
 * The width must be a multiple of 8 from 256 to 2288, and the refresh rate from 60 to 123 Hz.
 *
 * It's a compound literal, a struct value, so it can be assigned or used to initialize a field.
 */
#define PSPDISP_EDID_STANDARD_TIMING(width, aspect, refresh) ((PSPdispEDIDStandardTiming) { \
    .horizontal_active = (width) / 8 - 31,                                                  \
    .aspect_ratio      = (aspect),                                                          \
    .refresh_rate      = (refresh) - 60,                                                    \
})

/**
 * @brief Unused `PSPdispEDIDStandardTiming` slot, with 0x01 in both bytes (e.g. `.standard_timings = { [0 ... 7] = PSPDISP_EDID_STANDARD_TIMING_UNUSED }`).
 */
#define PSPDISP_EDID_STANDARD_TIMING_UNUSED ((PSPdispEDIDStandardTiming) { .horizontal_active = 0x01, .refresh_rate = 0x01 })

/**
 * @brief EDID sync type of a detailed timing (`PSPdispEDIDDetailedTiming.sync_type`), which also gives the meaning of `vsync_positive` and `hsync_positive`.
 */
typedef enum: uint8_t {
    PSPDISP_EDID_SYNC_TYPE_ANALOG_COMPOSITE         = 0, // Analog composite sync
    PSPDISP_EDID_SYNC_TYPE_BIPOLAR_ANALOG_COMPOSITE = 1, // Bipolar analog composite sync
    PSPDISP_EDID_SYNC_TYPE_DIGITAL_COMPOSITE        = 2, // Digital composite sync, on the horizontal sync line
    PSPDISP_EDID_SYNC_TYPE_DIGITAL_SEPARATE         = 3, // Digital separate horizontal and vertical sync
} PSPdispEDIDSyncType;

/**
 * @brief EDID stereo mode of a detailed timing (`PSPdispEDIDDetailedTiming.stereo`), refined by `PSPdispEDIDDetailedTiming.stereo_variant`.
 */
typedef enum: uint8_t {
    PSPDISP_EDID_STEREO_NONE        = 0, // Normal display, no stereo
    PSPDISP_EDID_STEREO_RIGHT       = 1, // Right image when the stereo sync is high (field sequential), or on even lines (2-way interleaved, with `stereo_variant`)
    PSPDISP_EDID_STEREO_LEFT        = 2, // Left image when the stereo sync is high (field sequential), or on even lines (2-way interleaved, with `stereo_variant`)
    PSPDISP_EDID_STEREO_INTERLEAVED = 3, // 4-way interleaved, or side-by-side interleaved (with `stereo_variant`)
} PSPdispEDIDStereo;

/**
 * @brief EDID detailed timing descriptor: a display mode with its exact timings.
 *
 * Values wider than 8 bits are split in a `*_low` and a `*_high` part, rebuilt as `high << 8 | low` (`high << 4 | low` for the vertical front porch and sync width).
 */
typedef struct {
    uint8_t pixel_clock_low;                 // byte 0: Pixel clock in units of 10 kHz, low 8 bits
    uint8_t pixel_clock_high;                // byte 1: Pixel clock, high 8 bits (the whole pixel clock is never 0, 0 marks a `PSPdispEDIDDisplayDescriptor`)
    uint8_t horizontal_active_low;           // byte 2: Horizontal active pixels, low 8 bits
    uint8_t horizontal_blanking_low;         // byte 3: Horizontal blanking pixels, low 8 bits
    uint8_t horizontal_active_high      : 4; // byte 4, bits 7-4: Horizontal active pixels, high 4 bits
    uint8_t horizontal_blanking_high    : 4; // byte 4, bits 3-0: Horizontal blanking pixels, high 4 bits
    uint8_t vertical_active_low;             // byte 5: Vertical active lines, low 8 bits
    uint8_t vertical_blanking_low;           // byte 6: Vertical blanking lines, low 8 bits
    uint8_t vertical_active_high        : 4; // byte 7, bits 7-4: Vertical active lines, high 4 bits
    uint8_t vertical_blanking_high      : 4; // byte 7, bits 3-0: Vertical blanking lines, high 4 bits
    uint8_t horizontal_front_porch_low;      // byte 8: Horizontal front porch (sync offset) in pixels, low 8 bits
    uint8_t horizontal_sync_width_low;       // byte 9: Horizontal sync pulse width in pixels, low 8 bits
    uint8_t vertical_front_porch_low    : 4; // byte 10, bits 7-4: Vertical front porch (sync offset) in lines, low 4 bits
    uint8_t vertical_sync_width_low     : 4; // byte 10, bits 3-0: Vertical sync pulse width in lines, low 4 bits
    uint8_t horizontal_front_porch_high : 2; // byte 11, bits 7-6: Horizontal front porch, high 2 bits
    uint8_t horizontal_sync_width_high  : 2; // byte 11, bits 5-4: Horizontal sync pulse width, high 2 bits
    uint8_t vertical_front_porch_high   : 2; // byte 11, bits 3-2: Vertical front porch, high 2 bits
    uint8_t vertical_sync_width_high    : 2; // byte 11, bits 1-0: Vertical sync pulse width, high 2 bits
    uint8_t horizontal_image_size_low;       // byte 12: Horizontal image size in mm, low 8 bits
    uint8_t vertical_image_size_low;         // byte 13: Vertical image size in mm, low 8 bits
    uint8_t horizontal_image_size_high  : 4; // byte 14, bits 7-4: Horizontal image size, high 4 bits
    uint8_t vertical_image_size_high    : 4; // byte 14, bits 3-0: Vertical image size, high 4 bits
    uint8_t horizontal_border;               // byte 15: Horizontal border in pixels, on each side
    uint8_t vertical_border;                 // byte 16: Vertical border in lines, on each side
    bool interlaced                     : 1; // byte 17, bit 7: Interlaced instead of progressive
    PSPdispEDIDStereo stereo            : 2; // byte 17, bits 6-5: Stereo mode, see `PSPdispEDIDStereo`
    PSPdispEDIDSyncType sync_type       : 2; // byte 17, bits 4-3: Sync type, see `PSPdispEDIDSyncType`
    bool vsync_positive                 : 1; // byte 17, bit 2: Positive vertical sync for digital separate sync, or serrations for composite sync
    bool hsync_positive                 : 1; // byte 17, bit 1: Positive horizontal sync for digital sync, or sync on all 3 RGB lines (not only green) for analog sync
    bool stereo_variant                 : 1; // byte 17, bit 0: 2-way interleaved (not field sequential) with `PSPDISP_EDID_STEREO_RIGHT` or `LEFT`, or side-by-side (not 4-way) with `INTERLEAVED`
} PSPdispEDIDDetailedTiming;

static_assert(sizeof(PSPdispEDIDDetailedTiming) == 18, "An EDID detailed timing must be 18 bytes");

/**
 * @brief Creates a `PSPdispEDIDDetailedTiming` from its timings (e.g. the PSP is `PSPDISP_EDID_DETAILED_TIMING(9000, 480, 45, 272, 14, 2, 41, 2, 10, 95, 54, .sync_type = PSPDISP_EDID_SYNC_TYPE_DIGITAL_SEPARATE)`).
 *
 * The pixel clock is in kHz, horizontal values in pixels, vertical values in lines and the image size in mm.
 *
 * Other fields (e.g. `interlaced`, `sync_type` or the borders) are set by designated initializers after the image size.
 *
 * It's a compound literal, a struct value, so it can be assigned or used to initialize a field.
 *
 * Each argument is used twice, so pass constants or expressions without side effects.
 */
#define PSPDISP_EDID_DETAILED_TIMING(clock_khz, h_active, h_blanking, v_active, v_blanking,      \
                                     h_front_porch, h_sync_width, v_front_porch, v_sync_width,   \
                                     h_image_mm, v_image_mm, ...) ((PSPdispEDIDDetailedTiming) { \
    .pixel_clock_low             = ((clock_khz) / 10) & 0xFF,                                    \
    .pixel_clock_high            = ((clock_khz) / 10) >> 8,                                      \
    .horizontal_active_low       = (h_active) & 0xFF,                                            \
    .horizontal_active_high      = (h_active) >> 8,                                              \
    .horizontal_blanking_low     = (h_blanking) & 0xFF,                                          \
    .horizontal_blanking_high    = (h_blanking) >> 8,                                            \
    .vertical_active_low         = (v_active) & 0xFF,                                            \
    .vertical_active_high        = (v_active) >> 8,                                              \
    .vertical_blanking_low       = (v_blanking) & 0xFF,                                          \
    .vertical_blanking_high      = (v_blanking) >> 8,                                            \
    .horizontal_front_porch_low  = (h_front_porch) & 0xFF,                                       \
    .horizontal_front_porch_high = (h_front_porch) >> 8,                                         \
    .horizontal_sync_width_low   = (h_sync_width) & 0xFF,                                        \
    .horizontal_sync_width_high  = (h_sync_width) >> 8,                                          \
    .vertical_front_porch_low    = (v_front_porch) & 0xF,                                        \
    .vertical_front_porch_high   = (v_front_porch) >> 4,                                         \
    .vertical_sync_width_low     = (v_sync_width) & 0xF,                                         \
    .vertical_sync_width_high    = (v_sync_width) >> 4,                                          \
    .horizontal_image_size_low   = (h_image_mm) & 0xFF,                                          \
    .horizontal_image_size_high  = (h_image_mm) >> 8,                                            \
    .vertical_image_size_low     = (v_image_mm) & 0xFF,                                          \
    .vertical_image_size_high    = (v_image_mm) >> 8,                                            \
    __VA_ARGS__                                                                                  \
})

/**
 * @brief EDID display descriptor tag: the kind of data of a display descriptor (`PSPdispEDIDDisplayDescriptor.tag`).
 *
 * Tags 0x00-0x0F are manufacturer specified.
 *
 * Tags 0x11-0xF6 are reserved.
 */
typedef enum: uint8_t {
    PSPDISP_EDID_TAG_SERIAL_NUMBER           = 0xFF, // Text: product serial number
    PSPDISP_EDID_TAG_ALPHANUMERIC_DATA       = 0xFE, // Text: free-form data
    PSPDISP_EDID_TAG_RANGE_LIMITS            = 0xFD, // Display range limits, see `PSPdispEDIDRangeLimits`
    PSPDISP_EDID_TAG_PRODUCT_NAME            = 0xFC, // Text: product name
    PSPDISP_EDID_TAG_COLOR_POINT             = 0xFB, // Additional white points
    PSPDISP_EDID_TAG_STANDARD_TIMINGS        = 0xFA, // 6 more `PSPdispEDIDStandardTiming`
    PSPDISP_EDID_TAG_COLOR_MANAGEMENT        = 0xF9, // Display colour management (DCM) data
    PSPDISP_EDID_TAG_CVT_TIMINGS             = 0xF8, // CVT 3-byte timing codes
    PSPDISP_EDID_TAG_ESTABLISHED_TIMINGS_III = 0xF7, // More legacy modes, one bit each
    PSPDISP_EDID_TAG_DUMMY                   = 0x10, // Unused descriptor, with all data 0
} PSPdispEDIDDescriptorTag;

/**
 * @brief EDID rate offset of a range limits descriptor on EDID 1.4 (`PSPdispEDIDDisplayDescriptor.horizontal_rate_offset` and `vertical_rate_offset`): which limits get 255 added, for rates above 255.
 *
 * Value 1 is reserved.
 */
typedef enum: uint8_t {
    PSPDISP_EDID_RATE_OFFSET_NONE    = 0, // No offset
    PSPDISP_EDID_RATE_OFFSET_MAX     = 2, // 255 added to the maximum rate
    PSPDISP_EDID_RATE_OFFSET_MIN_MAX = 3, // 255 added to the minimum and the maximum rates
} PSPdispEDIDRateOffset;

/**
 * @brief EDID video timing support of a range limits descriptor (`PSPdispEDIDRangeLimits.timing_support`): how the modes that are not listed may be timed.
 *
 * Other values are reserved.
 */
typedef enum: uint8_t {
    PSPDISP_EDID_TIMING_SUPPORT_DEFAULT_GTF       = 0x00, // Default GTF formula
    PSPDISP_EDID_TIMING_SUPPORT_RANGE_LIMITS_ONLY = 0x01, // No formula, only the range limits (EDID 1.4)
    PSPDISP_EDID_TIMING_SUPPORT_SECONDARY_GTF     = 0x02, // Secondary GTF curve, with its parameters in `timing_data`
    PSPDISP_EDID_TIMING_SUPPORT_CVT               = 0x04, // CVT formula, with its parameters in `timing_data` (EDID 1.4)
} PSPdispEDIDTimingSupport;

/**
 * @brief EDID range limits: the rates and pixel clock the display accepts, as the data of a `PSPDISP_EDID_TAG_RANGE_LIMITS` descriptor.
 *
 * It fills bytes 5-17 of the display descriptor (`PSPdispEDIDDisplayDescriptor.range_limits`).
 *
 * On EDID 1.4, the `horizontal_rate_offset` and `vertical_rate_offset` of the descriptor add 255 to the rates, see `PSPdispEDIDRateOffset`.
 */
typedef struct {
    uint8_t vertical_rate_min;               // byte 0: Minimum vertical rate in Hz
    uint8_t vertical_rate_max;               // byte 1: Maximum vertical rate in Hz
    uint8_t horizontal_rate_min;             // byte 2: Minimum horizontal rate in kHz
    uint8_t horizontal_rate_max;             // byte 3: Maximum horizontal rate in kHz
    uint8_t pixel_clock_max;                 // byte 4: Maximum pixel clock in units of 10 MHz
    PSPdispEDIDTimingSupport timing_support; // byte 5: Timing formula of the modes that are not listed, see `PSPdispEDIDTimingSupport`
    uint8_t timing_data[7];                  // bytes 6-12: Formula parameters for secondary GTF and CVT, otherwise 0x0A then 0x20 padding
} PSPdispEDIDRangeLimits;

static_assert(sizeof(PSPdispEDIDRangeLimits) == 13, "EDID range limits must be 13 bytes");

/**
 * @brief EDID display descriptor: data about the display other than a mode (e.g. its name).
 */
typedef struct {
    uint16_t zero;                                    // bytes 0-1: Always 0, what tells it from a `PSPdispEDIDDetailedTiming`
    uint8_t reserved;                                 // byte 2: Always 0
    PSPdispEDIDDescriptorTag tag;                     // byte 3: Kind of the descriptor, see `PSPdispEDIDDescriptorTag`
    uint8_t flags_reserved                       : 4; // byte 4, bits 7-4: Always 0
    PSPdispEDIDRateOffset horizontal_rate_offset : 2; // byte 4, bits 3-2: For range limits on EDID 1.4, otherwise 0
    PSPdispEDIDRateOffset vertical_rate_offset   : 2; // byte 4, bits 1-0: For range limits on EDID 1.4, otherwise 0
    union {
        char text[13];                       // bytes 5-17: For text tags: ASCII, terminated by 0x0A and padded with 0x20 when shorter than 13
        PSPdispEDIDRangeLimits range_limits; // bytes 5-17: For `PSPDISP_EDID_TAG_RANGE_LIMITS`
        uint8_t data[13];                    // bytes 5-17: Raw data, for any tag
    };
} PSPdispEDIDDisplayDescriptor;

static_assert(sizeof(PSPdispEDIDDisplayDescriptor) == 18, "An EDID display descriptor must be 18 bytes");

/**
 * @brief EDID descriptor: an 18-byte slot that holds either a detailed timing or a display descriptor.
 *
 * It's a detailed timing when its first 2 bytes (the pixel clock) are not 0, otherwise it's a display descriptor.
 */
typedef union {
    PSPdispEDIDDetailedTiming detailed_timing; // When the first 2 bytes are not 0
    PSPdispEDIDDisplayDescriptor display;      // When `display.zero` is 0
} PSPdispEDIDDescriptor;

static_assert(sizeof(PSPdispEDIDDescriptor) == 18, "An EDID descriptor must be 18 bytes");

#pragma scalar_storage_order little-endian // The multi-byte values of the base block are little-endian

/**
 * @brief EDID base block: the identification a display gives to the computer, as defined by VESA E-EDID 1.3 and 1.4.
 *
 * Multi-byte fields are little-endian (except `manufacturer_id`) whatever the host endianness, thanks to its little-endian scalar storage order.
 *
 * All the 128 bytes must sum to 0 (mod 256), what `checksum` is set for.
 */
typedef struct {
    uint64_t header;                                   // bytes 0-7: Fixed header that marks an EDID, always `PSPDISP_EDID_HEADER`
    PSPdispPNPID manufacturer_id;                      // bytes 8-9: PNP ID of the manufacturer
    uint16_t product_code;                             // bytes 10-11: Product code, chosen by the manufacturer
    uint32_t serial_number;                            // bytes 12-15: Serial number, 0 when unused
    PSPdispEDIDManufactureDate manufacture_date;       // bytes 16-17: Week and year of manufacture
    PSPdispEDIDVersion version;                        // bytes 18-19: EDID version (e.g. 1.4)
    PSPdispEDIDVideoInput video_input;                 // byte 20: Video input definition, digital or analog
    PSPdispEDIDScreenSize screen_size;                 // bytes 21-22: Physical screen size in cm, or the aspect ratio
    uint8_t gamma;                                     // byte 23: Gamma * 100 - 100, 0xFF when defined in an extension block
    PSPdispEDIDFeatures features;                      // byte 24: Feature support (DPMS, colour type and timing flags)
    PSPdispEDIDChromaticity chromaticity;              // bytes 25-34: Colours of the primaries and the white point
    PSPdispEDIDEstablishedTimings established_timings; // bytes 35-37: Classic VGA-era modes supported, one bit each
    PSPdispEDIDStandardTiming standard_timings[8];     // bytes 38-53: Standard timings, `PSPDISP_EDID_STANDARD_TIMING_UNUSED` when unused
    PSPdispEDIDDescriptor descriptors[4];              // bytes 54-125: Descriptors, the first is the preferred mode (a detailed timing)
    uint8_t extension_count;                           // byte 126: Number of extension blocks that follow this one
    uint8_t checksum;                                  // byte 127: Sets the sum of all the bytes to 0 (mod 256)
} PSPdispEDID;

static_assert(sizeof(PSPdispEDID) == 128, "An EDID base block (without extension blocks) must be 128 bytes");

/**
 * @brief Fixed pattern of the first 8 bytes of an EDID (00 FF FF FF FF FF FF 00), that marks the start of the base block.
 *
 * The bytes are the same in both directions, so as a `uint64_t` it's the same value whatever the host endianness.
 */
#define PSPDISP_EDID_HEADER UINT64_C(0x00FFFFFFFFFFFF00)

#pragma scalar_storage_order default
#pragma pack(pop)
