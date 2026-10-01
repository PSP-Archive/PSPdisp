/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * display.c - PSP display and provider functions    *
 *                                                   *
 * Copyright © 2026 Net-0                            *
 *****************************************************/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "./display.h"
#include "./edid.c"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Unload the provider, closing its module and clearing its error, making it unfit to use until loaded again.
 *
 * Safe to call on a provider that is not loaded.
 * Disconnect every display using it before, because the provider code (like the source callbacks) goes away with the module.
 *
 * @param display_provider The provider to unload.
 */
void pspdisp_display_provider_unload(PSPdispDisplayProvider *display_provider) {
    if (display_provider->module != NULL) {
        g_module_close(display_provider->module);
        display_provider->module = NULL;
    }
    if (display_provider->module_error != NULL) {
        g_error_free(display_provider->module_error);
        display_provider->module_error = NULL;
    }
    display_provider->provide = NULL;
}

/**
 * @brief Load the provider module and its `provide` function, making it ready to use if possible.
 *
 * On failure it prints why and `provide` stays `NULL`, when the module itself fails to open `module_error` tells why.
 *
 * @param display_provider The provider to load, it must not be loaded yet.
 */
void pspdisp_display_provider_load(PSPdispDisplayProvider *display_provider) {
    g_return_if_fail(display_provider != NULL); // There is no NULL provider
    g_return_if_fail(display_provider->provide == NULL); // If there is a function pointer, it's already loaded

    pspdisp_display_provider_unload(display_provider); // Cleanup any stale resource before trying to load (maybe again?)

    display_provider->module = g_module_open_full(display_provider->file_name, G_MODULE_BIND_LOCAL, &display_provider->module_error);
    if (display_provider->module == NULL) {
        g_printerr("pspdisp_display_provider_load(%s) -> failed to open module: %s\n", display_provider->name, display_provider->module_error->message);
        return;
    }

    if (!g_module_symbol(display_provider->module, display_provider->symbol_name, (gpointer *) &display_provider->provide)) {
        g_printerr("pspdisp_display_provider_load(%s) -> failed to retrieve symbol: %s\n", display_provider->name, g_module_error());
        return;
    }
}

/**
 * @brief Ask the provider to create the virtual display and return the source of its frames.
 *
 * @param display_provider The provider to use, it must be loaded.
 * @param display The display that receives the frames and the DPMS changes.
 * @return The source of frames (not attached to any context yet), or `NULL` if the provider is not loaded or failed.
 */
GSource *pspdisp_display_provider_provide(PSPdispDisplayProvider *display_provider, PSPdispDisplay *display) {
    g_return_val_if_fail(display_provider != NULL, NULL);
    g_return_val_if_fail(display != NULL, NULL);
    g_return_val_if_fail(display_provider->module != NULL, NULL);
    g_return_val_if_fail(display_provider->module_error == NULL, NULL);
    g_return_val_if_fail(display_provider->provide != NULL, NULL);
    return display_provider->provide(display);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Quit the event loop of the display thread, called when `thread_cancellable` is cancelled.
 *
 * @param cancellable The cancelled `thread_cancellable`.
 * @param display The display that owns the thread.
 * @return Always `G_SOURCE_REMOVE`, it's only needed once.
 */
gboolean pspdisp_display_thread_cancel(GCancellable *cancellable, PSPdispDisplay *display) {
    (void) cancellable;
    g_main_loop_quit(display->loop);
    return G_SOURCE_REMOVE;
}

/**
 * @brief Body of the display thread: runs the event loop of `context` until the display is destroyed.
 *
 * Makes `context` the thread default context (so sources created on this thread use it),
 * and watches `thread_cancellable` to quit the loop when `pspdisp_display_destroy()` cancels it.
 *
 * @param display The display that owns the thread.
 * @return Always `NULL`, nothing is given to `g_thread_join()`.
 */
gpointer pspdisp_display_thread_func(PSPdispDisplay *display) {
    g_main_context_push_thread_default(display->context);

    GSource *cancellable_source = g_cancellable_source_new(display->thread_cancellable);
    g_source_set_callback(cancellable_source, G_SOURCE_FUNC(pspdisp_display_thread_cancel), display, NULL); // Called as `GCancellableSourceFunc`
    g_source_attach(cancellable_source, display->context);
    g_source_unref(cancellable_source);

    g_main_loop_run(display->loop); // blocks until quit

    g_main_context_pop_thread_default(display->context);
    return NULL;
}

/**
 * @brief Create a new display, not connected yet (see `pspdisp_display_connect()`).
 *
 * Starts the display thread, which runs the event loop where the provider calls the frame and DPMS handlers.
 *
 * The model, the region and the serial number give the EDID of the display (see `pspdisp_display_edid_new()`).
 *
 * @param provider The provider used to create the virtual display when connecting.
 * @param model The PSP generation.
 * @param region The PSP sales region.
 * @param serial_number The serial number of the display, not 0 (see `pspdisp_display_edid_new()`).
 * @param frame_handler Handle each new frame, called on the display thread (can't be `NULL`).
 * @param dpms_handler Handle each DPMS change, called on the display thread (can't be `NULL`).
 * @return The new display, free it with `pspdisp_display_destroy()`, or `NULL` when a handler is `NULL`.
 */
PSPdispDisplay *pspdisp_display_new(PSPdispDisplayProvider *provider, PSPdispModel model, PSPdispRegion region, uint32_t serial_number, PSPdispFrameHandler frame_handler, PSPdispDPMSHandler dpms_handler) {
    g_return_val_if_fail(frame_handler != NULL, NULL);
    g_return_val_if_fail(dpms_handler != NULL, NULL);

    PSPdispDisplay *display = g_new(PSPdispDisplay, 1);
    display->provider = provider;
    display->frame_handler = frame_handler;
    display->dpms_handler = dpms_handler;
    display->edid = pspdisp_display_edid_new(model, region, serial_number);
    display->context = g_main_context_new();
    display->loop = g_main_loop_new(display->context, FALSE);
    display->thread_cancellable = g_cancellable_new();
    display->frame_source = NULL;
    display->thread = g_thread_new("pspdisp-display-frame-loop", (GThreadFunc) pspdisp_display_thread_func, display); // Last, the thread uses the fields above
    return display;
}

/**
 * @brief Connect the display, enabling it as a device: the provider creates the virtual display, so it appears on the computer.
 *
 * The source of frames is attached to `context`, so the provider callbacks (and the handlers) run on the display thread.
 *
 * @param display The display to connect.
 * @return `true` if it was connected now, `false` if it was already connected or the provider failed.
 */
bool pspdisp_display_connect(PSPdispDisplay *display) {
    if (display->frame_source != NULL) return false; // Already connected

    GSource *frame_source = pspdisp_display_provider_provide(display->provider, display);
    if (frame_source == NULL) return false; // The provider failed

    g_source_attach(frame_source, display->context);
    display->frame_source = frame_source;
    return true;
}

/**
 * @brief Disconnect the display, disabling it as a device: the virtual display is removed from the computer.
 *
 * Destroying the source of frames calls its destroy callback, where the provider frees its resources and removes the virtual display.
 *
 * @param display The display to disconnect.
 * @return `true` if it was disconnected now, `false` if it was already disconnected.
 */
bool pspdisp_display_disconnect(PSPdispDisplay *display) {
    if (display->frame_source == NULL) return false; // Already disconnected

    g_source_destroy(display->frame_source); // Distach from the GMainContext and GMainLoop, call the destroy callback and also clean up it's own resources
    g_source_unref(display->frame_source); // Release the reference returned by the provider
    display->frame_source = NULL;
    return true;
}

/**
 * @brief Destroy the display, freeing all resources and making it forever unusable.
 *
 * Disconnects it (if connected), then stops the display thread and waits for it to finish,
 * so it must not be called from the display thread (e.g. inside a handler), it would wait for itself.
 *
 * @param display The display to destroy.
 */
void pspdisp_display_destroy(PSPdispDisplay *display) {
    pspdisp_display_disconnect(display); // Just in case, it also releases the source of frames

    // Finish the event-loop worker thread
    g_cancellable_cancel(display->thread_cancellable);
    g_thread_join(display->thread);

    // Unref resources for cleanup
    g_main_loop_unref(display->loop);
    g_main_context_unref(display->context);
    g_object_unref(display->thread_cancellable);

    g_free(display);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief The known display providers of this platform.
 *
 * Remember to call `pspdisp_display_providers_load()` before using them.
 * The file names are relative to the working directory, so run the program from the folder that has the `display/` directory (e.g. `build/`).
 */
PSPdispDisplayProvider pspdisp_display_providers[] = {
    #if defined(__linux__)
        { .name = "EVDI", .file_name = "./display/provider/evdi.so", .symbol_name = "pspdisp_display_provider_evdi_provide", .module = NULL, .module_error = NULL, .provide = NULL },
    #else
        #error "Unsupported platform"
    #endif
};

/**
 * @brief Load all the known providers, making them ready to use if possible (see `pspdisp_display_provider_load()`).
 */
void pspdisp_display_providers_load() {
    for (uint8_t i = 0; i < sizeof(pspdisp_display_providers) / sizeof(pspdisp_display_providers[0]); i++)
        pspdisp_display_provider_load(&pspdisp_display_providers[i]);
}

/**
 * @brief Unload all the known providers, freeing resources and making them unfit to use (see `pspdisp_display_provider_unload()`).
 */
void pspdisp_display_providers_unload() {
    for (uint8_t i = 0; i < sizeof(pspdisp_display_providers) / sizeof(pspdisp_display_providers[0]); i++)
        pspdisp_display_provider_unload(&pspdisp_display_providers[i]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
