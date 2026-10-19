/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * status.c - What the daemon is doing               *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The status of the daemon, which it sends to the desktop app: if it runs, which PSP is connected, and how fast it streams.
 *
 * The daemon listens on a local socket, in a folder only the user can reach (see `pspdisp_status_socket_path()`).
 * It sends the status to each app connected about every second, and right away when it changes (e.g. the PSP asks to open the menu of PSPdisp).
 * The app knows the daemon runs while it's connected: when the daemon stops or dies, the connection closes, and the app tries again every `PSPDISP_STATUS_INTERVAL`.
 * Each status is a key file, ended by a null character.
 * Only one daemon listens: a second one finds the socket answering, and doesn't start.
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include "../settings/settings.c"

#define PSPDISP_STATUS_INTERVAL 1000 // Time between two statuses sent by the daemon, and between two tries of the desktop app to connect to it, in milliseconds

/**
 * @brief The status of the daemon.
 */
typedef struct {
	int64_t pid; // Process ID of the daemon
	bool enabled; // If the daemon serves the PSPs (`[General] Enabled`)
	bool usb; // If the USB mode works (the PSPs plugged are served)
	bool wlan; // If the WLAN mode works (the WLAN server listens)
	uint32_t wlan_port; // TCP port of the WLAN server
	char wlan_error[256]; // Why the WLAN server doesn't listen (`""` if it does, or isn't enabled)
	bool connected; // If a PSP is served
	bool connected_wlan; // If the PSP served is connected over WLAN
	char connected_via[64]; // How the PSP served is connected (e.g. `USB` or `WLAN (192.168.1.20)`)
	int64_t connected_since; // When the PSP served connected, in seconds since 1970 (UTC)
	uint32_t frames_per_second; // Frames sent to the PSP during the last second
	uint64_t bytes_per_second; // Bytes sent to the PSP during the last second
	uint64_t total_frames; // Frames sent to the PSP served
	uint64_t total_bytes; // Bytes sent to the PSP served
	char control[PSPDISP_COM_NAME_SIZE]; // What the PSP controls drive (e.g. `Gamepad`)
	char control_error[256]; // Why what the PSP controls drive can't be created (`""` if it works)
	bool display; // If the virtual display was created
	bool sound; // If the sound is captured
	uint32_t open_menu_requests; // Count of the requests of the PSP to open the menu of PSPdisp (the "OpenMenu" action of the control files), the desktop app shows itself on each one
} PSPdispStatus;

/**
 * @brief Get the path of the socket of the daemon, in a folder only the user can reach, which depends on the platform.
 *
 * @return The path, free it with `g_free()`.
 */
char *pspdisp_status_socket_path() {
	const char *folder = g_get_user_runtime_dir();
	return g_build_filename(folder, "pspdisp", "socket", NULL);
}

/**
 * @brief Make the message of a status: a key file, ended by a null character.
 *
 * @param status The status.
 * @return The message, free it with `g_bytes_unref()`.
 */
GBytes *pspdisp_status_message_new(const PSPdispStatus *status) {
	GKeyFile *file = g_key_file_new();

	g_key_file_set_int64(file, "Status", "PID", status->pid);
	g_key_file_set_boolean(file, "Status", "Enabled", status->enabled);
	g_key_file_set_boolean(file, "Status", "USB", status->usb);
	g_key_file_set_boolean(file, "Status", "WLAN", status->wlan);
	g_key_file_set_integer(file, "Status", "WLANPort", status->wlan_port);
	g_key_file_set_string(file, "Status", "WLANError", status->wlan_error);
	g_key_file_set_boolean(file, "Status", "Connected", status->connected);
	g_key_file_set_boolean(file, "Status", "ConnectedWLAN", status->connected_wlan);
	g_key_file_set_string(file, "Status", "ConnectedVia", status->connected_via);
	g_key_file_set_int64(file, "Status", "ConnectedSince", status->connected_since);
	g_key_file_set_integer(file, "Status", "FramesPerSecond", status->frames_per_second);
	g_key_file_set_uint64(file, "Status", "BytesPerSecond", status->bytes_per_second);
	g_key_file_set_uint64(file, "Status", "TotalFrames", status->total_frames);
	g_key_file_set_uint64(file, "Status", "TotalBytes", status->total_bytes);
	g_key_file_set_string(file, "Status", "Control", status->control);
	g_key_file_set_string(file, "Status", "ControlError", status->control_error);
	g_key_file_set_boolean(file, "Status", "Display", status->display);
	g_key_file_set_boolean(file, "Status", "Sound", status->sound);
	g_key_file_set_integer(file, "Status", "OpenMenuRequests", status->open_menu_requests);

	gsize length;
	char *content = g_key_file_to_data(file, &length, NULL);
	g_key_file_free(file);
	return g_bytes_new_take(content, length + 1); // With the null character that ends the text
}

/**
 * @brief Read a status from the key file of a message, without its null character.
 *
 * @param status Where to write the status (zeroed when the message isn't a key file).
 * @param text The key file.
 * @param length The length of the key file.
 * @return `true` if it was read.
 */
bool pspdisp_status_message_read(PSPdispStatus *status, const char *text, gsize length) {
	GKeyFile *file = g_key_file_new();
	bool loaded = g_key_file_load_from_data(file, text, length, G_KEY_FILE_NONE, NULL);

	*status = (PSPdispStatus) {};

	if (loaded) {
		status->pid = g_key_file_get_int64(file, "Status", "PID", NULL);
		status->enabled = g_key_file_get_boolean(file, "Status", "Enabled", NULL);
		status->usb = g_key_file_get_boolean(file, "Status", "USB", NULL);
		status->wlan = g_key_file_get_boolean(file, "Status", "WLAN", NULL);
		status->wlan_port = g_key_file_get_integer(file, "Status", "WLANPort", NULL);
		pspdisp_settings_read_string(file, "Status", "WLANError", status->wlan_error, sizeof(status->wlan_error));
		status->connected = g_key_file_get_boolean(file, "Status", "Connected", NULL);
		status->connected_wlan = g_key_file_get_boolean(file, "Status", "ConnectedWLAN", NULL);
		pspdisp_settings_read_string(file, "Status", "ConnectedVia", status->connected_via, sizeof(status->connected_via));
		status->connected_since = g_key_file_get_int64(file, "Status", "ConnectedSince", NULL);
		status->frames_per_second = g_key_file_get_integer(file, "Status", "FramesPerSecond", NULL);
		status->bytes_per_second = g_key_file_get_uint64(file, "Status", "BytesPerSecond", NULL);
		status->total_frames = g_key_file_get_uint64(file, "Status", "TotalFrames", NULL);
		status->total_bytes = g_key_file_get_uint64(file, "Status", "TotalBytes", NULL);
		pspdisp_settings_read_string(file, "Status", "Control", status->control, sizeof(status->control));
		pspdisp_settings_read_string(file, "Status", "ControlError", status->control_error, sizeof(status->control_error));
		status->display = g_key_file_get_boolean(file, "Status", "Display", NULL);
		status->sound = g_key_file_get_boolean(file, "Status", "Sound", NULL);
		status->open_menu_requests = g_key_file_get_integer(file, "Status", "OpenMenuRequests", NULL);
	}

	g_key_file_free(file);
	return loaded;
}

/**
 * @brief The socket of the daemon, sending its status to the desktop apps connected.
 */
typedef struct {
	GSocketService *service; // Accepts the apps, on the main context
	char *path; // Path of the socket, removed when it's destroyed
	GPtrArray *apps; // The apps connected (`PSPdispStatusApp *`)
	GBytes *last; // The last message, sent to each app as it connects (`NULL` before the first)
	GCancellable *cancellable; // Cancelled when it's destroyed, so the sends still running end without touching it
} PSPdispStatusServer;

/**
 * @brief A desktop app connected to the daemon.
 */
typedef struct {
	PSPdispStatusServer *server; // The socket it's connected to
	GSocketConnection *connection; // The connection
	GBytes *sending; // The message being sent (`NULL` if none)
	GBytes *pending; // The newest message, sent after `sending` (`NULL` if none), an older one is dropped
} PSPdispStatusApp;

void pspdisp_status_app_send(PSPdispStatusApp *app, GBytes *message);

/**
 * @brief Free an app, closing its connection.
 *
 * @param app The app.
 */
void pspdisp_status_app_free(PSPdispStatusApp *app) {
	g_io_stream_close(G_IO_STREAM(app->connection), NULL, NULL);
	g_object_unref(app->connection);
	g_clear_pointer(&app->sending, g_bytes_unref);
	g_clear_pointer(&app->pending, g_bytes_unref);
	g_free(app);
}

/**
 * @brief Send the next message to an app, or drop the app when it's gone, called by GIO once a message was sent.
 *
 * @param stream The output stream of the connection.
 * @param result The result of the send.
 * @param app The app, freed when the server is destroyed (then the send was cancelled).
 */
void pspdisp_status_app_on_sent(GOutputStream *stream, GAsyncResult *result, PSPdispStatusApp *app) {
	GError *error = NULL;

	if (!g_output_stream_write_all_finish(stream, result, NULL, &error)) {
		bool cancelled = g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
		g_error_free(error);

		if (!cancelled)
			g_ptr_array_remove(app->server->apps, app); // The app closed the connection, it's freed
		return;
	}

	g_clear_pointer(&app->sending, g_bytes_unref);

	if (app->pending != NULL) {
		GBytes *message = app->pending;
		app->pending = NULL;
		pspdisp_status_app_send(app, message);
		g_bytes_unref(message);
	}
}

/**
 * @brief Send a message to an app, without waiting: while one is being sent, it's kept for after, in place of an older one.
 *
 * @param app The app.
 * @param message The message.
 */
void pspdisp_status_app_send(PSPdispStatusApp *app, GBytes *message) {
	if (app->sending != NULL) {
		g_clear_pointer(&app->pending, g_bytes_unref);
		app->pending = g_bytes_ref(message);
		return;
	}

	gsize size;
	const void *data = g_bytes_get_data(message, &size);
	GOutputStream *stream = g_io_stream_get_output_stream(G_IO_STREAM(app->connection));

	app->sending = g_bytes_ref(message);
	g_output_stream_write_all_async(stream, data, size, G_PRIORITY_DEFAULT, app->server->cancellable, (GAsyncReadyCallback) pspdisp_status_app_on_sent, app);
}

/**
 * @brief Take an app that connected, and send it the last status, called by GIO on the main context.
 *
 * @param service The service of the server.
 * @param connection The connection of the app.
 * @param source Unused.
 * @param server The server.
 * @return Always `TRUE`, the connection is taken.
 */
gboolean pspdisp_status_server_on_incoming(GSocketService *service, GSocketConnection *connection, GObject *source, PSPdispStatusServer *server) {
	(void) service;
	(void) source;

	PSPdispStatusApp *app = g_new0(PSPdispStatusApp, 1);
	app->server = server;
	app->connection = g_object_ref(connection);
	g_ptr_array_add(server->apps, app);

	if (server->last != NULL)
		pspdisp_status_app_send(app, server->last);
	return TRUE;
}

/**
 * @brief Start listening for the desktop apps, on the main context.
 *
 * @param error Where to tell why it failed, `G_IO_ERROR_ADDRESS_IN_USE` when another daemon runs.
 * @return The server, free it with `pspdisp_status_server_destroy()`, or `NULL` if it failed.
 */
PSPdispStatusServer *pspdisp_status_server_new(GError **error) {
	char *path = pspdisp_status_socket_path();
	char *folder = g_path_get_dirname(path);
	GSocketAddress *address = g_unix_socket_address_new(path);

	g_mkdir_with_parents(folder, 0700);
	g_free(folder);

	// A socket that answers is another daemon running, one left by a daemon that died answers nothing and is replaced
	GSocketClient *probe = g_socket_client_new();
	GSocketConnection *other = g_socket_client_connect(probe, G_SOCKET_CONNECTABLE(address), NULL, NULL);
	g_object_unref(probe);

	if (other != NULL) {
		g_set_error(error, G_IO_ERROR, G_IO_ERROR_ADDRESS_IN_USE, "its socket %s answers", path);
		g_object_unref(other);
		g_object_unref(address);
		g_free(path);
		return NULL;
	}
	g_remove(path);

	GSocketService *service = g_socket_service_new();

	if (!g_socket_listener_add_address(G_SOCKET_LISTENER(service), address, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, NULL, NULL, error)) {
		g_prefix_error(error, "failed to listen on %s: ", path);
		g_object_unref(service);
		g_object_unref(address);
		g_free(path);
		return NULL;
	}
	g_object_unref(address);

	PSPdispStatusServer *server = g_new0(PSPdispStatusServer, 1);
	server->service = service;
	server->path = path;
	server->apps = g_ptr_array_new_with_free_func((GDestroyNotify) pspdisp_status_app_free);
	server->cancellable = g_cancellable_new();

	g_signal_connect(service, "incoming", G_CALLBACK(pspdisp_status_server_on_incoming), server);
	g_socket_service_start(service);
	return server;
}

/**
 * @brief Send a status to the desktop apps connected, and to the ones that connect after, on the main context.
 *
 * @param server The server.
 * @param status The status.
 */
void pspdisp_status_server_send(PSPdispStatusServer *server, const PSPdispStatus *status) {
	GBytes *message = pspdisp_status_message_new(status);

	g_clear_pointer(&server->last, g_bytes_unref);
	server->last = message;

	for (guint i = 0; i < server->apps->len; i++)
		pspdisp_status_app_send(server->apps->pdata[i], message);
}

/**
 * @brief Stop listening, close the connections and remove the socket, so the desktop apps tell the daemon doesn't run.
 *
 * @param server The server to destroy.
 */
void pspdisp_status_server_destroy(PSPdispStatusServer *server) {
	g_cancellable_cancel(server->cancellable);
	g_socket_service_stop(server->service);
	g_socket_listener_close(G_SOCKET_LISTENER(server->service));
	g_object_unref(server->service);
	g_ptr_array_unref(server->apps);
	g_remove(server->path);

	g_clear_pointer(&server->last, g_bytes_unref);
	g_object_unref(server->cancellable);
	g_free(server->path);
	g_free(server);
}

/**
 * @brief Type of a function pointer to handle the status of the daemon, called on the main context.
 *
 * @param status The status, `NULL` when the daemon doesn't run.
 * @param data The data given with the function.
 */
typedef void (*PSPdispStatusHandler) (const PSPdispStatus *status, gpointer data);

/**
 * @brief The connection of the desktop app to the daemon, for its status.
 */
typedef struct {
	PSPdispStatusHandler handler; // Handle each status
	gpointer data; // Given to `handler`
	GSocketClient *client; // Connects to the socket of the daemon
	GDataInputStream *input; // The messages of the daemon (`NULL` while not connected)
	GSocketConnection *connection; // The connection (`NULL` while not connected)
	bool told; // If the handler was told that the daemon doesn't run, since the last status
} PSPdispStatusClient;

void pspdisp_status_client_connect(PSPdispStatusClient *client);

/**
 * @brief Tell the handler that the daemon doesn't run, once until it runs again, then try to connect again later.
 *
 * @param client The client.
 */
void pspdisp_status_client_lost(PSPdispStatusClient *client) {
	g_clear_object(&client->input);
	if (client->connection != NULL)
		g_io_stream_close(G_IO_STREAM(client->connection), NULL, NULL);
	g_clear_object(&client->connection);

	if (!client->told)
		client->handler(NULL, client->data);
	client->told = true;

	g_timeout_add_once(PSPDISP_STATUS_INTERVAL, (GSourceOnceFunc) pspdisp_status_client_connect, client);
}

/**
 * @brief Handle a message of the daemon, then wait for the next one, called by GIO once one was read.
 *
 * @param input The messages of the daemon.
 * @param result The result of the read.
 * @param client The client.
 */
void pspdisp_status_client_on_message(GDataInputStream *input, GAsyncResult *result, PSPdispStatusClient *client) {
	gsize length = 0;
	char *text = g_data_input_stream_read_upto_finish(input, result, &length, NULL);
	GError *error = NULL;

	// The null character that ends the message is still to take, it's missing when the daemon closed the connection (it stopped or died)
	if (text != NULL)
		g_data_input_stream_read_byte(input, NULL, &error);

	if (text == NULL || error != NULL) {
		g_clear_error(&error);
		g_free(text);
		pspdisp_status_client_lost(client);
		return;
	}

	PSPdispStatus status;
	if (pspdisp_status_message_read(&status, text, length)) {
		client->told = false;
		client->handler(&status, client->data);
	}
	g_free(text);

	g_data_input_stream_read_upto_async(input, "", 1, G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) pspdisp_status_client_on_message, client);
}

/**
 * @brief Start reading the messages of the daemon, or try again later, called by GIO once the connection was made or failed.
 *
 * @param socket_client The client of the socket.
 * @param result The result of the connection.
 * @param client The client.
 */
void pspdisp_status_client_on_connected(GSocketClient *socket_client, GAsyncResult *result, PSPdispStatusClient *client) {
	client->connection = g_socket_client_connect_finish(socket_client, result, NULL);

	if (client->connection == NULL) {
		pspdisp_status_client_lost(client);
		return;
	}

	client->input = g_data_input_stream_new(g_io_stream_get_input_stream(G_IO_STREAM(client->connection)));
	g_data_input_stream_read_upto_async(client->input, "", 1, G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) pspdisp_status_client_on_message, client);
}

/**
 * @brief Try to connect to the daemon, without waiting.
 *
 * @param client The client.
 */
void pspdisp_status_client_connect(PSPdispStatusClient *client) {
	char *path = pspdisp_status_socket_path();
	GSocketAddress *address = g_unix_socket_address_new(path);

	g_socket_client_connect_async(client->client, G_SOCKET_CONNECTABLE(address), NULL, (GAsyncReadyCallback) pspdisp_status_client_on_connected, client);

	g_object_unref(address);
	g_free(path);
}

/**
 * @brief Connect to the daemon for its status, and again each time it starts, for as long as the app runs.
 *
 * The handler gets each status the daemon sends, and `NULL` once when the daemon doesn't run (also right away when it doesn't run yet).
 *
 * @param handler Handle the status, called on the main context.
 * @param data Given to the handler.
 * @return The client.
 */
PSPdispStatusClient *pspdisp_status_client_new(PSPdispStatusHandler handler, gpointer data) {
	PSPdispStatusClient *client = g_new0(PSPdispStatusClient, 1);
	client->handler = handler;
	client->data = data;
	client->client = g_socket_client_new();

	pspdisp_status_client_connect(client);
	return client;
}
