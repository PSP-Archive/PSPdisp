#ifndef PSPDISP_DESKTOP_UI_PAGE_WLAN
#define PSPDISP_DESKTOP_UI_PAGE_WLAN

#include <gtk/gtk.h>
#include "../groupbox.c"

// Notifications settings component
static GtkWidget *pspdisp_gtk_page_wlan_notifications_new() {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 24);

    GtkWidget *server_checkbox = gtk_check_button_new_with_label("Server has been started");
    GtkWidget *connected_checkbox = gtk_check_button_new_with_label("Client connected");
    GtkWidget *disconnected_checkbox = gtk_check_button_new_with_label("Client disconnected");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(server_checkbox), TRUE);

    gtk_grid_attach(GTK_GRID(grid), server_checkbox, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), connected_checkbox, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), disconnected_checkbox, 0, 1, 1, 1);

    return pspdisp_gtk_groupbox_new("Show notification on those events", grid);
}

// Password settings component
static GtkWidget *pspdisp_gtk_page_wlan_password_new() {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_max_length(GTK_ENTRY(entry), 32);
    gtk_entry_set_invisible_char(GTK_ENTRY(entry), L'*');
    gtk_widget_set_hexpand(entry, TRUE);

    gtk_box_append(GTK_BOX(row), gtk_label_new("Password (max 32 chars)"));
    gtk_box_append(GTK_BOX(row), entry);
    gtk_box_append(GTK_BOX(row), gtk_button_new_with_label("No password"));

    return pspdisp_gtk_groupbox_new("Password", row);
}

// IP address info component
static GtkWidget *pspdisp_gtk_page_wlan_ip_address_new() {
    GtkWidget *paragraph = gtk_label_new(
        "When WLAN mode is chosen and PSPdisp enabled, all IP addresses "
        "bound to this PC are shown here.");
    gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

    return pspdisp_gtk_groupbox_new("IP address", paragraph);
}

// "WLAN" page component
static GtkWidget *pspdisp_gtk_page_wlan_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);

    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_wlan_notifications_new());
    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_wlan_password_new());
    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_wlan_ip_address_new());
    return box;
}

#endif