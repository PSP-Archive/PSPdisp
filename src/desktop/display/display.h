/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * display.h - PSP display and provider types        *
 *                                                   *
 *  This file shall not define symbols.              *
 *                                                   *
 * Copyright © 2026 Net-0                            *
 *****************************************************/

#pragma once

#include <glib.h>
#include <gmodule.h>
#include <gio/gio.h>
#include <stdint.h>
#include "../../shared.h"

/**
 * @brief Type of a function pointer to handle a frame.
 *
 * Called by the provider on the display thread (the one running `PSPdispDisplay.context`), whenever the frame changes.
 *
 * The frame buffer always holds the full frame, the dirty rectangles only tell which areas changed since the previous frame.
 * It's owned by the provider and only valid until the handler returns, so copy what must outlive the call.
 *
 * @param pixels A pointer to the frame buffer, a chunk of memory with the pixels.
 * @param stride The number of pixels per row in the frame buffer, it's the resolution width plus padding.
 * @param format The pixel format of the frame buffer.
 * @param rectangles The dirty rectangles, the area that changed compared with the previous frame.
 * @param rectangles_length The number of dirty rectangles.
 */
typedef void (*PSPdispFrameHandler) (void *pixels, uint16_t stride, PSPdispPixelFormat format, PSPdispDirtyRectangle *rectangles, uint8_t rectangles_length);

/**
 * @brief Type of a function pointer to handle Display Power Management Signaling (DPMS).
 *
 * Called by the provider on the display thread (the one running `PSPdispDisplay.context`), when the computer turns the display on or off.
 *
 * @param mode The signaled DPMS mode.
 */
typedef void (*PSPdispDPMSHandler) (PSPdispDPMSMode mode);

/**
 * @brief Display provider responsible to use some kernel driver to manage the virtual display (e.g. EVDI on Linux).
 *
 * Each provider is a dynamic library (module), loaded at runtime from `file_name`, so the program still runs when a driver (and its library) is not installed.
 */
typedef struct PSPdispDisplayProvider PSPdispDisplayProvider;

/**
 * @brief Display interface whose frames are going to be sent to PSP.
 *
 * The core implementation varies based on the provider, but there is always some kernel driver to manage this virtual display.
 */
typedef struct PSPdispDisplay PSPdispDisplay;

struct PSPdispDisplay {
    PSPdispDisplayProvider *provider; // Responsible to provide the implementation to acquire the source of frames
    PSPdispFrameHandler frame_handler; // Handle each new frame to be drawn
    PSPdispDPMSHandler dpms_handler; // Display Power Management Signaling Handler

    // TODO: brightness handler

    GMainContext *context; // Context for the event loop, where the source of frames is dispatched
    GMainLoop *loop; // Event loop of frames
    GThread *thread; // Thread that runs the frames event loop
    GCancellable *thread_cancellable; // Cancellable used to finish the thread that runs the frames event loop
    GSource *frame_source; // The source of frames acquired from the provider (`NULL` while disconnected)
};

struct PSPdispDisplayProvider {
    char *name; // The name of this provider, a user friendly identification
    char *file_name; // The path of the binary file (dynamic library) for this provider, relative to the working directory
    char *symbol_name; // The name of the symbol of the `provide` function for this provider in the binary file
    GModule *module; // The module (dynamic library) loaded from the binary file (`NULL` while unloaded)
    GError *module_error; // The module issue (if there was one on the last loading)
    GSource *(*provide) (PSPdispDisplay *display); // Function loaded from `symbol_name`: creates the virtual display and returns the source of its frames (`NULL` on failure)
};

#define PSP_EDID_SIZE 128 // Size in bytes of an EDID base block (without extension blocks)

/**
 * @brief EDID of the PSP screen: the identification a monitor gives to the computer, used to connect the virtual display.
 *
 * EDID 1.4 base block (no extension blocks) of a digital display named "PSP Display",
 * with only the PSP native mode: 480x272 at 59.94 Hz (9 MHz pixel clock), sRGB colours and DPMS support.
 * The compositor reads it to know the modes the display supports.
 *
 * It's a compound literal, an array value (`uint8_t[PSP_EDID_SIZE]`), so pass `PSP_EDID` (not `&PSP_EDID`) where a pointer is expected.
 * When changing any byte, update the checksum (last byte), so all the `PSP_EDID_SIZE` bytes sum to 0 (mod 256).
 */
#define PSP_EDID (uint8_t[PSP_EDID_SIZE]) {                                       \
    /* ---- Header (bytes 0-7): fixed pattern that marks an EDID ------------- */ \
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,                               \
                                                                                  \
    /* ---- Vendor / product (bytes 8-17) ------------------------------------ */ \
    /* 8-9: manufacturer ID, 3 letters packed 5 bits each, big-endian.         */ \
    /*      'L','N','X' -> 0x31D8. "LNX" is what the Linux kernel's own        */ \
    /*      built-in EDIDs use; pick your own if you ship something.           */ \
    0x31, 0xD8,                                                                   \
    /* 10-11: product code, little-endian (0x5350 = "PS", arbitrary)           */ \
    0x50, 0x53,                                                                   \
    /* 12-15: serial number, little-endian (0 = unused)                        */ \
    0x00, 0x00, 0x00, 0x00,                                                       \
    /* 16: week of manufacture (0 = unspecified)                               */ \
    /* 17: year of manufacture - 1990 (0x24 = 2026)                            */ \
    0x00, 0x24,                                                                   \
                                                                                  \
    /* ---- EDID version (bytes 18-19): 1.4 ---------------------------------- */ \
    0x01, 0x04,                                                                   \
                                                                                  \
    /* ---- Basic display parameters (bytes 20-24) --------------------------- */ \
    /* 20: video input definition                                              */ \
    /*     bit 7    = 1   digital input                                        */ \
    /*     bits 6-4 = 010 8 bits per colour                                    */ \
    /*     bits 3-0 = 0   interface undefined (no real cable)                  */ \
    0xA0,                                                                         \
    /* 21-22: physical size in cm, H x V (~95 x 54 mm screen -> 10 x 5)        */ \
    0x0A, 0x05,                                                                   \
    /* 23: gamma = (value + 100) / 100 -> 0x78 = 120 -> 2.20                   */ \
    0x78,                                                                         \
    /* 24: feature support                                                     */ \
    /*     bit 5   = 1  DPMS active-off supported                              */ \
    /*     bits 4-3= 00 colour encoding: RGB 4:4:4 only                        */ \
    /*     bit 2   = 1  sRGB is the default colour space                       */ \
    /*     bit 1   = 1  preferred timing (first DTD) is the native mode        */ \
    /*     bit 0   = 0  not continuous-frequency                               */ \
    0x26,                                                                         \
                                                                                  \
    /* ---- Chromaticity (bytes 25-34): sRGB / BT.709 primaries, D65 white     */ \
    /*      (10-bit CIE xy coords; low bits packed in 25-26, high in 27-34)    */ \
    0xEE, 0x91, 0xA3, 0x54, 0x4C, 0x99, 0x26, 0x0F, 0x50, 0x54,                   \
                                                                                  \
    /* ---- Established timings (bytes 35-37): none (no VGA-era modes) ------- */ \
    0x00, 0x00, 0x00,                                                             \
                                                                                  \
    /* ---- Standard timings (bytes 38-53): 8 slots, 0x0101 = unused --------- */ \
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,                               \
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,                               \
                                                                                  \
    /* ---- Descriptor 1 (bytes 54-71): Detailed Timing = preferred mode ----- */ \
    /* 54-55: pixel clock / 10 kHz, little-endian: 900 -> 9.00 MHz             */ \
    0x84, 0x03,                                                                   \
    /* 56: H active low 8 bits (480 = 0x1E0)                                   */ \
    /* 57: H blank  low 8 bits (45; total 525)                                 */ \
    /* 58: high nibbles: H active = 1, H blank = 0                             */ \
    0xE0, 0x2D, 0x10,                                                             \
    /* 59: V active low 8 bits (272 = 0x110)                                   */ \
    /* 60: V blank  low 8 bits (14; total 286)                                 */ \
    /* 61: high nibbles: V active = 1, V blank = 0                             */ \
    0x10, 0x0E, 0x10,                                                             \
    /* 62: H front porch  = 2 pixels                                           */ \
    /* 63: H sync width   = 41 pixels                                          */ \
    /* 64: V front porch (high nibble) = 2, V sync width (low nibble) = 10     */ \
    /* 65: high bits of the above four fields (all 0)                          */ \
    0x02, 0x29, 0x2A, 0x00,                                                       \
    /* 66-67: image size in mm, low 8 bits: 95 x 54                            */ \
    /* 68:    high nibbles of image size (0)                                   */ \
    0x5F, 0x36, 0x00,                                                             \
    /* 69-70: H / V border (0)                                                 */ \
    0x00, 0x00,                                                                   \
    /* 71: flags: non-interlaced, digital separate sync, -hsync, -vsync        */ \
    0x18,                                                                         \
                                                                                  \
    /* ---- Descriptor 2 (bytes 72-89): Display Range Limits (tag 0xFD) ------ */ \
    /* 72-74: 0 = display descriptor, not a timing; 75: tag; 76: offsets       */ \
    0x00, 0x00, 0x00, 0xFD, 0x00,                                                 \
    /* 77-78: V rate min/max = 59-61 Hz                                        */ \
    /* 79-80: H rate min/max = 17-18 kHz                                       */ \
    /* 81:    max pixel clock / 10 MHz = 1 -> 10 MHz                           */ \
    0x3B, 0x3D, 0x11, 0x12, 0x01,                                                 \
    /* 82: 0x01 = range limits only (no GTF/CVT formula);                      */ \
    /* 83: 0x0A line feed, then space padding                                  */ \
    0x01, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,                               \
                                                                                  \
    /* ---- Descriptor 3 (bytes 90-107): Monitor Name (tag 0xFC) ------------- */ \
    /*      up to 13 ASCII chars, terminated by 0x0A, padded with 0x20         */ \
    0x00, 0x00, 0x00, 0xFC, 0x00,                                                 \
    'P', 'S', 'P', ' ', 'D', 'i', 's', 'p', 'l', 'a', 'y', 0x0A, 0x20,            \
                                                                                  \
    /* ---- Descriptor 4 (bytes 108-125): Dummy (tag 0x10), unused ----------- */ \
    0x00, 0x00, 0x00, 0x10, 0x00,                                                 \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                                                                  \
    /* ---- Trailer ---------------------------------------------------------- */ \
    /* 126: number of extension blocks (0)                                     */ \
    /* 127: checksum: all 128 bytes must sum to 0 mod 256                      */ \
    0x00, 0x49                                                                    \
}
