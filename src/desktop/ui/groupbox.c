#pragma once

#include <gtk/gtk.h>

// Groupbox componentv - used to create a box with curved border and with a title over the border
GtkWidget *pspdisp_gtk_groupbox_new(const char* title, GtkWidget *body) {
    static gboolean css_done = FALSE; // Just so we don't reload CSS between multiple calls
    if (!css_done) {
        GtkCssProvider *p = gtk_css_provider_new();
        gtk_css_provider_load_from_string(p,
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
        gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(p), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(p);
        css_done = TRUE;
    }

    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_add_css_class(frame, "pspdisp-css-groupbox");
    gtk_frame_set_child(GTK_FRAME(frame), body);
    gtk_widget_set_margin_top(frame, 9);

    GtkWidget *caption = gtk_label_new(title);
    gtk_widget_add_css_class(caption, "pspdisp-css-groupbox-caption");
    gtk_widget_set_halign(caption, GTK_ALIGN_START);
    gtk_widget_set_valign(caption, GTK_ALIGN_START);
    gtk_widget_set_margin_start(caption, 4);

    GtkWidget *overlay = gtk_overlay_new();
    gtk_overlay_set_child(GTK_OVERLAY(overlay), frame);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), caption);
    return overlay;
}
