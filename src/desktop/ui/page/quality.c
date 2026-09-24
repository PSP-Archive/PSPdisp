#ifndef PSPDISP_DESKTOP_UI_PAGE_QUALITY
#define PSPDISP_DESKTOP_UI_PAGE_QUALITY

#include <gtk/gtk.h>
#include "../groupbox.c"

// JPEG compression quality component
static GtkWidget *pspdisp_gtk_page_quality_jpeg_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *range_paragraph = gtk_label_new(
        "Quality can be set from 0 to 100. Higher values produce larger "
        "frames which can reduce the frame rate.");
    gtk_label_set_wrap(GTK_LABEL(range_paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(range_paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(range_paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(range_paragraph), 50);

    GtkWidget *limit_paragraph = gtk_label_new(
        "Values higher than 95 are usually not beneficial for the image "
        "quality and only unnecessarily increase the frame size.");
    gtk_label_set_wrap(GTK_LABEL(limit_paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(limit_paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(limit_paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(limit_paragraph), 50);

    // TODO: move the defaults to a shared place once the settings are wired
    const char *level_names[]  = { "Highest", "High", "Medium", "Low", "Lowest" };
    const int   level_values[] = { 80,        50,     30,       20,    10       };

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 24);
    gtk_widget_set_margin_top(grid, 16);

    for (int row = 0; row < 5; row++) {
        GtkWidget *key = gtk_label_new(level_names[row]);
        gtk_label_set_xalign(GTK_LABEL(key), 0.0f);

        // JPEG quality range is 0-100
        GtkWidget *value = gtk_spin_button_new_with_range(0, 100, 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(value), level_values[row]);
        gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(value), TRUE);

        gtk_grid_attach(GTK_GRID(grid), key,   0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), value, 1, row, 1, 1);
    }

    GtkWidget *reset = gtk_button_new_with_label("Reset to defaults");
    gtk_widget_set_valign(reset, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(reset, TRUE);
    gtk_widget_set_halign(reset, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), reset, 2, 0, 1, 5);

    gtk_box_append(GTK_BOX(box), range_paragraph);
    gtk_box_append(GTK_BOX(box), limit_paragraph);
    gtk_box_append(GTK_BOX(box), grid);

    return pspdisp_gtk_groupbox_new("JPEG compression quality", box);
}

// "Quality" page component
static GtkWidget *pspdisp_gtk_page_quality_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);

    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_quality_jpeg_new());
    return box;
}

#endif
