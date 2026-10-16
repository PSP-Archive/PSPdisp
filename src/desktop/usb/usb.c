/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * usb.c - USB link with the PSP                     *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The USB link with a PSP running the PSPdisp app in USB mode.
 *
 * The app runs the USBHostFS driver of PSPLink, which shows up as a Sony device with two bulk endpoints (0x02 to the PSP, 0x81 from it).
 * The link waits for a PSP to be plugged, opens it and does the handshake of USBHostFS (see `pspdisp_usb_handshake()`), then the data goes through those endpoints as it is.
 *
 * The system tells when a PSP is plugged or unplugged, through the USB host of the platform (see `host.c`), so nothing is polled while no PSP is plugged.
 * A plugged PSP that doesn't answer (e.g. the app is still starting its driver, or it stopped answering) is tried again every `PSPDISP_USB_RETRY_INTERVAL`.
 * Every wait and every transfer can be aborted with a `GCancellable`, e.g. to quit at once.
 */

#pragma once

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include "./host.c"

#define PSPDISP_USB_HOSTFS_MAGIC      0x782F0812 // `HOSTFS_MAGIC` of USBHostFS, starts the handshake
#define PSPDISP_USB_HANDSHAKE_TIMEOUT 500        // Timeout of each step of the handshake, in milliseconds
#define PSPDISP_USB_DRAIN_TIMEOUT     100        // Time to wait for what the PSP may still send after the handshake, in milliseconds
#define PSPDISP_USB_WRITE_TIMEOUT     10000      // Timeout to send a buffer, in milliseconds
#define PSPDISP_USB_RETRY_INTERVAL    1000       // Time between two tries of a plugged PSP that didn't answer, in milliseconds

/**
 * @brief The USB of the computer, watched for PSPs.
 */
typedef struct {
    PSPdispUSBWatch *watch; // Plugs and unplugs of the PSPs, from the USB host of the platform
    GHashTable *plugged; // Paths of the plugged PSPs (`char *`), each with if why it doesn't connect was told (`GINT_TO_POINTER()`)
} PSPdispUSB;

/**
 * @brief Start watching the USB of the computer for PSPs.
 *
 * @return The USB, free it with `pspdisp_usb_destroy()`, or `NULL` if the system can't tell the plugs.
 */
PSPdispUSB *pspdisp_usb_new() {
    PSPdispUSBWatch *watch = pspdisp_usb_watch_new();
    if (watch == NULL) return NULL;

    PSPdispUSB *usb = g_new0(PSPdispUSB, 1);
    usb->watch = watch;
    usb->plugged = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    return usb;
}

/**
 * @brief Stop watching the USB and free it, no PSP may be connected.
 *
 * @param usb The USB to destroy.
 */
void pspdisp_usb_destroy(PSPdispUSB *usb) {
    pspdisp_usb_watch_destroy(usb->watch);
    g_hash_table_destroy(usb->plugged);
    g_free(usb);
}

/**
 * @brief Take the plugs and unplugs the system told, without waiting.
 *
 * A PSP plugged twice (see `pspdisp_usb_watch_new()`) is only kept once.
 *
 * @param usb The USB.
 */
void pspdisp_usb_update(PSPdispUSB *usb) {
    bool plugged;
    char *path;

    while (pspdisp_usb_watch_next(usb->watch, &plugged, &path)) {
        g_message("PSP %s (%s)", plugged ? "plugged" : "unplugged", path);

        if (plugged && !g_hash_table_contains(usb->plugged, path)) {
            g_hash_table_insert(usb->plugged, path, GINT_TO_POINTER(false)); // Takes the path
            continue;
        }

        if (!plugged)
            g_hash_table_remove(usb->plugged, path);
        g_free(path);
    }
}

/**
 * @brief Do the handshake of USBHostFS, as the old desktop app (and PSPLink) does.
 *
 * The PSP waits for `HOSTFS_MAGIC`, then sends a hello command (12 bytes), and waits for it back.
 * When the PSP already did the handshake (e.g. the daemon restarted), it's reading frame headers, so it takes the magic and the next 12 bytes as a frame header with a wrong magic, and discards it.
 *
 * @param device The opened PSP.
 * @param cancellable Abort the handshake when cancelled (`NULL` to never abort it).
 * @param error Where to tell why it failed, with the step that failed.
 * @return `true` if the PSP is ready for the frames.
 */
bool pspdisp_usb_handshake(PSPdispUSBDevice *device, GCancellable *cancellable, GError **error) {
    uint8_t buffer[12] = {0};
    uint32_t magic = GUINT32_TO_LE(PSPDISP_USB_HOSTFS_MAGIC);
    uint32_t transferred = 0;

    memcpy(buffer, &magic, sizeof(magic));

    // The magic
    if (!pspdisp_usb_device_transfer(device, PSPDISP_USB_ENDPOINT_OUT, buffer, sizeof(magic), PSPDISP_USB_HANDSHAKE_TIMEOUT, cancellable, &transferred, error)) {
        g_prefix_error(error, "it didn't take the handshake magic: ");
        return false;
    }

    // The rest of a frame header: it only goes through when the PSP already did the handshake (it's not reading while it sends the hello)
    bool done = pspdisp_usb_device_transfer(device, PSPDISP_USB_ENDPOINT_OUT, buffer, sizeof(buffer), PSPDISP_USB_HANDSHAKE_TIMEOUT, cancellable, &transferred, NULL);

    // The hello command, sent back as its response
    if (!done) {
        if (!pspdisp_usb_device_transfer(device, PSPDISP_USB_ENDPOINT_IN, buffer, sizeof(buffer), PSPDISP_USB_HANDSHAKE_TIMEOUT, cancellable, &transferred, error)) {
            g_prefix_error(error, "it didn't send its hello: ");
            return false;
        }
        if (transferred != sizeof(buffer)) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "its hello was %u bytes instead of %zu", transferred, sizeof(buffer));
            return false;
        }
        if (!pspdisp_usb_device_transfer(device, PSPDISP_USB_ENDPOINT_OUT, buffer, sizeof(buffer), PSPDISP_USB_HANDSHAKE_TIMEOUT, cancellable, &transferred, error)) {
            g_prefix_error(error, "it didn't take its hello back: ");
            return false;
        }
    }

    // Drop what the PSP may still send (e.g. its hello, when the rest of the frame header was taken as its response), so the first response of the session isn't mixed with it
    uint8_t scrap[512];
    pspdisp_usb_device_transfer(device, PSPDISP_USB_ENDPOINT_IN, scrap, sizeof(scrap), PSPDISP_USB_DRAIN_TIMEOUT, cancellable, &transferred, NULL);

    return !g_cancellable_set_error_if_cancelled(cancellable, error);
}

/**
 * @brief Wait for a PSP running the PSPdisp app in USB mode, open it and do the handshake.
 *
 * It tries the plugged PSPs, then sleeps until the system tells a plug or an unplug.
 * While a plugged PSP didn't answer, it's tried again every `PSPDISP_USB_RETRY_INTERVAL`.
 * Why a PSP doesn't connect (e.g. it can't be opened, or the PSPdisp app doesn't answer) is told once per plug.
 *
 * @param usb The USB of the computer.
 * @param cancellable Abort the wait when cancelled (`NULL` to never abort it).
 * @return The PSP, ready for the frames, close it with `pspdisp_usb_disconnect()`, or `NULL` when cancelled.
 */
PSPdispUSBDevice *pspdisp_usb_connect(PSPdispUSB *usb, GCancellable *cancellable) {
    GPollFD fds[2];
    pspdisp_usb_watch_make_pollfd(usb->watch, &fds[0]);
    int fds_count = 1 + g_cancellable_make_pollfd(cancellable, &fds[1]);
    PSPdispUSBDevice *device = NULL;

    while (device == NULL && !g_cancellable_is_cancelled(cancellable)) {
        pspdisp_usb_update(usb);

        GHashTableIter iter;
        gpointer path, told;
        g_hash_table_iter_init(&iter, usb->plugged);

        while (device == NULL && g_hash_table_iter_next(&iter, &path, &told)) {
            GError *error = NULL;
            device = pspdisp_usb_device_open(path, &error);

            if (device == NULL) {
                if (!GPOINTER_TO_INT(told))
                    g_warning("%s(): %s", __FUNCTION__, error->message);
                g_hash_table_iter_replace(&iter, GINT_TO_POINTER(true));
                g_error_free(error);
            } else if (!pspdisp_usb_handshake(device, cancellable, &error)) {
                // Not a warning, it may only be starting (e.g. the app still starts its driver)
                if (!GPOINTER_TO_INT(told) && !g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
                    g_message("The PSP (%s) doesn't answer, tried again every %d ms (is the PSPdisp app running in USB mode?): %s", (const char *) path, PSPDISP_USB_RETRY_INTERVAL, error->message);
                g_hash_table_iter_replace(&iter, GINT_TO_POINTER(true));
                g_error_free(error);
                pspdisp_usb_device_close(device);
                device = NULL;
            } else {
                g_hash_table_iter_replace(&iter, GINT_TO_POINTER(false)); // Told again if it can't be opened after this connection
            }
        }

        // Sleep until a plug or an unplug, or until the next try of the plugged PSPs
        if (device == NULL)
            g_poll(fds, fds_count, g_hash_table_size(usb->plugged) > 0 ? PSPDISP_USB_RETRY_INTERVAL : -1);
    }

    if (fds_count > 1)
        g_cancellable_release_fd(cancellable);
    return device;
}

/**
 * @brief Close a PSP connected by `pspdisp_usb_connect()`.
 *
 * While it stays plugged, the next `pspdisp_usb_connect()` tries it again.
 *
 * @param device The PSP.
 */
void pspdisp_usb_disconnect(PSPdispUSBDevice *device) {
    pspdisp_usb_device_close(device);
}

/**
 * @brief Tell why a transfer failed, unless it was cancelled or the PSP was unplugged (the caller already knows why then).
 *
 * @param function The function that failed, for the message.
 * @param what What failed, for the message.
 * @param error Why it failed, it's freed.
 */
void pspdisp_usb_tell_failure(const char *function, const char *what, GError *error) {
    if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED))
        g_debug("%s(): %s: %s", function, what, error->message);
    else if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("%s(): %s: %s", function, what, error->message);
    g_error_free(error);
}

/**
 * @brief Send a whole buffer to the PSP.
 *
 * @param device The PSP.
 * @param buffer The buffer.
 * @param size The size of the buffer in bytes.
 * @param cancellable Abort the transfer when cancelled (`NULL` to never abort it).
 * @return `true` if it was all sent.
 */
bool pspdisp_usb_write(PSPdispUSBDevice *device, const void *buffer, uint32_t size, GCancellable *cancellable) {
    GError *error = NULL;
    uint32_t transferred = 0;

    if (pspdisp_usb_device_transfer(device, PSPDISP_USB_ENDPOINT_OUT, (void *) buffer, size, PSPDISP_USB_WRITE_TIMEOUT, cancellable, &transferred, &error)) return true;

    pspdisp_usb_tell_failure(__FUNCTION__, "failed to send to the PSP", error);
    return false;
}

/**
 * @brief Receive exactly `size` bytes from the PSP.
 *
 * @param device The PSP.
 * @param buffer Where to receive them.
 * @param size The size to receive in bytes.
 * @param timeout How long to wait, in milliseconds.
 * @param cancellable Abort the transfer when cancelled (`NULL` to never abort it).
 * @return `true` if they were all received.
 */
bool pspdisp_usb_read(PSPdispUSBDevice *device, void *buffer, uint32_t size, uint32_t timeout, GCancellable *cancellable) {
    GError *error = NULL;
    uint32_t transferred = 0;

    if (!pspdisp_usb_device_transfer(device, PSPDISP_USB_ENDPOINT_IN, buffer, size, timeout, cancellable, &transferred, &error)) {
        pspdisp_usb_tell_failure(__FUNCTION__, "failed to receive from the PSP", error);
        return false;
    }

    if (transferred != size) {
        g_warning("%s(): failed to receive from the PSP: short transfer (%u of %u bytes)", __FUNCTION__, transferred, size);
        return false;
    }

    return true;
}
