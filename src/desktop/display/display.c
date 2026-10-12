/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * display.c - PSP display and its connector         *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <glib.h>
#include <gio/gio.h>
#include "../../shared.h"
#include "./edid.c"
#include "./connector.c"

/**
 * @brief Display interface whose frames are going to be sent to PSP.
 *
 * The core implementation varies based on the connector, but there is always some kernel driver to manage this virtual display.
 */
typedef struct {
    PSPdispFrameHandler frame_handler; // Handle each new frame to be drawn
    PSPdispDPMSHandler dpms_handler; // Display Power Management Signaling Handler
    PSPdispEDID edid; // Identification given to the computer when connecting, from the PSP model, region and serial number (see `pspdisp_display_edid_new()`)

    // TODO: brightness handler
    // TODO: rotate handler

    GMainContext *context; // Context for the event loop, where the source of frames is dispatched
    GMainLoop *loop; // Event loop of frames
    GThread *thread; // Thread that runs the frames event loop
    GCancellable *thread_cancellable; // Cancellable used to finish the thread that runs the frames event loop
    GSource *frame_source; // Source of the display events, from the connector (see `pspdisp_display_connector_connect()`), attached to `context` (`NULL` while disconnected)
} PSPdispDisplay;

/**
 * @brief Quit the event loop of the display thread, called when `thread_cancellable` is cancelled.
 *
 * @param cancellable The cancelled `thread_cancellable`.
 * @param display The display that owns the thread.
 * @return Always `G_SOURCE_REMOVE`, it's only needed once.
 */
gboolean pspdisp_display_thread_cancel(GCancellable *cancellable, PSPdispDisplay *display) {
    (void) cancellable;
    g_main_loop_quit(display->loop);
    return G_SOURCE_REMOVE;
}

/**
 * @brief Body of the display thread: runs the event loop of `context` until the display is destroyed.
 *
 * Makes `context` the thread default context (so sources created on this thread use it),
 * and watches `thread_cancellable` to quit the loop when `pspdisp_display_destroy()` cancels it.
 *
 * @param display The display that owns the thread.
 * @return Always `NULL`, nothing is given to `g_thread_join()`.
 */
gpointer pspdisp_display_thread_func(PSPdispDisplay *display) {
    g_main_context_push_thread_default(display->context);

    GSource *cancellable_source = g_cancellable_source_new(display->thread_cancellable);
    g_source_set_callback(cancellable_source, G_SOURCE_FUNC(pspdisp_display_thread_cancel), display, NULL); // Called as `GCancellableSourceFunc`
    g_source_attach(cancellable_source, display->context);
    g_source_unref(cancellable_source);

    g_main_loop_run(display->loop); // blocks until quit

    g_main_context_pop_thread_default(display->context);
    return NULL;
}

/**
 * @brief Create a new display, not connected yet (see `pspdisp_display_connect()`).
 *
 * Starts the display thread, which runs the event loop where the connector calls the frame and DPMS handlers.
 *
 * The model, the region and the serial number give the EDID of the display (see `pspdisp_display_edid_new()`).
 *
 * @param model The PSP generation.
 * @param region The PSP sales region.
 * @param serial_number The serial number of the display, not 0 (see `pspdisp_display_edid_new()`).
 * @param frame_handler Handle each new frame, called on the display thread (can't be `NULL`).
 * @param dpms_handler Handle each DPMS change, called on the display thread (can't be `NULL`).
 * @return The new display, free it with `pspdisp_display_destroy()`, or `NULL` when a handler is `NULL`.
 */
PSPdispDisplay *pspdisp_display_new(PSPdispModel model, PSPdispRegion region, uint32_t serial_number, PSPdispFrameHandler frame_handler, PSPdispDPMSHandler dpms_handler) {
    g_return_val_if_fail(frame_handler != NULL, NULL);
    g_return_val_if_fail(dpms_handler != NULL, NULL);

    PSPdispDisplay *display = g_new(PSPdispDisplay, 1);
    display->frame_handler = frame_handler;
    display->dpms_handler = dpms_handler;
    display->edid = pspdisp_display_edid_new(model, region, serial_number);
    display->context = g_main_context_new();
    display->loop = g_main_loop_new(display->context, FALSE);
    display->thread_cancellable = g_cancellable_new();
    display->frame_source = NULL;
    display->thread = g_thread_new("pspdisp-display-frame-loop", (GThreadFunc) pspdisp_display_thread_func, display); // Last, the thread uses the fields above
    return display;
}

/**
 * @brief Connect the display, enabling it as a device: the connector creates the virtual display, so it appears on the computer.
 *
 * The source of the connector is attached to `context`, so the handlers run on the display thread.
 *
 * @param display The display to connect.
 * @return `true` if it was connected now, `false` if it was already connected or the connector failed.
 */
bool pspdisp_display_connect(PSPdispDisplay *display) {
    if (display->frame_source != NULL) return false; // Already connected

    GSource *frame_source = pspdisp_display_connector_connect(display->edid, display->frame_handler, display->dpms_handler);
    if (frame_source == NULL) return false; // The connector failed

    g_source_attach(frame_source, display->context);
    display->frame_source = frame_source;
    return true;
}

/**
 * @brief Disconnect the display, disabling it as a device: the virtual display is removed from the computer.
 *
 * Releasing the last reference to the source of the connector finalizes it, where the connector removes the virtual display.
 * It happens here, or on the display thread right after a dispatch that was still running there.
 *
 * @param display The display to disconnect.
 * @return `true` if it was disconnected now, `false` if it was already disconnected.
 */
bool pspdisp_display_disconnect(PSPdispDisplay *display) {
    if (display->frame_source == NULL) return false; // Already disconnected

    g_source_destroy(display->frame_source); // Never dispatched again (if the driver is gone, the connector already destroyed it)
    g_source_unref(display->frame_source); // Release the reference from `pspdisp_display_connect()`, the last one finalizes it
    display->frame_source = NULL;
    return true;
}

/**
 * @brief Destroy the display, freeing all resources and making it forever unusable.
 *
 * Disconnects it (if connected), then stops the display thread and waits for it to finish,
 * so it must not be called from the display thread (e.g. inside a handler), it would wait for itself.
 *
 * @param display The display to destroy.
 */
void pspdisp_display_destroy(PSPdispDisplay *display) {
    pspdisp_display_disconnect(display); // Just in case, it also releases the source of frames

    // Finish the event-loop worker thread
    g_cancellable_cancel(display->thread_cancellable);
    g_thread_join(display->thread);

    // Unref resources for cleanup
    g_main_loop_unref(display->loop);
    g_main_context_unref(display->context);
    g_object_unref(display->thread_cancellable);

    g_free(display);
}
