/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * pulse.c - Audio capture using PulseAudio (Linux)  *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * Records the monitor of the default output (`@DEFAULT_MONITOR@`), so the sound the desktop plays, through the PulseAudio API.
 *
 * It works on PulseAudio and on PipeWire (through pipewire-pulse), which both resample and mix the sound to the asked format.
 */

#pragma once

#include <string.h>
#include <pulse/pulseaudio.h>
#include "../audio.c" // Its declarations (when `audio.c` includes this file, it already has them and this include is skipped)

#define PSPDISP_AUDIO_CAPTURE_NAME "PulseAudio" // User friendly name of this audio capture

/**
 * @brief Audio capture using PulseAudio, created by `pspdisp_audio_new()`.
 *
 * The PulseAudio callbacks run on the thread of its threaded main loop, where the chunks are filled and handed to the handler.
 */
struct PSPdispAudio {
    pa_threaded_mainloop *mainloop; // Main loop of the PulseAudio callbacks, on its own thread
    pa_context *context; // Connection to the sound server
    pa_stream *stream; // Recording stream of the monitor of the default output
    PSPdispAudioHandler handler; // Handle each full chunk
    uint8_t *chunk; // The chunk being filled
    uint32_t chunk_size; // The size of a chunk in bytes
    uint32_t chunk_length; // How many bytes of `chunk` are filled
};

/**
 * @brief Wake up `pspdisp_audio_new()`, waiting for the context or the stream to be ready, called by PulseAudio when their state changes.
 *
 * @param object The context or the stream, unused.
 * @param audio The capture.
 */
void pspdisp_audio_pulse_on_state(void *object, PSPdispAudio *audio) {
    (void) object;
    pa_threaded_mainloop_signal(audio->mainloop, 0);
}

/**
 * @brief Fill the chunks with the recorded samples, and hand each full one to the handler, called by PulseAudio when the stream has data.
 *
 * A hole in the stream (e.g. after an overflow) is filled with silence, so the chunks keep the pace of the sound.
 *
 * @param stream The recording stream.
 * @param length The number of bytes available, unused (the fragments are read until there's none).
 * @param audio The capture.
 */
void pspdisp_audio_pulse_on_read(pa_stream *stream, size_t length, PSPdispAudio *audio) {
    (void) length;
    const void *data;
    size_t size;

    while (pa_stream_peek(stream, &data, &size) == 0 && size > 0) {
        const uint8_t *bytes = data;
        size_t position = 0;

        while (position < size) {
            uint32_t count = MIN(size - position, audio->chunk_size - audio->chunk_length);

            if (bytes != NULL) memcpy(audio->chunk + audio->chunk_length, bytes + position, count);
            else memset(audio->chunk + audio->chunk_length, 0, count); // A hole, silence
            audio->chunk_length += count;
            position += count;

            if (audio->chunk_length == audio->chunk_size) {
                audio->handler((const int16_t *) audio->chunk, audio->chunk_size);
                audio->chunk_length = 0;
            }
        }

        pa_stream_drop(stream);
    }
}

/**
 * @brief Start capturing the monitor of the default output with PulseAudio.
 *
 * It connects to the sound server and waits until the recording stream is ready.
 *
 * @param sample_rate The sample rate in Hz (e.g. 22050).
 * @param chunk_size The size of each chunk in bytes, a multiple of 4 (a stereo frame of 16-bit samples).
 * @param handler Handle each full chunk, called on the thread of the PulseAudio main loop.
 * @return The new capture, or `NULL` if there's no sound server or the stream failed.
 */
PSPdispAudio *pspdisp_audio_new(uint32_t sample_rate, uint32_t chunk_size, PSPdispAudioHandler handler) {
    g_return_val_if_fail(chunk_size > 0 && chunk_size % 4 == 0, NULL);
    g_return_val_if_fail(handler != NULL, NULL);

    PSPdispAudio *audio = g_new0(PSPdispAudio, 1);
    audio->handler = handler;
    audio->chunk = g_malloc0(chunk_size);
    audio->chunk_size = chunk_size;
    audio->chunk_length = 0;
    audio->mainloop = pa_threaded_mainloop_new();
    audio->context = pa_context_new(pa_threaded_mainloop_get_api(audio->mainloop), "PSPdisp");

    bool ready = false;
    pa_context_set_state_callback(audio->context, (pa_context_notify_cb_t) pspdisp_audio_pulse_on_state, audio);
    pa_threaded_mainloop_start(audio->mainloop);
    pa_threaded_mainloop_lock(audio->mainloop);

    do {
        // Connect to the sound server, without starting one
        if (pa_context_connect(audio->context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) break;

        pa_context_state_t context_state;
        while ((context_state = pa_context_get_state(audio->context)) != PA_CONTEXT_READY) {
            if (!PA_CONTEXT_IS_GOOD(context_state)) break;
            pa_threaded_mainloop_wait(audio->mainloop);
        }
        if (context_state != PA_CONTEXT_READY) break;

        // Record the monitor of the default output, in the asked format, a chunk at a time
        pa_sample_spec spec = { .format = PA_SAMPLE_S16LE, .rate = sample_rate, .channels = 2 };
        pa_buffer_attr attributes = { .maxlength = (uint32_t) -1, .tlength = (uint32_t) -1, .prebuf = (uint32_t) -1, .minreq = (uint32_t) -1, .fragsize = chunk_size };

        audio->stream = pa_stream_new(audio->context, "Desktop sound for the PSP", &spec, NULL);
        if (audio->stream == NULL) break;

        pa_stream_set_state_callback(audio->stream, (pa_stream_notify_cb_t) pspdisp_audio_pulse_on_state, audio);
        pa_stream_set_read_callback(audio->stream, (pa_stream_request_cb_t) pspdisp_audio_pulse_on_read, audio);
        if (pa_stream_connect_record(audio->stream, "@DEFAULT_MONITOR@", &attributes, PA_STREAM_ADJUST_LATENCY) < 0) break;

        pa_stream_state_t stream_state;
        while ((stream_state = pa_stream_get_state(audio->stream)) != PA_STREAM_READY) {
            if (!PA_STREAM_IS_GOOD(stream_state)) break;
            pa_threaded_mainloop_wait(audio->mainloop);
        }
        ready = stream_state == PA_STREAM_READY;
    } while (false);

    if (!ready) g_warning("%s(): failed to record the desktop sound: %s", __FUNCTION__, pa_strerror(pa_context_errno(audio->context)));
    pa_threaded_mainloop_unlock(audio->mainloop);

    if (!ready) {
        pspdisp_audio_destroy(audio);
        return NULL;
    }

    return audio;
}

/**
 * @brief Stop capturing and free the capture.
 *
 * Stopping the threaded main loop waits for its thread, so the handler is never called again.
 *
 * @param audio The capture to destroy.
 */
void pspdisp_audio_destroy(PSPdispAudio *audio) {
    pa_threaded_mainloop_stop(audio->mainloop);

    if (audio->stream != NULL) {
        pa_stream_disconnect(audio->stream);
        pa_stream_unref(audio->stream);
    }
    pa_context_disconnect(audio->context);
    pa_context_unref(audio->context);
    pa_threaded_mainloop_free(audio->mainloop);

    g_free(audio->chunk);
    g_free(audio);
}
