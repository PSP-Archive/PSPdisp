#ifndef PSPDISP_DESKTOP_UI_PAGE_CONTROL
#define PSPDISP_DESKTOP_UI_PAGE_CONTROL

#include <gtk/gtk.h>
#include "../groupbox.c"

// Control files editing component
static GtkWidget *pspdisp_gtk_page_control_files_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *open_folder = gtk_button_new_with_label("Open control file folder to modify / add control files");

    GtkWidget *paragraph = gtk_label_new(
        "To create a new control file, copy over an existing one (like "
        "'mouse.control') and edit this. You can also modify an existing "
        "control scheme.");
    gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

    GtkWidget *refresh_list = gtk_button_new_with_label("Refresh control file list");
    GtkWidget *reload_current = gtk_button_new_with_label("Reload current control file");

    gtk_box_append(GTK_BOX(box), open_folder);
    gtk_box_append(GTK_BOX(box), paragraph);
    gtk_box_append(GTK_BOX(box), refresh_list);
    gtk_box_append(GTK_BOX(box), reload_current);

    return pspdisp_gtk_groupbox_new("Edit files", box);
}

// Scancode viewer component
static GtkWidget *pspdisp_gtk_page_control_scancode_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *paragraph = gtk_label_new(
        "Click into the edit field and press a key to view its scancode. "
        "Some keys do not show their scancode here, please refer to the "
        "listing in the help file instead.");
    gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

    // TODO: attach a GtkEventControllerKey to fill this with the pressed key's scancode
    GtkWidget *scancode = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(scancode), FALSE);
    gtk_widget_set_halign(scancode, GTK_ALIGN_CENTER);

    gtk_box_append(GTK_BOX(box), paragraph);
    gtk_box_append(GTK_BOX(box), scancode);

    return pspdisp_gtk_groupbox_new("Scancode", box);
}

// "Control" page component
static GtkWidget *pspdisp_gtk_page_control_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);

    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_control_files_new());
    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_control_scancode_new());
    return box;
}

#endif
