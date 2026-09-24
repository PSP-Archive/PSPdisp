#ifndef PSPDISP_DESKTOP_UI_PAGE_DISPLAY
#define PSPDISP_DESKTOP_UI_PAGE_DISPLAY

#include <gtk/gtk.h>
#include "../groupbox.c"

// Draws the displays layout preview (the original draws the PSPdisp display as a white box and the other displays as grey boxes)
static void pspdisp_gtk_page_display_preview_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
    // TODO: draw the monitors from gdk_display_get_monitors()
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);
}

// Displays layout preview component
static GtkWidget *pspdisp_gtk_page_display_preview_new() {
    GtkWidget *area = gtk_drawing_area_new();
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), 178);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), pspdisp_gtk_page_display_preview_draw, NULL, NULL);
    gtk_widget_set_hexpand(area, TRUE);
    return area;
}

// Extended display position component
static GtkWidget *pspdisp_gtk_page_display_position_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *paragraph = gtk_label_new(
        "In the graphic above, the white box represents the PSPdisp display "
        "and the grey boxes your other displays.");
    gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

    GtkWidget *x_value = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(x_value), "0");
    gtk_editable_set_width_chars(GTK_EDITABLE(x_value), 6);

    GtkWidget *y_value = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(y_value), "0");
    gtk_editable_set_width_chars(GTK_EDITABLE(y_value), 6);

    GtkWidget *apply = gtk_button_new_with_label("Apply");
    gtk_widget_set_hexpand(apply, TRUE);
    gtk_widget_set_halign(apply, GTK_ALIGN_END);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(row), gtk_label_new("X"));
    gtk_box_append(GTK_BOX(row), x_value);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Y"));
    gtk_box_append(GTK_BOX(row), y_value);
    gtk_box_append(GTK_BOX(row), apply);

    gtk_box_append(GTK_BOX(box), paragraph);
    gtk_box_append(GTK_BOX(box), row);

    return pspdisp_gtk_groupbox_new("PSPdisp extended display position", box);
}

// "Display" page component
static GtkWidget *pspdisp_gtk_page_display_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);

    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_display_preview_new());
    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_display_position_new());
    return box;
}

#endif
