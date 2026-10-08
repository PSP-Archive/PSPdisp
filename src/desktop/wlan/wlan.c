/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * wlan.c - WLAN link with the PSP                   *
 *                                                   *
 * Copyright © 2026 Net-0                            *
 *****************************************************/

/*
 * The WLAN link with a PSP running the PSPdisp app in WLAN mode.
 *
 * The PSP joins the wireless network, connects to the desktop over TCP, and sends its password (`PSPDISP_WLAN_PASSWORD_SIZE` bytes): the desktop closes the connection when it doesn't match, and the PSP connects again.
 * So the desktop is the server: it listens on `PSPDISP_WLAN_PORT` (plus the port offset set on the PSP), on every network interface.
 * After the password, the data goes through the connection as it is, as the frames and the responses over USB.
 *
 * It's only a TCP server, through GIO, so it's the same on every platform: whether the PSP is on a wireless network doesn't matter to the desktop.
 * Every wait and every transfer can be aborted with a `GCancellable`, e.g. to quit at once.
 * The old protocol sends the password and the frames in clear, so anyone on the network can read them.
 */

#pragma once

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <gio/gnetworking.h>

#define PSPDISP_WLAN_PORT             17584 // TCP port the PSP connects to, plus the port offset set on it (0 to 9)
#define PSPDISP_WLAN_PASSWORD_SIZE    32    // Size in bytes of the password the PSP sends: a string, null-terminated unless it's 32 characters long
#define PSPDISP_WLAN_PASSWORD_TIMEOUT 3000  // Time for the PSP to send its password once connected, in milliseconds (it gives up after 3 seconds without data too)
#define PSPDISP_WLAN_WRITE_TIMEOUT    10000 // Timeout to send a buffer, in milliseconds
#define PSPDISP_WLAN_ACCEPT_RETRY     100   // Pause after a failure to accept a connection (e.g. too many files opened), in milliseconds

/**
 * @brief The TCP server the PSPs connect to.
 */
typedef struct {
    GSocketListener *listener; // Listens on the port, on every network interface
    char password[PSPDISP_WLAN_PASSWORD_SIZE]; // The password, padded with zeros
    char *refused; // Address of the last PSP refused for its password (`NULL` if none), so it's told once while it connects again
} PSPdispWLAN;

/**
 * @brief A connection with a PSP, its password checked.
 */
typedef struct {
    GSocket *socket; // The connection, not blocking: the waits go through `g_socket_condition_timed_wait()`
    char *address; // Address of the PSP (e.g. `192.168.1.20`), for the messages
} PSPdispWLANConnection;

/**
 * @brief Start listening for the PSPs.
 *
 * @param port The TCP port, `PSPDISP_WLAN_PORT` plus the port offset set on the PSP.
 * @param password The password set on the PSP, at most `PSPDISP_WLAN_PASSWORD_SIZE` characters (`""` for none).
 * @param error Where to tell why it failed (e.g. the port is used by another program).
 * @return The server, free it with `pspdisp_wlan_destroy()`, or `NULL` if it failed.
 */
PSPdispWLAN *pspdisp_wlan_new(uint16_t port, const char *password, GError **error) {
    if (strlen(password) > PSPDISP_WLAN_PASSWORD_SIZE) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "the password is longer than %d characters, the PSP can't send it", PSPDISP_WLAN_PASSWORD_SIZE);
        return NULL;
    }

    GSocketListener *listener = g_socket_listener_new();

    // On IPv4 and IPv6, but the PSP only knows IPv4
    if (!g_socket_listener_add_inet_port(listener, port, NULL, error)) {
        g_object_unref(listener);
        return NULL;
    }

    PSPdispWLAN *wlan = g_new0(PSPdispWLAN, 1);
    wlan->listener = listener;
    memcpy(wlan->password, password, strlen(password)); // The rest stays zero
    return wlan;
}

/**
 * @brief Stop listening and free the server, no PSP may be connected.
 *
 * @param wlan The server to destroy.
 */
void pspdisp_wlan_destroy(PSPdispWLAN *wlan) {
    g_socket_listener_close(wlan->listener);
    g_object_unref(wlan->listener);
    g_free(wlan->refused);
    g_free(wlan);
}

/**
 * @brief Send or receive exactly `size` bytes, waiting until the deadline.
 *
 * @param socket The connection.
 * @param sending If the bytes are sent (`true`) or received (`false`).
 * @param buffer The bytes to send, or where to receive them.
 * @param size The size of `buffer` in bytes.
 * @param deadline When it times out, in the time of `g_get_monotonic_time()`.
 * @param cancellable Abort it when cancelled (`NULL` to never abort it).
 * @param error Where to tell why it failed, in the `G_IO_ERROR` domain: `G_IO_ERROR_TIMED_OUT`, `G_IO_ERROR_CANCELLED`, `G_IO_ERROR_CONNECTION_CLOSED` (the PSP closed it), or another code.
 * @return `true` if all the bytes were transferred.
 */
bool pspdisp_wlan_transfer(GSocket *socket, bool sending, uint8_t *buffer, uint32_t size, gint64 deadline, GCancellable *cancellable, GError **error) {
    uint32_t position = 0;

    while (position < size) {
        gint64 remaining = deadline - g_get_monotonic_time();

        if (remaining <= 0) {
            g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "the PSP didn't answer in time");
            return false;
        }

        if (!g_socket_condition_timed_wait(socket, sending ? G_IO_OUT : G_IO_IN, remaining, cancellable, error)) return false;

        GError *failure = NULL;
        gssize count = sending ? g_socket_send(socket, (const gchar *) buffer + position, size - position, cancellable, &failure)
                               : g_socket_receive(socket, (gchar *) buffer + position, size - position, cancellable, &failure);

        if (count < 0) {
            if (g_error_matches(failure, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK)) { // Woken up for nothing
                g_error_free(failure);
                continue;
            }

            g_propagate_error(error, failure);
            return false;
        }

        if (count == 0 && !sending) {
            g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_CONNECTION_CLOSED, "the PSP closed the connection");
            return false;
        }

        position += count;
    }

    return true;
}

/**
 * @brief Get the address of the other end of a connection, for the messages.
 *
 * @param socket The connection.
 * @return The address (e.g. `192.168.1.20`, not as an IPv6 address mapping it), free it with `g_free()`.
 */
char *pspdisp_wlan_address(GSocket *socket) {
    GSocketAddress *remote = g_socket_get_remote_address(socket, NULL);
    char *address = NULL;

    if (G_IS_INET_SOCKET_ADDRESS(remote)) {
        address = g_inet_address_to_string(g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(remote)));

        // An IPv4 client of an IPv6 socket
        if (g_str_has_prefix(address, "::ffff:") && strchr(address, '.') != NULL) {
            char *ipv4 = g_strdup(address + strlen("::ffff:"));
            g_free(address);
            address = ipv4;
        }
    }

    g_clear_object(&remote);
    return address != NULL ? address : g_strdup("unknown address");
}

/**
 * @brief Check the password a PSP sends once connected, then make the connection.
 *
 * Both passwords are compared as strings of at most `PSPDISP_WLAN_PASSWORD_SIZE` characters, as the PSP may send anything after the null character.
 * They're compared in a constant time, so the time of the answer doesn't tell how much of the password was right.
 *
 * @param wlan The server.
 * @param socket The accepted connection, closed when the password doesn't match.
 * @param cancellable Abort the wait for the password when cancelled.
 * @return The connection, or `NULL` if the password didn't come or didn't match.
 */
PSPdispWLANConnection *pspdisp_wlan_check(PSPdispWLAN *wlan, GSocket *socket, GCancellable *cancellable) {
    char *address = pspdisp_wlan_address(socket);
    uint8_t received[PSPDISP_WLAN_PASSWORD_SIZE] = {};
    GError *error = NULL;

    g_socket_set_blocking(socket, FALSE);
    g_socket_set_option(socket, IPPROTO_TCP, TCP_NODELAY, 1, NULL); // The frame header and its data go at once, without waiting for the PSP to acknowledge the header

    if (!pspdisp_wlan_transfer(socket, false, received, sizeof(received), g_get_monotonic_time() + PSPDISP_WLAN_PASSWORD_TIMEOUT * 1000, cancellable, &error)) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) g_debug("%s(): no password from %s: %s", __FUNCTION__, address, error->message);
        g_error_free(error);
        g_free(address);
        return NULL;
    }

    // As strings: what follows the null character is zeroed, then all the bytes are compared
    size_t length = strnlen((const char *) received, sizeof(received));
    memset(&received[length], 0, sizeof(received) - length);

    uint8_t difference = 0;
    for (size_t i = 0; i < sizeof(received); i++) difference |= received[i] ^ (uint8_t) wlan->password[i];

    if (difference != 0) {
        if (g_strcmp0(wlan->refused, address) != 0) g_warning("%s(): the PSP at %s sent a wrong password, set the same on the desktop and on the PSP", __FUNCTION__, address);
        g_free(wlan->refused);
        wlan->refused = address;
        return NULL;
    }

    g_clear_pointer(&wlan->refused, g_free);

    PSPdispWLANConnection *connection = g_new0(PSPdispWLANConnection, 1);
    connection->socket = g_object_ref(socket);
    connection->address = address;
    return connection;
}

/**
 * @brief Wait for a PSP running the PSPdisp app in WLAN mode to connect with the right password.
 *
 * The connections without the right password are closed, then the PSP connects again by itself.
 * A wrong password is told once while the same PSP connects again.
 *
 * @param wlan The server.
 * @param cancellable Abort the wait when cancelled (`NULL` to never abort it).
 * @return The connection, ready for the frames, close it with `pspdisp_wlan_disconnect()`, or `NULL` when cancelled.
 */
PSPdispWLANConnection *pspdisp_wlan_connect(PSPdispWLAN *wlan, GCancellable *cancellable) {
    while (!g_cancellable_is_cancelled(cancellable)) {
        GError *error = NULL;
        GSocket *socket = g_socket_listener_accept_socket(wlan->listener, NULL, cancellable, &error);

        if (socket == NULL) {
            if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
                g_warning("%s(): failed to accept a connection: %s", __FUNCTION__, error->message);
                g_usleep(PSPDISP_WLAN_ACCEPT_RETRY * 1000);
            }
            g_error_free(error);
            continue;
        }

        PSPdispWLANConnection *connection = pspdisp_wlan_check(wlan, socket, cancellable);

        if (connection == NULL) g_socket_close(socket, NULL);
        g_object_unref(socket);

        if (connection != NULL) return connection;
    }

    return NULL;
}

/**
 * @brief Close a connection made by `pspdisp_wlan_connect()`.
 *
 * The PSP then connects again by itself.
 *
 * @param connection The connection.
 */
void pspdisp_wlan_disconnect(PSPdispWLANConnection *connection) {
    g_socket_close(connection->socket, NULL);
    g_object_unref(connection->socket);
    g_free(connection->address);
    g_free(connection);
}

/**
 * @brief Tell why a transfer failed, unless it was cancelled or the PSP closed the connection (the caller already knows why then).
 *
 * @param function The function that failed, for the message.
 * @param what What failed, for the message.
 * @param error Why it failed, it's freed.
 */
void pspdisp_wlan_tell_failure(const char *function, const char *what, GError *error) {
    if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CONNECTION_CLOSED)) g_debug("%s(): %s: %s", function, what, error->message);
    else if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) g_warning("%s(): %s: %s", function, what, error->message);
    g_error_free(error);
}

/**
 * @brief Send a whole buffer to the PSP.
 *
 * @param connection The connection.
 * @param buffer The buffer.
 * @param size The size of the buffer in bytes.
 * @param cancellable Abort the transfer when cancelled (`NULL` to never abort it).
 * @return `true` if it was all sent.
 */
bool pspdisp_wlan_write(PSPdispWLANConnection *connection, const void *buffer, uint32_t size, GCancellable *cancellable) {
    GError *error = NULL;

    if (pspdisp_wlan_transfer(connection->socket, true, (uint8_t *) buffer, size, g_get_monotonic_time() + PSPDISP_WLAN_WRITE_TIMEOUT * 1000, cancellable, &error)) return true;

    pspdisp_wlan_tell_failure(__FUNCTION__, "failed to send to the PSP", error);
    return false;
}

/**
 * @brief Receive exactly `size` bytes from the PSP.
 *
 * @param connection The connection.
 * @param buffer Where to receive them.
 * @param size The size to receive in bytes.
 * @param timeout How long to wait, in milliseconds.
 * @param cancellable Abort the transfer when cancelled (`NULL` to never abort it).
 * @return `true` if they were all received.
 */
bool pspdisp_wlan_read(PSPdispWLANConnection *connection, void *buffer, uint32_t size, uint32_t timeout, GCancellable *cancellable) {
    GError *error = NULL;

    if (pspdisp_wlan_transfer(connection->socket, false, buffer, size, g_get_monotonic_time() + (gint64) timeout * 1000, cancellable, &error)) return true;

    pspdisp_wlan_tell_failure(__FUNCTION__, "failed to receive from the PSP", error);
    return false;
}
