/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * evdi.c - Display provider using EVDI (Linux)     *
 *                                                   *
 * Copyright © 2026 Net-0                            *
 *****************************************************/

#pragma once

#include <evdi_lib.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_mode.h>
#include <glib-unix.h>
#include "../display.h"

#define PSPDISP_EVDI_BUFFER_ID  1 // ID of the only buffer registered on EVDI, where the frames are grabbed into
#define PSPDISP_EVDI_MAX_RECTS 16 // Max dirty rectangles EVDI gives per frame (`MAX_DIRTS` on the EVDI kernel module and library, not exported)

/**
 * @brief State of the EVDI display provider, shared by the `GSource` callbacks and the EVDI event handlers.
 *
 * Created by `pspdisp_display_provider_evdi_provide()` and freed by `pspdisp_display_provider_evdi_destroy()`.
 */
typedef struct {
    PSPdispDisplay *display; // The display that receives the frames and the DPMS changes
    struct evdi_event_context event_context; // Handlers called by `evdi_handle_events()`, with this context as the user data
    evdi_handle handle; // The opened EVDI device
    struct evdi_buffer buffer; // Buffer registered on EVDI, where `evdi_grab_pixels()` copies the dirty rectangles (on the mode pixel format)
    uint32_t *pixels; // Frame converted to `PSPDISP_PIXEL_FORMAT_8888`, only the dirty rectangles are updated on each frame
    struct evdi_mode mode; // The current mode (resolution, refresh rate and pixel format), from the last mode change
} PSPdispDisplayEVDIProviderContext;

/**
 * @brief Handle the pending EVDI events, called by the source when the EVDI file descriptor is readable.
 *
 * @param context The provider context.
 * @return Always `G_SOURCE_CONTINUE`, to keep handling the events while the display exists.
 */
gboolean pspdisp_display_provider_evdi_func(PSPdispDisplayEVDIProviderContext *context) {
    evdi_handle_events(context->handle, &context->event_context);
    return G_SOURCE_CONTINUE;
}

/**
 * @brief Free the provider context, called by GLib when the source is destroyed.
 *
 * Closing the EVDI device also disconnects the virtual display, so it disappears from the compositor.
 *
 * @param context The provider context.
 */
void pspdisp_display_provider_evdi_destroy(PSPdispDisplayEVDIProviderContext *context) {
    evdi_close(context->handle);
    evdi_unregister_buffer(context->handle, context->buffer.id);
    g_free(context->buffer.buffer);
    g_free(context->pixels);
}

/**
 * @brief Store the new mode of the EVDI display.
 *
 * EVDI calls it (through `evdi_handle_events()`) when the compositor sets a mode on the display,
 * its pixel format tells `pspdisp_display_provider_evdi_on_update_ready()` how to convert the pixels.
 *
 * @param mode The new mode: resolution, refresh rate, bits per pixel and pixel format (a DRM fourcc code).
 * @param context The provider context, from the `user_data` of the EVDI event context.
 */
void pspdisp_display_provider_evdi_on_mode_changed(struct evdi_mode mode, PSPdispDisplayEVDIProviderContext *context) {
    context->mode = mode;
}

/**
 * @brief Handle a frame ready on EVDI: grab its dirty pixels, convert them to the PSP pixel format and hand them to the frame handler.
 *
 * EVDI calls it (through `evdi_handle_events()`) when the frame asked with `evdi_request_update()` is ready.
 * But when `evdi_request_update()` returns `true`, the frame is already ready and EVDI does not send the event, so it must be called directly.
 *
 * Only the dirty rectangles are converted into `context->pixels`, the rest keeps the previous frames, so it always holds a full frame.
 * Nothing is handed to the frame handler when the pixel format is unsupported.
 *
 * @param buffer_to_be_updated The ID of the buffer asked on `evdi_request_update()`, always `PSPDISP_EVDI_BUFFER_ID`.
 * @param context The provider context, from the `user_data` of the EVDI event context.
 */
void pspdisp_display_provider_evdi_on_update_ready(int buffer_to_be_updated, PSPdispDisplayEVDIProviderContext *context) {
    g_return_if_fail(buffer_to_be_updated == PSPDISP_EVDI_BUFFER_ID);

    struct evdi_rect rects[PSPDISP_EVDI_MAX_RECTS];
    int num_rects = 0;

    evdi_grab_pixels(context->handle, rects, &num_rects);

    PSPdispDirtyRectangle rectangles[PSPDISP_EVDI_MAX_RECTS];
    uint8_t rectangles_length = num_rects;

    for (uint8_t i = 0; i < rectangles_length; i++) {
        struct evdi_rect rect = rects[i];
        rectangles[i] = (PSPdispDirtyRectangle) { .x1 = rect.x1, .y1 = rect.y1, .x2 = rect.x2, .y2 = rect.y2 };
    }

    const uint8_t *source = context->buffer.buffer;
    uint32_t *pixels = context->pixels;

    // Note: EVDI only have those 4 pixel formats, so for PSP is always going to be PSPDISP_PIXEL_FORMAT_8888 (R,G,B,A in memory), but we may need to swap bytes
    // Note: Converted into a separate buffer, because in place the overlap of 2 dirty rectangles would be swapped twice (back to the original)
    switch (context->mode.pixel_format) {
        case DRM_FORMAT_XRGB8888:
        case DRM_FORMAT_ARGB8888:
            // B,G,R,X in memory: swap R and B, force opaque alpha
            for (int i = 0; i < num_rects; i++) {
                for (int y = rects[i].y1; y < rects[i].y2; y++) {
                    const uint32_t *source_row = (const uint32_t *) (source + y * context->buffer.stride);
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
                    const uint32_t *source_row = (const uint32_t *) (source + y * context->buffer.stride);
                    uint32_t *row = pixels + y * PSPDISP_DISPLAY_WIDTH;

                    for (int x = rects[i].x1; x < rects[i].x2; x++) {
                        row[x] = source_row[x] | 0xFF000000u;
                    }
                }
            }
            break;
        default:
            g_warning("%s(): unsupported pixel format %.4s", __FUNCTION__, (const char *) &context->mode.pixel_format);
            return;
    }

    context->display->frame_handler(pixels, PSPDISP_DISPLAY_WIDTH, PSPDISP_PIXEL_FORMAT_8888, rectangles, rectangles_length);
}

/**
 * @brief Handle a power state change of the EVDI display and hand it to the DPMS handler.
 *
 * EVDI calls it (through `evdi_handle_events()`) when the compositor turns the display on or off,
 * e.g. the screen blanks after being idle, the display is disabled on the settings or the user switches to another VT.
 *
 * EVDI uses the DRM DPMS modes, which have the same order as `PSPdispDPMSMode`, but in practice only sends `DRM_MODE_DPMS_ON` and `DRM_MODE_DPMS_OFF`.
 *
 * @param dpms_mode The DRM DPMS mode: `DRM_MODE_DPMS_ON`, `DRM_MODE_DPMS_STANDBY`, `DRM_MODE_DPMS_SUSPEND` or `DRM_MODE_DPMS_OFF`.
 * @param context The provider context, from the `user_data` of the EVDI event context.
 */
void pspdisp_display_provider_evdi_on_dpms(int dpms_mode, PSPdispDisplayEVDIProviderContext *context) {
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

    context->display->dpms_handler(mode);
}

/**
 * @brief Create the virtual display on EVDI and the source of its events.
 *
 * Opens an EVDI device (not attached to a parent device, like a USB dock), registers a buffer of the PSP resolution to grab the frames,
 * and connects the display with the PSP EDID, limited to the PSP pixel area, so the compositor only sets modes that fit the PSP.
 *
 * @param display The display that receives the frames and the DPMS changes.
 * @return A source that dispatches when EVDI has events to handle, it owns the provider context (freed when the source is destroyed).
 */
GSource* pspdisp_display_provider_evdi_provide(PSPdispDisplay *display) {
    PSPdispDisplayEVDIProviderContext *context = g_new(PSPdispDisplayEVDIProviderContext, 1);
    context->display = display;
    context->event_context = (struct evdi_event_context) {
        .mode_changed_handler = (void (*)(struct evdi_mode, void*)) pspdisp_display_provider_evdi_on_mode_changed,
        .update_ready_handler = (void (*)(int, void*)) pspdisp_display_provider_evdi_on_update_ready,
        .dpms_handler         = (void (*)(int, void*)) pspdisp_display_provider_evdi_on_dpms,
        .user_data            = context,
    };
    context->handle = evdi_open_attached_to(NULL);
    context->buffer = (struct evdi_buffer) {
        .id = PSPDISP_EVDI_BUFFER_ID,
        .buffer = g_new0(uint32_t, PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT),
        .width = PSPDISP_DISPLAY_WIDTH,
        .height = PSPDISP_DISPLAY_HEIGHT,
        .stride = PSPDISP_DISPLAY_WIDTH * 4,
        .rects = NULL,
        .rect_count = 0,
    };
    context->pixels = g_new0(uint32_t, PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT);

    evdi_register_buffer(context->handle, context->buffer);
    evdi_connect(context->handle, PSP_EDID, PSP_EDID_SIZE, PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT);

    GSource *source = g_unix_fd_source_new(evdi_get_event_ready(context->handle), G_IO_IN);
    g_source_set_callback(source, (GSourceFunc) pspdisp_display_provider_evdi_func, context, (GDestroyNotify) pspdisp_display_provider_evdi_destroy);
    return source;
}
