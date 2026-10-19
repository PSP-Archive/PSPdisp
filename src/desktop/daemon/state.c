/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * state.c - What the daemon is doing, as it goes    *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * What the daemon is doing, updated by its threads as it goes, and sent to the desktop app by the main thread (see `status/status.c`): about every second, and right away when it changes.
 */

#pragma once

#include <string.h>
#include <glib.h>
#include "../status/status.c"
#include "./system.c"

/**
 * @brief What the daemon is doing, updated from any thread.
 */
typedef struct {
	GMutex mutex; // Guards the fields below, but `server`
	PSPdispStatus status; // The state, as it's sent (the per second values are made about every second, see `pspdisp_daemon_state_tick()`)
	uint64_t measured_frames; // Frames sent at the last measure, for the frames per second
	uint64_t measured_bytes; // Bytes sent at the last measure, for the bytes per second
	gint64 measured_time; // When the last measure was made, in microseconds (monotonic)
	guint send_source; // The send queued on the main thread after a change (0 if none)
	PSPdispStatusServer *server; // Sends the state to the desktop apps (`NULL` while it's not sent), only used on the main thread
} PSPdispDaemonState;

void pspdisp_daemon_state_send(PSPdispDaemonState *state);

/**
 * @brief Create the state, not sent until `pspdisp_daemon_state_start()`.
 *
 * @return The state, free it with `pspdisp_daemon_state_destroy()`.
 */
PSPdispDaemonState *pspdisp_daemon_state_new() {
	PSPdispDaemonState *state = g_new0(PSPdispDaemonState, 1);
	g_mutex_init(&state->mutex);
	return state;
}

/**
 * @brief Send the state queued after a change, called on the main thread.
 *
 * @param state The state.
 * @return Always `G_SOURCE_REMOVE`, it's queued again with the next change.
 */
gboolean pspdisp_daemon_state_on_changed(PSPdispDaemonState *state) {
	g_mutex_lock(&state->mutex);
	state->send_source = 0;
	g_mutex_unlock(&state->mutex);

	pspdisp_daemon_state_send(state);
	return G_SOURCE_REMOVE;
}

/**
 * @brief Queue a send of the state on the main thread, after it changed, from any thread.
 *
 * The changes coming together are sent once.
 *
 * @param state The state.
 */
void pspdisp_daemon_state_changed(PSPdispDaemonState *state) {
	g_mutex_lock(&state->mutex);
	if (state->send_source == 0)
		state->send_source = g_idle_add(G_SOURCE_FUNC(pspdisp_daemon_state_on_changed), state);
	g_mutex_unlock(&state->mutex);
}

/**
 * @brief Tell the state that a PSP is served now.
 *
 * @param state The state.
 * @param via How the PSP is connected (e.g. `USB` or `WLAN (192.168.1.20)`).
 * @param wlan If it's connected over WLAN.
 */
void pspdisp_daemon_state_connected(PSPdispDaemonState *state, const char *via, bool wlan) {
	g_mutex_lock(&state->mutex);
	state->status.connected = true;
	state->status.connected_wlan = wlan;
	g_strlcpy(state->status.connected_via, via, sizeof(state->status.connected_via));
	state->status.connected_since = g_get_real_time() / G_USEC_PER_SEC;
	state->status.total_frames = 0;
	state->status.total_bytes = 0;
	state->measured_frames = 0;
	state->measured_bytes = 0;
	g_mutex_unlock(&state->mutex);
	pspdisp_daemon_state_changed(state);
}

/**
 * @brief Tell the state that no PSP is served anymore.
 *
 * @param state The state.
 */
void pspdisp_daemon_state_disconnected(PSPdispDaemonState *state) {
	g_mutex_lock(&state->mutex);
	state->status.connected = false;
	state->status.connected_wlan = false;
	state->status.connected_via[0] = '\0';
	state->status.display = false;
	state->status.sound = false;
	state->status.control[0] = '\0';
	state->status.control_error[0] = '\0';
	g_mutex_unlock(&state->mutex);
	pspdisp_daemon_state_changed(state);
}

/**
 * @brief Count a frame sent to the PSP.
 *
 * @param state The state.
 * @param bytes Its size in bytes, with its header.
 */
void pspdisp_daemon_state_frame(PSPdispDaemonState *state, uint32_t bytes) {
	g_mutex_lock(&state->mutex);
	state->status.total_frames++;
	state->status.total_bytes += bytes;
	g_mutex_unlock(&state->mutex);
}

/**
 * @brief Tell the state if the daemon serves the PSPs, and how.
 *
 * @param state The state.
 * @param enabled If the daemon serves the PSPs.
 */
void pspdisp_daemon_state_set_enabled(PSPdispDaemonState *state, bool enabled) {
	g_mutex_lock(&state->mutex);
	state->status.enabled = enabled;
	g_mutex_unlock(&state->mutex);
	pspdisp_daemon_state_changed(state);
}

/**
 * @brief Tell the state if the USB mode works.
 *
 * @param state The state.
 * @param usb If the PSPs plugged are served.
 */
void pspdisp_daemon_state_set_usb(PSPdispDaemonState *state, bool usb) {
	g_mutex_lock(&state->mutex);
	state->status.usb = usb;
	g_mutex_unlock(&state->mutex);
	pspdisp_daemon_state_changed(state);
}

/**
 * @brief Tell the state if the WLAN server listens.
 *
 * @param state The state.
 * @param wlan If it listens.
 * @param port Its port.
 * @param error Why it doesn't listen (`NULL` if it does, or if it's not enabled).
 */
void pspdisp_daemon_state_set_wlan(PSPdispDaemonState *state, bool wlan, uint32_t port, const char *error) {
	g_mutex_lock(&state->mutex);
	state->status.wlan = wlan;
	state->status.wlan_port = port;
	g_strlcpy(state->status.wlan_error, error != NULL ? error : "", sizeof(state->status.wlan_error));
	g_mutex_unlock(&state->mutex);
	pspdisp_daemon_state_changed(state);
}

/**
 * @brief Tell the state what the PSP controls drive.
 *
 * @param state The state.
 * @param control The control (e.g. `Gamepad`).
 * @param error Why it can't be created (`NULL` if it works).
 */
void pspdisp_daemon_state_set_control(PSPdispDaemonState *state, const char *control, const char *error) {
	g_mutex_lock(&state->mutex);
	g_strlcpy(state->status.control, control, sizeof(state->status.control));
	g_strlcpy(state->status.control_error, error != NULL ? error : "", sizeof(state->status.control_error));
	g_mutex_unlock(&state->mutex);
	pspdisp_daemon_state_changed(state);
}

/**
 * @brief Tell the state if the virtual display was created.
 *
 * @param state The state.
 * @param display If it was created.
 */
void pspdisp_daemon_state_set_display(PSPdispDaemonState *state, bool display) {
	g_mutex_lock(&state->mutex);
	state->status.display = display;
	g_mutex_unlock(&state->mutex);
	pspdisp_daemon_state_changed(state);
}

/**
 * @brief Tell the state if the sound is captured.
 *
 * @param state The state.
 * @param sound If it's captured.
 */
void pspdisp_daemon_state_set_sound(PSPdispDaemonState *state, bool sound) {
	g_mutex_lock(&state->mutex);
	state->status.sound = sound;
	g_mutex_unlock(&state->mutex);
	pspdisp_daemon_state_changed(state);
}

/**
 * @brief Count requests of the PSP to open the menu of PSPdisp, so the desktop app shows itself.
 *
 * @param state The state.
 * @param count How many requests.
 */
void pspdisp_daemon_state_open_menu(PSPdispDaemonState *state, uint32_t count) {
	g_mutex_lock(&state->mutex);
	state->status.open_menu_requests += count;
	g_mutex_unlock(&state->mutex);
	pspdisp_daemon_state_changed(state);
}

/**
 * @brief Send the state to the desktop apps, on the main thread.
 *
 * @param state The state.
 */
void pspdisp_daemon_state_send(PSPdispDaemonState *state) {
	if (state->server == NULL) return;

	g_mutex_lock(&state->mutex);
	PSPdispStatus copy = state->status;
	g_mutex_unlock(&state->mutex);

	pspdisp_status_server_send(state->server, &copy);
}

/**
 * @brief Measure the frames and bytes per second since the last measure, then send the state, called about every second on the main thread.
 *
 * @param state The state.
 */
void pspdisp_daemon_state_tick(PSPdispDaemonState *state) {
	gint64 now = g_get_monotonic_time();

	g_mutex_lock(&state->mutex);
	PSPdispStatus *status = &state->status;
	double seconds = state->measured_time != 0 ? (now - state->measured_time) / (double) G_USEC_PER_SEC : 1;

	status->frames_per_second = seconds > 0 ? (uint32_t) ((status->total_frames - state->measured_frames) / seconds + 0.5) : 0;
	status->bytes_per_second = seconds > 0 ? (uint64_t) ((status->total_bytes - state->measured_bytes) / seconds) : 0;
	state->measured_frames = status->total_frames;
	state->measured_bytes = status->total_bytes;
	state->measured_time = now;
	g_mutex_unlock(&state->mutex);

	pspdisp_daemon_state_send(state);
}

/**
 * @brief Start sending the state to the desktop apps, on the main thread.
 *
 * @param state The state.
 * @param server The socket of the daemon, kept until `pspdisp_daemon_state_destroy()` (`NULL` to send nothing).
 */
void pspdisp_daemon_state_start(PSPdispDaemonState *state, PSPdispStatusServer *server) {
	g_mutex_lock(&state->mutex);
	state->status.pid = pspdisp_daemon_system_pid();
	g_mutex_unlock(&state->mutex);

	state->server = server;
	pspdisp_daemon_state_tick(state);
}

/**
 * @brief Stop sending the state and free it, before the socket of the daemon is destroyed, on the main thread once no other thread uses it.
 *
 * @param state The state.
 */
void pspdisp_daemon_state_destroy(PSPdispDaemonState *state) {
	if (state->send_source != 0)
		g_source_remove(state->send_source);
	g_mutex_clear(&state->mutex);
	g_free(state);
}
