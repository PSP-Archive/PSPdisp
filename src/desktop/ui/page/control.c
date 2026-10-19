/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * control.c - "Control" page of the desktop app     *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <gtk/gtk.h>
#include "../app.c"
#include "../components/groupbox.c"
#include "../components/page.c"
#include "../components/paragraph.c"
#include "../../controller/input.c"

/**
 * @brief The widgets of the "Control" page that aren't bound to a setting.
 */
typedef struct {
	GtkWidget *files; // The control files
} PSPdispGtkPageControl;

/**
 * @brief Show the control files, after the settings were loaded again.
 */
void pspdisp_gtk_page_control_refresh(PSPdispUI *ui, PSPdispGtkPageControl *page) {
	GString *text = g_string_new(NULL);

	for (guint i = 0; i < ui->controls->len; i++) {
		const PSPdispSettingsControl *control = ui->controls->pdata[i];
		char *file = g_path_get_basename(control->path);
		g_string_append_printf(text, "%s%s (%s)", text->len > 0 ? "\n" : "", control->name, file);
		g_free(file);
	}
	if (text->len == 0)
		g_string_append(text, "No control file, add one to the folder of the control files.");

	gtk_label_set_text(GTK_LABEL(page->files), text->str);
	g_string_free(text, TRUE);
}

void pspdisp_gtk_page_control_on_open_folder(GtkButton *button, PSPdispUI *ui) {
	(void) button;
	char *path = pspdisp_settings_control_dir(ui->settings_dir);
	pspdisp_ui_open_folder(ui, path);
	g_free(path);
}

void pspdisp_gtk_page_control_on_refresh(GtkButton *button, PSPdispUI *ui) {
	(void) button;
	pspdisp_ui_reload(ui);
}

/**
 * @brief Show the scan code of the key pressed in the entry, as a control file writes it (e.g. 001E for A, E048 for up).
 */
gboolean pspdisp_gtk_page_control_on_key(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, GtkWidget *entry) {
	(void) controller;
	(void) keyval;
	(void) state;
	uint16_t scancode = pspdisp_input_scancode_of_key(keycode);
	char text[32];

	if (scancode != 0)
		g_snprintf(text, sizeof(text), "%04X", scancode);
	else
		g_strlcpy(text, "no scan code", sizeof(text));

	gtk_editable_set_text(GTK_EDITABLE(entry), text);
	return TRUE; // The key isn't typed
}

// Control files editing component
GtkWidget *pspdisp_gtk_page_control_files_new(PSPdispUI *ui, PSPdispGtkPageControl *page) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

	GtkWidget *choice = pspdisp_ui_component_paragraph_new(
		"What the PSP buttons drive is chosen on the \"Streaming\" page, or in the \"PC Settings\" of the PSP: "
		"a virtual gamepad (\"Gamepad\"), nothing (\"None\"), or a virtual keyboard and mouse as a control file says.");

	page->files = gtk_label_new("");
	gtk_label_set_xalign(GTK_LABEL(page->files), 0.0f);
	gtk_widget_set_margin_start(page->files, 12);

	GtkWidget *open_folder = gtk_button_new_with_label("Open control file folder to modify / add control files");
	g_signal_connect(open_folder, "clicked", G_CALLBACK(pspdisp_gtk_page_control_on_open_folder), ui);

	GtkWidget *paragraph = gtk_label_new(
		"To create a new control file, copy over an existing one (like "
		"'mouse.control', or 'control.template' for an empty one) and edit this. "
		"You can also modify an existing control scheme: the changes are used at once.");
	gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
	gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
	gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
	gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

	GtkWidget *refresh_list = gtk_button_new_with_label("Refresh control file list");
	g_signal_connect(refresh_list, "clicked", G_CALLBACK(pspdisp_gtk_page_control_on_refresh), ui);

	gtk_box_append(GTK_BOX(box), choice);
	gtk_box_append(GTK_BOX(box), page->files);
	gtk_box_append(GTK_BOX(box), open_folder);
	gtk_box_append(GTK_BOX(box), paragraph);
	gtk_box_append(GTK_BOX(box), refresh_list);

	pspdisp_ui_on_refresh(ui, (PSPdispUIRefresh) pspdisp_gtk_page_control_refresh, page);
	return pspdisp_ui_component_groupbox_new("Edit files", box);
}

// Scancode viewer component
GtkWidget *pspdisp_gtk_page_control_scancode_new() {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

	GtkWidget *paragraph = gtk_label_new(
		"Click into the edit field and press a key to view its scancode. "
		"Some keys do not show their scancode here, please refer to the "
		"listing in the help file instead.");
	gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
	gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
	gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
	gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

	GtkWidget *scancode = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(scancode), FALSE);
	gtk_widget_set_halign(scancode, GTK_ALIGN_CENTER);

	GtkEventController *keys = gtk_event_controller_key_new();
	gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
	g_signal_connect(keys, "key-pressed", G_CALLBACK(pspdisp_gtk_page_control_on_key), scancode);
	gtk_widget_add_controller(scancode, keys);

	gtk_box_append(GTK_BOX(box), paragraph);
	gtk_box_append(GTK_BOX(box), scancode);

	return pspdisp_ui_component_groupbox_new("Scancode", box);
}

// "Control" page component
GtkWidget *pspdisp_gtk_page_control_new(PSPdispUI *ui) {
	PSPdispGtkPageControl *page = g_new0(PSPdispGtkPageControl, 1);
	GtkWidget *box;
	GtkWidget *scroller = pspdisp_ui_component_page_new(&box);

	gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_control_files_new(ui, page));
	gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_control_scancode_new());

	g_signal_connect_swapped(scroller, "destroy", G_CALLBACK(g_free), page);
	return scroller;
}
