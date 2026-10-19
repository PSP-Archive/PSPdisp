/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * paragraph.c - A paragraph of text                 *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * A paragraph of a page: a label wrapped at a width that reads well, aligned to the left.
 */

#pragma once

#include <gtk/gtk.h>

/**
 * @brief Make a paragraph of text.
 *
 * @param text The text.
 * @return The label of the paragraph.
 */
GtkWidget *pspdisp_ui_component_paragraph_new(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 60);
    return label;
}
