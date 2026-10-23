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
 * It goes through GUsb, the GObject library over libusb, the same on every platform.
 * The plugs and unplugs are the signals of a GUsb context, emitted on the global default context (GUsb adds them there with `g_idle_add()`), so they only come while its main loop runs (e.g. the one of the daemon).
 * They're queued for the link, and each one wakes up the context the link waits on (see `pspdisp_usb_watch_wait()`), so nothing is polled while no PSP is plugged.
 * A PSP is known by its path, the platform ID GUsb gives it, the same on its plug and on its unplug.
 * The data goes through the asynchronous bulk transfers of GUsb, each waited for on a context of its own.
 * A plugged PSP that doesn't answer (e.g. the app is still starting its driver, or it stopped answering) is tried again every `PSPDISP_USB_RETRY_INTERVAL`.
 * Every wait and every transfer can be aborted with a `GCancellable`, e.g. to quit at once.
 *
 * On Linux, the udev rule `daemon/60-pspdisp.rules` gives the PSP to the user.
 * On Windows, the PSP needs the WinUSB driver: the "WinUSB (PSP Type C)" USB mode of the PSP app, or WinUSB installed with Zadig for type B.
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <gusb.h>

#define PSPDISP_USB_HOST_NAME "GUsb" // User friendly name of the USB host

#define PSPDISP_USB_VENDOR_ID         0x054C // Sony
#define PSPDISP_USB_PRODUCT_ID_TYPE_B 0x01C9 // USBHostFS, as type B (the default "USB driver" of the PSP app)
#define PSPDISP_USB_PRODUCT_ID_TYPE_C 0x01CA // USBHostFS, as type C
#define PSPDISP_USB_INTERFACE         0      // The only interface of the PSP, with both endpoints
#define PSPDISP_USB_ENDPOINT_OUT      0x02   // Bulk endpoint to the PSP
#define PSPDISP_USB_ENDPOINT_IN       0x81   // Bulk endpoint from the PSP

#define PSPDISP_USB_HOSTFS_MAGIC      0x782F0812 // `HOSTFS_MAGIC` of USBHostFS, starts the handshake
#define PSPDISP_USB_HANDSHAKE_TIMEOUT 500        // Timeout of each step of the handshake, in milliseconds
#define PSPDISP_USB_DRAIN_TIMEOUT     100        // Time to wait for what the PSP may still send after the handshake, in milliseconds
#define PSPDISP_USB_WRITE_TIMEOUT     10000      // Timeout to send a buffer, in milliseconds
#define PSPDISP_USB_RETRY_INTERVAL    1000       // Time between two tries of a plugged PSP that didn't answer, in milliseconds

// What gives the program access to a PSP it can't open
#define PSPDISP_USB_ACCESS_HINT "install the udev rule (sudo make install) then plug the PSP again"

/**
 * @brief Watch of the PSPs being plugged and unplugged, created by `pspdisp_usb_watch_new()`.
 */
typedef struct {
	GMainContext *context; // What the link waits on, run by `pspdisp_usb_watch_wait()`, woken up by each plug and unplug
	GUsbContext *usb; // The GUsb context, with its devices
	GAsyncQueue *events; // Plugs and unplugs of the PSPs not taken yet (`PSPdispUSBEvent *`), oldest first, pushed from the thread of the global default context
} PSPdispUSBWatch;

/**
 * @brief A plug or an unplug of a PSP, waiting to be taken by `pspdisp_usb_watch_next()`.
 */
typedef struct {
	bool plugged; // If the PSP was plugged (`true`) or unplugged (`false`)
	char *path; // The platform ID of the PSP
} PSPdispUSBEvent;

/**
 * @brief An opened PSP, its interface claimed, created by `pspdisp_usb_device_open()`.
 */
typedef struct {
	GUsbDevice *usb; // The GUsb device, opened and its interface claimed
	GMainContext *context; // Where its transfers complete, run by `pspdisp_usb_device_transfer()` until they do
} PSPdispUSBDevice;

/**
 * @brief Tell if a USB device is a PSP running the PSPdisp app in USB mode, from its IDs.
 *
 * @param vendor_id The vendor ID of the device.
 * @param product_id The product ID of the device.
 * @return `true` for the USBHostFS driver of the PSP, as type B or C.
 */
bool pspdisp_usb_is_psp(uint32_t vendor_id, uint32_t product_id) {
	return vendor_id == PSPDISP_USB_VENDOR_ID && (product_id == PSPDISP_USB_PRODUCT_ID_TYPE_B || product_id == PSPDISP_USB_PRODUCT_ID_TYPE_C);
}

/**
 * @brief Free a plug or an unplug of a PSP.
 *
 * @param event The plug or unplug.
 */
void pspdisp_usb_event_free(PSPdispUSBEvent *event) {
	g_free(event->path);
	g_free(event);
}

/**
 * @brief Queue the plug or the unplug of a USB device, if it's a PSP, and wake up the link waiting for it, called by GUsb on the thread of the global default context.
 *
 * @param usb Unused, the GUsb context.
 * @param device The device.
 * @param watch The watch.
 * @param plugged If the device was plugged (`true`) or unplugged (`false`).
 */
void pspdisp_usb_watch_queue(GUsbContext *usb, GUsbDevice *device, PSPdispUSBWatch *watch, bool plugged) {
	(void) usb;
	if (!pspdisp_usb_is_psp(g_usb_device_get_vid(device), g_usb_device_get_pid(device))) return;

	PSPdispUSBEvent *event = g_new0(PSPdispUSBEvent, 1);
	event->plugged = plugged;
	event->path = g_strdup(g_usb_device_get_platform_id(device));
	g_async_queue_push(watch->events, event);
	g_main_context_wakeup(watch->context); // Ends the wait of the link, on its own thread (see `pspdisp_usb_watch_wait()`)
}

/**
 * @brief Queue the plug of a USB device, if it's a PSP, the handler of the `device-added` signal of GUsb.
 *
 * @param usb The GUsb context.
 * @param device The device.
 * @param watch The watch.
 */
void pspdisp_usb_watch_on_added(GUsbContext *usb, GUsbDevice *device, PSPdispUSBWatch *watch) {
	pspdisp_usb_watch_queue(usb, device, watch, true);
}

/**
 * @brief Queue the unplug of a USB device, if it's a PSP, the handler of the `device-removed` signal of GUsb.
 *
 * @param usb The GUsb context.
 * @param device The device.
 * @param watch The watch.
 */
void pspdisp_usb_watch_on_removed(GUsbContext *usb, GUsbDevice *device, PSPdispUSBWatch *watch) {
	pspdisp_usb_watch_queue(usb, device, watch, false);
}

/**
 * @brief Start watching the PSPs being plugged and unplugged, with a GUsb context.
 *
 * The PSPs already plugged are given first by `pspdisp_usb_watch_next()`, as plugs.
 * One plugged while the watch starts may be given twice, the USB link ignores the second one.
 * The ones plugged and unplugged after it only come while the global default context runs (e.g. the main loop of the daemon).
 *
 * @return The new watch, free it with `pspdisp_usb_watch_destroy()`, or `NULL` if it failed.
 */
PSPdispUSBWatch *pspdisp_usb_watch_new() {
	GError *error = NULL;
	GUsbContext *usb = g_usb_context_new(&error);

	if (usb == NULL) {
		g_warning("%s(): failed to watch the USB devices with GUsb: %s", __FUNCTION__, error->message);
		g_error_free(error);
		return NULL;
	}

	PSPdispUSBWatch *watch = g_new0(PSPdispUSBWatch, 1);
	watch->context = g_main_context_new();
	watch->usb = usb;
	watch->events = g_async_queue_new_full((GDestroyNotify) pspdisp_usb_event_free);

	// GUsb emits them on the global default context whatever context it's given (`g_usb_context_set_main_context()` is only for its synchronous transfers)
	g_signal_connect(usb, "device-added", G_CALLBACK(pspdisp_usb_watch_on_added), watch);
	g_signal_connect(usb, "device-removed", G_CALLBACK(pspdisp_usb_watch_on_removed), watch);
	g_usb_context_enumerate(usb); // The devices already plugged, as plugs
	return watch;
}

/**
 * @brief Stop watching and free the watch.
 *
 * @param watch The watch to destroy.
 */
void pspdisp_usb_watch_destroy(PSPdispUSBWatch *watch) {
	g_signal_handlers_disconnect_by_data(watch->usb, watch);
	g_object_unref(watch->usb);
	g_main_context_unref(watch->context);
	g_async_queue_unref(watch->events); // With the events not taken
	g_free(watch);
}

/**
 * @brief Take the next plug or unplug of a PSP, without waiting.
 *
 * @param watch The watch.
 * @param plugged Where to write if the PSP was plugged (`true`) or unplugged (`false`).
 * @param path Where to write the path of the PSP, free it with `g_free()`.
 * @return `true` if there was one, `false` if there's none for now.
 */
bool pspdisp_usb_watch_next(PSPdispUSBWatch *watch, bool *plugged, char **path) {
	PSPdispUSBEvent *event = g_async_queue_try_pop(watch->events);
	if (event == NULL) return false;

	*plugged = event->plugged;
	*path = g_steal_pointer(&event->path);
	pspdisp_usb_event_free(event);
	return true;
}

/**
 * @brief Tell that the wait is over, the callback of the timeout of `pspdisp_usb_watch_wait()`.
 *
 * @param data Where to write that the wait is over (`bool *`).
 * @return Always `G_SOURCE_REMOVE`, it's done once.
 */
gboolean pspdisp_usb_watch_on_timeout(gpointer data) {
	*(bool *) data = true;
	return G_SOURCE_REMOVE;
}

/**
 * @brief Tell that the wait is over, the callback of the cancellation of `pspdisp_usb_watch_wait()`.
 *
 * @param cancellable Unused, the cancelled cancellable.
 * @param data Where to write that the wait is over (`bool *`).
 * @return Always `G_SOURCE_REMOVE`, it's done once.
 */
gboolean pspdisp_usb_watch_on_cancelled(GCancellable *cancellable, gpointer data) {
	(void) cancellable;
	*(bool *) data = true;
	return G_SOURCE_REMOVE;
}

/**
 * @brief Wait for a plug or an unplug, once `pspdisp_usb_watch_next()` gave all it had.
 *
 * It returns when one comes, after the timeout, or once cancelled, whichever comes first.
 *
 * @param watch The watch.
 * @param timeout How long to wait at most, in milliseconds (-1 for no limit).
 * @param cancellable Stop waiting when cancelled (`NULL` to never stop).
 */
void pspdisp_usb_watch_wait(PSPdispUSBWatch *watch, int timeout, GCancellable *cancellable) {
	bool over = false; // If the timeout or the cancellation came
	GSource *timeout_source = timeout >= 0 ? g_timeout_source_new(timeout) : NULL;
	GSource *cancelled_source = cancellable != NULL ? g_cancellable_source_new(cancellable) : NULL;

	if (timeout_source != NULL) {
		g_source_set_callback(timeout_source, pspdisp_usb_watch_on_timeout, &over, NULL);
		g_source_attach(timeout_source, watch->context);
	}
	if (cancelled_source != NULL) {
		g_source_set_callback(cancelled_source, G_SOURCE_FUNC(pspdisp_usb_watch_on_cancelled), &over, NULL); // Called as `GCancellableSourceFunc`
		g_source_attach(cancelled_source, watch->context);
	}

	// Each plug and unplug wakes it up (see `pspdisp_usb_watch_queue()`), even one queued before it blocks
	while (!over && g_async_queue_length(watch->events) == 0)
		g_main_context_iteration(watch->context, TRUE);

	if (timeout_source != NULL) {
		g_source_destroy(timeout_source);
		g_source_unref(timeout_source);
	}
	if (cancelled_source != NULL) {
		g_source_destroy(cancelled_source);
		g_source_unref(cancelled_source);
	}
}

/**
 * @brief Make the error of the USB host from an error of GUsb.
 *
 * @param error The error of GUsb, it's freed.
 * @return The error, in the `G_IO_ERROR` domain.
 */
GError *pspdisp_usb_error(GError *error) {
	if (error->domain != G_USB_DEVICE_ERROR) return error; // Already one of GIO (e.g. cancelled before it started)

	GIOErrorEnum code;
	switch (error->code) {
		case G_USB_DEVICE_ERROR_TIMED_OUT:
			code = G_IO_ERROR_TIMED_OUT;
			break;
		case G_USB_DEVICE_ERROR_CANCELLED:
			code = G_IO_ERROR_CANCELLED;
			break;
		case G_USB_DEVICE_ERROR_NO_DEVICE:
			code = G_IO_ERROR_NOT_CONNECTED;
			break;
		case G_USB_DEVICE_ERROR_PERMISSION_DENIED:
			code = G_IO_ERROR_PERMISSION_DENIED;
			break;
		case G_USB_DEVICE_ERROR_BUSY:
			code = G_IO_ERROR_BUSY;
			break;
		case G_USB_DEVICE_ERROR_NOT_SUPPORTED:
			code = G_IO_ERROR_NOT_SUPPORTED;
			break;
		default:
			code = G_IO_ERROR_FAILED;
			break;
	}

	GError *converted = g_error_new(G_IO_ERROR, code, "%s", code == G_IO_ERROR_NOT_CONNECTED ? "the PSP was unplugged" : code == G_IO_ERROR_TIMED_OUT ? "the PSP didn't answer in time" : error->message);
	g_error_free(error);
	return converted;
}

/**
 * @brief Open a PSP and claim its interface.
 *
 * @param watch The watch that told the PSP.
 * @param path The path of the PSP, from `pspdisp_usb_watch_next()`.
 * @param error Where to tell why it failed, ready to be shown to the user (e.g. the driver to install).
 * @return The opened PSP, close it with `pspdisp_usb_device_close()`, or `NULL` if it failed.
 */
PSPdispUSBDevice *pspdisp_usb_device_open(PSPdispUSBWatch *watch, const char *path, GError **error) {
	GError *failure = NULL;
	GUsbDevice *usb = g_usb_context_find_by_platform_id(watch->usb, path, NULL);

	// It must still be a PSP, as the platform ID of an unplugged one may go to another device
	if (usb == NULL || !pspdisp_usb_is_psp(g_usb_device_get_vid(usb), g_usb_device_get_pid(usb))) {
		g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "%s is not a PSP anymore", path);
		g_clear_object(&usb);
		return NULL;
	}

	if (!g_usb_device_open(usb, &failure)) {
		failure = pspdisp_usb_error(failure);

		if (g_error_matches(failure, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED) || g_error_matches(failure, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
			g_set_error(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED, "the PSP is plugged (%s), but there's no access to it, " PSPDISP_USB_ACCESS_HINT, path);
		else
			g_set_error(error, G_IO_ERROR, failure->code, "failed to open the PSP (%s): %s", path, failure->message);
		g_error_free(failure);
		g_object_unref(usb);
		return NULL;
	}

	// The interface, after detaching the kernel driver bound to it (if any), attached again once released
	// No SET_INTERFACE: the PSP drops the read it posts for the handshake magic on attach, and only posts another on the next attach (a replug)
	if (!g_usb_device_claim_interface(usb, PSPDISP_USB_INTERFACE, G_USB_DEVICE_CLAIM_INTERFACE_BIND_KERNEL_DRIVER, &failure)) {
		failure = pspdisp_usb_error(failure);

		if (g_error_matches(failure, G_IO_ERROR, G_IO_ERROR_BUSY))
			g_set_error(error, G_IO_ERROR, G_IO_ERROR_BUSY, "the PSP (%s) is used by another program", path);
		else
			g_set_error(error, G_IO_ERROR, failure->code, "failed to claim the interface of the PSP (%s): %s", path, failure->message);
		g_error_free(failure);
		g_usb_device_close(usb, NULL);
		g_object_unref(usb);
		return NULL;
	}

	PSPdispUSBDevice *device = g_new0(PSPdispUSBDevice, 1);
	device->usb = usb;
	device->context = g_main_context_new();
	return device;
}

/**
 * @brief Release the interface of a PSP and close it.
 *
 * @param device The opened PSP.
 */
void pspdisp_usb_device_close(PSPdispUSBDevice *device) {
	g_usb_device_release_interface(device->usb, PSPDISP_USB_INTERFACE, G_USB_DEVICE_CLAIM_INTERFACE_BIND_KERNEL_DRIVER, NULL);
	g_usb_device_close(device->usb, NULL);
	g_object_unref(device->usb);
	g_main_context_unref(device->context);
	g_free(device);
}

/**
 * @brief A bulk transfer of GUsb, and its result.
 */
typedef struct {
	bool done; // If GUsb completed it
	gssize transferred; // How many bytes were transferred, -1 if it failed
	GError *error; // Why it failed
} PSPdispUSBTransfer;

/**
 * @brief Take the result of a bulk transfer, called by GUsb on the context of the device once it's done.
 *
 * @param source The GUsb device.
 * @param result The result of the transfer.
 * @param data The transfer.
 */
void pspdisp_usb_device_on_transfer(GObject *source, GAsyncResult *result, gpointer data) {
	PSPdispUSBTransfer *transfer = data;
	transfer->transferred = g_usb_device_bulk_transfer_finish(G_USB_DEVICE(source), result, &transfer->error);
	transfer->done = true;
}

/**
 * @brief Send or receive data through a bulk endpoint, waiting until it's done.
 *
 * A transfer to the PSP only succeeds when the whole buffer was sent.
 * A transfer from the PSP also succeeds with less than `size` bytes, when the PSP sent a short packet.
 * On a timeout or a cancellation, the transfer is aborted before returning, so the buffer is free again.
 *
 * @param device The opened PSP.
 * @param endpoint The endpoint, `PSPDISP_USB_ENDPOINT_OUT` or `PSPDISP_USB_ENDPOINT_IN`.
 * @param buffer The data to send, or where to receive it.
 * @param size The size of `buffer` in bytes.
 * @param timeout How long the transfer may take, in milliseconds.
 * @param cancellable Abort the transfer when cancelled (`NULL` to never abort it).
 * @param transferred Where to write how many bytes were transferred.
 * @param error Where to tell why it failed, in the `G_IO_ERROR` domain: `G_IO_ERROR_TIMED_OUT`, `G_IO_ERROR_CANCELLED`, `G_IO_ERROR_NOT_CONNECTED` (the PSP was unplugged), or another code.
 * @return `true` if the transfer is done.
 */
bool pspdisp_usb_device_transfer(PSPdispUSBDevice *device, uint8_t endpoint, void *buffer, uint32_t size, uint32_t timeout, GCancellable *cancellable, uint32_t *transferred, GError **error) {
	PSPdispUSBTransfer transfer = { .done = false, .transferred = -1, .error = NULL };
	*transferred = 0;

	// An asynchronous transfer of GUsb (libusb splits a big one into URBs that go back to back, and ends one from the PSP at its short packet)
	// It completes on the thread default context, run until then, so it's aborted (and its buffer free) before returning
	g_main_context_push_thread_default(device->context);
	g_usb_device_bulk_transfer_async(device->usb, endpoint, buffer, size, timeout, cancellable, pspdisp_usb_device_on_transfer, &transfer);
	while (!transfer.done)
		g_main_context_iteration(device->context, TRUE);
	g_main_context_pop_thread_default(device->context);

	if (transfer.transferred < 0) {
		g_propagate_error(error, pspdisp_usb_error(transfer.error));
		return false;
	}

	*transferred = transfer.transferred;
	return true;
}

/**
 * @brief The USB of the computer, watched for PSPs.
 */
typedef struct {
	PSPdispUSBWatch *watch; // Plugs and unplugs of the PSPs, from the USB host
	GHashTable *plugged; // Paths of the plugged PSPs (`char *`), each with if why it doesn't connect was told (`GINT_TO_POINTER()`)
} PSPdispUSB;

/**
 * @brief Start watching the USB of the computer for PSPs.
 *
 * The PSPs plugged and unplugged after it only come while the global default context runs (e.g. the main loop of the daemon).
 *
 * @return The USB, free it with `pspdisp_usb_destroy()`, or `NULL` if it failed.
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
	PSPdispUSBDevice *device = NULL;

	while (device == NULL && !g_cancellable_is_cancelled(cancellable)) {
		pspdisp_usb_update(usb);

		GHashTableIter iter;
		gpointer path, told;
		g_hash_table_iter_init(&iter, usb->plugged);

		while (device == NULL && g_hash_table_iter_next(&iter, &path, &told)) {
			GError *error = NULL;
			device = pspdisp_usb_device_open(usb->watch, path, &error);

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
			pspdisp_usb_watch_wait(usb->watch, g_hash_table_size(usb->plugged) > 0 ? PSPDISP_USB_RETRY_INTERVAL : -1, cancellable);
	}

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
