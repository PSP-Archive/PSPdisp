#ifndef PSPDISP_DESKTOP_UI_PAGE_UPDATE_INTERVAL
#define PSPDISP_DESKTOP_UI_PAGE_UPDATE_INTERVAL

#include <gtk/gtk.h>
#include "../groupbox.c"

// Delay between frames component
static GtkWidget *pspdisp_gtk_page_update_interval_delay_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *menu_paragraph = gtk_label_new(
        "If chosen in the 'Update Interval' menu, PSPdisp will delay the "
        "display of the next frame. This limits the maximum framerate.");
    gtk_label_set_wrap(GTK_LABEL(menu_paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(menu_paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(menu_paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(menu_paragraph), 50);

    GtkWidget *tradeoff_paragraph = gtk_label_new(
        "Higher delays lead to choppier display with lower fps. Lower delays "
        "will produce a smoother display, but utilize more CPU cycles.");
    gtk_label_set_wrap(GTK_LABEL(tradeoff_paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(tradeoff_paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(tradeoff_paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(tradeoff_paragraph), 50);

    // TODO: move the defaults to a shared place once the settings are wired
    const char *level_names[]  = { "Shortest", "Short", "Medium", "Long", "Longest" };
    const int   level_values[] = { 30,         40,      50,       100,    200       };

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 24);
    gtk_widget_set_margin_top(grid, 16);

    for (int row = 0; row < 5; row++) {
        GtkWidget *key = gtk_label_new(level_names[row]);
        gtk_label_set_xalign(GTK_LABEL(key), 0.0f);

        // Delay range is 0-1000 ms
        GtkWidget *value = gtk_spin_button_new_with_range(0, 1000, 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(value), level_values[row]);
        gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(value), TRUE);

        GtkWidget *unit = gtk_label_new("ms");
        gtk_label_set_xalign(GTK_LABEL(unit), 0.0f);

        gtk_grid_attach(GTK_GRID(grid), key,   0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), value, 1, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), unit,  2, row, 1, 1);
    }

    GtkWidget *reset = gtk_button_new_with_label("Reset to defaults");
    gtk_widget_set_valign(reset, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(reset, TRUE);
    gtk_widget_set_halign(reset, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), reset, 3, 0, 1, 5);

    gtk_box_append(GTK_BOX(box), menu_paragraph);
    gtk_box_append(GTK_BOX(box), tradeoff_paragraph);
    gtk_box_append(GTK_BOX(box), grid);

    return pspdisp_gtk_groupbox_new("Delay between frames", box);
}

// "Update interval" page component
static GtkWidget *pspdisp_gtk_page_update_interval_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);

    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_update_interval_delay_new());
    return box;
}

#endif
