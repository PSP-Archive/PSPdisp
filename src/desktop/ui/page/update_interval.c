/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * update_interval.c - "Update interval" page        *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <gtk/gtk.h>
#include "../app.c"
#include "../components/groupbox.c"
#include "../components/page.c"

/**
 * @brief Set the default pause of each update interval, as the old desktop app.
 */
void pspdisp_gtk_page_update_interval_on_reset(GtkButton *button, PSPdispUI *ui) {
	(void) button;
	PSPdispSettings defaults;
	pspdisp_settings_defaults(&defaults);

	for (size_t i = PSPDISP_COM_UPDATE_INTERVAL_SHORTEST; i <= PSPDISP_COM_UPDATE_INTERVAL_LONGEST; i++) {
		char value[16];
		g_snprintf(value, sizeof(value), "%u", defaults.update_interval[i]);
		pspdisp_ui_set(ui, "UpdateInterval", pspdisp_settings_update_interval_names[i], value);
	}
}

// Delay between frames component
GtkWidget *pspdisp_gtk_page_update_interval_delay_new(PSPdispUI *ui) {
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
		"will produce a smoother display, but utilize more CPU cycles. "
		"With the sound streamed, its pace is the one of the frames instead.");
	gtk_label_set_wrap(GTK_LABEL(tradeoff_paragraph), TRUE);
	gtk_label_set_wrap_mode(GTK_LABEL(tradeoff_paragraph), PANGO_WRAP_WORD);
	gtk_label_set_xalign(GTK_LABEL(tradeoff_paragraph), 0.0f);
	gtk_label_set_max_width_chars(GTK_LABEL(tradeoff_paragraph), 50);

	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 24);
	gtk_widget_set_margin_top(grid, 16);

	for (int row = 0; row < (int) PSPDISP_COM_UPDATE_INTERVAL_LONGEST; row++) {
		int interval = PSPDISP_COM_UPDATE_INTERVAL_SHORTEST + row;
		GtkWidget *key = gtk_label_new(pspdisp_settings_update_interval_names[interval]);
		gtk_label_set_xalign(GTK_LABEL(key), 0.0f);

		// Delay range is 0-10000 ms
		GtkWidget *value = gtk_spin_button_new_with_range(0, 10000, 1);
		gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(value), TRUE);
		pspdisp_ui_bind(ui, value, PSPDISP_UI_BINDING_SPIN, "UpdateInterval", pspdisp_settings_update_interval_names[interval], offsetof(PSPdispSettings, update_interval) + interval * sizeof(uint32_t), NULL);

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
	g_signal_connect(reset, "clicked", G_CALLBACK(pspdisp_gtk_page_update_interval_on_reset), ui);
	gtk_grid_attach(GTK_GRID(grid), reset, 3, 0, 1, 5);

	gtk_box_append(GTK_BOX(box), menu_paragraph);
	gtk_box_append(GTK_BOX(box), tradeoff_paragraph);
	gtk_box_append(GTK_BOX(box), grid);

	return pspdisp_ui_component_groupbox_new("Delay between frames", box);
}

// "Update interval" page component
GtkWidget *pspdisp_gtk_page_update_interval_new(PSPdispUI *ui) {
	GtkWidget *box;
	GtkWidget *scroller = pspdisp_ui_component_page_new(&box);

	gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_update_interval_delay_new(ui));
	return scroller;
}
