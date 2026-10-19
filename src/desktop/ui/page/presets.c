/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * presets.c - "Presets" page of the desktop app     *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <gtk/gtk.h>
#include "../app.c"
#include "../components/groupbox.c"
#include "../components/page.c"
#include "../components/paragraph.c"

/**
 * @brief The widgets of the "Presets" page.
 */
typedef struct {
	PSPdispUI *ui; // The app
	GtkWidget *name; // The name of the preset to save
	GtkWidget *list; // The presets, after "<new preset>"
	GtkWidget *load; // Loads the preset selected
	GtkWidget *delete; // Deletes the preset selected
	char *selected; // The file name of the preset selected (`NULL` for "<new preset>"), kept when the list is filled again
} PSPdispGtkPagePresets;

/**
 * @brief Get the preset of a row of the list.
 *
 * @return The preset (`NULL` for "<new preset>").
 */
const PSPdispSettingsPreset *pspdisp_gtk_page_presets_of_row(PSPdispGtkPagePresets *page, GtkListBoxRow *row) {
	if (row == NULL) return NULL;
	int index = gtk_list_box_row_get_index(row) - 1;
	return index >= 0 && index < (int) page->ui->presets->len ? page->ui->presets->pdata[index] : NULL;
}

/**
 * @brief Show the presets, after the settings were loaded again, keeping the one selected.
 */
void pspdisp_gtk_page_presets_refresh(PSPdispUI *ui, PSPdispGtkPagePresets *page) {
	gtk_list_box_remove_all(GTK_LIST_BOX(page->list));

	GtkWidget *new_preset = gtk_label_new("<new preset>");
	gtk_label_set_xalign(GTK_LABEL(new_preset), 0.0f);
	gtk_list_box_append(GTK_LIST_BOX(page->list), new_preset);
	GtkListBoxRow *selected = gtk_list_box_get_row_at_index(GTK_LIST_BOX(page->list), 0);

	for (guint i = 0; i < ui->presets->len; i++) {
		const PSPdispSettingsPreset *preset = ui->presets->pdata[i];
		char *text = g_strdup_printf("%s (%s)", preset->name, preset->file_name);
		GtkWidget *label = gtk_label_new(text);
		gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
		gtk_list_box_append(GTK_LIST_BOX(page->list), label);
		g_free(text);

		if (g_strcmp0(page->selected, preset->file_name) == 0)
			selected = gtk_list_box_get_row_at_index(GTK_LIST_BOX(page->list), i + 1);
	}

	gtk_list_box_select_row(GTK_LIST_BOX(page->list), selected);
}

/**
 * @brief Show the name of the preset selected, and enable its buttons, as the old desktop app.
 */
void pspdisp_gtk_page_presets_on_select(GtkListBox *list, GtkListBoxRow *row, PSPdispGtkPagePresets *page) {
	(void) list;
	const PSPdispSettingsPreset *preset = pspdisp_gtk_page_presets_of_row(page, row);

	gtk_widget_set_sensitive(page->load, preset != NULL);
	gtk_widget_set_sensitive(page->delete, preset != NULL);

	if (page->ui->updating) return; // Filled again, the name typed stays
	g_free(page->selected);
	page->selected = preset != NULL ? g_strdup(preset->file_name) : NULL;
	if (page->name != NULL)
		gtk_editable_set_text(GTK_EDITABLE(page->name), preset != NULL ? preset->name : "");
}

/**
 * @brief Make a file name for a new preset from its name (e.g. "my-preset.preset"), not used by another preset.
 *
 * @return The file name, free it with `g_free()`.
 */
char *pspdisp_gtk_page_presets_file_name(PSPdispUI *ui, const char *name) {
	char *lower = g_utf8_strdown(name, -1);
	GString *base = g_string_new(NULL);

	for (const char *c = lower; *c != '\0'; c++) {
		if (g_ascii_isalnum(*c))
			g_string_append_c(base, *c);
		else if (base->len > 0 && base->str[base->len - 1] != '-')
			g_string_append_c(base, '-');
	}

	// Without the dashes at the end
	gsize length = base->len;
	while (length > 0 && base->str[length - 1] == '-')
		length--;
	g_string_truncate(base, length);
	if (base->len == 0)
		g_string_append(base, "preset");

	char *presets_dir = pspdisp_settings_presets_dir(ui->settings_dir);
	char *file_name = g_strdup_printf("%s.preset", base->str);

	for (int i = 2; ; i++) {
		char *path = g_build_filename(presets_dir, file_name, NULL);
		bool exists = g_file_test(path, G_FILE_TEST_EXISTS);
		g_free(path);
		if (!exists) break;

		g_free(file_name);
		file_name = g_strdup_printf("%s-%d.preset", base->str, i);
	}

	g_free(presets_dir);
	g_string_free(base, TRUE);
	g_free(lower);
	return file_name;
}

/**
 * @brief Save the streaming settings as the preset selected, or as a new one.
 */
void pspdisp_gtk_page_presets_on_save(GtkButton *button, PSPdispGtkPagePresets *page) {
	(void) button;
	PSPdispUI *ui = page->ui;
	const char *name = gtk_editable_get_text(GTK_EDITABLE(page->name));

	char *trimmed = g_strstrip(g_strdup(name));
	bool empty = trimmed[0] == '\0';
	g_free(trimmed);

	if (empty) {
		pspdisp_ui_error(ui, "Please enter a name for the preset", NULL);
		return;
	}

	char *file_name = page->selected != NULL ? g_strdup(page->selected) : pspdisp_gtk_page_presets_file_name(ui, name);
	GError *error = NULL;

	if (pspdisp_settings_preset_save(ui->settings_dir, file_name, name, &ui->settings.streaming, &error)) {
		g_free(page->selected);
		page->selected = file_name;
		pspdisp_ui_reload(ui);
	} else {
		pspdisp_ui_error(ui, "The preset can't be saved", error);
		g_error_free(error);
		g_free(file_name);
	}
}

/**
 * @brief Apply the preset selected: its streaming settings are saved, as when it's chosen on the PSP.
 */
void pspdisp_gtk_page_presets_on_load(GtkButton *button, PSPdispGtkPagePresets *page) {
	(void) button;
	if (page->selected == NULL) return;

	PSPdispSettingsStreaming streaming_settings = page->ui->settings.streaming;
	GError *error = NULL;

	if (pspdisp_settings_preset_load(page->ui->settings_dir, page->selected, &streaming_settings, &error)) {
		pspdisp_ui_set_streaming_settings(page->ui, &streaming_settings);
	} else {
		pspdisp_ui_error(page->ui, "The preset can't be loaded", error);
		g_error_free(error);
	}
}

void pspdisp_gtk_page_presets_on_delete(GtkButton *button, PSPdispGtkPagePresets *page) {
	(void) button;
	if (page->selected == NULL) return;

	GError *error = NULL;

	if (pspdisp_settings_preset_delete(page->ui->settings_dir, page->selected, &error)) {
		g_clear_pointer(&page->selected, g_free);
		pspdisp_ui_reload(page->ui);
	} else {
		pspdisp_ui_error(page->ui, "The preset can't be deleted", error);
		g_error_free(error);
	}
}

void pspdisp_gtk_page_presets_on_refresh(GtkButton *button, PSPdispGtkPagePresets *page) {
	(void) button;
	pspdisp_ui_reload(page->ui);
}

void pspdisp_gtk_page_presets_on_open_folder(GtkButton *button, PSPdispGtkPagePresets *page) {
	(void) button;
	char *path = pspdisp_settings_presets_dir(page->ui->settings_dir);
	pspdisp_ui_open_folder(page->ui, path);
	g_free(path);
}

// Preset files component
GtkWidget *pspdisp_gtk_page_presets_files_new(PSPdispUI *ui, PSPdispGtkPagePresets *page) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

	GtkWidget *paragraph = gtk_label_new(
		"Various capturing settings can be saved to preset files to make "
		"switching between different use cases easier. You could e.g. have "
		"a preset for desktop remote control and another one for high "
		"quality audio streaming. The PSP lists them in its \"PC Settings\" too.");
	gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
	gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
	gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
	gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

	// Left column: preset name + preset list
	page->name = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(page->name), "Please enter a name");
	gtk_entry_set_max_length(GTK_ENTRY(page->name), PSPDISP_COM_NAME_SIZE - 1);
	gtk_widget_set_hexpand(page->name, TRUE);
	g_object_add_weak_pointer(G_OBJECT(page->name), (gpointer *) &page->name); // `NULL` once destroyed: when the window closes, the list clears its selection after it

	page->list = gtk_list_box_new();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(page->list), GTK_SELECTION_SINGLE);
	g_signal_connect(page->list, "row-selected", G_CALLBACK(pspdisp_gtk_page_presets_on_select), page);

	GtkWidget *scroller = gtk_scrolled_window_new();
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), page->list);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroller), 140);
	gtk_widget_set_vexpand(scroller, TRUE);
	gtk_widget_add_css_class(scroller, "frame");

	// Right column: actions
	page->load = gtk_button_new_with_label("Load preset");
	page->delete = gtk_button_new_with_label("Delete preset");
	gtk_widget_set_sensitive(page->load, FALSE);   // enabled once a preset is selected
	gtk_widget_set_sensitive(page->delete, FALSE); // enabled once a preset is selected
	g_signal_connect(page->load, "clicked", G_CALLBACK(pspdisp_gtk_page_presets_on_load), page);
	g_signal_connect(page->delete, "clicked", G_CALLBACK(pspdisp_gtk_page_presets_on_delete), page);

	GtkWidget *refresh = gtk_button_new_with_label("Refresh list");
	GtkWidget *open_folder = gtk_button_new_with_label("Open preset folder");
	g_signal_connect(refresh, "clicked", G_CALLBACK(pspdisp_gtk_page_presets_on_refresh), page);
	g_signal_connect(open_folder, "clicked", G_CALLBACK(pspdisp_gtk_page_presets_on_open_folder), page);

	GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_box_append(GTK_BOX(actions), page->load);
	gtk_box_append(GTK_BOX(actions), refresh);
	gtk_box_append(GTK_BOX(actions), open_folder);
	gtk_box_append(GTK_BOX(actions), page->delete);

	GtkWidget *save = gtk_button_new_with_label("Save preset");
	g_signal_connect(save, "clicked", G_CALLBACK(pspdisp_gtk_page_presets_on_save), page);

	gtk_grid_attach(GTK_GRID(grid), page->name, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), save,       1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), scroller,   0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), actions,    1, 1, 1, 1);

	GtkWidget *hint = pspdisp_ui_component_paragraph_new(
		"A preset saves the image, sound and control settings of the \"Streaming\" page. "
		"To make a preset change only some settings, delete the other lines of its file (e.g. \"Control=\").");
	gtk_widget_add_css_class(hint, "dim-label");

	gtk_box_append(GTK_BOX(box), paragraph);
	gtk_box_append(GTK_BOX(box), grid);
	gtk_box_append(GTK_BOX(box), hint);

	pspdisp_ui_on_refresh(ui, (PSPdispUIRefresh) pspdisp_gtk_page_presets_refresh, page);
	return pspdisp_ui_component_groupbox_new("Preset files", box);
}

// "Presets" page component
GtkWidget *pspdisp_gtk_page_presets_new(PSPdispUI *ui) {
	PSPdispGtkPagePresets *page = g_new0(PSPdispGtkPagePresets, 1);
	page->ui = ui;

	GtkWidget *box;
	GtkWidget *scroller = pspdisp_ui_component_page_new(&box);

	gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_presets_files_new(ui, page));
	return scroller;
}
