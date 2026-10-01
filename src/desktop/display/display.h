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
#include "./edid.h"

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
    PSPdispEDID edid; // Identification given to the computer when connecting, from the PSP model, region and serial number (see `pspdisp_display_edid_new()`)

    // TODO: brightness handler
    // TODO: rotate handler

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
