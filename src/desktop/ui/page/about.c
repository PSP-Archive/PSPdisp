#ifndef PSPDISP_DESKTOP_UI_PAGE_ABOUT
#define PSPDISP_DESKTOP_UI_PAGE_ABOUT

#include <gtk/gtk.h>

// Title component - uses Pango markup to style the project name
static GtkWidget *pspdisp_gtk_page_about_title_new() {
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span size='xx-large' weight='bold'>PSPdisp</span> © 2008-2015 JJS, © 2026 Net-0");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    return label;
}

// Version info component
static GtkWidget *pspdisp_gtk_page_about_version_new() {
    GtkWidget *label = gtk_label_new("Version 0.0.1"); // TODO: see how to put here the current repo version?
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    return label;
}

// Links component - contains useful links that the user can access
static GtkWidget *pspdisp_gtk_page_about_links_new() {    
    GtkWidget *github_link = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(github_link), "Visit the PSPdisp GitHub repository <a href=\"https://github.com/Net-0/PSPdisp\">Net-0/PSPdisp</a>");
    gtk_label_set_xalign(GTK_LABEL(github_link), 0.0f);
    gtk_widget_set_margin_top(github_link, 16);

    GtkWidget *jjs_link = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(jjs_link), "Visit the original JJS homepage @ <a href=\"http://www.jjs.at/redirect/pspdisp/v0.6\">jjs.at</a>");
    gtk_label_set_xalign(GTK_LABEL(jjs_link), 0.0f);
    gtk_widget_set_margin_top(jjs_link, 16);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_append(GTK_BOX(box), github_link);
    gtk_box_append(GTK_BOX(box), jjs_link);
    return box;
}

// Status component - contains use infos about the PSP display
static GtkWidget *pspdisp_gtk_page_about_status_new() {
    // TODO: create here a worker thread to update the infos?
    // TODO: what should we show when there is no PSP connected? Maybe another text?
    // TODO: add the "Connected since" info to show since when the PSP is connected?
    // TODO: add "Average frame size" with compressed and uncompressed sizes
    // TODO: add "Total amount sent" with comrpessed and uncompressed sizes

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 32);
    gtk_widget_set_margin_top(grid, 10);
    gtk_widget_set_margin_bottom(grid, 8);
    gtk_widget_set_margin_start(grid, 12);
    gtk_widget_set_margin_end(grid, 12);

    //////////////////////////////////////////////////////////////////
    // "Frames per second" row
    const int fps_grid_row = 0;
    GtkWidget *fps_key = gtk_label_new("Frames per second");
    gtk_label_set_xalign(GTK_LABEL(fps_key), 0.0f);

    GtkWidget *fps_value = gtk_label_new("0");
    gtk_label_set_xalign(GTK_LABEL(fps_value), 0.0f);

    gtk_grid_attach(GTK_GRID(grid), fps_key,   0, fps_grid_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), fps_value, 1, fps_grid_row, 1, 1);
    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////
    // "Average frame size" row
    const int avg_size_grid_row = 1;
    GtkWidget *avg_size_key = gtk_label_new("Average frame size");
    gtk_label_set_xalign(GTK_LABEL(avg_size_key), 0.0f);

    GtkWidget *avg_size_value = gtk_label_new("3 KiB");
    gtk_label_set_xalign(GTK_LABEL(avg_size_value), 0.0f);

    gtk_grid_attach(GTK_GRID(grid), avg_size_key,   0, avg_size_grid_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), avg_size_value, 1, avg_size_grid_row, 1, 1);
    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////
    // "Total frames sent" row
    const int frames_sent_grid_row = 2;
    GtkWidget *frames_sent_key = gtk_label_new("Total frames sent");
    gtk_label_set_xalign(GTK_LABEL(frames_sent_key), 0.0f);

    GtkWidget *frames_sent_value = gtk_label_new("2483");
    gtk_label_set_xalign(GTK_LABEL(frames_sent_value), 0.0f);

    gtk_grid_attach(GTK_GRID(grid), frames_sent_key,   0, frames_sent_grid_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), frames_sent_value, 1, frames_sent_grid_row, 1, 1);
    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////
    // "Total amount sent" row
    const int total_sent_grid_row = 3;
    GtkWidget *total_sent_key = gtk_label_new("Total amount sent");
    gtk_label_set_xalign(GTK_LABEL(total_sent_key), 0.0f);

    GtkWidget *total_sent_value = gtk_label_new("21 MiB");
    gtk_label_set_xalign(GTK_LABEL(total_sent_value), 0.0f);

    gtk_grid_attach(GTK_GRID(grid), total_sent_key,   0, total_sent_grid_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), total_sent_value, 1, total_sent_grid_row, 1, 1);
    //////////////////////////////////////////////////////////////////

    static gboolean css_done = FALSE; // Just so we don't reload CSS between multiple calls1
    if (!css_done) {
        GtkCssProvider *p = gtk_css_provider_new();
        gtk_css_provider_load_from_string(p,
            ".pspdisp-page-about-status-groupbox {"
            "  background-color: @theme_bg_color;"
            "  border: 1px solid alpha(currentColor, 0.3);"
            "  border-radius: 6px;"
            "}"
            ".pspdisp-page-about-status-groupbox-caption {"
            "  background-color: @theme_bg_color;"
            "  padding-left: 4px;"
            "  padding-right: 4px;"
            "}"
        );
        gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(p), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(p);
        css_done = TRUE;
    }

    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_add_css_class(frame, "pspdisp-page-about-status-groupbox");
    gtk_frame_set_child(GTK_FRAME(frame), grid);
    gtk_widget_set_margin_top(frame, 9);

    GtkWidget *caption = gtk_label_new(" Status ");
    gtk_widget_add_css_class(caption, "pspdisp-page-about-status-groupbox-caption");
    gtk_widget_set_halign(caption, GTK_ALIGN_START);
    gtk_widget_set_valign(caption, GTK_ALIGN_START);
    gtk_widget_set_margin_start(caption, 8);

    GtkWidget *overlay = gtk_overlay_new();
    gtk_overlay_set_child(GTK_OVERLAY(overlay), frame);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), caption);
    return overlay;
}

// "About" page component
static GtkWidget *pspdisp_gtk_page_about_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);

    GtkWidget *title = pspdisp_gtk_page_about_title_new();
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    GtkWidget *version = pspdisp_gtk_page_about_version_new();
    gtk_widget_set_halign(version, GTK_ALIGN_START);

    GtkWidget *links = pspdisp_gtk_page_about_links_new();
    gtk_widget_set_halign(links, GTK_ALIGN_START);
    
    GtkWidget *status = pspdisp_gtk_page_about_status_new();
    gtk_widget_set_halign(status, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(status, 24);

    gtk_box_append(GTK_BOX(box), title);
    gtk_box_append(GTK_BOX(box), version);
    gtk_box_append(GTK_BOX(box), links);
    gtk_box_append(GTK_BOX(box), status);
    return box;
}

#endif