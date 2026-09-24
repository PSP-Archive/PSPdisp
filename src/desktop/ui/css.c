#ifndef PSPDISP_DESKTOP_UI_CSS
#define PSPDISP_DESKTOP_UI_CSS

#include <gtk/gtk.h>

// Return the registered CSS class for groupbox in GTK
const char *pspdisp_css_groupbox_class() {
    static gboolean css_done = FALSE; // Just so we don't reload CSS between multiple calls
    if (!css_done) {
        GtkCssProvider *p = gtk_css_provider_new();
        gtk_css_provider_load_from_string(p,
            ".pspdisp-css-groupbox {"
            "  background-color: @theme_bg_color;"
            "  border: 1px solid alpha(currentColor, 0.3);"
            "  border-radius: 6px;"
            "}"
        );
        gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(p), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(p);
        css_done = TRUE;
    }
    return "pspdisp-css-groupbox";
}

// Return the registered CSS class for groupbox captions in GTK
const char *pspdisp_css_groupbox_caption_class() {
    static gboolean css_done = FALSE; // Just so we don't reload CSS between multiple calls
    if (!css_done) {
        GtkCssProvider *p = gtk_css_provider_new();
        gtk_css_provider_load_from_string(p,
            ".pspdisp-css-groupbox-caption {"
            "  background-color: @theme_bg_color;"
            "  padding-left: 4px;"
            "  padding-right: 4px;"
            "}"
        );
        gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(p), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(p);
        css_done = TRUE;
    }
    return "pspdisp-css-groupbox-caption";
}

#endif