/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * main.c - Daemon that streams the desktop to a PSP *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The PSPdisp daemon waits for a PSP running the PSPdisp app, plugged in USB mode or connecting in WLAN mode.
 * Once it's connected, the daemon creates a virtual display the size of the PSP and captures the desktop sound, then streams both to the PSP, until it's unplugged, disconnected or the app quits.
 * Meanwhile the PSP buttons and analog stick drive a virtual gamepad, or a virtual keyboard and mouse as a control file says, as chosen in the settings or on the "PC Settings" menu of the PSP.
 *
 * Its settings are files edited by the desktop app (see `settings/settings.c`), in the folder of the settings of the user (see `pspdisp_settings_dir_resolve()`), or the one given with `--config-dir`.
 * When they change, it takes them at once (see `snapshot.c`): e.g. it starts or stops serving over USB or WLAN, and the PSP being served gets the new streaming settings.
 * It sends what it does to the desktop app, about every second and right away when it changes (see `status/status.c`).
 *
 * USB and WLAN each have their own thread waiting for a PSP, but only one PSP is served at a time, as there's one virtual display and one sound capture (see `pspdisp_daemon_take_turn()`).
 * Another PSP plugged meanwhile waits for its turn, and another PSP connecting over WLAN meanwhile is refused (it would give up after a few seconds without frames anyway), then it connects again by itself.
 * So the WLAN thread keeps accepting the connections, and serves each PSP on a thread of its own.
 *
 * It talks the old protocol of the PSP app (see `shared.old.h`).
 * It runs until it's asked to stop (e.g. with Ctrl+C, see `system.c`), which aborts the waits for a PSP and the running transfer, so it stops at once.
 *
 * It runs as the user, in the desktop session, as the desktop app: the install gives it the access to the PSP and to the virtual devices (see the README).
 * It refuses to run as an administrator, as it would then write in the folder of the settings of a user, who could turn it against the administrator (e.g. with symbolic links).
 */

#include <stdlib.h>
#include "../display/display.c"
#include "../audio/audio.c"
#include "../usb/usb.c"
#include "../wlan/wlan.c"
#include "../settings/settings.c"
#include "./config.c"
#include "./media.c"
#include "./snapshot.c"
#include "./state.c"
#include "./session.c"
#include "./system.c"

#define PSPDISP_DAEMON_WLAN_RETRY 2000 // Time between two tries to start the WLAN server, e.g. while its port is used by another program, in milliseconds

/**
 * @brief A way the PSPs connect (USB or WLAN), started and stopped as the settings ask, by rounds.
 *
 * Its thread runs rounds while it's wanted: a round waits for a PSP and serves it (or starts the WLAN server and serves the PSPs connecting).
 * A change of the settings about it cancels the round, so its thread starts a new one with them (or waits until it's wanted again).
 */
typedef struct {
	const char *name; // Its name, for the messages
	GMutex mutex; // Guards the fields below
	GCond cond; // Signaled when `wanted` changes, or the daemon stops
	bool wanted; // If the settings ask for it
	uint32_t port; // The TCP port, for WLAN
	GCancellable *round; // Cancelled to end the round running (`NULL` if none)
	PSPdispWLAN *wlan; // The WLAN server listening, to change its password (`NULL` if none)
} PSPdispDaemonTransport;

/**
 * @brief The transport of a session link: the USB device or the WLAN connection, with the round it runs in.
 */
typedef struct {
	void *transport; // The `PSPdispUSBDevice` or the `PSPdispWLANConnection`
	GCancellable *round; // Cancelled to end the round, it aborts the transfers
} PSPdispDaemonLinkData;

/**
 * @brief The daemon: what it serves the PSPs with, shared by its threads.
 */
typedef struct {
	GMainLoop *loop; // The main loop, quit to stop the daemon
	GCancellable *cancellable; // Cancelled to stop the daemon
	PSPdispDaemonSnapshotStore *snapshot_store; // The settings
	PSPdispDaemonState *state; // What the daemon is doing, sent to the desktop app
	PSPdispDaemonMedia *media; // The frames and the sound waiting to be sent to the PSP served
	PSPdispUSB *usb; // The USB of the computer (`NULL` if it can't be watched)
	PSPdispDaemonTransport usb_transport; // The USB mode
	PSPdispDaemonTransport wlan_transport; // The WLAN mode
	GMutex turn_mutex; // Guards `turn_taken`
	GCond turn_cond; // Signaled when the turn is given back
	bool turn_taken; // If a PSP is served
	PSPdispAudio *audio; // The PSP audio, connected (`NULL` when stopped), only used by the PSP being served
	int higher_priority; // 1 if the threads have a higher priority, 0 if not (-1 before it's set), only used on the main thread
} PSPdispDaemon;

char *pspdisp_daemon_option_config_dir = NULL; // `--config-dir`
gboolean pspdisp_daemon_option_version = FALSE; // `--version`

/**
 * @brief The options of the command line.
 */
GOptionEntry pspdisp_daemon_options[] = {
	{ "config-dir", 'c', 0, G_OPTION_ARG_FILENAME, &pspdisp_daemon_option_config_dir, "Folder of the settings, shared with the desktop app (default: ${XDG_CONFIG_HOME}/pspdisp)", "DIR" },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &pspdisp_daemon_option_version, "Print the version and quit", NULL },
	G_OPTION_ENTRY_NULL
};

/**
 * @brief Take the turn to serve a PSP, as only one is served at a time.
 *
 * It's given back on any thread, e.g. by the thread serving a PSP connected over WLAN, so it's not a mutex.
 *
 * @param daemon The daemon.
 * @param wait If it waits for the PSP being served to be disconnected, otherwise it gives up at once.
 * @param round The round of the PSP waiting, it gives up when it's cancelled.
 * @return `true` if the turn was taken, give it back with `pspdisp_daemon_give_turn()`, `false` if a PSP is served (without waiting) or the round is cancelled.
 */
bool pspdisp_daemon_take_turn(PSPdispDaemon *daemon, bool wait, GCancellable *round) {
	g_mutex_lock(&daemon->turn_mutex);

	// The round may be cancelled without the turn given back, so it's checked again from time to time
	while (wait && daemon->turn_taken && !g_cancellable_is_cancelled(round)) g_cond_wait_until(&daemon->turn_cond, &daemon->turn_mutex, g_get_monotonic_time() + 200 * G_TIME_SPAN_MILLISECOND);

	bool taken = !daemon->turn_taken && !g_cancellable_is_cancelled(round);
	if (taken)
		daemon->turn_taken = true;

	g_mutex_unlock(&daemon->turn_mutex);
	return taken;
}

/**
 * @brief Give back the turn to serve a PSP, taken with `pspdisp_daemon_take_turn()`.
 *
 * @param daemon The daemon.
 */
void pspdisp_daemon_give_turn(PSPdispDaemon *daemon) {
	g_mutex_lock(&daemon->turn_mutex);
	daemon->turn_taken = false;
	g_cond_broadcast(&daemon->turn_cond);
	g_mutex_unlock(&daemon->turn_mutex);
}

/**
 * @brief Connect or disconnect the PSP audio, the sound handler of the session (see `PSPdispDaemonSoundHandler`).
 *
 * @param enabled If the PSP gets the sound.
 * @param sample_rate The sample rate in Hz.
 * @param chunk_size The size of each chunk in bytes.
 * @param daemon The daemon.
 * @return `true` if the PSP audio is connected now.
 */
bool pspdisp_daemon_on_sound(bool enabled, uint32_t sample_rate, uint32_t chunk_size, PSPdispDaemon *daemon) {
	if (daemon->audio != NULL) {
		pspdisp_audio_destroy(daemon->audio); // Also disconnects it
		daemon->audio = NULL;
	}
	pspdisp_daemon_media_drop_audio(daemon->media); // The chunks of the previous connection may have another size

	if (!enabled) return false;

	daemon->audio = pspdisp_audio_new(PSPDISP_DAEMON_PSP_MODEL, PSPDISP_DAEMON_PSP_REGION, sample_rate, chunk_size, (PSPdispAudioHandler) pspdisp_daemon_media_on_audio, daemon->media);
	if (pspdisp_audio_connect(daemon->audio)) return true;

	g_warning("The PSP audio can't be connected with %s, the PSP gets silence", PSPDISP_AUDIO_CONNECTOR_NAME);
	pspdisp_audio_destroy(daemon->audio);
	daemon->audio = NULL;
	return false;
}

/**
 * @brief Send a whole buffer over USB, the `write` of the USB session link.
 *
 * @param data The transport of the link (`PSPdispDaemonLinkData *`).
 * @param buffer The data to send.
 * @param size The size of `buffer` in bytes.
 * @return `true` if it was sent, `false` if it failed.
 */
bool pspdisp_daemon_usb_write(void *data, const void *buffer, uint32_t size) {
	PSPdispDaemonLinkData *link = data;
	return pspdisp_usb_write(link->transport, buffer, size, link->round);
}

/**
 * @brief Receive exactly `size` bytes over USB, the `read` of the USB session link.
 *
 * @param data The transport of the link (`PSPdispDaemonLinkData *`).
 * @param buffer Where to write the data.
 * @param size The number of bytes to receive.
 * @param timeout How long to wait for them, in milliseconds.
 * @return `true` if they were received, `false` if it failed.
 */
bool pspdisp_daemon_usb_read(void *data, void *buffer, uint32_t size, uint32_t timeout) {
	PSPdispDaemonLinkData *link = data;
	return pspdisp_usb_read(link->transport, buffer, size, timeout, link->round);
}

/**
 * @brief Send a whole buffer over WLAN, the `write` of the WLAN session link.
 *
 * @param data The transport of the link (`PSPdispDaemonLinkData *`).
 * @param buffer The data to send.
 * @param size The size of `buffer` in bytes.
 * @return `true` if it was sent, `false` if it failed.
 */
bool pspdisp_daemon_wlan_write(void *data, const void *buffer, uint32_t size) {
	PSPdispDaemonLinkData *link = data;
	return pspdisp_wlan_write(link->transport, buffer, size, link->round);
}

/**
 * @brief Receive exactly `size` bytes over WLAN, the `read` of the WLAN session link.
 *
 * @param data The transport of the link (`PSPdispDaemonLinkData *`).
 * @param buffer Where to write the data.
 * @param size The number of bytes to receive.
 * @param timeout How long to wait for them, in milliseconds.
 * @return `true` if they were received, `false` if it failed.
 */
bool pspdisp_daemon_wlan_read(void *data, void *buffer, uint32_t size, uint32_t timeout) {
	PSPdispDaemonLinkData *link = data;
	return pspdisp_wlan_read(link->transport, buffer, size, timeout, link->round);
}

/**
 * @brief Copy a frame of the virtual display into the media, the frame handler of the display (see `PSPdispFrameHandler`).
 *
 * @param pixels The frame buffer.
 * @param stride The number of pixels per row in the frame buffer.
 * @param format The pixel format of the frame buffer.
 * @param rectangles The dirty rectangles.
 * @param rectangles_length The number of dirty rectangles.
 * @param daemon The daemon.
 */
void pspdisp_daemon_on_frame(void *pixels, uint16_t stride, PSPdispPixelFormat format, PSPdispDirtyRectangle *rectangles, uint8_t rectangles_length, PSPdispDaemon *daemon) {
	pspdisp_daemon_media_on_frame(pixels, stride, format, rectangles, rectangles_length, daemon->media);
}

/**
 * @brief Keep the power mode of the virtual display in the media, the DPMS handler of the display (see `PSPdispDPMSHandler`).
 *
 * @param mode The DPMS mode.
 * @param daemon The daemon.
 */
void pspdisp_daemon_on_dpms(PSPdispDPMSMode mode, PSPdispDaemon *daemon) {
	pspdisp_daemon_media_on_dpms(mode, daemon->media);
}

/**
 * @brief Tell the loss of the virtual display, the lost handler of the display (see `PSPdispLostHandler`).
 *
 * The PSP gets a cleared screen instead of the last frame, as when the display is turned off.
 *
 * @param daemon The daemon.
 */
void pspdisp_daemon_on_display_lost(PSPdispDaemon *daemon) {
	g_warning("The virtual display was lost (its driver is gone), the PSP gets a black screen");
	pspdisp_daemon_media_on_dpms(PSPDISP_DPMS_MODE_OFF, daemon->media);
	pspdisp_daemon_state_set_display(daemon->state, false);
}

/**
 * @brief Serve a connected PSP: create the virtual display, run the session, then remove the virtual display.
 *
 * The caller has the turn (see `pspdisp_daemon_take_turn()`).
 *
 * @param daemon The daemon.
 * @param link The link with the PSP.
 * @param via How the PSP is connected, for the messages (e.g. `USB`).
 * @param round The round of the PSP, it ends the session when it's cancelled.
 */
void pspdisp_daemon_serve(PSPdispDaemon *daemon, PSPdispDaemonLink *link, const char *via, GCancellable *round) {
	g_message("PSP connected over %s", via);
	pspdisp_daemon_state_connected(daemon->state, via, link->wlan);
	pspdisp_daemon_media_reset(daemon->media);

	PSPdispDisplay *display = pspdisp_display_new(PSPDISP_DAEMON_PSP_MODEL, PSPDISP_DAEMON_PSP_REGION, PSPDISP_DAEMON_PSP_SERIAL_NUMBER, (PSPdispFrameHandler) pspdisp_daemon_on_frame, (PSPdispDPMSHandler) pspdisp_daemon_on_dpms, (PSPdispLostHandler) pspdisp_daemon_on_display_lost, daemon);
	bool connected = pspdisp_display_connect(display);

	if (!connected)
		g_warning("The virtual display can't be created with %s, the PSP gets a black screen", PSPDISP_DISPLAY_CONNECTOR_NAME);
	pspdisp_daemon_state_set_display(daemon->state, connected);

	bool stopped = pspdisp_daemon_session_run(link, daemon->snapshot_store, daemon->state, daemon->media, (PSPdispDaemonSoundHandler) pspdisp_daemon_on_sound, daemon, round);

	pspdisp_display_destroy(display); // Also removes the virtual display
	pspdisp_daemon_state_disconnected(daemon->state);
	g_message(stopped ? "Disconnected from the PSP" : "PSP disconnected");
}

/**
 * @brief Change what the settings ask for a transport, on the main thread: the round running is cancelled if it changed.
 *
 * @param transport The transport.
 * @param wanted If it's wanted.
 * @param port The TCP port, for WLAN.
 */
void pspdisp_daemon_transport_update(PSPdispDaemonTransport *transport, bool wanted, uint32_t port) {
	g_mutex_lock(&transport->mutex);

	if (wanted != transport->wanted || (wanted && port != transport->port)) {
		if (wanted != transport->wanted)
			g_message("%s mode %s", transport->name, wanted ? "enabled" : "disabled");

		transport->wanted = wanted;
		transport->port = port;
		if (transport->round != NULL)
			g_cancellable_cancel(transport->round);
		g_cond_broadcast(&transport->cond);
	}

	g_mutex_unlock(&transport->mutex);
}

/**
 * @brief Begin a round of a transport, on its thread: wait until it's wanted.
 *
 * @param transport The transport.
 * @param stop Cancelled when the daemon stops.
 * @param port Where to write its TCP port, for WLAN (`NULL` to ignore it).
 * @return The cancellable of the round, end it with `pspdisp_daemon_transport_end()`, or `NULL` when the daemon stops.
 */
GCancellable *pspdisp_daemon_transport_begin(PSPdispDaemonTransport *transport, GCancellable *stop, uint32_t *port) {
	GCancellable *round = NULL;

	g_mutex_lock(&transport->mutex);
	while (!transport->wanted && !g_cancellable_is_cancelled(stop)) g_cond_wait(&transport->cond, &transport->mutex);

	if (!g_cancellable_is_cancelled(stop)) {
		round = g_cancellable_new();
		transport->round = g_object_ref(round);
		if (port != NULL)
			*port = transport->port;
	}

	g_mutex_unlock(&transport->mutex);
	return round;
}

/**
 * @brief End a round of a transport, on its thread.
 *
 * @param transport The transport.
 * @param round The cancellable of the round.
 */
void pspdisp_daemon_transport_end(PSPdispDaemonTransport *transport, GCancellable *round) {
	g_mutex_lock(&transport->mutex);
	if (transport->round == round)
		g_clear_object(&transport->round);
	g_mutex_unlock(&transport->mutex);
	g_object_unref(round);
}

/**
 * @brief Stop a transport, when the daemon stops: cancel its round, and wake its thread up.
 *
 * @param transport The transport.
 */
void pspdisp_daemon_transport_stop(PSPdispDaemonTransport *transport) {
	g_mutex_lock(&transport->mutex);
	if (transport->round != NULL)
		g_cancellable_cancel(transport->round);
	g_cond_broadcast(&transport->cond);
	g_mutex_unlock(&transport->mutex);
}

/**
 * @brief Body of the USB thread: wait for a PSP plugged in USB mode and serve it, round after round, while USB is wanted, until the daemon stops.
 *
 * A PSP plugged while another one is served waits for its turn.
 *
 * @param daemon The daemon, with the USB of the computer.
 * @return Always `NULL`, nothing is given to `g_thread_join()`.
 */
gpointer pspdisp_daemon_usb_thread(PSPdispDaemon *daemon) {
	GCancellable *round;

	while ((round = pspdisp_daemon_transport_begin(&daemon->usb_transport, daemon->cancellable, NULL)) != NULL) {
		PSPdispUSBDevice *device = pspdisp_usb_connect(daemon->usb, round);

		if (device != NULL) {
			if (pspdisp_daemon_take_turn(daemon, true, round)) {
				PSPdispDaemonLinkData data = { .transport = device, .round = round };
				PSPdispDaemonLink link = { .write = pspdisp_daemon_usb_write, .read = pspdisp_daemon_usb_read, .data = &data, .com_settings_always = true, .wlan = false };

				pspdisp_daemon_serve(daemon, &link, "USB", round);
				pspdisp_daemon_give_turn(daemon);
			}

			pspdisp_usb_disconnect(device);
		}

		pspdisp_daemon_transport_end(&daemon->usb_transport, round);
	}

	return NULL;
}

/**
 * @brief A PSP connected over WLAN, given to the thread serving it.
 */
typedef struct {
	PSPdispDaemon *daemon; // The daemon
	PSPdispWLANConnection *connection; // The connection
	GCancellable *round; // The round of the WLAN server
} PSPdispDaemonWLANSession;

/**
 * @brief Body of the thread serving a PSP connected over WLAN, then closing its connection and giving back the turn.
 *
 * @param session The PSP, with the turn taken for it, freed at the end.
 * @return Always `NULL`, nothing is given to `g_thread_join()`.
 */
gpointer pspdisp_daemon_wlan_session_thread(PSPdispDaemonWLANSession *session) {
	char *via = g_strdup_printf("WLAN (%s)", session->connection->address);
	PSPdispDaemonLinkData data = { .transport = session->connection, .round = session->round };
	PSPdispDaemonLink link = { .write = pspdisp_daemon_wlan_write, .read = pspdisp_daemon_wlan_read, .data = &data, .com_settings_always = false, .wlan = true };

	pspdisp_daemon_serve(session->daemon, &link, via, session->round);
	pspdisp_wlan_disconnect(session->connection);
	pspdisp_daemon_give_turn(session->daemon);

	g_object_unref(session->round);
	g_free(session);
	g_free(via);
	return NULL;
}

/**
 * @brief Run a round of the WLAN server: accept the PSPs connecting in WLAN mode, and serve each one on a thread of its own, until the round is cancelled.
 *
 * It keeps accepting while a PSP is served, to refuse the others at once: they give up after a few seconds without frames anyway, then they connect again by themselves.
 * Without that, they would wait in the queue of the system, and be served once dead.
 *
 * @param daemon The daemon.
 * @param wlan The server the PSPs connect to.
 * @param round The cancellable of the round.
 */
void pspdisp_daemon_wlan_serve(PSPdispDaemon *daemon, PSPdispWLAN *wlan, GCancellable *round) {
	GThread *session = NULL; // The thread serving the last PSP connected over WLAN
	PSPdispWLANConnection *connection;

	while ((connection = pspdisp_wlan_connect(wlan, round)) != NULL) {
		if (!pspdisp_daemon_take_turn(daemon, false, round)) {
			g_debug("The PSP at %s connected over WLAN while another PSP is served, refused", connection->address);
			pspdisp_wlan_disconnect(connection);
			continue;
		}

		if (session != NULL)
			g_thread_join(session); // It gave the turn back, so it's ending

		PSPdispDaemonWLANSession *data = g_new0(PSPdispDaemonWLANSession, 1);
		data->daemon = daemon;
		data->connection = connection;
		data->round = g_object_ref(round);
		session = g_thread_new("pspdisp-wlan-session", (GThreadFunc) pspdisp_daemon_wlan_session_thread, data);
	}

	if (session != NULL)
		g_thread_join(session);
}

/**
 * @brief Body of the WLAN thread: start the WLAN server and serve the PSPs, round after round, while WLAN is wanted, until the daemon stops.
 *
 * When the server can't start (e.g. its port is used by another program), it's tried again from time to time.
 *
 * @param daemon The daemon.
 * @return Always `NULL`, nothing is given to `g_thread_join()`.
 */
gpointer pspdisp_daemon_wlan_thread(PSPdispDaemon *daemon) {
	PSPdispDaemonTransport *transport = &daemon->wlan_transport;
	char *last_error = NULL; // The last failure to start, told once
	GCancellable *round;
	uint32_t port;

	while ((round = pspdisp_daemon_transport_begin(transport, daemon->cancellable, &port)) != NULL) {
		PSPdispDaemonSnapshot *snapshot = pspdisp_daemon_snapshot_store_get(daemon->snapshot_store);
		GError *error = NULL;
		PSPdispWLAN *wlan = pspdisp_wlan_new(port, snapshot->settings.wlan_password, &error);
		bool no_password = snapshot->settings.wlan_password[0] == '\0';
		pspdisp_daemon_snapshot_unref(snapshot);

		if (wlan == NULL) {
			if (g_strcmp0(last_error, error->message) != 0)
				g_warning("The WLAN server can't start on port %u, tried again every %d seconds: %s", port, PSPDISP_DAEMON_WLAN_RETRY / 1000, error->message);
			g_free(last_error);
			last_error = g_strdup(error->message);
			pspdisp_daemon_state_set_wlan(daemon->state, false, port, error->message);
			g_error_free(error);

			for (int waited = 0; waited < PSPDISP_DAEMON_WLAN_RETRY && !g_cancellable_is_cancelled(round); waited += 100)
				g_usleep(100 * 1000);
		} else {
			g_clear_pointer(&last_error, g_free);
			g_mutex_lock(&transport->mutex);
			transport->wlan = wlan;
			g_mutex_unlock(&transport->mutex);

			g_message("WLAN server listening on port %u", port);
			if (no_password)
				g_warning("The WLAN mode has no password: anyone on the network can connect and see the desktop (set one in the desktop app, and the same on the PSP)");
			pspdisp_daemon_state_set_wlan(daemon->state, true, port, NULL);

			pspdisp_daemon_wlan_serve(daemon, wlan, round);

			g_mutex_lock(&transport->mutex);
			transport->wlan = NULL;
			g_mutex_unlock(&transport->mutex);

			pspdisp_wlan_destroy(wlan);
			pspdisp_daemon_state_set_wlan(daemon->state, false, port, NULL);
			g_message("WLAN server stopped");
		}

		pspdisp_daemon_transport_end(transport, round);
	}

	g_free(last_error);
	return NULL;
}

/**
 * @brief Set the priority of the threads of the daemon, as the "Higher process priority" of the settings asks, when it changed.
 *
 * @param daemon The daemon.
 * @param higher If the priority is higher than the one of the other programs.
 */
void pspdisp_daemon_set_priority(PSPdispDaemon *daemon, bool higher) {
	if (daemon->higher_priority == higher) return;

	pspdisp_daemon_system_set_priority(higher);
	daemon->higher_priority = higher;
}

/**
 * @brief Follow the new settings, on the main thread: start or stop the transports, and change the WLAN password.
 *
 * The session takes the streaming settings by itself (see `pspdisp_daemon_session_refresh()`).
 *
 * @param snapshot The new settings.
 * @param daemon The daemon.
 */
void pspdisp_daemon_on_snapshot(PSPdispDaemonSnapshot *snapshot, PSPdispDaemon *daemon) {
	const PSPdispSettings *settings = &snapshot->settings;
	bool usb = settings->enabled && settings->use_usb && daemon->usb != NULL;

	pspdisp_daemon_set_priority(daemon, settings->higher_priority);
	pspdisp_daemon_state_set_enabled(daemon->state, settings->enabled);
	pspdisp_daemon_state_set_usb(daemon->state, usb);
	pspdisp_daemon_transport_update(&daemon->usb_transport, usb, 0);
	pspdisp_daemon_transport_update(&daemon->wlan_transport, settings->enabled && settings->use_wlan, PSPDISP_WLAN_PORT + settings->wlan_port_offset);

	g_mutex_lock(&daemon->wlan_transport.mutex);
	if (daemon->wlan_transport.wlan != NULL)
		pspdisp_wlan_set_password(daemon->wlan_transport.wlan, settings->wlan_password);
	g_mutex_unlock(&daemon->wlan_transport.mutex);
}

/**
 * @brief Send the status to the desktop app, with the frames and bytes per second, called every `PSPDISP_STATUS_INTERVAL` on the main thread.
 *
 * @param daemon The daemon.
 * @return Always `G_SOURCE_CONTINUE`.
 */
gboolean pspdisp_daemon_on_status(PSPdispDaemon *daemon) {
	pspdisp_daemon_state_tick(daemon->state);
	return G_SOURCE_CONTINUE;
}

/**
 * @brief Stop the daemon, called on the main thread when the user asks it (see `pspdisp_daemon_system_watch_stop()`).
 *
 * @param daemon The daemon.
 * @return Always `G_SOURCE_CONTINUE`.
 */
gboolean pspdisp_daemon_on_stop(PSPdispDaemon *daemon) {
	g_message("Stopping...");
	g_cancellable_cancel(daemon->cancellable);
	pspdisp_daemon_transport_stop(&daemon->usb_transport);
	pspdisp_daemon_transport_stop(&daemon->wlan_transport);
	g_main_loop_quit(daemon->loop);
	return G_SOURCE_CONTINUE;
}

/**
 * @brief Run the daemon until it is asked to stop (see the top of this file).
 *
 * @param argc The number of arguments of the command line.
 * @param argv The arguments of the command line (see `pspdisp_daemon_options`).
 * @return 0 once it stopped (or after `--version`), 1 if it couldn't start.
 */
int main(int argc, char **argv) {
	GOptionContext *options = g_option_context_new("- stream the desktop to a PSP");
	GError *error = NULL;

	g_option_context_add_main_entries(options, pspdisp_daemon_options, NULL);
	g_option_context_set_summary(options, "Waits for a PSP running the PSPdisp app, in USB or WLAN mode, then streams a virtual display and the desktop sound to it.\nIts settings are edited by the desktop app.");
	g_option_context_set_description(options, "Environment variables:\n" PSPDISP_SETTINGS_ENVIRONMENT_HELP);

	if (!g_option_context_parse(options, &argc, &argv, &error)) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
		g_option_context_free(options);
		return 1;
	}
	g_option_context_free(options);

	if (pspdisp_daemon_option_version) {
		g_print("PSPdisp daemon %s\n", PSPDISP_VERSION);
		return 0;
	}

	if (pspdisp_daemon_system_runs_as_administrator()) {
		g_printerr("The daemon runs as the user, not as an administrator\n");
		return 1;
	}

	// Only one daemon runs, as the PSPs would be fought over
	PSPdispStatusServer *status_server = pspdisp_status_server_new(&error);

	if (status_server == NULL) {
		bool running = g_error_matches(error, G_IO_ERROR, G_IO_ERROR_ADDRESS_IN_USE);
		if (running)
			g_printerr("The daemon is already running, %s\n", error->message);
		else
			g_warning("The desktop app can't see the daemon: %s", error->message);
		g_clear_error(&error);
		if (running) return 1;
	}

	g_message("PSPdisp daemon %s, with the %s display connector, the %s audio connector and the %s USB host", PSPDISP_VERSION, PSPDISP_DISPLAY_CONNECTOR_NAME, PSPDISP_AUDIO_CONNECTOR_NAME, PSPDISP_USB_HOST_NAME);

	char *settings_dir = pspdisp_settings_dir_resolve(pspdisp_daemon_option_config_dir);
	g_message("Settings in %s", settings_dir);

	PSPdispDaemon daemon = {
		.loop = g_main_loop_new(NULL, FALSE),
		.cancellable = g_cancellable_new(),
		.state = pspdisp_daemon_state_new(),
		.media = pspdisp_daemon_media_new(),
		.usb = pspdisp_usb_new(),
		.usb_transport = { .name = "USB" },
		.wlan_transport = { .name = "WLAN" },
		.higher_priority = -1,
	};
	g_mutex_init(&daemon.usb_transport.mutex);
	g_cond_init(&daemon.usb_transport.cond);
	g_mutex_init(&daemon.wlan_transport.mutex);
	g_cond_init(&daemon.wlan_transport.cond);
	g_mutex_init(&daemon.turn_mutex);
	g_cond_init(&daemon.turn_cond);
	if (daemon.usb == NULL)
		g_warning("The USB mode is unavailable");

	pspdisp_daemon_system_watch_stop(G_SOURCE_FUNC(pspdisp_daemon_on_stop), &daemon);

	// The settings, then the transports they ask for
	daemon.snapshot_store = pspdisp_daemon_snapshot_store_new(settings_dir, (PSPdispDaemonSnapshotHandler) pspdisp_daemon_on_snapshot, &daemon);
	PSPdispDaemonSnapshot *snapshot = pspdisp_daemon_snapshot_store_get(daemon.snapshot_store);
	pspdisp_daemon_on_snapshot(snapshot, &daemon);
	if (!snapshot->settings.enabled)
		g_message("Not enabled in the settings, enable it in the desktop app to serve the PSPs");
	pspdisp_daemon_snapshot_unref(snapshot);

	pspdisp_daemon_state_start(daemon.state, status_server);
	g_timeout_add(PSPDISP_STATUS_INTERVAL, G_SOURCE_FUNC(pspdisp_daemon_on_status), &daemon);

	GThread *usb_thread = daemon.usb != NULL ? g_thread_new("pspdisp-usb", (GThreadFunc) pspdisp_daemon_usb_thread, &daemon) : NULL;
	GThread *wlan_thread = g_thread_new("pspdisp-wlan", (GThreadFunc) pspdisp_daemon_wlan_thread, &daemon);
	g_main_loop_run(daemon.loop); // Until the user asks the daemon to stop (see `pspdisp_daemon_on_stop()`)

	// The cancellation aborts the waits for a PSP and the running transfer
	if (usb_thread != NULL)
		g_thread_join(usb_thread);
	g_thread_join(wlan_thread);

	// The desktop apps tell the daemon doesn't run anymore
	pspdisp_daemon_state_destroy(daemon.state);
	if (status_server != NULL)
		pspdisp_status_server_destroy(status_server);
	pspdisp_daemon_snapshot_store_destroy(daemon.snapshot_store);
	pspdisp_daemon_media_destroy(daemon.media);
	if (daemon.usb != NULL)
		pspdisp_usb_destroy(daemon.usb);
	g_cond_clear(&daemon.turn_cond);
	g_mutex_clear(&daemon.turn_mutex);
	g_cond_clear(&daemon.wlan_transport.cond);
	g_mutex_clear(&daemon.wlan_transport.mutex);
	g_cond_clear(&daemon.usb_transport.cond);
	g_mutex_clear(&daemon.usb_transport.mutex);
	g_object_unref(daemon.cancellable);
	g_main_loop_unref(daemon.loop);
	g_free(settings_dir);
	return 0;
}
