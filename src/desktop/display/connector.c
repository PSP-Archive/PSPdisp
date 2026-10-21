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
 * @brief Type of a function pointer to handle a frame of the connector, at the resolution of the mode the computer set on the virtual display.
 *
 * Called by the connector on the thread that runs the context of its source (for a display, the display thread), with each frame it asked for.
 *
 * The frame buffer always holds the full frame, the dirty rectangles only tell which areas changed since the previous frame.
 * It's owned by the connector and only valid until the handler returns, so copy what must outlive the call.
 *
 * @param pixels A pointer to the frame buffer, a chunk of memory with the pixels.
 * @param width The width of the frame in pixels.
 * @param height The height of the frame in lines.
 * @param stride The number of pixels per row in the frame buffer, it's the width plus padding.
 * @param format The pixel format of the frame buffer.
 * @param rectangles The dirty rectangles, the area that changed compared with the previous frame.
 * @param rectangles_length The number of dirty rectangles.
 * @param data The data given with the function.
 */
typedef void (*PSPdispConnectorFrameHandler) (void *pixels, uint16_t width, uint16_t height, uint16_t stride, PSPdispPixelFormat format, PSPdispDirtyRectangle *rectangles, uint8_t rectangles_length, gpointer data);

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
 * @brief Type of a function pointer to handle the loss of the virtual display, when its driver is gone.
 *
 * Called by the connector on the thread that runs the context of its source (for a display, the display thread), once, right before its source destroys itself.
 * It's not called when the owner of the source destroys it (see `pspdisp_display_connector_connect()`).
 *
 * @param data The data given with the function.
 */
typedef void (*PSPdispLostHandler) (gpointer data);

/**
 * @brief Connect a virtual display through the connector of this platform, so it appears on the computer.
 *
 * There's one connector per platform, included below at compile time (e.g. `connector/evdi.c`), which implements this function and defines `PSPDISP_DISPLAY_CONNECTOR_NAME`.
 * It uses some kernel driver to manage the virtual display (e.g. EVDI on Linux), and gives its frames and its power changes through the handlers.
 * How it gets them, and how fast it asks for frames, is up to the connector (e.g. EVDI at the PSP refresh rate, IddCx as fast as Windows draws).
 *
 * The returned source is not attached yet: attach it to a context, whose thread then calls the handlers.
 * To disconnect, destroy and unref it on the thread that runs its context: the virtual display is removed when it's finalized, which mustn't happen while that thread waits on it (GLib never does it during a dispatch).
 * When the driver is gone, the source tells the lost handler and destroys itself, but the virtual display is only removed when its last reference is released.
 *
 * @param edid The identification given to the computer.
 * @param frame_handler Handle each new frame.
 * @param dpms_handler Handle each DPMS change.
 * @param lost_handler Handle the loss of the virtual display.
 * @param data Given to the handlers.
 * @return The source of the display events, or `NULL` if it failed.
 */
GSource *pspdisp_display_connector_connect(PSPdispEDID edid, PSPdispConnectorFrameHandler frame_handler, PSPdispDPMSHandler dpms_handler, PSPdispLostHandler lost_handler, gpointer data);

// The display connector of this platform, it implements `pspdisp_display_connector_connect()` and defines `PSPDISP_DISPLAY_CONNECTOR_NAME`
#if defined(__linux__)
	#include "./connector/evdi.c"
#else
	#error "Unsupported platform"
#endif
