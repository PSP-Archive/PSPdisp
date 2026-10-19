/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * audio.c - PSP audio and its connector             *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <stdint.h>
#include <glib.h>
#include "./connector.c"

/**
 * @brief Audio interface whose sound is going to be sent to PSP.
 *
 * While it's connected, the sound for the PSP comes to its handler in chunks.
 * Where that sound comes from depends on the implementation of the connector of each platform.
 */
typedef struct {
	PSPdispModel model; // PSP generation, with the region its name shown to the user (see `pspdisp_psp_name()`)
	PSPdispRegion region; // PSP sales region
	PSPdispAudioHandler audio_handler; // Handle each full chunk
	gpointer handler_data; // Given to `audio_handler`
	uint32_t sample_rate; // Sample rate of the chunks in Hz
	uint32_t chunk_size; // Size of each chunk in bytes
	PSPdispAudioConnector *connector; // The connector (`NULL` while disconnected)
} PSPdispAudio;

/**
 * @brief Create a new audio, not connected yet (see `pspdisp_audio_connect()`).
 *
 * @param model The PSP generation.
 * @param region The PSP sales region.
 * @param sample_rate The sample rate in Hz (e.g. 22050).
 * @param chunk_size The size of each chunk in bytes, a multiple of 4 (a stereo frame of 16-bit samples).
 * @param audio_handler Handle each full chunk, called on the connector thread (can't be `NULL`).
 * @param data Given to the handler.
 * @return The new audio, free it with `pspdisp_audio_destroy()`, or `NULL` if an argument is invalid.
 */
PSPdispAudio *pspdisp_audio_new(PSPdispModel model, PSPdispRegion region, uint32_t sample_rate, uint32_t chunk_size, PSPdispAudioHandler audio_handler, gpointer data) {
	g_return_val_if_fail(chunk_size > 0 && chunk_size % 4 == 0, NULL);
	g_return_val_if_fail(audio_handler != NULL, NULL);

	PSPdispAudio *audio = g_new(PSPdispAudio, 1);
	audio->model = model;
	audio->region = region;
	audio->audio_handler = audio_handler;
	audio->handler_data = data;
	audio->sample_rate = sample_rate;
	audio->chunk_size = chunk_size;
	audio->connector = NULL;
	return audio;
}

/**
 * @brief Connect the audio, enabling it as a device: the connector starts handing the chunks of the sound for the PSP.
 *
 * @param audio The audio to connect.
 * @return `true` if it was connected now, `false` if it was already connected or it failed.
 */
bool pspdisp_audio_connect(PSPdispAudio *audio) {
	if (audio->connector != NULL) return false; // Already connected

	audio->connector = pspdisp_audio_connector_connect(audio->model, audio->region, audio->sample_rate, audio->chunk_size, audio->audio_handler, audio->handler_data);
	return audio->connector != NULL;
}

/**
 * @brief Disconnect the audio, disabling it as a device: no chunk comes after it.
 *
 * The connector undoes what it set up on the computer, which depends on the implementation of each platform.
 * It must not be called from the handler, it waits for the connector thread.
 *
 * @param audio The audio to disconnect.
 * @return `true` if it was disconnected now, `false` if it was already disconnected.
 */
bool pspdisp_audio_disconnect(PSPdispAudio *audio) {
	if (audio->connector == NULL) return false; // Already disconnected

	pspdisp_audio_connector_disconnect(audio->connector);
	audio->connector = NULL;
	return true;
}

/**
 * @brief Destroy the audio, freeing all resources and making it forever unusable.
 *
 * Disconnects it (if connected), so it must not be called from the handler either.
 *
 * @param audio The audio to destroy.
 */
void pspdisp_audio_destroy(PSPdispAudio *audio) {
	pspdisp_audio_disconnect(audio);

	g_free(audio);
}
