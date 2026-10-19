/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * grid.c - A grid of fields                         *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * A grid of fields: a row for each one, with its name on the left and its widget on the right, which takes the width left.
 */

#pragma once

#include <gtk/gtk.h>
#include "./field.c"

/**
 * @brief Make an empty grid of fields.
 *
 * @return The grid.
 */
GtkWidget *pspdisp_ui_component_grid_new() {
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 24);
	return grid;
}

/**
 * @brief Add a field to a grid of fields: its name, then its widget.
 *
 * @param grid The grid.
 * @param row The row of the field.
 * @param name The name of the field.
 * @param widget The widget of the field.
 */
void pspdisp_ui_component_grid_add_row(GtkWidget *grid, int row, const char *name, GtkWidget *widget) {
	gtk_grid_attach(GTK_GRID(grid), pspdisp_ui_component_field_new(name), 0, row, 1, 1);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
}
