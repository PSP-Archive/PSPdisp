/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * media.c - Frames and sound waiting to be sent     *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The media the daemon sends to the PSP: the frames of the virtual display and the chunks of the desktop sound.
 *
 * The display and the audio connectors call the handlers below on their own threads, and the session takes the media on the thread serving the PSP (USB or WLAN), so everything here is guarded.
 */

#pragma once

#include <string.h>
#include <glib.h>
#include "../../shared.h"

#define PSPDISP_DAEMON_MEDIA_MAX_AUDIO_CHUNKS 4 // Max chunks of audio waiting to be sent, the oldest ones are dropped to bound the latency

/**
 * @brief The media waiting to be sent to the PSP.
 */
typedef struct {
	GMutex mutex; // Guards the frame, its count and the power mode
	uint32_t frame[PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT]; // The latest frame, on `PSPDISP_PIXEL_FORMAT_8888`
	uint64_t frame_count; // Incremented on each change of the frame or of the power mode
	bool display_on; // If the computer has the display turned on (DPMS)
	GAsyncQueue *audio_chunks; // Chunks of audio waiting to be sent (`GBytes`), oldest first
} PSPdispDaemonMedia;

/**
 * @brief Create the media, with a black frame and the display turned on.
 *
 * @return The media, free it with `pspdisp_daemon_media_destroy()`.
 */
PSPdispDaemonMedia *pspdisp_daemon_media_new() {
	PSPdispDaemonMedia *media = g_new0(PSPdispDaemonMedia, 1);
	g_mutex_init(&media->mutex);
	media->display_on = true;
	media->audio_chunks = g_async_queue_new_full((GDestroyNotify) g_bytes_unref);
	return media;
}

/**
 * @brief Free the media, once no handler and no session uses it anymore.
 *
 * @param media The media.
 */
void pspdisp_daemon_media_destroy(PSPdispDaemonMedia *media) {
	g_async_queue_unref(media->audio_chunks);
	g_mutex_clear(&media->mutex);
	g_free(media);
}

/**
 * @brief Drop the chunks of audio waiting to be sent, e.g. after the sample rate changed.
 *
 * @param media The media.
 */
void pspdisp_daemon_media_drop_audio(PSPdispDaemonMedia *media) {
	GBytes *chunk;
	while ((chunk = g_async_queue_try_pop(media->audio_chunks)) != NULL) g_bytes_unref(chunk);
}

/**
 * @brief Start over for a new connection: a black frame, the display turned on and no audio waiting.
 *
 * @param media The media.
 */
void pspdisp_daemon_media_reset(PSPdispDaemonMedia *media) {
	g_mutex_lock(&media->mutex);
	memset(media->frame, 0, sizeof(media->frame));
	media->frame_count = 0;
	media->display_on = true;
	g_mutex_unlock(&media->mutex);

	pspdisp_daemon_media_drop_audio(media);
}

/**
 * @brief Copy the dirty rectangles of a frame of the virtual display, the frame handler of the display (see `PSPdispFrameHandler`).
 *
 * @param pixels The frame buffer.
 * @param stride The number of pixels per row in the frame buffer.
 * @param format The pixel format of the frame buffer, only `PSPDISP_PIXEL_FORMAT_8888` is used.
 * @param rectangles The dirty rectangles.
 * @param rectangles_length The number of dirty rectangles.
 * @param media The media.
 */
void pspdisp_daemon_media_on_frame(void *pixels, uint16_t stride, PSPdispPixelFormat format, PSPdispDirtyRectangle *rectangles, uint8_t rectangles_length, PSPdispDaemonMedia *media) {
	if (format != PSPDISP_PIXEL_FORMAT_8888) {
		g_warning("%s(): unsupported pixel format %d", __FUNCTION__, format);
		return;
	}

	const uint32_t *source = pixels;

	g_mutex_lock(&media->mutex);
	for (uint8_t i = 0; i < rectangles_length; i++) {
		uint16_t x1 = MIN(rectangles[i].x1, PSPDISP_DISPLAY_WIDTH);
		uint16_t y1 = MIN(rectangles[i].y1, PSPDISP_DISPLAY_HEIGHT);
		uint16_t x2 = MIN(rectangles[i].x2, PSPDISP_DISPLAY_WIDTH);
		uint16_t y2 = MIN(rectangles[i].y2, PSPDISP_DISPLAY_HEIGHT);

		for (uint16_t y = y1; y < y2 && x1 < x2; y++) {
			memcpy(&media->frame[y * PSPDISP_DISPLAY_WIDTH + x1], &source[y * stride + x1], (x2 - x1) * sizeof(uint32_t));
		}
	}
	if (rectangles_length > 0)
		media->frame_count++;
	g_mutex_unlock(&media->mutex);
}

/**
 * @brief Keep the power mode of the virtual display, the DPMS handler of the display (see `PSPdispDPMSHandler`).
 *
 * @param mode The DPMS mode, only `PSPDISP_DPMS_MODE_ON` shows the frames.
 * @param media The media.
 */
void pspdisp_daemon_media_on_dpms(PSPdispDPMSMode mode, PSPdispDaemonMedia *media) {
	g_mutex_lock(&media->mutex);
	media->display_on = mode == PSPDISP_DPMS_MODE_ON;
	media->frame_count++;
	g_mutex_unlock(&media->mutex);
}

/**
 * @brief Queue a chunk of the sound for the PSP, the handler of the PSP audio (see `PSPdispAudioHandler`).
 *
 * When too many chunks wait, the oldest ones are dropped, so the sound never lags much behind.
 *
 * @param samples The samples of the chunk.
 * @param size The size of the chunk in bytes.
 * @param media The media.
 */
void pspdisp_daemon_media_on_audio(const int16_t *samples, uint32_t size, PSPdispDaemonMedia *media) {
	GAsyncQueue *queue = media->audio_chunks;

	g_async_queue_lock(queue);
	while (g_async_queue_length_unlocked(queue) >= PSPDISP_DAEMON_MEDIA_MAX_AUDIO_CHUNKS) g_bytes_unref(g_async_queue_pop_unlocked(queue));
	g_async_queue_push_unlocked(queue, g_bytes_new(samples, size));
	g_async_queue_unlock(queue);
}

/**
 * @brief Copy the latest frame, if it changed.
 *
 * @param media The media.
 * @param frame_count The count of the frame the caller has, updated when the frame is copied.
 * @param frame Where to copy the frame, `PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT` pixels.
 * @param display_on Where to copy if the computer has the display turned on.
 * @return `true` if the frame or the power mode changed since `frame_count` (and they were copied), `false` otherwise.
 */
bool pspdisp_daemon_media_take_frame(PSPdispDaemonMedia *media, uint64_t *frame_count, uint32_t *frame, bool *display_on) {
	bool changed = false;

	g_mutex_lock(&media->mutex);
	if (media->frame_count != *frame_count) {
		memcpy(frame, media->frame, sizeof(media->frame));
		*frame_count = media->frame_count;
		*display_on = media->display_on;
		changed = true;
	}
	g_mutex_unlock(&media->mutex);

	return changed;
}

/**
 * @brief Take the oldest chunk of audio waiting to be sent, waiting for one if needed.
 *
 * @param media The media.
 * @param timeout How long to wait for a chunk, in microseconds.
 * @return The chunk, free it with `g_bytes_unref()`, or `NULL` if none came in time.
 */
GBytes *pspdisp_daemon_media_take_audio(PSPdispDaemonMedia *media, guint64 timeout) {
	return g_async_queue_timeout_pop(media->audio_chunks, timeout);
}
