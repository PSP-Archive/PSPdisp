/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * session.c - Exchange of frames with the PSP       *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The session with a connected PSP, over the old protocol (see `shared.old.h`): for each frame, the daemon sends a frame header and its data, and the PSP answers with its controls and its settings.
 * Over USB the PSP sends its settings in every response, over WLAN only when they changed (see `PSPdispDaemonLink.com_settings_always`).
 *
 * With sound, every frame carries a chunk of audio, because the PSP app resets its sound when a frame comes without one, so the audio chunks give the pace of the frames.
 * Without sound, the frames go as fast as the PSP takes them (it waits for its vertical blank between two frames), then the update interval of the settings.
 * An image is only sent when the frame of the virtual display changed (or the PSP asks for one), the other frames carry the sound and the communication settings only.
 *
 * The streaming settings start as saved (see `settings/settings.c`), the PSP changes them on its "PC Settings" menu (or applies a preset), and they're saved again, so the desktop app shows them.
 * When they change on the disk (e.g. from the desktop app), the session takes them before the next frame (see `snapshot.c`).
 * What the virtual display can't do is turned back to its default and sent back, so the PSP menu shows what is used: the screen modes and viewports (the virtual display is already the size of the PSP), the SideShow mode and the autoselection of the display (there's only one).
 *
 * The PSP controls drive what the settings choose: the virtual gamepad, a control file (a virtual keyboard and mouse), or nothing (see `controller/controller.c`).
 */

#pragma once

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include "../../shared.h"
#include "../settings/settings.c"
#include "../controller/controller.c"
#include "./config.c"
#include "./media.c"
#include "./image.c"
#include "./snapshot.c"
#include "./state.c"

#define PSPDISP_DAEMON_SESSION_READ_TIMEOUT 2000 // Time to wait for a response of the PSP, in milliseconds
#define PSPDISP_DAEMON_SESSION_DATA_CAPACITY (400 * 1024) // Max size of the data of a frame, the size of the PSP receive buffer (`COM_IMAGE_BUFFER_SIZE` of the PSP app)

static_assert(PSPDISP_DAEMON_IMAGE_UNCOMPRESSED_SIZE + 2688 * 2 + sizeof(PSPdispComSettings) + PSPDISP_COM_NAME_SIZE * (PSPDISP_COM_MAX_DISPLAY_DEVICES + PSPDISP_COM_MAX_CONTROLS + PSPDISP_COM_MAX_PRESETS) <= PSPDISP_DAEMON_SESSION_DATA_CAPACITY, "The biggest frame data must fit the PSP receive buffer");

/**
 * @brief Link to the PSP, the transport of the session (USB or WLAN).
 */
typedef struct {
    bool (*write) (void *data, const void *buffer, uint32_t size); // Send the whole buffer, `false` on failure
    bool (*read) (void *data, void *buffer, uint32_t size, uint32_t timeout); // Receive exactly `size` bytes within `timeout` milliseconds, `false` on failure
    void *data; // The transport, given to `write` and `read`
    bool com_settings_always; // If the PSP sends its settings in every response (USB), otherwise only when they changed (WLAN)
    bool wlan; // If it's WLAN, which has its own sample rate (as it carries less than USB)
} PSPdispDaemonLink;

/**
 * @brief Type of a function pointer to start or stop the capture of the desktop sound.
 *
 * It stops any running capture, and starts a new one when enabled, whose chunks go to the media of the session (see `pspdisp_daemon_media_on_audio()`).
 *
 * @param enabled If the sound must be captured.
 * @param sample_rate The sample rate in Hz.
 * @param chunk_size The size of each chunk in bytes.
 * @param data The data given with the function.
 * @return `true` if a capture runs now, `false` if it's stopped or it failed.
 */
typedef bool (*PSPdispDaemonSoundHandler) (bool enabled, uint32_t sample_rate, uint32_t chunk_size, gpointer data);

/**
 * @brief State of the session with the connected PSP.
 */
typedef struct {
    PSPdispDaemonLink *link; // The transport
    PSPdispDaemonSnapshotStore *snapshot_store; // Where the settings come from, and are saved to
    PSPdispDaemonState *state; // What the daemon is doing, told to the desktop app
    PSPdispDaemonMedia *media; // The frames and the sound waiting to be sent
    PSPdispDaemonSoundHandler sound_handler; // Starts and stops the sound capture
    gpointer sound_data; // Given to `sound_handler`
    PSPdispDaemonSnapshot *snapshot; // The settings in use
    PSPdispSettingsStreaming streaming_settings; // The streaming settings in use, as saved, then as changed on the PSP
    gint64 saved_at; // When the session last saved the streaming settings, in microseconds (monotonic), the snapshots read before don't have them
    PSPdispComSettings com_settings; // The streaming settings in the form of the protocol, as the PSP shows them
    uint32_t preset_index; // The preset chosen last on the PSP, so its menu stays on it
    bool send_com_settings; // If the next frame carries the communication settings
    bool force_image; // If the next frame carries an image, even if the virtual display didn't change
    bool capturing; // If the sound capture runs
    bool screen_cleared; // If the PSP screen was cleared, and no image was sent since
    bool display_on; // If the computer has the display turned on (DPMS)
    PSPdispController *controller; // What the PSP controls drive (`NULL` for nothing)
    char controller_name[PSPDISP_COM_NAME_SIZE]; // The control `controller` was made for, or that failed (`""` for none)
    uint64_t controller_generation; // The generation of the control files `controller` was made with
    bool controller_failed; // If `controller_name` failed, so it's not tried again with each frame
    uint32_t controller_menu_requests; // Requests of `controller` to open the menu of PSPdisp, already counted
    uint64_t frame_count; // Count of `frame`, from the media
    uint32_t frame[PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT]; // The latest frame of the virtual display
    PSPdispDaemonImage *image; // Encoder of the images
    uint8_t data[PSPDISP_DAEMON_SESSION_DATA_CAPACITY]; // Data of the frame being sent: the image, the audio chunk, then the communication settings
} PSPdispDaemonSession;

/**
 * @brief Get the size of the audio chunks of a sample rate, with its flags.
 *
 * As the old desktop app: 2240 samples (4480 bytes) at 11025 and 22050 Hz, and 2688 samples (5376 bytes) at 44100 Hz.
 *
 * @param sample_rate The sample rate setting.
 * @param flags Where to add the audio flags of the frame header (`NULL` to ignore them).
 * @return The size of a chunk in bytes.
 */
uint32_t pspdisp_daemon_session_chunk_size(PSPdispComSampleRate sample_rate, uint32_t *flags) {
    switch (sample_rate) {
        case PSPDISP_COM_SAMPLE_RATE_11025:
            if (flags != NULL)
                *flags |= PSPDISP_COM_FLAG_AUDIO_11025_HZ | PSPDISP_COM_FLAG_AUDIO_CHUNK_2240;
            return 2240 * 2;
        case PSPDISP_COM_SAMPLE_RATE_44100:
            if (flags != NULL)
                *flags |= PSPDISP_COM_FLAG_AUDIO_44100_HZ | PSPDISP_COM_FLAG_AUDIO_CHUNK_2688;
            return 2688 * 2;
        default:
            if (flags != NULL)
                *flags |= PSPDISP_COM_FLAG_AUDIO_22050_HZ | PSPDISP_COM_FLAG_AUDIO_CHUNK_2240;
            return 2240 * 2;
    }
}

/**
 * @brief Get the sample rate setting of the link: the one of USB or of WLAN.
 */
PSPdispComSampleRate pspdisp_daemon_session_sample_rate(PSPdispDaemonSession *session, const PSPdispSettingsStreaming *streaming_settings) {
    return session->link->wlan ? streaming_settings->sample_rate_wlan : streaming_settings->sample_rate_usb;
}

/**
 * @brief Start or stop the sound capture as the settings ask.
 *
 * @param session The session.
 */
void pspdisp_daemon_session_update_sound(PSPdispDaemonSession *session) {
    PSPdispComSampleRate sample_rate = pspdisp_daemon_session_sample_rate(session, &session->streaming_settings);
    uint32_t chunk_size = pspdisp_daemon_session_chunk_size(sample_rate, NULL);

    session->capturing = session->sound_handler(session->streaming_settings.sound, pspdisp_settings_sample_rate_hz(sample_rate), chunk_size, session->sound_data);
    pspdisp_daemon_state_set_sound(session->state, session->capturing);
}

/**
 * @brief Make the settings the PSP shows from the streaming settings, with what the virtual display can't do turned back to its default.
 *
 * @param session The session, whose `com_settings` are made.
 */
void pspdisp_daemon_session_build_com_settings(PSPdispDaemonSession *session) {
    const PSPdispSettingsStreaming *streaming_settings = &session->streaming_settings;
    int control_index = pspdisp_daemon_snapshot_find_control(session->snapshot, streaming_settings->control, NULL);

    session->com_settings = (PSPdispComSettings) {
        .device_index = 0,
        .device_use_sideshow = 0, // No SideShow, it's a Windows Vista gadget platform
        .screen_mode = PSPDISP_COM_SCREEN_MODE_FIT_SCREEN, // The virtual display is already the size of the PSP
        .screen_rotation = PSPDISP_COM_ROTATION_0, // The display settings of the computer rotate the virtual display
        .screen_viewport = PSPDISP_COM_VIEWPORT_480X272,
        .quality = streaming_settings->quality,
        .screen_no_compression = streaming_settings->uncompressed,
        .update_interval = streaming_settings->update_interval,
        .sound_enabled = streaming_settings->sound,
        .sound_sample_rate = pspdisp_daemon_session_sample_rate(session, streaming_settings),
        .control_index = control_index >= 0 ? control_index : 1, // A control file that's gone is none
        .control_count = PSPDISP_DAEMON_SNAPSHOT_FIXED_CONTROLS + session->snapshot->controls->len,
        .display_device_index = 0,
        .display_device_count = G_N_ELEMENTS(pspdisp_daemon_display_devices),
        .display_enable = streaming_settings->image,
        .display_autoselect = 0, // There's only one display
        .preset_index = session->preset_index < session->snapshot->presets->len ? session->preset_index : 0,
        .preset_count = session->snapshot->presets->len,
        .preset_do_set = 0,
    };
}

/**
 * @brief Use new streaming settings: send them to the PSP with an image, restart the sound if it changed, and save them if asked.
 *
 * @param session The session.
 * @param streaming_settings The new streaming settings.
 * @param save If they're saved (when they come from the PSP), so the desktop app shows them.
 */
void pspdisp_daemon_session_set_streaming_settings(PSPdispDaemonSession *session, const PSPdispSettingsStreaming *streaming_settings, bool save) {
    bool sound_changed = streaming_settings->sound != session->streaming_settings.sound || pspdisp_daemon_session_sample_rate(session, streaming_settings) != pspdisp_daemon_session_sample_rate(session, &session->streaming_settings);
    PSPdispSettingsStreaming before = session->streaming_settings;

    session->streaming_settings = *streaming_settings;
    pspdisp_daemon_session_build_com_settings(session);
    session->send_com_settings = true;
    session->force_image = true;

    const PSPdispSettings *settings = &session->snapshot->settings;
    g_message("Settings: quality %u%s, image %s, sound %s at %u Hz, update interval %u ms, control \"%s\"",
              settings->jpeg_quality[streaming_settings->quality], streaming_settings->uncompressed ? " (uncompressed)" : "",
              streaming_settings->image ? "on" : "off", streaming_settings->sound ? "on" : "off", pspdisp_settings_sample_rate_hz(pspdisp_daemon_session_sample_rate(session, streaming_settings)),
              settings->update_interval[streaming_settings->update_interval], streaming_settings->control);

    // Only what changed, so what the desktop app changed meanwhile stays
    if (save && !pspdisp_settings_streaming_equal(&before, streaming_settings)) {
        GError *error = NULL;

        if (!pspdisp_settings_update_streaming(session->snapshot_store->dir, &before, streaming_settings, &error)) {
            g_warning("%s(): failed to save the settings changed on the PSP: %s", __FUNCTION__, error->message);
            g_error_free(error);
        }
        session->saved_at = g_get_monotonic_time();
    }

    if (sound_changed)
        pspdisp_daemon_session_update_sound(session);
}

/**
 * @brief Apply the settings changed on the PSP: a preset, or each valid option, then save them.
 *
 * Invalid values are ignored, keeping the current ones.
 * The resulting settings are sent back with the next frame, so the PSP shows the ones in use, and the image is sent again.
 *
 * @param session The session.
 * @param received The settings received from the PSP.
 */
void pspdisp_daemon_session_apply_com_settings(PSPdispDaemonSession *session, const PSPdispComSettings *received) {
    PSPdispSettingsStreaming streaming_settings = session->streaming_settings;
    GPtrArray *presets = session->snapshot->presets;

    if (received->preset_do_set && received->preset_index < presets->len) {
        const PSPdispSettingsPreset *preset = presets->pdata[received->preset_index];
        GError *error = NULL;

        session->preset_index = received->preset_index;
        if (pspdisp_settings_preset_load(session->snapshot_store->dir, preset->file_name, &streaming_settings, &error)) {
            g_message("Preset \"%s\" applied from the PSP", preset->name);
        } else {
            g_warning("%s(): failed to apply the preset \"%s\": %s", __FUNCTION__, preset->name, error->message);
            g_error_free(error);
        }
    } else {
        if (received->quality <= PSPDISP_COM_QUALITY_HIGHEST)
            streaming_settings.quality = received->quality;
        if (received->update_interval <= PSPDISP_COM_UPDATE_INTERVAL_LONGEST)
            streaming_settings.update_interval = received->update_interval;

        if (received->sound_sample_rate <= PSPDISP_COM_SAMPLE_RATE_44100) {
            if (session->link->wlan)
                streaming_settings.sample_rate_wlan = received->sound_sample_rate;
            else
                streaming_settings.sample_rate_usb = received->sound_sample_rate;
        }

        const char *control = pspdisp_daemon_snapshot_control_name(session->snapshot, received->control_index);
        if (control != NULL && received->control_index != session->com_settings.control_index)
            g_strlcpy(streaming_settings.control, control, sizeof(streaming_settings.control));

        if (received->preset_index < presets->len)
            session->preset_index = received->preset_index;
        streaming_settings.uncompressed = received->screen_no_compression != 0;
        streaming_settings.sound = received->sound_enabled != 0;
        streaming_settings.image = received->display_enable != 0;
        g_message("Settings changed on the PSP");
    }

    pspdisp_daemon_session_set_streaming_settings(session, &streaming_settings, true);
}

/**
 * @brief Take the latest settings, before a frame: the streaming settings changed on the disk, the lists of presets and control files, and the sound source.
 *
 * Streaming settings saved by the session itself are the ones it has, so they change nothing.
 * A snapshot read before the last save of the session doesn't have it, so its streaming settings are ignored: the snapshot read after the save follows.
 *
 * @param session The session.
 */
void pspdisp_daemon_session_refresh(PSPdispDaemonSession *session) {
    PSPdispDaemonSnapshot *latest = pspdisp_daemon_snapshot_store_get(session->snapshot_store);
    PSPdispDaemonSnapshot *previous = session->snapshot;

    if (latest->generation == previous->generation) {
        pspdisp_daemon_snapshot_unref(latest);
        return;
    }

    session->snapshot = latest;

    const PSPdispSettingsStreaming *saved = &latest->settings.streaming;
    bool streaming_changed = latest->loaded_at > session->saved_at && !pspdisp_settings_streaming_equal(saved, &previous->settings.streaming) && !pspdisp_settings_streaming_equal(saved, &session->streaming_settings);
    bool lists_changed = !pspdisp_daemon_snapshot_lists_equal(latest->presets, previous->presets) || !pspdisp_daemon_snapshot_lists_equal(latest->controls, previous->controls);
    bool quality_changed = latest->settings.jpeg_quality[session->streaming_settings.quality] != previous->settings.jpeg_quality[session->streaming_settings.quality];

    if (streaming_changed) {
        g_message("Settings changed on the desktop");
        pspdisp_daemon_session_set_streaming_settings(session, saved, false);
    } else if (lists_changed) {
        pspdisp_daemon_session_build_com_settings(session);
        session->send_com_settings = true; // The names of the presets and control files
    }

    if (quality_changed)
        session->force_image = true;

    pspdisp_daemon_snapshot_unref(previous);
}

/**
 * @brief Drive what the PSP controls drive, with each response: create it when the control changes (or its control file changed), then update it.
 *
 * A control that fails is told once, and tried again when the control changes.
 *
 * @param session The session.
 * @param buttons The pressed buttons, `PSPdispButton` bits, with a character of the on-screen keyboard in the highest byte (none while the PSP menu is open).
 * @param analog_x The analog stick, from the left (0) to the right (255).
 * @param analog_y The analog stick, from the top (0) to the bottom (255).
 */
void pspdisp_daemon_session_update_controls(PSPdispDaemonSession *session, uint32_t buttons, uint8_t analog_x, uint8_t analog_y) {
    const char *wanted = session->streaming_settings.control;
    const char *path = NULL;
    int index = pspdisp_daemon_snapshot_find_control(session->snapshot, wanted, &path);
    bool none = index < 0 || index == 1;
    bool changed = strcmp(session->controller_name, wanted) != 0 || (path != NULL && session->controller_generation != session->snapshot->controls_generation);

    if (changed) {
        if (session->controller != NULL) {
            pspdisp_controller_destroy(session->controller);
            session->controller = NULL;
            g_message("Control \"%s\" released", session->controller_name);
        }

        g_strlcpy(session->controller_name, wanted, sizeof(session->controller_name));
        session->controller_generation = session->snapshot->controls_generation;
        session->controller_failed = false;
        session->controller_menu_requests = 0;
        session->force_image = true; // The flag of the on-screen keyboard may change

        if (index < 0) {
            g_warning("The control file \"%s\" doesn't exist, the PSP controls drive nothing", wanted);
            pspdisp_daemon_state_set_control(session->state, wanted, "the control file doesn't exist");
        } else if (none) {
            pspdisp_daemon_state_set_control(session->state, wanted, NULL);
        }
    }

    if (session->controller == NULL && !session->controller_failed && !none) {
        GError *error = NULL;
        session->controller = path != NULL ? pspdisp_controller_new_control_file(path, &error) : pspdisp_controller_new(&error);

        if (session->controller != NULL) {
            g_message("Control \"%s\": the PSP drives a virtual %s", wanted, path != NULL ? "keyboard and mouse" : "gamepad");
            pspdisp_daemon_state_set_control(session->state, wanted, NULL);
        } else {
            g_warning("The control \"%s\" can't be used, the PSP controls drive nothing: %s", wanted, error->message);
            pspdisp_daemon_state_set_control(session->state, wanted, error->message);
            g_error_free(error);
            session->controller_failed = true;
        }
    }

    if (session->controller != NULL) {
        pspdisp_controller_update(session->controller, buttons, analog_x, analog_y);

        uint32_t requests = pspdisp_controller_open_menu_requests(session->controller);
        if (requests > session->controller_menu_requests)
            pspdisp_daemon_state_open_menu(session->state, requests - session->controller_menu_requests);
        session->controller_menu_requests = requests;
    }
}

/**
 * @brief Write the communication settings, followed by the names of the display devices, the control files and the presets.
 *
 * @param session The session.
 * @param output Where to write them.
 * @return The size written in bytes.
 */
uint32_t pspdisp_daemon_session_write_com_settings(PSPdispDaemonSession *session, uint8_t *output) {
    uint32_t size = 0;

    memcpy(output, &session->com_settings, sizeof(PSPdispComSettings));
    size += sizeof(PSPdispComSettings);

    // Each name in `PSPDISP_COM_NAME_SIZE` bytes, null-terminated and padded with zeros
    GPtrArray *names = g_ptr_array_new();

    for (size_t i = 0; i < G_N_ELEMENTS(pspdisp_daemon_display_devices); i++)
        g_ptr_array_add(names, (gpointer) pspdisp_daemon_display_devices[i]);
    for (uint32_t i = 0; i < session->com_settings.control_count; i++)
        g_ptr_array_add(names, (gpointer) pspdisp_daemon_snapshot_control_name(session->snapshot, i));
    for (guint i = 0; i < session->snapshot->presets->len; i++)
        g_ptr_array_add(names, ((PSPdispSettingsPreset *) session->snapshot->presets->pdata[i])->name);

    for (guint i = 0; i < names->len; i++) {
        memset(&output[size], 0, PSPDISP_COM_NAME_SIZE);
        strncpy((char *) &output[size], names->pdata[i], PSPDISP_COM_NAME_SIZE - 1);
        size += PSPDISP_COM_NAME_SIZE;
    }

    g_ptr_array_unref(names);
    return size;
}

/**
 * @brief Build and send a frame, then receive the response of the PSP.
 *
 * @param session The session.
 * @param chunk The audio chunk of the frame (`NULL` for silence), only used when the sound is enabled.
 * @return `true` if the PSP answered, `false` if it failed.
 */
bool pspdisp_daemon_session_exchange(PSPdispDaemonSession *session, GBytes *chunk) {
    PSPdispComFrameHeader header = { .magic = PSPDISP_COM_HEADER_MAGIC };
    uint32_t data_size = 0;

    // The image, when the virtual display changed (or the PSP asked for one)
    bool changed = pspdisp_daemon_media_take_frame(session->media, &session->frame_count, session->frame, &session->display_on);

    if (session->com_settings.display_enable && session->display_on) {
        if (changed || session->force_image) {
            uint32_t quality = session->snapshot->settings.jpeg_quality[session->streaming_settings.quality];

            header.image_size = pspdisp_daemon_image_encode(session->image, session->frame, &session->com_settings, quality, session->data, &header.flags);
            data_size += header.image_size;
            session->force_image = false;
            session->screen_cleared = false;
        }
    } else if (!session->screen_cleared || session->force_image) {
        header.flags |= PSPDISP_COM_FLAG_IMAGE_CLEAR_SCREEN; // The streaming is disabled, or the display turned off
        session->force_image = false;
        session->screen_cleared = true;
    }

    // The audio chunk, with sound
    if (session->com_settings.sound_enabled) {
        uint32_t chunk_size = pspdisp_daemon_session_chunk_size(session->com_settings.sound_sample_rate, &header.flags);

        if (chunk != NULL && g_bytes_get_size(chunk) == chunk_size)
            memcpy(&session->data[data_size], g_bytes_get_data(chunk, NULL), chunk_size);
        else
            memset(&session->data[data_size], 0, chunk_size); // Silence
        data_size += chunk_size;
        header.flags |= PSPDISP_COM_FLAG_CONTAINS_AUDIO_DATA;
    }

    // The communication settings, at the start and after a change
    if (session->send_com_settings) {
        header.settings_size = pspdisp_daemon_session_write_com_settings(session, &session->data[data_size]);
        data_size += header.settings_size;
        header.flags |= PSPDISP_COM_FLAG_CONTAINS_SETTINGS_DATA;
        session->send_com_settings = false;
    }

    // The on-screen keyboard, when the control file enables it
    if (session->controller != NULL && pspdisp_controller_enables_osk(session->controller))
        header.flags |= PSPDISP_COM_FLAG_ENABLE_OSK;

    // The header, then the data, as two transfers (the PSP reads the header first, to know the size of the data)
    if (!session->link->write(session->link->data, &header, sizeof(header))) return false;
    if (data_size > 0 && !session->link->write(session->link->data, session->data, data_size)) return false;

    pspdisp_daemon_state_frame(session->state, sizeof(header) + data_size);

    // The response: its header and the communication settings, always over USB (as one transfer), only when they changed over WLAN (they follow the header)
    PSPdispComSettingsResponse response;
    uint32_t response_size = session->link->com_settings_always ? sizeof(response) : sizeof(response.response);

    if (!session->link->read(session->link->data, &response, response_size, PSPDISP_DAEMON_SESSION_READ_TIMEOUT)) return false;

    if (response.response.magic != PSPDISP_COM_HEADER_MAGIC) {
        g_warning("%s(): the PSP answered without the magic (0x%08X)", __FUNCTION__, response.response.magic);
        return false;
    }

    bool com_settings_changed = response.response.flags & PSPDISP_COM_FLAG_CONTAINS_SETTINGS_DATA;
    if (com_settings_changed && !session->link->com_settings_always && !session->link->read(session->link->data, &response.settings, sizeof(response.settings), PSPDISP_DAEMON_SESSION_READ_TIMEOUT)) return false;

    if (com_settings_changed)
        pspdisp_daemon_session_apply_com_settings(session, &response.settings);
    if (response.response.flags & PSPDISP_COM_FLAG_FORCE_UPDATE)
        session->force_image = true;

    // The controls, after the communication settings, as they may choose what the controls drive
    pspdisp_daemon_session_update_controls(session, response.response.buttons, response.response.analog_x, response.response.analog_y);

    return true;
}

/**
 * @brief Run the session with a connected PSP, until the link fails or it's stopped.
 *
 * It starts the sound capture as the settings ask, and stops it before returning, as it releases what the controls drive.
 *
 * @param link The transport to the PSP.
 * @param snapshot_store Where the settings come from, and are saved to.
 * @param state What the daemon is doing, told to the desktop app.
 * @param media The frames and the sound to send.
 * @param sound_handler Starts and stops the sound capture.
 * @param sound_data Given to the sound handler.
 * @param cancellable Cancelled to stop the session, it also aborts the transfers of the link.
 * @return `true` if it was stopped, `false` if it failed.
 */
bool pspdisp_daemon_session_run(PSPdispDaemonLink *link, PSPdispDaemonSnapshotStore *snapshot_store, PSPdispDaemonState *state, PSPdispDaemonMedia *media, PSPdispDaemonSoundHandler sound_handler, gpointer sound_data, GCancellable *cancellable) {
    PSPdispDaemonImage *image = pspdisp_daemon_image_new();
    if (image == NULL) return false;

    PSPdispDaemonSession *session = g_new0(PSPdispDaemonSession, 1);
    session->link = link;
    session->snapshot_store = snapshot_store;
    session->state = state;
    session->media = media;
    session->sound_handler = sound_handler;
    session->sound_data = sound_data;
    session->snapshot = pspdisp_daemon_snapshot_store_get(session->snapshot_store);
    session->streaming_settings = session->snapshot->settings.streaming;
    session->send_com_settings = true;
    session->force_image = true;
    session->display_on = true;
    session->image = image;
    pspdisp_daemon_session_build_com_settings(session);
    pspdisp_daemon_session_update_sound(session);

    bool linked = true;

    while (linked && !g_cancellable_is_cancelled(cancellable)) {
        GBytes *chunk = NULL;

        pspdisp_daemon_session_refresh(session);

        if (session->com_settings.sound_enabled) {
            // The pace of the frames is the pace of the sound, when the capture is late (or stopped) the frame carries silence
            uint32_t chunk_size = pspdisp_daemon_session_chunk_size(session->com_settings.sound_sample_rate, NULL);
            guint64 duration = (guint64) chunk_size / 4 * G_USEC_PER_SEC / pspdisp_settings_sample_rate_hz(session->com_settings.sound_sample_rate);

            if (session->capturing)
                chunk = pspdisp_daemon_media_take_audio(session->media, duration * 3 / 2);
            else
                g_usleep(duration);
        }

        linked = pspdisp_daemon_session_exchange(session, chunk);
        if (chunk != NULL)
            g_bytes_unref(chunk);

        // Without sound, the update interval between the frames
        uint32_t interval = session->snapshot->settings.update_interval[session->com_settings.update_interval];
        if (linked && !session->com_settings.sound_enabled && interval > 0)
            g_usleep(interval * 1000);
    }

    if (session->capturing)
        session->sound_handler(false, 0, 0, session->sound_data);
    if (session->controller != NULL)
        pspdisp_controller_destroy(session->controller);

    pspdisp_daemon_snapshot_unref(session->snapshot);
    pspdisp_daemon_image_destroy(session->image);
    g_free(session);
    return g_cancellable_is_cancelled(cancellable); // A transfer aborted by the stop fails the link too
}
