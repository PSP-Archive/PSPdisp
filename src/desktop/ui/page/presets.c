#ifndef PSPDISP_DESKTOP_UI_PAGE_PRESETS
#define PSPDISP_DESKTOP_UI_PAGE_PRESETS

#include <gtk/gtk.h>
#include "../groupbox.c"

// Preset files component
static GtkWidget *pspdisp_gtk_page_presets_files_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *paragraph = gtk_label_new(
        "Various capturing settings can be saved to preset files to make "
        "switching between different use cases easier. You could e.g. have "
        "a preset for desktop remote control and another one for high "
        "quality audio streaming.");
    gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    // Left column: preset name + preset list
    GtkWidget *name = gtk_entry_new();
    gtk_widget_set_hexpand(name, TRUE);

    // TODO: fill with the preset files
    GtkWidget *list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_SINGLE);

    GtkWidget *scroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), list);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_widget_add_css_class(scroller, "frame");

    // Right column: actions
    GtkWidget *load = gtk_button_new_with_label("Load preset");
    GtkWidget *delete = gtk_button_new_with_label("Delete preset");
    gtk_widget_set_sensitive(load, FALSE);   // enabled once a preset is selected
    gtk_widget_set_sensitive(delete, FALSE); // enabled once a preset is selected

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(actions), load);
    gtk_box_append(GTK_BOX(actions), gtk_button_new_with_label("Refresh list"));
    gtk_box_append(GTK_BOX(actions), gtk_button_new_with_label("Open preset folder"));
    gtk_box_append(GTK_BOX(actions), delete);

    gtk_grid_attach(GTK_GRID(grid), name,                                     0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_button_new_with_label("Save preset"), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), scroller,                                 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), actions,                                  1, 1, 1, 1);

    gtk_box_append(GTK_BOX(box), paragraph);
    gtk_box_append(GTK_BOX(box), grid);

    return pspdisp_gtk_groupbox_new("Preset files", box);
}

// "Presets" page component
static GtkWidget *pspdisp_gtk_page_presets_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);

    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_presets_files_new());
    return box;
}

#endif
