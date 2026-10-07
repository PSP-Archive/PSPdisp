/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * host.c - USB host of the platform                 *
 *                                                   *
 * Copyright © 2026 Net-0                            *
 *****************************************************/

/*
 * What the USB link needs from the system: being told when a PSP is plugged or unplugged, and moving data through the bulk endpoints of an opened one.
 *
 * Its implementation varies with the platform, chosen at compile time: udev and usbfs on Linux (`host/usbfs.c`), the Configuration Manager and WinUSB on Windows (`host/winusb.c`).
 * A PSP is known by its path, a string given by the system (e.g. `/dev/bus/usb/001/005` on Linux), the same on its plug and on its unplug.
 */

#pragma once

#include <stdint.h>
#include <glib.h>
#include <gio/gio.h>

#define PSPDISP_USB_VENDOR_ID         0x054C // Sony
#define PSPDISP_USB_PRODUCT_ID_TYPE_B 0x01C9 // USBHostFS, as type B (the default "USB driver" of the PSP app)
#define PSPDISP_USB_PRODUCT_ID_TYPE_C 0x01CA // USBHostFS, as type C
#define PSPDISP_USB_INTERFACE         0      // The only interface of the PSP, with both endpoints
#define PSPDISP_USB_ENDPOINT_OUT      0x02   // Bulk endpoint to the PSP
#define PSPDISP_USB_ENDPOINT_IN       0x81   // Bulk endpoint from the PSP

/**
 * @brief Watch of the PSPs being plugged and unplugged.
 *
 * Its platform implementation (included below, e.g. `host/usbfs.c`) declares this struct with the data it needs.
 */
typedef struct PSPdispUSBWatch PSPdispUSBWatch;

/**
 * @brief An opened PSP, its interface claimed.
 *
 * Its platform implementation (included below, e.g. `host/usbfs.c`) declares this struct with the data it needs.
 */
typedef struct PSPdispUSBDevice PSPdispUSBDevice;

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
 * @brief Start watching the PSPs being plugged and unplugged, implemented by the platform.
 *
 * The PSPs already plugged are given first by `pspdisp_usb_watch_next()`, as plugs.
 * One plugged while the watch starts may be given twice, the USB link ignores the second one.
 *
 * @return The new watch, free it with `pspdisp_usb_watch_destroy()`, or `NULL` if the system can't tell the plugs.
 */
PSPdispUSBWatch *pspdisp_usb_watch_new(void);

/**
 * @brief Stop watching and free the watch, implemented by the platform.
 *
 * @param watch The watch to destroy.
 */
void pspdisp_usb_watch_destroy(PSPdispUSBWatch *watch);

/**
 * @brief Get what to poll to wait for a plug or an unplug, implemented by the platform.
 *
 * It's ready when a plug or an unplug comes, so it's polled with `g_poll()` (e.g. along with a `GCancellable`) once `pspdisp_usb_watch_next()` gave all it had.
 *
 * @param watch The watch.
 * @param poll Where to write it: a file descriptor on Linux, an event `HANDLE` on Windows.
 */
void pspdisp_usb_watch_make_pollfd(PSPdispUSBWatch *watch, GPollFD *poll);

/**
 * @brief Take the next plug or unplug of a PSP, without waiting, implemented by the platform.
 *
 * @param watch The watch.
 * @param plugged Where to write if the PSP was plugged (`true`) or unplugged (`false`).
 * @param path Where to write the path of the PSP, free it with `g_free()`.
 * @return `true` if there was one, `false` if there's none for now.
 */
bool pspdisp_usb_watch_next(PSPdispUSBWatch *watch, bool *plugged, char **path);

/**
 * @brief Open a PSP and claim its interface, implemented by the platform.
 *
 * @param path The path of the PSP, from `pspdisp_usb_watch_next()`.
 * @param error Where to tell why it failed, ready to be shown to the user (e.g. the driver to install).
 * @return The opened PSP, close it with `pspdisp_usb_device_close()`, or `NULL` if it failed.
 */
PSPdispUSBDevice *pspdisp_usb_device_open(const char *path, GError **error);

/**
 * @brief Release the interface of a PSP and close it, implemented by the platform.
 *
 * @param device The opened PSP.
 */
void pspdisp_usb_device_close(PSPdispUSBDevice *device);

/**
 * @brief Send or receive data through a bulk endpoint, waiting until it's done, implemented by the platform.
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
bool pspdisp_usb_device_transfer(PSPdispUSBDevice *device, uint8_t endpoint, void *buffer, uint32_t size, uint32_t timeout, GCancellable *cancellable, uint32_t *transferred, GError **error);

// The USB host of this platform, it declares `PSPdispUSBWatch` and `PSPdispUSBDevice`, implements their functions and defines `PSPDISP_USB_HOST_NAME`
#if defined(__linux__)
    #include "./host/usbfs.c"
#else
    #error "Unsupported platform"
#endif
