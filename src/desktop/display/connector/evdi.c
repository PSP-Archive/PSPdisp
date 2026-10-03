/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * evdi.c - Display connector using EVDI (Linux)     *
 *                                                   *
 * Copyright © 2026 Net-0                            *
 *****************************************************/

#pragma once

#include <evdi_lib.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_mode.h>
#include "../connector.c" // Its declarations (when `connector.c` includes this file, it already has them and this include is skipped)

#define PSPDISP_DISPLAY_CONNECTOR_NAME "EVDI" // User friendly name of this connector

#define PSPDISP_EVDI_BUFFER_ID  1 // ID of the only buffer registered on EVDI, where the frames are grabbed into
#define PSPDISP_EVDI_MAX_RECTS 16 // Max dirty rectangles EVDI gives per frame (`MAX_DIRTS` on the EVDI kernel module and library, not exported)
#define PSPDISP_EVDI_FRAME_INTERVAL (G_USEC_PER_SEC / PSPDISP_DISPLAY_FREQUENCY) // Time between 2 frame requests, in microseconds

/**
 * @brief Display connector using EVDI, the source returned by `pspdisp_display_connector_connect()`.
 *
 * GLib polls the EVDI file descriptor, readable when EVDI has events: mode and power changes come on their own, frames only after `evdi_request_update()`.
 * Its ready time is the next frame request, so it also dispatches at the PSP refresh rate to ask EVDI for a frame, because EVDI doesn't pace the frames by the refresh rate of the EDID.
 */
typedef struct {
    GSource source; // The base source, first so this struct is also a `GSource`
    PSPdispFrameHandler frame_handler; // Handle each new frame
    PSPdispDPMSHandler dpms_handler; // Handle each DPMS change
    struct evdi_event_context event_context; // Handlers called by `evdi_handle_events()`, with this connector as the user data
    evdi_handle handle; // The opened EVDI device
    gpointer fd_tag; // Tag of the EVDI file descriptor on this source, to query its conditions
    struct evdi_buffer buffer; // Buffer registered on EVDI, where `evdi_grab_pixels()` copies the dirty rectangles (on the mode pixel format)
    uint32_t *pixels; // Frame converted to `PSPDISP_PIXEL_FORMAT_8888`, only the dirty rectangles are updated on each frame
    struct evdi_mode mode; // The current mode (resolution, refresh rate and pixel format), from the last mode change
    bool update_requested; // If an update was asked with `evdi_request_update()` and EVDI did not answer yet
} PSPdispDisplayConnector;

/**
 * @brief Store the new mode of the EVDI display.
 *
 * EVDI calls it (through `evdi_handle_events()`) when the compositor sets a mode on the display.
 * Its pixel format tells `pspdisp_display_connector_evdi_on_update_ready()` how to convert the pixels.
 *
 * @param mode The new mode: resolution, refresh rate, bits per pixel and pixel format (a DRM fourcc code).
 * @param display_connector The connector, from the `user_data` of the EVDI event context.
 */
void pspdisp_display_connector_evdi_on_mode_changed(struct evdi_mode mode, PSPdispDisplayConnector *display_connector) {
    display_connector->mode = mode;
}

/**
 * @brief Handle a frame ready on EVDI: grab its dirty pixels, convert them to the PSP pixel format and hand them to the frame handler.
 *
 * EVDI calls it (through `evdi_handle_events()`) when the frame asked with `evdi_request_update()` is ready.
 * But when `evdi_request_update()` returns `true`, the frame is already ready and EVDI does not send the event, so it must be called directly.
 *
 * Only the dirty rectangles are converted into `display_connector->pixels`, the rest keeps the previous frames, so it always holds a full frame.
 * Nothing is handed to the frame handler when the pixel format is unsupported.
 *
 * @param buffer_to_be_updated The ID of the buffer asked on `evdi_request_update()`, always `PSPDISP_EVDI_BUFFER_ID`.
 * @param display_connector The connector, from the `user_data` of the EVDI event context.
 */
void pspdisp_display_connector_evdi_on_update_ready(int buffer_to_be_updated, PSPdispDisplayConnector *display_connector) {
    display_connector->update_requested = false; // Answered, so a new update can be asked

    g_return_if_fail(buffer_to_be_updated == PSPDISP_EVDI_BUFFER_ID);

    struct evdi_rect rects[PSPDISP_EVDI_MAX_RECTS];
    int num_rects = 0;

    evdi_grab_pixels(display_connector->handle, rects, &num_rects);

    PSPdispDirtyRectangle rectangles[PSPDISP_EVDI_MAX_RECTS];
    uint8_t rectangles_length = num_rects;

    for (uint8_t i = 0; i < rectangles_length; i++) {
        struct evdi_rect rect = rects[i];
        rectangles[i] = (PSPdispDirtyRectangle) { .x1 = rect.x1, .y1 = rect.y1, .x2 = rect.x2, .y2 = rect.y2 };
    }

    const uint8_t *source = display_connector->buffer.buffer;
    uint32_t *pixels = display_connector->pixels;

    // Note: EVDI only have those 4 pixel formats, so for PSP is always going to be PSPDISP_PIXEL_FORMAT_8888 (R,G,B,A in memory), but we may need to swap bytes
    // Note: Converted into a separate buffer, because in place the overlap of 2 dirty rectangles would be swapped twice (back to the original)
    switch (display_connector->mode.pixel_format) {
        case DRM_FORMAT_XRGB8888:
        case DRM_FORMAT_ARGB8888:
            // B,G,R,X in memory: swap R and B, force opaque alpha
            for (int i = 0; i < num_rects; i++) {
                for (int y = rects[i].y1; y < rects[i].y2; y++) {
                    const uint32_t *source_row = (const uint32_t *) (source + y * display_connector->buffer.stride);
                    uint32_t *row = pixels + y * PSPDISP_DISPLAY_WIDTH;

                    for (int x = rects[i].x1; x < rects[i].x2; x++) {
                        uint32_t pixel = source_row[x]; // 0xXXRRGGBB
                        row[x] = 0xFF000000u                    // A = opaque
                               | ((pixel & 0x000000FFu) << 16)  // B -> bits 16-23
                               |  (pixel & 0x0000FF00u)         // G unchanged
                               | ((pixel & 0x00FF0000u) >> 16); // R -> bits 0-7
                    }
                }
            }
            break;
        case DRM_FORMAT_XBGR8888:
        case DRM_FORMAT_ABGR8888:
            // R,G,B,X in memory: already on the right layout, only force opaque alpha (it's the display frame, opacity does not matter here but can break PSP!)
            for (int i = 0; i < num_rects; i++) {
                for (int y = rects[i].y1; y < rects[i].y2; y++) {
                    const uint32_t *source_row = (const uint32_t *) (source + y * display_connector->buffer.stride);
                    uint32_t *row = pixels + y * PSPDISP_DISPLAY_WIDTH;

                    for (int x = rects[i].x1; x < rects[i].x2; x++) {
                        row[x] = source_row[x] | 0xFF000000u;
                    }
                }
            }
            break;
        default:
            g_warning("%s(): unsupported pixel format %.4s", __FUNCTION__, (const char *) &display_connector->mode.pixel_format);
            return;
    }

    display_connector->frame_handler(pixels, PSPDISP_DISPLAY_WIDTH, PSPDISP_PIXEL_FORMAT_8888, rectangles, rectangles_length);
}

/**
 * @brief Handle a power state change of the EVDI display and hand it to the DPMS handler.
 *
 * EVDI calls it (through `evdi_handle_events()`) when the compositor turns the display on or off, e.g. the screen blanks after being idle, the display is disabled on the settings or the user switches to another VT.
 * It comes on its own, without asking for it.
 *
 * EVDI uses the DRM DPMS modes, which have the same order as `PSPdispDPMSMode`, but in practice only sends `DRM_MODE_DPMS_ON` and `DRM_MODE_DPMS_OFF`.
 *
 * @param dpms_mode The DRM DPMS mode: `DRM_MODE_DPMS_ON`, `DRM_MODE_DPMS_STANDBY`, `DRM_MODE_DPMS_SUSPEND` or `DRM_MODE_DPMS_OFF`.
 * @param display_connector The connector, from the `user_data` of the EVDI event context.
 */
void pspdisp_display_connector_evdi_on_dpms(int dpms_mode, PSPdispDisplayConnector *display_connector) {
    PSPdispDPMSMode mode;

    switch (dpms_mode) {
        case DRM_MODE_DPMS_ON:
            mode = PSPDISP_DPMS_MODE_ON;
            break;
        case DRM_MODE_DPMS_STANDBY:
            mode = PSPDISP_DPMS_MODE_STANDBY;
            break;
        case DRM_MODE_DPMS_SUSPEND:
            mode = PSPDISP_DPMS_MODE_SUSPEND;
            break;
        case DRM_MODE_DPMS_OFF:
            mode = PSPDISP_DPMS_MODE_OFF;
            break;
        default:
            g_warning("%s(): unknown DPMS mode %d", __FUNCTION__, dpms_mode);
            return;
    }

    display_connector->dpms_handler(mode);
}

/**
 * @brief Ask EVDI for the next frame.
 *
 * EVDI ignores a new request while the last one was not answered, so it waits for `pspdisp_display_connector_evdi_on_update_ready()`.
 * When a frame is already ready, EVDI answers right away (and sends no event), so the frame is handled here.
 *
 * @param display_connector The connector.
 */
void pspdisp_display_connector_evdi_request_frame(PSPdispDisplayConnector *display_connector) {
    if (display_connector->update_requested) return; // Still waiting for the last one

    if (evdi_request_update(display_connector->handle, PSPDISP_EVDI_BUFFER_ID)) {
        pspdisp_display_connector_evdi_on_update_ready(PSPDISP_EVDI_BUFFER_ID, display_connector); // Already ready, EVDI sends no event
    } else {
        display_connector->update_requested = true; // EVDI calls `pspdisp_display_connector_evdi_on_update_ready()` when ready
    }
}

/**
 * @brief Handle the pending EVDI events, then ask EVDI for a frame when it's time, called by GLib when the EVDI file descriptor is ready or the ready time is reached.
 *
 * @param source The connector.
 * @param callback Unused, the connector calls its handlers itself.
 * @param user_data Unused.
 * @return `G_SOURCE_CONTINUE` while the display exists, or `G_SOURCE_REMOVE` when the EVDI device is gone.
 */
gboolean pspdisp_display_connector_evdi_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
    (void) callback;
    (void) user_data;
    PSPdispDisplayConnector *display_connector = (PSPdispDisplayConnector *) source;
    GIOCondition condition = g_source_query_unix_fd(source, display_connector->fd_tag);

    if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
        g_warning("%s(): the EVDI device is gone", __FUNCTION__);
        return G_SOURCE_REMOVE;
    }

    if (condition & G_IO_IN) evdi_handle_events(display_connector->handle, &display_connector->event_context); // Only when readable, it reads the file descriptor and would block otherwise

    gint64 time = g_source_get_time(source);
    gint64 ready_time = g_source_get_ready_time(source);

    if (time >= ready_time) {
        gint64 next_ready_time = ready_time + PSPDISP_EVDI_FRAME_INTERVAL;
        g_source_set_ready_time(source, next_ready_time > time ? next_ready_time : time + PSPDISP_EVDI_FRAME_INTERVAL); // When late (e.g. a slow frame handler), the missed frames are skipped
        pspdisp_display_connector_evdi_request_frame(display_connector);
    }

    return G_SOURCE_CONTINUE;
}

/**
 * @brief Free the connector resources, called by GLib when the last reference to the source is released.
 *
 * Closing the EVDI device also disconnects the virtual display, so it disappears from the compositor.
 *
 * @param source The connector.
 */
void pspdisp_display_connector_evdi_finalize(GSource *source) {
    PSPdispDisplayConnector *display_connector = (PSPdispDisplayConnector *) source;

    evdi_unregister_buffer(display_connector->handle, display_connector->buffer.id);
    evdi_close(display_connector->handle); // Last, it frees the handle
    g_free(display_connector->buffer.buffer);
    g_free(display_connector->pixels);
}

/**
 * @brief Functions of the connector source.
 *
 * No `prepare` nor `check`: GLib dispatches the source when the EVDI file descriptor is ready (it's added with `g_source_add_unix_fd()`) or the ready time is reached.
 */
static GSourceFuncs pspdisp_display_connector_evdi_source_funcs = {
    .dispatch = pspdisp_display_connector_evdi_dispatch,
    .finalize = pspdisp_display_connector_evdi_finalize,
};

/**
 * @brief Create the virtual display on EVDI, and the source of its events.
 *
 * Opens an EVDI device (not attached to a parent device, like a USB dock), registers a buffer of the PSP resolution to grab the frames, and connects the display with the EDID.
 * The display is limited to the PSP pixel area, so the compositor only sets modes that fit the PSP.
 *
 * @param edid The identification given to the computer.
 * @param frame_handler Handle each new frame.
 * @param dpms_handler Handle each DPMS change.
 * @return The source of the display events (not attached yet), or `NULL` if no EVDI device could be opened.
 */
GSource *pspdisp_display_connector_connect(PSPdispEDID edid, PSPdispFrameHandler frame_handler, PSPdispDPMSHandler dpms_handler) {
    evdi_handle handle = evdi_open_attached_to_fixed(NULL, 0);
    if (handle == EVDI_INVALID_HANDLE) {
        g_warning("%s(): failed to open an EVDI device", __FUNCTION__);
        return NULL;
    }

    GSource *source = g_source_new(&pspdisp_display_connector_evdi_source_funcs, sizeof(PSPdispDisplayConnector));
    PSPdispDisplayConnector *display_connector = (PSPdispDisplayConnector *) source;
    display_connector->frame_handler = frame_handler;
    display_connector->dpms_handler = dpms_handler;
    display_connector->event_context = (struct evdi_event_context) {
        .mode_changed_handler = (void (*)(struct evdi_mode, void*)) pspdisp_display_connector_evdi_on_mode_changed,
        .update_ready_handler = (void (*)(int, void*)) pspdisp_display_connector_evdi_on_update_ready,
        .dpms_handler         = (void (*)(int, void*)) pspdisp_display_connector_evdi_on_dpms,
        .user_data            = display_connector,
    };
    display_connector->handle = handle;
    display_connector->buffer = (struct evdi_buffer) {
        .id = PSPDISP_EVDI_BUFFER_ID,
        .buffer = g_new0(uint32_t, PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT),
        .width = PSPDISP_DISPLAY_WIDTH,
        .height = PSPDISP_DISPLAY_HEIGHT,
        .stride = PSPDISP_DISPLAY_WIDTH * 4,
        .rects = NULL,
        .rect_count = 0,
    };
    display_connector->pixels = g_new0(uint32_t, PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT);
    display_connector->mode = (struct evdi_mode) {}; // No mode until the compositor sets one
    display_connector->update_requested = false;

    evdi_register_buffer(display_connector->handle, display_connector->buffer);
    evdi_connect(display_connector->handle, (const unsigned char *) &edid, sizeof(PSPdispEDID), PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT);

    display_connector->fd_tag = g_source_add_unix_fd(source, evdi_get_event_ready(display_connector->handle), G_IO_IN);
    g_source_set_ready_time(source, g_get_monotonic_time() + PSPDISP_EVDI_FRAME_INTERVAL); // The first frame request
    return source;
}
