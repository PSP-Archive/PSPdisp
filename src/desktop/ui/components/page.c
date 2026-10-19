/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * page.c - The scrolled box of a page               *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * A page of the window: a box with margins, which the page fills, in a scrolled window, as the pages are taller than a small window.
 */

#pragma once

#include <gtk/gtk.h>

/**
 * @brief Make a page, and the box to fill.
 *
 * @param box Where to write the box to fill.
 * @return The page.
 */
GtkWidget *pspdisp_ui_component_page_new(GtkWidget **box) {
    *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_top(*box, 24);
    gtk_widget_set_margin_bottom(*box, 24);
    gtk_widget_set_margin_start(*box, 24);
    gtk_widget_set_margin_end(*box, 24);

    // Only vertically, the pages are as wide as the window
    GtkWidget *scroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), *box);
    gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scroller), TRUE);
    return scroller;
}
