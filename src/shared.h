/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * shared.h - Common metadata betwen PSP and Desktop *
 *                                                   *
 *  This file shall not define symbols.              *
 *                                                   *
 * Copyright © 2008-2015 Jochen Schleu               *
 * Copyright © 2026 Net-0                            *
 *****************************************************/

#pragma once

#define PSPDISP_DISPLAY_WIDTH               480 // The PSP display width in pixels
#define PSPDISP_DISPLAY_HEIGHT              272 // The PSP display height in pixels
#define PSPDISP_DISPLAY_FREQUENCY            60 // The PSP display refresh rate in hertz
#define PSPDISP_DISPLAY_FRAME_BUFFER_STRIBE 512 // The PSP display frame-buffer pixels per row in memory

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
} __attribute__((__packed__)) PSPdispDirtyRectangle;

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
