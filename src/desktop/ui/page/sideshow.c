#ifndef PSPDISP_DESKTOP_UI_PAGE_SIDESHOW
#define PSPDISP_DESKTOP_UI_PAGE_SIDESHOW

#include <gtk/gtk.h>
#include "../groupbox.c"

// Gadget cycling component
static GtkWidget *pspdisp_gtk_page_sideshow_cycling_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *paragraph = gtk_label_new(
        "Interval in which the gadgets are automatically cycled. You can "
        "also set the direction in which the next gadget to display is chosen.");
    gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 24);

    //////////////////////////////////////////////////////////////////
    // "Interval" row
    const int interval_grid_row = 0;
    GtkWidget *interval_key = gtk_label_new("Interval");
    gtk_label_set_xalign(GTK_LABEL(interval_key), 0.0f);

    // Interval range is 1-3600 seconds
    GtkWidget *interval_value = gtk_spin_button_new_with_range(1, 3600, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(interval_value), 30);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(interval_value), TRUE);

    GtkWidget *interval_unit = gtk_label_new("s");
    gtk_label_set_xalign(GTK_LABEL(interval_unit), 0.0f);

    gtk_grid_attach(GTK_GRID(grid), interval_key,   0, interval_grid_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), interval_value, 1, interval_grid_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), interval_unit,  2, interval_grid_row, 1, 1);
    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////
    // "Direction" row
    const int direction_grid_row = 1;
    GtkWidget *direction_key = gtk_label_new("Direction");
    gtk_label_set_xalign(GTK_LABEL(direction_key), 0.0f);

    GtkWidget *direction_value = gtk_drop_down_new_from_strings((const char *[]){ "Left", "Right", NULL });
    gtk_drop_down_set_selected(GTK_DROP_DOWN(direction_value), 1);

    gtk_grid_attach(GTK_GRID(grid), direction_key,   0, direction_grid_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), direction_value, 1, direction_grid_row, 1, 1);
    //////////////////////////////////////////////////////////////////

    gtk_box_append(GTK_BOX(box), paragraph);
    gtk_box_append(GTK_BOX(box), grid);

    return pspdisp_gtk_groupbox_new("Gadget cycling", box);
}

// JPEG compression quality component
static GtkWidget *pspdisp_gtk_page_sideshow_quality_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *paragraph = gtk_label_new(
        "SideShow content is mostly static. Therefore a high quality can be "
        "chosen without affecting performance (default = 95).");
    gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

    // JPEG quality range is 0-100
    GtkWidget *quality = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(quality), 95);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(quality), TRUE);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 24);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Quality"));
    gtk_box_append(GTK_BOX(row), quality);

    gtk_box_append(GTK_BOX(box), paragraph);
    gtk_box_append(GTK_BOX(box), row);

    return pspdisp_gtk_groupbox_new("JPEG compression quality", box);
}

// "SideShow" page component
static GtkWidget *pspdisp_gtk_page_sideshow_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);

    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_sideshow_cycling_new());
    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_sideshow_quality_new());
    return box;
}

#endif
