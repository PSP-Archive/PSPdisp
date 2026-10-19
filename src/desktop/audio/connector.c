/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * connector.c - Audio connector                     *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <stdint.h>
#include <glib.h>
#include "../../shared.h"

/**
 * @brief Type of a function pointer to handle a chunk of audio.
 *
 * Called by the connector on its own thread, with each full chunk.
 * The chunk is owned by the connector and only valid until the handler returns, so copy what must outlive the call.
 *
 * @param samples The samples, 16-bit signed, stereo and interleaved (left, right).
 * @param size The size of the chunk in bytes, the one asked on `pspdisp_audio_connector_connect()`.
 * @param data The data given with the function.
 */
typedef void (*PSPdispAudioHandler) (const int16_t *samples, uint32_t size, gpointer data);

/**
 * @brief Audio connector, declared by the connector of this platform with the data it needs.
 */
typedef struct PSPdispAudioConnector PSPdispAudioConnector;

/**
 * @brief Connect the PSP audio through the connector of this platform, so the sound for the PSP comes in chunks.
 *
 * There's one connector per platform, included below at compile time, which implements the functions below and defines `PSPDISP_AUDIO_CONNECTOR_NAME`.
 * Where the sound comes from, what the connector sets up on the computer, and where the user sees the name of the PSP depend on the implementation of each platform.
 * The sound is converted to 16-bit signed little-endian samples, stereo and interleaved (left, right), at the asked sample rate.
 *
 * @param model The PSP generation, with the region its name shown to the user (see `pspdisp_psp_name()`).
 * @param region The PSP sales region.
 * @param sample_rate The sample rate in Hz (e.g. 22050).
 * @param chunk_size The size of each chunk in bytes, a multiple of 4 (a stereo frame of 16-bit samples).
 * @param audio_handler Handle each full chunk.
 * @param data Given to the handler.
 * @return The connector, or `NULL` if it failed.
 */
PSPdispAudioConnector *pspdisp_audio_connector_connect(PSPdispModel model, PSPdispRegion region, uint32_t sample_rate, uint32_t chunk_size, PSPdispAudioHandler audio_handler, gpointer data);

/**
 * @brief Disconnect the PSP audio and free the connector, undoing what it set up on the computer.
 *
 * After it returns, the handler is never called again.
 * It must not be called from the handler, it waits for the connector thread.
 *
 * @param audio_connector The connector.
 */
void pspdisp_audio_connector_disconnect(PSPdispAudioConnector *audio_connector);

// The audio connector of this platform, it declares `PSPdispAudioConnector`, implements its functions and defines `PSPDISP_AUDIO_CONNECTOR_NAME`
#if defined(__linux__)
	#include "./connector/pipewire.c"
#else
	#error "Unsupported platform"
#endif
