/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * connector.c - Display connector                   *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <stdint.h>
#include <glib.h>
#include "../../shared.h"
#include "./edid.c"

/**
 * @brief Type of a function pointer to handle a frame.
 *
 * Called by the connector on the thread that runs the context of its source (for a display, the display thread), with each frame it asked for.
 *
 * The frame buffer always holds the full frame, the dirty rectangles only tell which areas changed since the previous frame.
 * It's owned by the connector and only valid until the handler returns, so copy what must outlive the call.
 *
 * @param pixels A pointer to the frame buffer, a chunk of memory with the pixels.
 * @param stride The number of pixels per row in the frame buffer, it's the resolution width plus padding.
 * @param format The pixel format of the frame buffer.
 * @param rectangles The dirty rectangles, the area that changed compared with the previous frame.
 * @param rectangles_length The number of dirty rectangles.
 * @param data The data given with the function.
 */
typedef void (*PSPdispFrameHandler) (void *pixels, uint16_t stride, PSPdispPixelFormat format, PSPdispDirtyRectangle *rectangles, uint8_t rectangles_length, gpointer data);

/**
 * @brief Type of a function pointer to handle Display Power Management Signaling (DPMS).
 *
 * Called by the connector on the thread that runs the context of its source (for a display, the display thread), when the computer turns the display on or off.
 *
 * @param mode The signaled DPMS mode.
 * @param data The data given with the function.
 */
typedef void (*PSPdispDPMSHandler) (PSPdispDPMSMode mode, gpointer data);

/**
 * @brief Connect a virtual display through the connector of this platform, so it appears on the computer.
 *
 * There's one connector per platform, included below at compile time (e.g. `connector/evdi.c`), which implements this function and defines `PSPDISP_DISPLAY_CONNECTOR_NAME`.
 * It uses some kernel driver to manage the virtual display (e.g. EVDI on Linux), and gives its frames and its power changes through the handlers.
 * How it gets them, and how fast it asks for frames, is up to the connector (e.g. EVDI at the PSP refresh rate, IddCx as fast as Windows draws).
 *
 * The returned source is not attached yet: attach it to a context, whose thread then calls the handlers.
 * To disconnect, destroy and unref it: the virtual display is removed when it's finalized, which GLib never does during a dispatch.
 * When the driver is gone, the source destroys itself, but the virtual display is only removed when its last reference is released.
 *
 * @param edid The identification given to the computer.
 * @param frame_handler Handle each new frame.
 * @param dpms_handler Handle each DPMS change.
 * @param data Given to the handlers.
 * @return The source of the display events, or `NULL` if it failed.
 */
GSource *pspdisp_display_connector_connect(PSPdispEDID edid, PSPdispFrameHandler frame_handler, PSPdispDPMSHandler dpms_handler, gpointer data);

// The display connector of this platform, it implements `pspdisp_display_connector_connect()` and defines `PSPDISP_DISPLAY_CONNECTOR_NAME`
#if defined(__linux__)
    #include "./connector/evdi.c"
#else
    #error "Unsupported platform"
#endif
