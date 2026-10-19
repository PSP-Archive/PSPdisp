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
	PSPdispLostHandler lost_handler; // Handle the loss of the virtual display, when its driver is gone
	gpointer handler_data; // Given to the handlers
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
 * @param lost_handler Handle the loss of the virtual display, called on the display thread (can't be `NULL`).
 * @param data Given to the handlers.
 * @return The new display, free it with `pspdisp_display_destroy()`, or `NULL` if an argument is invalid.
 */
PSPdispDisplay *pspdisp_display_new(PSPdispModel model, PSPdispRegion region, uint32_t serial_number, PSPdispFrameHandler frame_handler, PSPdispDPMSHandler dpms_handler, PSPdispLostHandler lost_handler, gpointer data) {
	g_return_val_if_fail(frame_handler != NULL, NULL);
	g_return_val_if_fail(dpms_handler != NULL, NULL);
	g_return_val_if_fail(lost_handler != NULL, NULL);

	PSPdispDisplay *display = g_new(PSPdispDisplay, 1);
	display->frame_handler = frame_handler;
	display->dpms_handler = dpms_handler;
	display->lost_handler = lost_handler;
	display->handler_data = data;
	display->edid = pspdisp_display_edid_new(model, region, serial_number);
	display->context = g_main_context_new();
	display->loop = g_main_loop_new(display->context, FALSE);
	display->thread_cancellable = g_cancellable_new();
	display->frame_source = NULL;
	display->thread = g_thread_new("pspdisp-display-frame-loop", (GThreadFunc) pspdisp_display_thread_func, display); // Last, the thread uses the fields above
	return display;
}

/**
 * @brief A source of the connector to release on the display thread, and the wait for it (see `pspdisp_display_release()`).
 */
typedef struct {
	GSource *source; // The source to destroy and release
	GMutex mutex; // Guards `released`
	GCond cond; // Signaled once the source is released
	bool released; // If the source is released
} PSPdispDisplayRelease;

/**
 * @brief Destroy and release a source of the connector, called on the display thread.
 *
 * @param release The source, and the wait for it.
 * @return Always `G_SOURCE_REMOVE`, it's done once.
 */
gboolean pspdisp_display_on_release(PSPdispDisplayRelease *release) {
	g_source_destroy(release->source); // Never dispatched again (if the driver is gone, the connector already destroyed it)
	g_source_unref(release->source); // The last reference finalizes it, where the connector removes the virtual display

	g_mutex_lock(&release->mutex);
	release->released = true;
	g_cond_signal(&release->cond);
	g_mutex_unlock(&release->mutex);
	return G_SOURCE_REMOVE;
}

/**
 * @brief Destroy and release a source of the connector on the display thread, and wait until it's done.
 *
 * Finalized on the display thread, the source never closes what that thread waits on (e.g. the file descriptor of EVDI) while it waits.
 *
 * @param display The display.
 * @param source The source of the connector, its reference is taken.
 */
void pspdisp_display_release(PSPdispDisplay *display, GSource *source) {
	PSPdispDisplayRelease release = { .source = source };
	g_mutex_init(&release.mutex);
	g_cond_init(&release.cond);

	// Called at once on the display thread (e.g. from a handler), otherwise queued there, between two waits
	g_main_context_invoke(display->context, G_SOURCE_FUNC(pspdisp_display_on_release), &release);

	g_mutex_lock(&release.mutex);
	while (!release.released)
		g_cond_wait(&release.cond, &release.mutex);
	g_mutex_unlock(&release.mutex);

	g_cond_clear(&release.cond);
	g_mutex_clear(&release.mutex);
}

/**
 * @brief Connect the display, enabling it as a device: the connector creates the virtual display, so it appears on the computer.
 *
 * The source of the connector is attached to `context`, so the handlers run on the display thread.
 * After the loss of the virtual display (see `PSPdispLostHandler`), it connects again.
 *
 * @param display The display to connect.
 * @return `true` if it was connected now, `false` if it was already connected or it failed.
 */
bool pspdisp_display_connect(PSPdispDisplay *display) {
	// A source that destroyed itself (its driver is gone) is released first, which removes the virtual display
	if (display->frame_source != NULL && g_source_is_destroyed(display->frame_source))
		pspdisp_display_release(display, g_steal_pointer(&display->frame_source));
	if (display->frame_source != NULL) return false; // Already connected

	GSource *frame_source = pspdisp_display_connector_connect(display->edid, display->frame_handler, display->dpms_handler, display->lost_handler, display->handler_data);
	if (frame_source == NULL) return false; // The connector failed

	g_source_attach(frame_source, display->context);
	display->frame_source = frame_source;
	return true;
}

/**
 * @brief Disconnect the display, disabling it as a device: the virtual display is removed from the computer.
 *
 * The source of the connector is destroyed and released on the display thread, where the connector removes the virtual display, and it waits until it's done.
 * It may be called from a handler, on the display thread.
 *
 * @param display The display to disconnect.
 * @return `true` if it was disconnected now, `false` if it was already disconnected.
 */
bool pspdisp_display_disconnect(PSPdispDisplay *display) {
	if (display->frame_source == NULL) return false; // Already disconnected

	pspdisp_display_release(display, g_steal_pointer(&display->frame_source));
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
