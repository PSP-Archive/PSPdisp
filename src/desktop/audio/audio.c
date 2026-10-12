/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * audio.c - Capture of the desktop sound            *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The audio capture records the sound the desktop plays (not a microphone), in chunks to be sent to the PSP.
 *
 * Its implementation varies with the platform, chosen at compile time: PulseAudio on Linux (`capture/pulse.c`), which also works on PipeWire through pipewire-pulse, and WASAPI on Windows (`capture/wasapi.c`).
 * The samples are always 16-bit signed little-endian, stereo and interleaved (left, right), at the asked sample rate.
 */

#pragma once

#include <stdint.h>
#include <glib.h>

/**
 * @brief Type of a function pointer to handle a chunk of audio.
 *
 * Called by the audio capture on its own thread, with each full chunk.
 * The chunk is owned by the capture and only valid until the handler returns, so copy what must outlive the call.
 *
 * @param samples The samples, 16-bit signed, stereo and interleaved (left, right).
 * @param size The size of the chunk in bytes, the one asked on `pspdisp_audio_new()`.
 */
typedef void (*PSPdispAudioHandler) (const int16_t *samples, uint32_t size);

/**
 * @brief Capture of the sound the desktop plays.
 *
 * Its platform implementation (included below, e.g. `capture/pulse.c`) declares this struct with the data it needs, and defines `PSPDISP_AUDIO_CAPTURE_NAME`.
 */
typedef struct PSPdispAudio PSPdispAudio;

/**
 * @brief Start capturing the sound the desktop plays, implemented by the platform.
 *
 * The platform resamples the sound to the asked sample rate, and mixes it to stereo.
 *
 * @param sample_rate The sample rate in Hz (e.g. 22050).
 * @param chunk_size The size of each chunk in bytes, a multiple of 4 (a stereo frame of 16-bit samples).
 * @param handler Handle each full chunk, called on the capture thread.
 * @return The new capture, free it with `pspdisp_audio_destroy()`, or `NULL` if it failed (e.g. there's no sound server).
 */
PSPdispAudio *pspdisp_audio_new(uint32_t sample_rate, uint32_t chunk_size, PSPdispAudioHandler handler);

/**
 * @brief Stop capturing and free the capture, implemented by the platform.
 *
 * After it returns, the handler is never called again.
 * It must not be called from the handler, it waits for the capture thread.
 *
 * @param audio The capture to destroy.
 */
void pspdisp_audio_destroy(PSPdispAudio *audio);

// The audio capture of this platform, it declares `PSPdispAudio`, implements its functions and defines `PSPDISP_AUDIO_CAPTURE_NAME`
#if defined(__linux__)
    #include "./capture/pulse.c"
#else
    #error "Unsupported platform"
#endif
