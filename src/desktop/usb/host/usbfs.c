/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * usbfs.c - USB host using udev and usbfs (Linux)   *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The plugs and unplugs come from a udev monitor, told once udev applied its rules, so the PSP is already accessible then (with the udev rule `daemon/60-pspdisp.rules`).
 * The data goes through usbfs (`/dev/bus/usb/BBB/DDD`), as URBs submitted to the kernel and reaped once they complete, so a transfer waits with `g_poll()` and can be aborted.
 */

#pragma once

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/usb/ch9.h>
#include <linux/usbdevice_fs.h>
#include <libudev.h>
#include "../host.c" // Its declarations (when `host.c` includes this file, it already has them and this include is skipped)

#define PSPDISP_USB_HOST_NAME "usbfs" // User friendly name of this USB host

#define PSPDISP_USB_USBFS_URB_SIZE (16 * 1024) // Max size of a URB, as libusb splits them: a bigger one needs a contiguous kernel buffer when the host controller has no scatter-gather

/**
 * @brief Watch of the PSPs using a udev monitor, created by `pspdisp_usb_watch_new()`.
 */
struct PSPdispUSBWatch {
    struct udev *udev; // The udev context
    struct udev_monitor *monitor; // Plugs and unplugs of the USB devices, told once udev applied its rules
    GQueue *present; // Paths of the PSPs plugged when the watch started (`char *`), given first as plugs
};

/**
 * @brief A PSP opened through usbfs, created by `pspdisp_usb_device_open()`.
 */
struct PSPdispUSBDevice {
    int fd; // The usbfs file of the PSP, opened for reading and writing
};

/**
 * @brief Tell if a udev device is a PSP running the PSPdisp app in USB mode, from its `PRODUCT` property (e.g. `54c/1c9/100`).
 *
 * The kernel gives that property on the plug and on the unplug, when the attributes of the device can't be read anymore.
 *
 * @param device The udev device, a USB device (not one of its interfaces).
 * @return `true` if it's a PSP.
 */
bool pspdisp_usb_usbfs_is_psp(struct udev_device *device) {
    const char *product = udev_device_get_property_value(device, "PRODUCT");
    unsigned int vendor_id = 0, product_id = 0;

    return product != NULL && sscanf(product, "%x/%x", &vendor_id, &product_id) == 2 && pspdisp_usb_is_psp(vendor_id, product_id);
}

/**
 * @brief Start watching the PSPs with a udev monitor, then list the PSPs already plugged.
 *
 * The monitor starts first, so a PSP plugged meanwhile isn't missed.
 *
 * @return The new watch, or `NULL` if udev can't be used.
 */
PSPdispUSBWatch *pspdisp_usb_watch_new() {
    struct udev *udev = udev_new();
    struct udev_monitor *monitor = udev != NULL ? udev_monitor_new_from_netlink(udev, "udev") : NULL;

    if (monitor == NULL || udev_monitor_filter_add_match_subsystem_devtype(monitor, "usb", "usb_device") < 0 || udev_monitor_enable_receiving(monitor) < 0) {
        g_warning("%s(): failed to watch the USB devices with udev", __FUNCTION__);
        udev_monitor_unref(monitor);
        udev_unref(udev);
        return NULL;
    }

    PSPdispUSBWatch *watch = g_new0(PSPdispUSBWatch, 1);
    watch->udev = udev;
    watch->monitor = monitor;
    watch->present = g_queue_new();

    // The USB devices of Sony (only they have `idVendor`, not their interfaces), then their product
    char vendor_id[5];
    g_snprintf(vendor_id, sizeof(vendor_id), "%04x", PSPDISP_USB_VENDOR_ID);

    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_add_match_sysattr(enumerate, "idVendor", vendor_id);
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *entry;
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(enumerate)) {
        struct udev_device *device = udev_device_new_from_syspath(udev, udev_list_entry_get_name(entry));
        if (device == NULL) continue;

        const char *devnode = udev_device_get_devnode(device);
        if (devnode != NULL && pspdisp_usb_usbfs_is_psp(device)) g_queue_push_tail(watch->present, g_strdup(devnode));
        udev_device_unref(device);
    }

    udev_enumerate_unref(enumerate);
    return watch;
}

/**
 * @brief Stop watching and free the watch.
 *
 * @param watch The watch to destroy.
 */
void pspdisp_usb_watch_destroy(PSPdispUSBWatch *watch) {
    udev_monitor_unref(watch->monitor);
    udev_unref(watch->udev);
    g_queue_free_full(watch->present, g_free);
    g_free(watch);
}

/**
 * @brief Get the socket of the udev monitor, readable when a USB device was plugged or unplugged.
 *
 * @param watch The watch.
 * @param poll Where to write it.
 */
void pspdisp_usb_watch_make_pollfd(PSPdispUSBWatch *watch, GPollFD *poll) {
    *poll = (GPollFD) { .fd = udev_monitor_get_fd(watch->monitor), .events = G_IO_IN };
}

/**
 * @brief Take the next plug or unplug of a PSP: the PSPs plugged when the watch started, then what the udev monitor received.
 *
 * The other USB devices, and the other actions (e.g. a driver bound), are skipped.
 *
 * @param watch The watch.
 * @param plugged Where to write if the PSP was plugged (`true`) or unplugged (`false`).
 * @param path Where to write the usbfs file of the PSP (e.g. `/dev/bus/usb/001/005`), free it with `g_free()`.
 * @return `true` if there was one, `false` if there's none for now.
 */
bool pspdisp_usb_watch_next(PSPdispUSBWatch *watch, bool *plugged, char **path) {
    if (!g_queue_is_empty(watch->present)) {
        *plugged = true;
        *path = g_queue_pop_head(watch->present);
        return true;
    }

    struct udev_device *device;

    // The socket doesn't block, so it gives `NULL` once it's empty
    while ((device = udev_monitor_receive_device(watch->monitor)) != NULL) {
        const char *action = udev_device_get_action(device);
        const char *devnode = udev_device_get_devnode(device);
        bool added = g_strcmp0(action, "add") == 0;
        bool found = (added || g_strcmp0(action, "remove") == 0) && devnode != NULL && pspdisp_usb_usbfs_is_psp(device);

        if (found) {
            *plugged = added;
            *path = g_strdup(devnode);
        }
        udev_device_unref(device);

        if (found) return true;
    }

    return false;
}

/**
 * @brief Open a PSP through usbfs, and claim its interface.
 *
 * Linux already set the only configuration of the PSP when it was plugged, so only its interface is claimed, then set to its alternate setting 0, as the old desktop app does.
 *
 * @param path The usbfs file of the PSP (e.g. `/dev/bus/usb/001/005`).
 * @param error Where to tell why it failed.
 * @return The opened PSP, or `NULL` if it failed.
 */
PSPdispUSBDevice *pspdisp_usb_device_open(const char *path, GError **error) {
    int fd = open(path, O_RDWR | O_CLOEXEC);

    if (fd < 0) {
        int code = errno;

        if (code == EACCES || code == EPERM) g_set_error(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED, "the PSP is plugged (%s), but there's no access to it, install the udev rule (sudo make install) then plug the PSP again", path);
        else g_set_error(error, G_IO_ERROR, g_io_error_from_errno(code), "failed to open the PSP (%s): %s", path, g_strerror(code));
        return NULL;
    }

    // usbfs gives the device descriptor first, in the CPU byte order: it must still be a PSP, as the path of an unplugged one may go to another device
    struct usb_device_descriptor descriptor;

    if (read(fd, &descriptor, USB_DT_DEVICE_SIZE) != USB_DT_DEVICE_SIZE || !pspdisp_usb_is_psp(descriptor.idVendor, descriptor.idProduct)) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "%s is not a PSP anymore", path);
        close(fd);
        return NULL;
    }

    // The interface, after disconnecting the kernel driver bound to it (if any), but not another program using it through usbfs
    struct usbdevfs_disconnect_claim claim = { .interface = PSPDISP_USB_INTERFACE, .flags = USBDEVFS_DISCONNECT_CLAIM_EXCEPT_DRIVER, .driver = "usbfs" };

    if (ioctl(fd, USBDEVFS_DISCONNECT_CLAIM, &claim) < 0) {
        int code = errno;

        if (code == EBUSY) g_set_error(error, G_IO_ERROR, G_IO_ERROR_BUSY, "the PSP (%s) is used by another program", path);
        else g_set_error(error, G_IO_ERROR, g_io_error_from_errno(code), "failed to claim the interface of the PSP (%s): %s", path, g_strerror(code));
        close(fd);
        return NULL;
    }

    struct usbdevfs_setinterface setting = { .interface = PSPDISP_USB_INTERFACE, .altsetting = 0 };
    ioctl(fd, USBDEVFS_SETINTERFACE, &setting);

    PSPdispUSBDevice *device = g_new0(PSPdispUSBDevice, 1);
    device->fd = fd;
    return device;
}

/**
 * @brief Release the interface of the PSP and close it.
 *
 * @param device The opened PSP.
 */
void pspdisp_usb_device_close(PSPdispUSBDevice *device) {
    unsigned int interface = PSPDISP_USB_INTERFACE;

    ioctl(device->fd, USBDEVFS_RELEASEINTERFACE, &interface);
    close(device->fd);
    g_free(device);
}

/**
 * @brief Make the error of a failed transfer, from an `errno` value or the status of a URB.
 *
 * @param code The error, positive (e.g. `ENODEV`).
 * @return The error, in the `G_IO_ERROR` domain.
 */
GError *pspdisp_usb_usbfs_error(int code) {
    if (code == ENODEV || code == ESHUTDOWN) return g_error_new_literal(G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED, "the PSP was unplugged");
    return g_error_new_literal(G_IO_ERROR, g_io_error_from_errno(code), g_strerror(code));
}

/**
 * @brief Run the URBs of a transfer: submit them all, so they go back to back, then reap them all.
 *
 * On a failure, a timeout or a cancellation, the URBs still running are discarded, and reaped too, so none is left to the next transfer.
 *
 * @param device The opened PSP.
 * @param endpoint The endpoint.
 * @param buffer The data to send, or where to receive it.
 * @param size The size of `buffer` in bytes.
 * @param deadline When the transfer times out, in the time of `g_get_monotonic_time()`.
 * @param cancellable Abort the URBs when cancelled (`NULL` to never abort them).
 * @param transferred Where to write how many bytes were transferred, up to the first URB not filled.
 * @param error Where to tell why it failed.
 * @return `true` if all the URBs completed.
 */
bool pspdisp_usb_usbfs_run(PSPdispUSBDevice *device, uint8_t endpoint, uint8_t *buffer, uint32_t size, gint64 deadline, GCancellable *cancellable, uint32_t *transferred, GError **error) {
    uint32_t count = (size + PSPDISP_USB_USBFS_URB_SIZE - 1) / PSPDISP_USB_USBFS_URB_SIZE;
    struct usbdevfs_urb *urbs = g_new0(struct usbdevfs_urb, count);
    bool *running = g_new0(bool, count); // If each URB was submitted and not reaped yet
    uint32_t running_count = 0;
    GError *failure = NULL; // The first failure, the transfer stops on it

    for (uint32_t i = 0; i < count && failure == NULL; i++) {
        urbs[i].type = USBDEVFS_URB_TYPE_BULK;
        urbs[i].endpoint = endpoint;
        urbs[i].buffer = buffer + i * PSPDISP_USB_USBFS_URB_SIZE;
        urbs[i].buffer_length = MIN(size - i * PSPDISP_USB_USBFS_URB_SIZE, PSPDISP_USB_USBFS_URB_SIZE);

        if (ioctl(device->fd, USBDEVFS_SUBMITURB, &urbs[i]) == 0) {
            running[i] = true;
            running_count++;
        } else {
            failure = pspdisp_usb_usbfs_error(errno);
        }
    }

    // usbfs is writable when a URB completed, and hangs up when the PSP is unplugged
    GPollFD fds[2] = { { .fd = device->fd, .events = G_IO_OUT } };
    int fds_count = 1 + g_cancellable_make_pollfd(cancellable, &fds[1]);
    bool discarded = false;

    while (running_count > 0) {
        // On a failure, the URBs still running are discarded, then they complete right away
        if (failure != NULL && !discarded) {
            for (uint32_t i = 0; i < count; i++) {
                if (running[i]) ioctl(device->fd, USBDEVFS_DISCARDURB, &urbs[i]);
            }
            discarded = true;
        }

        struct usbdevfs_urb *urb = NULL;
        int result = ioctl(device->fd, discarded ? USBDEVFS_REAPURB : USBDEVFS_REAPURBNDELAY, &urb);
        int code = errno;

        if (result == 0) {
            if (urb >= urbs && urb < urbs + count && running[urb - urbs]) {
                running[urb - urbs] = false;
                running_count--;
                if (urb->status != 0 && failure == NULL) failure = pspdisp_usb_usbfs_error(-urb->status);
            }
            continue;
        }

        if (code == EINTR) continue;
        if (code != EAGAIN) { // `ENODEV`: the PSP was unplugged, its URBs are gone
            if (failure == NULL) failure = pspdisp_usb_usbfs_error(code);
            break;
        }

        // None completed yet: wait for one, until the deadline or the cancellation
        gint64 remaining = deadline - g_get_monotonic_time();
        int ready = remaining > 0 ? g_poll(fds, fds_count, (int) ((remaining + 999) / 1000)) : 0;

        if (!g_cancellable_set_error_if_cancelled(cancellable, &failure) && ready == 0) {
            failure = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "the PSP didn't answer in time");
        }
    }

    if (fds_count > 1) g_cancellable_release_fd(cancellable);

    *transferred = 0;
    for (uint32_t i = 0; i < count; i++) {
        *transferred += urbs[i].actual_length;
        if (urbs[i].actual_length < urbs[i].buffer_length) break;
    }

    g_free(urbs);
    g_free(running);

    if (failure != NULL) {
        g_propagate_error(error, failure);
        return false;
    }

    return true;
}

/**
 * @brief Send or receive data through a bulk endpoint of the PSP, as usbfs URBs.
 *
 * To the PSP, the URBs are all submitted at once, so they go back to back.
 * From the PSP, they're submitted one at a time, as a short packet ends the transfer: a URB submitted after it would take the data of the next transfer.
 *
 * @param device The opened PSP.
 * @param endpoint The endpoint, `PSPDISP_USB_ENDPOINT_OUT` or `PSPDISP_USB_ENDPOINT_IN`.
 * @param buffer The data to send, or where to receive it.
 * @param size The size of `buffer` in bytes.
 * @param timeout How long the transfer may take, in milliseconds.
 * @param cancellable Abort the transfer when cancelled (`NULL` to never abort it).
 * @param transferred Where to write how many bytes were transferred.
 * @param error Where to tell why it failed.
 * @return `true` if the transfer is done.
 */
bool pspdisp_usb_device_transfer(PSPdispUSBDevice *device, uint8_t endpoint, void *buffer, uint32_t size, uint32_t timeout, GCancellable *cancellable, uint32_t *transferred, GError **error) {
    gint64 deadline = g_get_monotonic_time() + (gint64) timeout * 1000;

    *transferred = 0;
    if (g_cancellable_set_error_if_cancelled(cancellable, error)) return false;

    if (!(endpoint & USB_DIR_IN)) return pspdisp_usb_usbfs_run(device, endpoint, buffer, size, deadline, cancellable, transferred, error);

    while (*transferred < size) {
        uint32_t length = MIN(size - *transferred, PSPDISP_USB_USBFS_URB_SIZE);
        uint32_t received = 0;
        bool done = pspdisp_usb_usbfs_run(device, endpoint, (uint8_t *) buffer + *transferred, length, deadline, cancellable, &received, error);

        *transferred += received;
        if (!done) return false;
        if (received < length) break; // A short packet, the end of the transfer
    }

    return true;
}
