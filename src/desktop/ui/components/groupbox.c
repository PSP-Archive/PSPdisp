/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * groupbox.c - A frame titled over its border       *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * A group box, as the group boxes of the old desktop app: a frame with a rounded border around a part of a page, with its title over the top border.
 */

#pragma once

#include <gtk/gtk.h>

/**
 * @brief Make a group box around a body, with its title over the top border.
 *
 * @param title The title.
 * @param body The widget inside the box.
 * @return The group box.
 */
GtkWidget *pspdisp_ui_component_groupbox_new(const char *title, GtkWidget *body) {
    static bool style_added = false; // The style of the group boxes is added to the display once, for all of them

    if (!style_added) {
        // The title has the background of the window, so the border doesn't go through it
        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_string(provider,
            ".pspdisp-css-groupbox {"
            "  background-color: @theme_bg_color;"
            "  border: 1px solid alpha(currentColor, 0.3);"
            "  border-radius: 6px;"
            "  padding: 20px;"
            "}"
            ".pspdisp-css-groupbox-caption {"
            "  background-color: @theme_bg_color;"
            "  padding: 0 6px;"
            "}"
        );
        gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(provider);
        style_added = true;
    }

    // The frame starts lower than the title, so the title lies across its top border
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_add_css_class(frame, "pspdisp-css-groupbox");
    gtk_frame_set_child(GTK_FRAME(frame), body);
    gtk_widget_set_margin_top(frame, 9);

    GtkWidget *caption = gtk_label_new(title);
    gtk_widget_add_css_class(caption, "pspdisp-css-groupbox-caption");
    gtk_widget_set_halign(caption, GTK_ALIGN_START);
    gtk_widget_set_valign(caption, GTK_ALIGN_START);
    gtk_widget_set_margin_start(caption, 4);

    // The title over the frame
    GtkWidget *overlay = gtk_overlay_new();
    gtk_overlay_set_child(GTK_OVERLAY(overlay), frame);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), caption);
    return overlay;
}
