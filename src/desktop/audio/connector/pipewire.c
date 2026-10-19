/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * pipewire.c - Audio connector on PipeWire (Linux)  *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * Connects the PSP audio through the PipeWire API: it adds a sound output named after the PSP (e.g. "PSP-3001").
 *
 * The output is the stream of the connector itself, shown by PipeWire as a sink (`Audio/Sink`), so the apps play straight into it.
 * The user chooses it in the sound settings for the whole desktop or for some apps, with its own volume and mute, which PipeWire applies before handing the samples.
 * It exists as long as the stream, so it's there while a PSP gets the sound, as the output of a monitor plugged in, and it goes away with the daemon even when it crashes.
 * PipeWire only hands samples while something plays on it, the PSP gets silence otherwise.
 * The name of the PSP is the description of the output, shown to the user (e.g. in pavucontrol), while its node name stays the same, so the volume and the mute the session manager keeps for it follow every PSP.
 *
 * PipeWire resamples and mixes the sound to the asked format.
 */

#pragma once

#include <string.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include "../connector.c" // Its declarations (when `connector.c` includes this file, it already has them and this include is skipped)

#define PSPDISP_AUDIO_CONNECTOR_NAME "PipeWire" // User friendly name of this connector

#define PSPDISP_PIPEWIRE_SINK "pspdisp" // Node name of the output, whatever the name of the PSP

/**
 * @brief Audio connector using PipeWire, returned by `pspdisp_audio_connector_connect()`.
 *
 * The stream events run on the thread of its thread loop, where the chunks are filled and handed to the handler.
 */
struct PSPdispAudioConnector {
	struct pw_thread_loop *loop; // Loop of the stream events, on its own thread
	struct pw_stream *stream; // The output, the stream shown as a sink
	PSPdispAudioHandler audio_handler; // Handle each full chunk
	gpointer handler_data; // Given to `audio_handler`
	uint8_t *chunk; // The chunk being filled
	uint32_t chunk_size; // The size of a chunk in bytes
	uint32_t chunk_length; // How many bytes of `chunk` are filled
};

/**
 * @brief Fill the chunks with recorded samples, and hand each full one to the handler.
 *
 * @param audio_connector The connector.
 * @param data The samples, or `NULL` for silence.
 * @param size The size of the samples in bytes.
 */
void pspdisp_audio_connector_pipewire_push(PSPdispAudioConnector *audio_connector, const uint8_t *data, uint32_t size) {
	uint32_t position = 0;

	while (position < size) {
		uint32_t count = MIN(size - position, audio_connector->chunk_size - audio_connector->chunk_length);
		uint8_t *end = audio_connector->chunk + audio_connector->chunk_length; // Where the chunk goes on

		if (data != NULL)
			memcpy(end, data + position, count);
		else
			memset(end, 0, count);
		audio_connector->chunk_length += count;
		position += count;

		if (audio_connector->chunk_length == audio_connector->chunk_size) {
			audio_connector->audio_handler((const int16_t *) audio_connector->chunk, audio_connector->chunk_size, audio_connector->handler_data);
			audio_connector->chunk_length = 0;
		}
	}
}

/**
 * @brief Wake up the wait for the stream to connect, and hand the end of the sound when it stops, called by PipeWire when the state of the stream changes.
 *
 * @param data The connector.
 * @param old The previous state.
 * @param state The new state.
 * @param error Why it failed, with `PW_STREAM_STATE_ERROR`.
 */
void pspdisp_audio_connector_pipewire_on_state_changed(void *data, enum pw_stream_state old, enum pw_stream_state state, const char *error) {
	PSPdispAudioConnector *audio_connector = data;

	// The chunk being filled would otherwise start the next sound, so it's completed with silence
	if (old == PW_STREAM_STATE_STREAMING && audio_connector->chunk_length > 0)
		pspdisp_audio_connector_pipewire_push(audio_connector, NULL, audio_connector->chunk_size - audio_connector->chunk_length);

	if (state == PW_STREAM_STATE_ERROR && old != PW_STREAM_STATE_CONNECTING)
		g_warning("%s(): the sound for the PSP stopped: %s", __FUNCTION__, error);
	pw_thread_loop_signal(audio_connector->loop, false);
}

/**
 * @brief Fill the chunks with the samples of a buffer, called by PipeWire when the stream has one.
 *
 * @param data The connector.
 */
void pspdisp_audio_connector_pipewire_on_process(void *data) {
	PSPdispAudioConnector *audio_connector = data;
	struct pw_buffer *buffer = pw_stream_dequeue_buffer(audio_connector->stream);
	if (buffer == NULL) return;

	struct spa_data *samples = &buffer->buffer->datas[0];
	if (samples->data != NULL && samples->chunk != NULL) {
		uint32_t offset = MIN(samples->chunk->offset, samples->maxsize);
		uint32_t size = MIN(samples->chunk->size, samples->maxsize - offset);
		bool empty = (samples->chunk->flags & SPA_CHUNK_FLAG_EMPTY) != 0; // Silence, the samples may hold anything

		pspdisp_audio_connector_pipewire_push(audio_connector, empty ? NULL : SPA_PTROFF(samples->data, offset, uint8_t), size);
	}

	pw_stream_queue_buffer(audio_connector->stream, buffer);
}

const struct pw_stream_events pspdisp_audio_connector_pipewire_events = {
	PW_VERSION_STREAM_EVENTS,
	.state_changed = pspdisp_audio_connector_pipewire_on_state_changed,
	.process = pspdisp_audio_connector_pipewire_on_process,
};

/**
 * @brief Connect the PSP audio with PipeWire: add a sound output named after the PSP.
 *
 * It connects to PipeWire and waits until the stream is connected.
 *
 * @param model The PSP generation, with the region the description of the output (see `pspdisp_psp_name()`).
 * @param region The PSP sales region.
 * @param sample_rate The sample rate in Hz (e.g. 22050).
 * @param chunk_size The size of each chunk in bytes, a multiple of 4 (a stereo frame of 16-bit samples).
 * @param audio_handler Handle each full chunk, called on the thread of the PipeWire thread loop.
 * @param data Given to the handler.
 * @return The connector, or `NULL` if it failed.
 */
PSPdispAudioConnector *pspdisp_audio_connector_connect(PSPdispModel model, PSPdispRegion region, uint32_t sample_rate, uint32_t chunk_size, PSPdispAudioHandler audio_handler, gpointer data) {
	const char *name = pspdisp_psp_name(model, region);
	pw_init(NULL, NULL);

	PSPdispAudioConnector *audio_connector = g_new0(PSPdispAudioConnector, 1);
	audio_connector->audio_handler = audio_handler;
	audio_connector->handler_data = data;
	audio_connector->chunk = g_malloc0(chunk_size);
	audio_connector->chunk_size = chunk_size;
	audio_connector->chunk_length = 0;
	audio_connector->loop = pw_thread_loop_new("pspdisp-audio", NULL);

	// The stream itself is the output, shown as a sink
	struct pw_properties *properties = pw_properties_new(
		PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture", PW_KEY_MEDIA_CLASS, "Audio/Sink", PW_KEY_APP_NAME, "PSPdisp",
		PW_KEY_NODE_NAME, PSPDISP_PIPEWIRE_SINK, PW_KEY_NODE_DESCRIPTION, name, NULL);

	// 16-bit stereo at the asked sample rate, PipeWire converts the sound to it
	uint8_t pod[1024];
	struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(pod, sizeof(pod));
	const struct spa_pod *format = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_S16_LE, .rate = sample_rate, .channels = 2, .position = { SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR }));

	bool ready = false;
	const char *error = NULL;
	pw_thread_loop_start(audio_connector->loop);
	pw_thread_loop_lock(audio_connector->loop);

	// Not linked to anything by the session manager (no `PW_STREAM_FLAG_AUTOCONNECT`), the apps are linked to it
	audio_connector->stream = pw_stream_new_simple(pw_thread_loop_get_loop(audio_connector->loop), name, properties, &pspdisp_audio_connector_pipewire_events, audio_connector);

	if (audio_connector->stream != NULL && pw_stream_connect(audio_connector->stream, PW_DIRECTION_INPUT, PW_ID_ANY, PW_STREAM_FLAG_MAP_BUFFERS, &format, 1) >= 0) {
		enum pw_stream_state state;
		while ((state = pw_stream_get_state(audio_connector->stream, &error)) == PW_STREAM_STATE_CONNECTING) pw_thread_loop_wait(audio_connector->loop);
		ready = state == PW_STREAM_STATE_PAUSED || state == PW_STREAM_STATE_STREAMING;
	}

	if (!ready)
		g_warning("%s(): failed to add the \"%s\" sound output: %s", __FUNCTION__, name, error != NULL ? error : "PipeWire doesn't run");
	else
		g_message("Added the \"%s\" sound output, choose it in the sound settings to play on the PSP", name);
	pw_thread_loop_unlock(audio_connector->loop);

	if (!ready) {
		pspdisp_audio_connector_disconnect(audio_connector);
		return NULL;
	}

	return audio_connector;
}

/**
 * @brief Disconnect the PSP audio: remove its output, and free the connector.
 *
 * Stopping the thread loop waits for its thread, so the handler is never called again, except with the end of the sound while the stream is destroyed.
 *
 * @param audio_connector The connector.
 */
void pspdisp_audio_connector_disconnect(PSPdispAudioConnector *audio_connector) {
	pw_thread_loop_stop(audio_connector->loop);

	// The sound played on the output moves to another output
	if (audio_connector->stream != NULL)
		pw_stream_destroy(audio_connector->stream);
	pw_thread_loop_destroy(audio_connector->loop);
	pw_deinit();

	g_free(audio_connector->chunk);
	g_free(audio_connector);
}
