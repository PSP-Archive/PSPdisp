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
 * @brief Type of a function pointer to handle a frame of the display, at the PSP resolution.
 *
 * Called on the display thread with each frame of the connector, scaled to the PSP resolution when the computer set a bigger mode (see `pspdisp_display_scale()`).
 *
 * The frame buffer always holds the full frame, the dirty rectangles only tell which areas changed since the previous frame.
 * It's owned by the display (or its connector) and only valid until the handler returns, so copy what must outlive the call.
 *
 * @param pixels A pointer to the frame buffer, `PSPDISP_DISPLAY_WIDTH` x `PSPDISP_DISPLAY_HEIGHT` pixels.
 * @param stride The number of pixels per row in the frame buffer, it's the PSP width plus padding.
 * @param format The pixel format of the frame buffer.
 * @param rectangles The dirty rectangles, the area that changed compared with the previous frame.
 * @param rectangles_length The number of dirty rectangles.
 * @param data The data given with the function.
 */
typedef void (*PSPdispFrameHandler) (void *pixels, uint16_t stride, PSPdispPixelFormat format, PSPdispDirtyRectangle *rectangles, uint8_t rectangles_length, gpointer data);

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
	uint32_t frame[PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT]; // The frame scaled to the PSP resolution, from the frames of a bigger mode (only used on the display thread)

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

	PSPdispDisplay *display = g_new0(PSPdispDisplay, 1); // Zeroed for `frame`, until the first frame of a bigger mode
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
 * @brief Scale the area of a frame under a dirty rectangle to the PSP resolution, each PSP pixel being the average of the pixels it covers.
 *
 * @param frame Where to write the scaled pixels, `PSPDISP_DISPLAY_WIDTH` x `PSPDISP_DISPLAY_HEIGHT` on `PSPDISP_PIXEL_FORMAT_8888`.
 * @param pixels The frame, on `PSPDISP_PIXEL_FORMAT_8888`.
 * @param width The width of the frame in pixels, not 0.
 * @param height The height of the frame in lines, not 0.
 * @param stride The number of pixels per row of the frame.
 * @param rectangle The dirty rectangle of the frame.
 * @return The dirty rectangle of the scaled frame.
 */
PSPdispDirtyRectangle pspdisp_display_scale(uint32_t *frame, const uint32_t *pixels, uint16_t width, uint16_t height, uint16_t stride, PSPdispDirtyRectangle rectangle) {
	// The PSP pixels the rectangle touches, rounded out
	const uint32_t x1 = MIN(rectangle.x1, width) * PSPDISP_DISPLAY_WIDTH / width;
	const uint32_t y1 = MIN(rectangle.y1, height) * PSPDISP_DISPLAY_HEIGHT / height;
	const uint32_t x2 = (MIN(rectangle.x2, width) * PSPDISP_DISPLAY_WIDTH + width - 1) / width;
	const uint32_t y2 = (MIN(rectangle.y2, height) * PSPDISP_DISPLAY_HEIGHT + height - 1) / height;

	for (uint32_t y = y1; y < y2; y++) {
		// The lines of the frame under this PSP line
		const uint32_t top = y * height / PSPDISP_DISPLAY_HEIGHT;
		const uint32_t bottom = ((y + 1) * height + PSPDISP_DISPLAY_HEIGHT - 1) / PSPDISP_DISPLAY_HEIGHT;
		uint32_t *row = frame + y * PSPDISP_DISPLAY_WIDTH;

		for (uint32_t x = x1; x < x2; x++) {
			// The columns of the frame under this PSP pixel, rounded out as the lines: k x k pixels of a mode k times the PSP size, overlapping ones of another size
			const uint32_t left = x * width / PSPDISP_DISPLAY_WIDTH;
			const uint32_t right = ((x + 1) * width + PSPDISP_DISPLAY_WIDTH - 1) / PSPDISP_DISPLAY_WIDTH;
			const uint32_t count = (bottom - top) * (right - left);
			uint32_t red = 0;
			uint32_t green = 0;
			uint32_t blue = 0;

			for (uint32_t source_y = top; source_y < bottom; source_y++) {
				const uint32_t *source_row = pixels + source_y * stride;

				for (uint32_t source_x = left; source_x < right; source_x++) {
					const uint32_t pixel = source_row[source_x]; // 0xAABBGGRR
					red += pixel & 0xFF;
					green += (pixel >> 8) & 0xFF;
					blue += (pixel >> 16) & 0xFF;
				}
			}

			// Rounded averages, opaque
			row[x] = 0xFF000000u | ((blue + count / 2) / count) << 16 | ((green + count / 2) / count) << 8 | (red + count / 2) / count;
		}
	}

	return (PSPdispDirtyRectangle) { .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2 };
}

static_assert(PSPDISP_DISPLAY_MAX_SCALE <= 16, "The sums of red and blue of `pspdisp_display_scale_by()` hold up to 16x16 pixels");

/**
 * @brief Scale the area of a frame under a dirty rectangle to the PSP resolution, for a frame of the PSP resolution scaled up a whole number of times, each PSP pixel being the average of the pixels it covers.
 *
 * Note: it's always inlined on purpose, to scale down faster than `pspdisp_display_scale()`: called with a constant scale, each copy is made for that scale (unrolled loops, shifts instead of divisions).
 *
 * @param frame Where to write the scaled pixels, `PSPDISP_DISPLAY_WIDTH` x `PSPDISP_DISPLAY_HEIGHT` on `PSPDISP_PIXEL_FORMAT_8888`.
 * @param pixels The frame, `scale` times the PSP resolution, on `PSPDISP_PIXEL_FORMAT_8888`.
 * @param stride The number of pixels per row of the frame.
 * @param rectangle The dirty rectangle of the frame.
 * @param scale How many times the PSP resolution the frame is, from 1 to 16.
 * @return The dirty rectangle of the scaled frame.
 */
inline __attribute__((always_inline)) PSPdispDirtyRectangle pspdisp_display_scale_by(uint32_t *frame, const uint32_t *pixels, uint16_t stride, PSPdispDirtyRectangle rectangle, uint32_t scale) {
	const uint32_t count = scale * scale;

	// The PSP pixels the rectangle touches, rounded out
	const uint32_t x1 = MIN(rectangle.x1, PSPDISP_DISPLAY_WIDTH * scale) / scale;
	const uint32_t y1 = MIN(rectangle.y1, PSPDISP_DISPLAY_HEIGHT * scale) / scale;
	const uint32_t x2 = (MIN(rectangle.x2, PSPDISP_DISPLAY_WIDTH * scale) + scale - 1) / scale;
	const uint32_t y2 = (MIN(rectangle.y2, PSPDISP_DISPLAY_HEIGHT * scale) + scale - 1) / scale;

	for (uint32_t y = y1; y < y2; y++) {
		const uint32_t *block_row = pixels + y * scale * stride; // The first line of the frame under this PSP line
		uint32_t *row = frame + y * PSPDISP_DISPLAY_WIDTH;

		for (uint32_t x = x1; x < x2; x++) {
			const uint32_t *block = block_row + x * scale;

			// Red and blue summed together, in 2 lanes of 16 bits (each holds 257 pixels of 255), and green apart
			uint32_t red_blue = 0;
			uint32_t green = 0;

			for (uint32_t i = 0; i < scale; i++) {
				const uint32_t *source_row = block + i * stride;

				for (uint32_t j = 0; j < scale; j++) {
					red_blue += source_row[j] & 0x00FF00FF; // 0xAABBGGRR
					green += source_row[j] & 0x0000FF00;
				}
			}

			const uint32_t red = red_blue & 0xFFFF;
			const uint32_t blue = red_blue >> 16;
			green >>= 8;

			// Rounded averages, opaque
			row[x] = 0xFF000000u | ((blue + count / 2) / count) << 16 | ((green + count / 2) / count) << 8 | (red + count / 2) / count;
		}
	}

	return (PSPdispDirtyRectangle) { .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2 };
}

/**
 * @brief Hand a frame of the connector to the frame handler, scaled to the PSP resolution when the mode is another one, called by the connector on the display thread.
 *
 * @param pixels The frame buffer.
 * @param width The width of the frame in pixels.
 * @param height The height of the frame in lines.
 * @param stride The number of pixels per row in the frame buffer.
 * @param format The pixel format of the frame buffer, only `PSPDISP_PIXEL_FORMAT_8888` is scaled.
 * @param rectangles The dirty rectangles.
 * @param rectangles_length The number of dirty rectangles.
 * @param display The display.
 */
void pspdisp_display_on_frame(void *pixels, uint16_t width, uint16_t height, uint16_t stride, PSPdispPixelFormat format, PSPdispDirtyRectangle *rectangles, uint8_t rectangles_length, PSPdispDisplay *display) {
	if (width == PSPDISP_DISPLAY_WIDTH && height == PSPDISP_DISPLAY_HEIGHT) {
		display->frame_handler(pixels, stride, format, rectangles, rectangles_length, display->handler_data);
		return;
	}

	if (width == 0 || height == 0) return;
	if (format != PSPDISP_PIXEL_FORMAT_8888) {
		g_warning("%s(): can't scale the pixel format %d, the frame is dropped", __FUNCTION__, format);
		return;
	}

	// The scaled up modes of the EDID with their own copy of `pspdisp_display_scale_by()`, made for their scale (see its note), the other sizes with `pspdisp_display_scale()`
	const uint32_t scale = width / PSPDISP_DISPLAY_WIDTH;
	const bool whole = width == PSPDISP_DISPLAY_WIDTH * scale && height == PSPDISP_DISPLAY_HEIGHT * scale;
	PSPdispDirtyRectangle scaled[UINT8_MAX];

	for (uint8_t i = 0; i < rectangles_length; i++) {
		switch (whole ? scale : 0) {
			case 2:
				scaled[i] = pspdisp_display_scale_by(display->frame, pixels, stride, rectangles[i], 2);
				break;
			case 4:
				scaled[i] = pspdisp_display_scale_by(display->frame, pixels, stride, rectangles[i], 4);
				break;
			default:
				// Only because the EDID announces its modes without enforcing them: the computer may still set another one (e.g. a custom mode of xrandr or KWin)
				scaled[i] = pspdisp_display_scale(display->frame, pixels, width, height, stride, rectangles[i]);
				break;
		}
	}

	display->frame_handler(display->frame, PSPDISP_DISPLAY_WIDTH, PSPDISP_PIXEL_FORMAT_8888, scaled, rectangles_length, display->handler_data);
}

/**
 * @brief Hand a DPMS change to the DPMS handler, called by the connector on the display thread.
 *
 * @param mode The DPMS mode.
 * @param display The display.
 */
void pspdisp_display_on_dpms(PSPdispDPMSMode mode, PSPdispDisplay *display) {
	display->dpms_handler(mode, display->handler_data);
}

/**
 * @brief Tell the lost handler that the virtual display is gone, called by the connector on the display thread.
 *
 * @param display The display.
 */
void pspdisp_display_on_lost(PSPdispDisplay *display) {
	display->lost_handler(display->handler_data);
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

	GSource *frame_source = pspdisp_display_connector_connect(display->edid, (PSPdispConnectorFrameHandler) pspdisp_display_on_frame, (PSPdispDPMSHandler) pspdisp_display_on_dpms, (PSPdispLostHandler) pspdisp_display_on_lost, display);
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
