/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * field.c - The name of a field                     *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The name of a field of a page (e.g. "Quality"), aligned to the left, next to its widget.
 */

#pragma once

#include <gtk/gtk.h>

/**
 * @brief Make the name of a field.
 *
 * @param text The name.
 * @return The label of the name.
 */
GtkWidget *pspdisp_ui_component_field_new(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    return label;
}
