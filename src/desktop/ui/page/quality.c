/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * quality.c - "Quality" page of the desktop app     *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <gtk/gtk.h>
#include "../app.c"
#include "../components/groupbox.c"
#include "../components/page.c"

/**
 * @brief Set the default JPEG quality of each level, as the old desktop app.
 */
void pspdisp_gtk_page_quality_on_reset(GtkButton *button, PSPdispUI *ui) {
	(void) button;
	PSPdispSettings defaults;
	pspdisp_settings_defaults(&defaults);

	for (size_t i = 0; i <= PSPDISP_COM_QUALITY_HIGHEST; i++) {
		char value[16];
		g_snprintf(value, sizeof(value), "%u", defaults.jpeg_quality[i]);
		pspdisp_ui_set(ui, "Quality", pspdisp_settings_quality_names[i], value);
	}
}

// JPEG compression quality component
GtkWidget *pspdisp_gtk_page_quality_jpeg_new(PSPdispUI *ui) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

	GtkWidget *range_paragraph = gtk_label_new(
		"Quality can be set from 1 to 100. Higher values produce larger "
		"frames which can reduce the frame rate.");
	gtk_label_set_wrap(GTK_LABEL(range_paragraph), TRUE);
	gtk_label_set_wrap_mode(GTK_LABEL(range_paragraph), PANGO_WRAP_WORD);
	gtk_label_set_xalign(GTK_LABEL(range_paragraph), 0.0f);
	gtk_label_set_max_width_chars(GTK_LABEL(range_paragraph), 50);

	GtkWidget *limit_paragraph = gtk_label_new(
		"Values higher than 95 are usually not beneficial for the image "
		"quality and only unnecessarily increase the frame size.");
	gtk_label_set_wrap(GTK_LABEL(limit_paragraph), TRUE);
	gtk_label_set_wrap_mode(GTK_LABEL(limit_paragraph), PANGO_WRAP_WORD);
	gtk_label_set_xalign(GTK_LABEL(limit_paragraph), 0.0f);
	gtk_label_set_max_width_chars(GTK_LABEL(limit_paragraph), 50);

	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 24);
	gtk_widget_set_margin_top(grid, 16);

	// From the highest to the lowest, as the old desktop app
	for (int row = 0; row <= (int) PSPDISP_COM_QUALITY_HIGHEST; row++) {
		int level = PSPDISP_COM_QUALITY_HIGHEST - row;
		GtkWidget *key = gtk_label_new(pspdisp_settings_quality_names[level]);
		gtk_label_set_xalign(GTK_LABEL(key), 0.0f);

		// JPEG quality range is 1-100
		GtkWidget *value = gtk_spin_button_new_with_range(1, 100, 1);
		gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(value), TRUE);
		pspdisp_ui_bind(ui, value, PSPDISP_UI_BINDING_SPIN, "Quality", pspdisp_settings_quality_names[level], offsetof(PSPdispSettings, jpeg_quality) + level * sizeof(uint32_t), NULL);

		gtk_grid_attach(GTK_GRID(grid), key,   0, row, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), value, 1, row, 1, 1);
	}

	GtkWidget *reset = gtk_button_new_with_label("Reset to defaults");
	gtk_widget_set_valign(reset, GTK_ALIGN_CENTER);
	gtk_widget_set_hexpand(reset, TRUE);
	gtk_widget_set_halign(reset, GTK_ALIGN_END);
	g_signal_connect(reset, "clicked", G_CALLBACK(pspdisp_gtk_page_quality_on_reset), ui);
	gtk_grid_attach(GTK_GRID(grid), reset, 2, 0, 1, 5);

	gtk_box_append(GTK_BOX(box), range_paragraph);
	gtk_box_append(GTK_BOX(box), limit_paragraph);
	gtk_box_append(GTK_BOX(box), grid);

	return pspdisp_ui_component_groupbox_new("JPEG compression quality", box);
}

// "Quality" page component
GtkWidget *pspdisp_gtk_page_quality_new(PSPdispUI *ui) {
	GtkWidget *box;
	GtkWidget *scroller = pspdisp_ui_component_page_new(&box);

	gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_quality_jpeg_new(ui));
	return scroller;
}
