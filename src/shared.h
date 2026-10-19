/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * shared.h - Metadata shared by PSP and Desktop     *
 *                                                   *
 *  This file shall only define static symbols.      *
 *                                                   *
 * Copyright © 2008-2015 Jochen Schleu               *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <stdint.h>

// TODO: rework the old protocol between Desktop and PSP
#include "./shared.old.h"

#define PSPDISP_DISPLAY_WIDTH               480 // The PSP display width in pixels
#define PSPDISP_DISPLAY_HEIGHT              272 // The PSP display height in pixels
#define PSPDISP_DISPLAY_FREQUENCY            60 // The PSP display refresh rate in hertz
#define PSPDISP_DISPLAY_FRAME_BUFFER_STRIBE 512 // The PSP display frame-buffer pixels per row in memory

// All the types of this file are packed and little-endian whatever the host, so the PSP and the desktop share the same layout.
// Both pragmas are reset at the end of this file, so they don't leak into the files that include it.
#pragma pack(push, 1)
#pragma scalar_storage_order little-endian

/**
 * @brief Pixel formats supported by the PSP frame buffer
 * 
 * Values match the `PspDisplayPixelFormats` from `pspdisplay.h`.
 *
 * Little-endian, red in the lowest bits.
 */
typedef enum: uint8_t {
	PSPDISP_PIXEL_FORMAT_565  = 0, // 16 bpp: R5 G6 B5, no alpha
	PSPDISP_PIXEL_FORMAT_5551 = 1, // 16 bpp: R5 G5 B5 A1
	PSPDISP_PIXEL_FORMAT_4444 = 2, // 16 bpp: R4 G4 B4 A4
	PSPDISP_PIXEL_FORMAT_8888 = 3, // 32 bpp: R8 G8 B8 A8
} PSPdispPixelFormat;

/**
 * @brief Display power management modes, modeled after VESA DPMS.
 *
 * Values are ordered by increasing power saving, so they can be compared directly (e.g. `mode >= PSPDISP_DPMS_MODE_SUSPEND`).
 */
typedef enum: uint8_t {
    PSPDISP_DPMS_MODE_ON,      // Display fully powered and showing output
    PSPDISP_DPMS_MODE_STANDBY, // Light power saving with near-instant recovery
    PSPDISP_DPMS_MODE_SUSPEND, // Deeper power saving with slower recovery
    PSPDISP_DPMS_MODE_OFF,     // Display powered down; slowest recovery
} PSPdispDPMSMode;

/**
 * @brief Rectangle of pixels that changed, if compared with the previous frame.
 *
 * Corners are absolute screen coordinates (origin top-left, x grows right, y grows down).
 *
 * The region is half-open: it covers [x1, x2) horizontally and [y1, y2) vertically, so width = x2 - x1 and height = y2 - y1.
 */
typedef struct {
    uint16_t x1; // Left edge, inclusive
    uint16_t y1; // Top edge, inclusive
    uint16_t x2; // Right edge, exclusive
    uint16_t y2; // Bottom edge, exclusive
} PSPdispDirtyRectangle;

/**
 * @brief Buttons of the PSP, combined with bitwise OR.
 *
 * Values match `PspCtrlButtons` from `pspctrl.h` (the bits of `SceCtrlData.Buttons`).
 *
 * The PSP app reads them in user mode, so it only sends the first 12, the others are only readable in kernel mode.
 */
typedef enum: uint32_t {
    PSPDISP_BUTTON_SELECT   = 0x000001, // Select
    PSPDISP_BUTTON_START    = 0x000008, // Start
    PSPDISP_BUTTON_UP       = 0x000010, // Up of the directional pad
    PSPDISP_BUTTON_RIGHT    = 0x000020, // Right of the directional pad
    PSPDISP_BUTTON_DOWN     = 0x000040, // Down of the directional pad
    PSPDISP_BUTTON_LEFT     = 0x000080, // Left of the directional pad
    PSPDISP_BUTTON_L        = 0x000100, // Left shoulder button ("L trigger")
    PSPDISP_BUTTON_R        = 0x000200, // Right shoulder button ("R trigger")
    PSPDISP_BUTTON_TRIANGLE = 0x001000, // Triangle, the top face button
    PSPDISP_BUTTON_CIRCLE   = 0x002000, // Circle, the right face button
    PSPDISP_BUTTON_CROSS    = 0x004000, // Cross, the bottom face button
    PSPDISP_BUTTON_SQUARE   = 0x008000, // Square, the left face button
    PSPDISP_BUTTON_HOME     = 0x010000, // Home (kernel mode only)
    PSPDISP_BUTTON_HOLD     = 0x020000, // Hold switch (kernel mode only)
    PSPDISP_BUTTON_WLAN_UP  = 0x040000, // WLAN switch turned on (kernel mode only)
    PSPDISP_BUTTON_REMOTE   = 0x080000, // Remote control connected (kernel mode only)
    PSPDISP_BUTTON_VOLUP    = 0x100000, // Volume up (kernel mode only)
    PSPDISP_BUTTON_VOLDOWN  = 0x200000, // Volume down (kernel mode only)
    PSPDISP_BUTTON_SCREEN   = 0x400000, // Screen, the brightness button (kernel mode only)
    PSPDISP_BUTTON_NOTE     = 0x800000, // Note, the sound button (kernel mode only)
} PSPdispButton;

/**
 * @brief PSP hardware generation.
 * 
 * Values match the return of `kuKernelGetModel()` from `kubridge.h`.
 *
 * The value is the "Xg" generation number minus one.
 */
typedef enum: uint8_t {
    PSPDISP_MODEL_01G = 0,  // PSP-1000 "fat"
    PSPDISP_MODEL_02G = 1,  // PSP-2000 "slim"
    PSPDISP_MODEL_03G = 2,  // PSP-3000 "brite", early boards
    PSPDISP_MODEL_04G = 3,  // PSP-3000, TA-093
    PSPDISP_MODEL_05G = 4,  // PSP Go (N1000)
    PSPDISP_MODEL_06G = 5,  // Never released
    PSPDISP_MODEL_07G = 6,  // PSP-3000, TA-095
    PSPDISP_MODEL_08G = 7,  // Never released
    PSPDISP_MODEL_09G = 8,  // PSP-3000, last revision
    PSPDISP_MODEL_10G = 9,  // Never released
    PSPDISP_MODEL_11G = 10, // PSP-E1000 "Street"
} PSPdispModel;

/**
 * @brief PSP sales region.
 *
 * Values match `ScePsCode.productCode` as filled by `sceChkregGetPsCode()` from uOFW's `chkreg.h` (listed as `SCE_PSP_PRODUCT_CODE_*` in `openpsid_kernel.h`).
 *
 * Constants are ordered by the last digits of the retail model given in the comments (e.g. PSP-2001 -> PSP_REGION_NORTH_AMERICA).
 */
typedef enum: uint8_t {
    PSPDISP_REGION_JAPAN         = 0x03, // x000, Japan
    PSPDISP_REGION_NORTH_AMERICA = 0x04, // x001, North America
    PSPDISP_REGION_AUSTRALIA     = 0x09, // x002, Australia / New Zealand
    PSPDISP_REGION_UK            = 0x07, // x003, United Kingdom
    PSPDISP_REGION_EUROPE        = 0x05, // x004, Europe / Middle East
    PSPDISP_REGION_KOREA         = 0x06, // x005, Korea
    PSPDISP_REGION_HONG_KONG     = 0x0A, // x006, Hong Kong / Singapore
    PSPDISP_REGION_TAIWAN        = 0x0B, // x007, Taiwan
    PSPDISP_REGION_RUSSIA        = 0x0C, // x008, Russia
    PSPDISP_REGION_CHINA         = 0x0D, // x009, China
    PSPDISP_REGION_MEXICO        = 0x08, // x010, Mexico / Latin America
    PSPDISP_REGION_TEST_UNIT     = 0x00, // No retail model, prototype / test unit
    PSPDISP_REGION_DEVKIT        = 0x01, // No retail model, DEM-1000 / DTP-T1000
    PSPDISP_REGION_TESTKIT       = 0x02, // No retail model, DTP-H1500
} PSPdispRegion;

#pragma scalar_storage_order default
#pragma pack(pop)

/**
 * @brief Get the model number of a PSP, as printed on its label (e.g. "PSP-3004"), to name it to the user.
 *
 * The model number is the series of the generation (e.g. PSP-3000 for the 03g, 04g, 07g and 09g boards) with the region in its last digits.
 *
 * Names follow that pattern even for combinations that were never sold (e.g. "PSP-E1001", the PSP-E1000 was only sold in Europe).
 *
 * Development units get the model number of the first kit of their kind (e.g. "DTP-T1000" for a DevKit), whatever the generation.
 *
 * Never released generations get "?" as their series (e.g. "PSP-?001"), and unknown regions get the generation instead (e.g. "PSP-01g?").
 *
 * @param model The PSP generation.
 * @param region The PSP sales region.
 * @return The model number, a static string of up to 13 characters (it fits an EDID text descriptor).
 */
static inline const char *pspdisp_psp_name(PSPdispModel model, PSPdispRegion region) {
    switch (region) {
        case PSPDISP_REGION_JAPAN:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1000";
                case PSPDISP_MODEL_02G: return "PSP-2000";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3000";
                case PSPDISP_MODEL_05G: return "PSP-N1000";
                case PSPDISP_MODEL_11G: return "PSP-E1000";
                default: return "PSP-?000"; // Never released
            }
        case PSPDISP_REGION_NORTH_AMERICA:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1001";
                case PSPDISP_MODEL_02G: return "PSP-2001";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3001";
                case PSPDISP_MODEL_05G: return "PSP-N1001";
                case PSPDISP_MODEL_11G: return "PSP-E1001";
                default: return "PSP-?001"; // Never released
            }
        case PSPDISP_REGION_AUSTRALIA:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1002";
                case PSPDISP_MODEL_02G: return "PSP-2002";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3002";
                case PSPDISP_MODEL_05G: return "PSP-N1002";
                case PSPDISP_MODEL_11G: return "PSP-E1002";
                default: return "PSP-?002"; // Never released
            }
        case PSPDISP_REGION_UK:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1003";
                case PSPDISP_MODEL_02G: return "PSP-2003";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3003";
                case PSPDISP_MODEL_05G: return "PSP-N1003";
                case PSPDISP_MODEL_11G: return "PSP-E1003";
                default: return "PSP-?003"; // Never released
            }
        case PSPDISP_REGION_EUROPE:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1004";
                case PSPDISP_MODEL_02G: return "PSP-2004";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3004";
                case PSPDISP_MODEL_05G: return "PSP-N1004";
                case PSPDISP_MODEL_11G: return "PSP-E1004";
                default: return "PSP-?004"; // Never released
            }
        case PSPDISP_REGION_KOREA:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1005";
                case PSPDISP_MODEL_02G: return "PSP-2005";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3005";
                case PSPDISP_MODEL_05G: return "PSP-N1005";
                case PSPDISP_MODEL_11G: return "PSP-E1005";
                default: return "PSP-?005"; // Never released
            }
        case PSPDISP_REGION_HONG_KONG:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1006";
                case PSPDISP_MODEL_02G: return "PSP-2006";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3006";
                case PSPDISP_MODEL_05G: return "PSP-N1006";
                case PSPDISP_MODEL_11G: return "PSP-E1006";
                default: return "PSP-?006"; // Never released
            }
        case PSPDISP_REGION_TAIWAN:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1007";
                case PSPDISP_MODEL_02G: return "PSP-2007";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3007";
                case PSPDISP_MODEL_05G: return "PSP-N1007";
                case PSPDISP_MODEL_11G: return "PSP-E1007";
                default: return "PSP-?007"; // Never released
            }
        case PSPDISP_REGION_RUSSIA:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1008";
                case PSPDISP_MODEL_02G: return "PSP-2008";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3008";
                case PSPDISP_MODEL_05G: return "PSP-N1008";
                case PSPDISP_MODEL_11G: return "PSP-E1008";
                default: return "PSP-?008"; // Never released
            }
        case PSPDISP_REGION_CHINA:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1009";
                case PSPDISP_MODEL_02G: return "PSP-2009";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3009";
                case PSPDISP_MODEL_05G: return "PSP-N1009";
                case PSPDISP_MODEL_11G: return "PSP-E1009";
                default: return "PSP-?009"; // Never released
            }
        case PSPDISP_REGION_MEXICO:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1010";
                case PSPDISP_MODEL_02G: return "PSP-2010";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3010";
                case PSPDISP_MODEL_05G: return "PSP-N1010";
                case PSPDISP_MODEL_11G: return "PSP-E1010";
                default: return "PSP-?010"; // Never released
            }
        case PSPDISP_REGION_TEST_UNIT:  return "PSP-0000";
        case PSPDISP_REGION_DEVKIT:     return "DTP-T1000";
        case PSPDISP_REGION_TESTKIT:    return "DTP-H1500";
        default: break;
    }

    // Unknown region
    switch (model) {
        case PSPDISP_MODEL_01G: return "PSP-01g?";
        case PSPDISP_MODEL_02G: return "PSP-02g?";
        case PSPDISP_MODEL_03G: return "PSP-03g?";
        case PSPDISP_MODEL_04G: return "PSP-04g?";
        case PSPDISP_MODEL_05G: return "PSP-05g?";
        case PSPDISP_MODEL_06G: return "PSP-06g?";
        case PSPDISP_MODEL_07G: return "PSP-07g?";
        case PSPDISP_MODEL_08G: return "PSP-08g?";
        case PSPDISP_MODEL_09G: return "PSP-09g?";
        case PSPDISP_MODEL_10G: return "PSP-10g?";
        case PSPDISP_MODEL_11G: return "PSP-11g?";
    }
    return "PSP-?"; // Unknown model
}
