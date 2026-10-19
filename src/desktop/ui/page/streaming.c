/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * streaming.c - "Streaming" page of the desktop app *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The "Streaming" page: what the daemon is doing, and the options of the tray menu of the old desktop app (enabled, connection, quality, update interval, sound, control and presets).
 * The streaming options are the ones the PSP shows on its "PC Settings" menu, so a change on one side shows on the other.
 */

#pragma once

#include <gtk/gtk.h>
#include "../app.c"
#include "../components/drop_down.c"
#include "../components/field.c"
#include "../components/grid.c"
#include "../components/groupbox.c"
#include "../components/page.c"
#include "../components/paragraph.c"

/**
 * @brief The widgets of the page that aren't bound to a setting.
 */
typedef struct {
	GtkWidget *state; // What the daemon is doing
	GtkWidget *detail; // What goes wrong (e.g. the WLAN port is used), hidden when nothing does
	GtkWidget *start; // Starts the daemon
	GtkWidget *stop; // Stops the daemon
	GtkWidget *quality; // The quality levels, with their JPEG quality
	GtkWidget *update_interval; // The update intervals, with their pause
	GtkWidget *control; // The controls: the gamepad, none, then the control files
	GtkStringList *controls; // The names of `control`
	GtkWidget *preset; // The presets
	GtkStringList *presets; // The names of `preset`
} PSPdispGtkPageStreaming;

const char *const pspdisp_gtk_page_streaming_sample_rates[] = { "11025 Hz (about 10 frames per second)", "22050 Hz (about 20 frames per second)", "44100 Hz (about 33 frames per second)", NULL };

/**
 * @brief Show the settings that need more than a binding: the values of the quality levels and the update intervals, and the lists of controls and presets.
 */
void pspdisp_gtk_page_streaming_refresh(PSPdispUI *ui, PSPdispGtkPageStreaming *page) {
	const PSPdispSettings *settings = &ui->settings;
	char *items[16] = {};

	for (size_t i = 0; i <= PSPDISP_COM_QUALITY_HIGHEST; i++)
		items[i] = g_strdup_printf("%s (JPEG quality %u)", pspdisp_settings_quality_names[i], settings->jpeg_quality[i]);
	pspdisp_ui_component_drop_down_set_items(GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(page->quality))), page->quality, (const char *const *) items, settings->streaming.quality);
	for (size_t i = 0; items[i] != NULL; i++)
		g_clear_pointer(&items[i], g_free);

	items[0] = g_strdup("Continuous (as fast as the PSP takes the frames)");
	for (size_t i = PSPDISP_COM_UPDATE_INTERVAL_SHORTEST; i <= PSPDISP_COM_UPDATE_INTERVAL_LONGEST; i++)
		items[i] = g_strdup_printf("%s (at least %u ms per frame)", pspdisp_settings_update_interval_names[i], settings->update_interval[i]);
	pspdisp_ui_component_drop_down_set_items(GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(page->update_interval))), page->update_interval, (const char *const *) items, settings->streaming.update_interval);
	for (size_t i = 0; items[i] != NULL; i++)
		g_clear_pointer(&items[i], g_free);

	// The controls as the PSP lists them, with the one chosen added if its control file is gone
	GPtrArray *controls = g_ptr_array_new();
	guint selected = G_MAXUINT;

	g_ptr_array_add(controls, PSPDISP_SETTINGS_CONTROL_GAMEPAD);
	g_ptr_array_add(controls, PSPDISP_SETTINGS_CONTROL_NONE);
	for (guint i = 0; i < ui->controls->len; i++)
		g_ptr_array_add(controls, ((PSPdispSettingsControl *) ui->controls->pdata[i])->name);
	for (guint i = 0; i < controls->len && selected == G_MAXUINT; i++) {
		if (strcmp(controls->pdata[i], settings->streaming.control) == 0)
			selected = i;
	}
	if (selected == G_MAXUINT) {
		selected = controls->len;
		g_ptr_array_add(controls, (gpointer) settings->streaming.control);
	}
	g_ptr_array_add(controls, NULL);
	pspdisp_ui_component_drop_down_set_items(page->controls, page->control, (const char *const *) controls->pdata, selected);
	g_ptr_array_unref(controls);

	GPtrArray *presets = g_ptr_array_new();
	for (guint i = 0; i < ui->presets->len; i++)
		g_ptr_array_add(presets, ((PSPdispSettingsPreset *) ui->presets->pdata[i])->name);
	g_ptr_array_add(presets, NULL);
	guint preset = gtk_drop_down_get_selected(GTK_DROP_DOWN(page->preset));
	pspdisp_ui_component_drop_down_set_items(page->presets, page->preset, (const char *const *) presets->pdata, preset < ui->presets->len ? preset : 0);
	g_ptr_array_unref(presets);
}

/**
 * @brief Show what the daemon is doing, called about every second.
 */
void pspdisp_gtk_page_streaming_status(PSPdispUI *ui, PSPdispGtkPageStreaming *page) {
	const PSPdispStatus *status = &ui->status;
	GString *detail = g_string_new(NULL);
	char *state;

	if (!ui->running) {
		char *command = pspdisp_ui_daemon_command(ui);
		state = g_strdup("<b>The daemon isn't running</b>, so no PSP is served.");
		g_string_append_printf(detail, "Start it with the button below, or in a terminal: %s", command);
		g_free(command);
	} else if (status->connected) {
		GDateTime *since = g_date_time_new_from_unix_local(status->connected_since);
		char *time = g_date_time_format(since, "%X");
		state = g_markup_printf_escaped("<b>PSP connected</b> over %s, since %s, at %u frames per second.", status->connected_via, time, status->frames_per_second);
		g_free(time);
		g_date_time_unref(since);
	} else if (!status->enabled) {
		state = g_strdup("<b>Not enabled</b>: the daemon runs, but serves no PSP.");
	} else {
		GString *ways = g_string_new(NULL);
		if (status->usb)
			g_string_append(ways, "plugged in USB mode");
		if (status->wlan)
			g_string_append_printf(ways, "%sconnecting in WLAN mode (port %u)", ways->len > 0 ? ", or " : "", status->wlan_port);
		state = ways->len > 0 ? g_markup_printf_escaped("<b>Waiting for a PSP</b>, %s.", ways->str) : g_strdup("<b>Waiting</b>, but neither USB nor WLAN is enabled.");
		g_string_free(ways, TRUE);
	}

	if (ui->running && status->wlan_error[0] != '\0')
		g_string_append_printf(detail, "%sThe WLAN server can't start: %s.", detail->len > 0 ? "\n" : "", status->wlan_error);
	if (ui->running && status->control_error[0] != '\0')
		g_string_append_printf(detail, "%sThe control \"%s\" can't be used: %s.", detail->len > 0 ? "\n" : "", status->control, status->control_error);
	if (ui->running && status->connected && !status->display)
		g_string_append_printf(detail, "%sThe virtual display can't be created (no free EVDI device, see the README), the PSP gets a black screen.", detail->len > 0 ? "\n" : "");
	if (ui->running && status->connected && ui->settings.streaming.sound && !status->sound)
		g_string_append_printf(detail, "%sThe sound can't be captured, the PSP gets silence.", detail->len > 0 ? "\n" : "");

	gtk_label_set_markup(GTK_LABEL(page->state), state);
	gtk_label_set_text(GTK_LABEL(page->detail), detail->str);
	gtk_widget_set_visible(page->detail, detail->len > 0);
	gtk_widget_set_visible(page->start, !ui->running);
	gtk_widget_set_visible(page->stop, ui->running);

	g_string_free(detail, TRUE);
	g_free(state);
}

/**
 * @brief Save the control chosen.
 */
void pspdisp_gtk_page_streaming_on_control(GtkDropDown *drop_down, GParamSpec *spec, PSPdispUI *ui) {
	(void) spec;
	if (ui->updating) return;

	GtkStringObject *item = gtk_drop_down_get_selected_item(drop_down);
	if (item != NULL)
		pspdisp_ui_set(ui, "Streaming", "Control", gtk_string_object_get_string(item));
}

/**
 * @brief Apply the preset chosen: its streaming settings are saved, as when it's chosen on the PSP.
 */
void pspdisp_gtk_page_streaming_on_apply_preset(GtkButton *button, PSPdispUI *ui) {
	PSPdispGtkPageStreaming *page = g_object_get_data(G_OBJECT(button), "page");
	guint index = gtk_drop_down_get_selected(GTK_DROP_DOWN(page->preset));
	if (index >= ui->presets->len) return;

	PSPdispSettingsStreaming streaming_settings = ui->settings.streaming;
	GError *error = NULL;

	if (pspdisp_settings_preset_load(ui->settings_dir, ((PSPdispSettingsPreset *) ui->presets->pdata[index])->file_name, &streaming_settings, &error)) {
		pspdisp_ui_set_streaming_settings(ui, &streaming_settings);
	} else {
		pspdisp_ui_error(ui, "The preset can't be applied", error);
		g_error_free(error);
	}
}

void pspdisp_gtk_page_streaming_on_start(GtkButton *button, PSPdispUI *ui) {
	(void) button;
	pspdisp_ui_start_daemon(ui);
}

void pspdisp_gtk_page_streaming_on_stop(GtkButton *button, PSPdispUI *ui) {
	(void) button;
	pspdisp_ui_stop_daemon(ui);
}

// "Streaming" page component
GtkWidget *pspdisp_gtk_page_streaming_new(PSPdispUI *ui) {
	PSPdispGtkPageStreaming *page = g_new0(PSPdispGtkPageStreaming, 1);
	GtkWidget *box;
	GtkWidget *scroller = pspdisp_ui_component_page_new(&box);

	//////////////////////////////////////////////////////////////////
	// The daemon
	GtkWidget *daemon = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	page->state = pspdisp_ui_component_paragraph_new("");
	page->detail = pspdisp_ui_component_paragraph_new("");
	gtk_widget_add_css_class(page->detail, "dim-label");
	gtk_label_set_selectable(GTK_LABEL(page->detail), TRUE);
	page->start = gtk_button_new_with_label("Start the daemon");
	page->stop = gtk_button_new_with_label("Stop the daemon");
	gtk_widget_set_halign(page->start, GTK_ALIGN_START);
	gtk_widget_set_halign(page->stop, GTK_ALIGN_START);
	g_signal_connect(page->start, "clicked", G_CALLBACK(pspdisp_gtk_page_streaming_on_start), ui);
	g_signal_connect(page->stop, "clicked", G_CALLBACK(pspdisp_gtk_page_streaming_on_stop), ui);

	GtkWidget *enabled = pspdisp_ui_bind(ui, gtk_switch_new(), PSPDISP_UI_BINDING_SWITCH, "General", "Enabled", offsetof(PSPdispSettings, enabled), NULL);
	gtk_widget_set_valign(enabled, GTK_ALIGN_CENTER);
	GtkWidget *enabled_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_box_append(GTK_BOX(enabled_row), enabled);
	gtk_box_append(GTK_BOX(enabled_row), pspdisp_ui_component_field_new("Enabled: serve the PSPs"));

	GtkWidget *connection = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 24);
	gtk_box_append(GTK_BOX(connection), pspdisp_ui_bind(ui, gtk_check_button_new_with_label("USB"), PSPDISP_UI_BINDING_CHECK, "General", "USB", offsetof(PSPdispSettings, use_usb), NULL));
	gtk_box_append(GTK_BOX(connection), pspdisp_ui_bind(ui, gtk_check_button_new_with_label("WLAN"), PSPDISP_UI_BINDING_CHECK, "General", "WLAN", offsetof(PSPdispSettings, use_wlan), NULL));

	gtk_box_append(GTK_BOX(daemon), page->state);
	gtk_box_append(GTK_BOX(daemon), page->detail);
	gtk_box_append(GTK_BOX(daemon), page->start);
	gtk_box_append(GTK_BOX(daemon), page->stop);
	gtk_box_append(GTK_BOX(daemon), enabled_row);
	gtk_box_append(GTK_BOX(daemon), connection);
	gtk_box_append(GTK_BOX(daemon), pspdisp_ui_bind(ui, gtk_check_button_new_with_label("Higher process priority (may reduce stuttering)"), PSPDISP_UI_BINDING_CHECK, "General", "HigherPriority", offsetof(PSPdispSettings, higher_priority), NULL));
	gtk_box_append(GTK_BOX(box), pspdisp_ui_component_groupbox_new("PSPdisp", daemon));
	//////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////
	// The image
	GtkWidget *image = pspdisp_ui_component_grid_new();
	page->quality = gtk_drop_down_new(G_LIST_MODEL(gtk_string_list_new(NULL)), NULL);
	page->update_interval = gtk_drop_down_new(G_LIST_MODEL(gtk_string_list_new(NULL)), NULL);

	gtk_grid_attach(GTK_GRID(image), pspdisp_ui_bind(ui, gtk_check_button_new_with_label("Stream the display (turn it off for a better sound over WLAN)"), PSPDISP_UI_BINDING_CHECK, "Streaming", "Image", offsetof(PSPdispSettings, streaming.image), NULL), 0, 0, 2, 1);
	pspdisp_ui_component_grid_add_row(image, 1, "Quality", page->quality);
	gtk_grid_attach(GTK_GRID(image), pspdisp_ui_bind(ui, gtk_check_button_new_with_label("Uncompressed (less CPU, but USB only gets about 30 frames per second)"), PSPDISP_UI_BINDING_CHECK, "Streaming", "Uncompressed", offsetof(PSPdispSettings, streaming.uncompressed), NULL), 0, 2, 2, 1);
	pspdisp_ui_component_grid_add_row(image, 3, "Update interval", page->update_interval);
	gtk_box_append(GTK_BOX(box), pspdisp_ui_component_groupbox_new("Image", image));
	//////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////
	// The sound
	GtkWidget *sound = pspdisp_ui_component_grid_new();
	gtk_grid_attach(GTK_GRID(sound), pspdisp_ui_bind(ui, gtk_check_button_new_with_label("Stream the sound (the frames then follow its pace)"), PSPDISP_UI_BINDING_CHECK, "Streaming", "Sound", offsetof(PSPdispSettings, streaming.sound), NULL), 0, 0, 2, 1);
	pspdisp_ui_component_grid_add_row(sound, 1, "Sample rate over USB", pspdisp_ui_bind(ui, gtk_drop_down_new_from_strings(pspdisp_gtk_page_streaming_sample_rates), PSPDISP_UI_BINDING_VALUES, "Streaming", "SampleRateUSB", offsetof(PSPdispSettings, streaming.sample_rate_usb), pspdisp_settings_sample_rates));
	pspdisp_ui_component_grid_add_row(sound, 2, "Sample rate over WLAN", pspdisp_ui_bind(ui, gtk_drop_down_new_from_strings(pspdisp_gtk_page_streaming_sample_rates), PSPDISP_UI_BINDING_VALUES, "Streaming", "SampleRateWLAN", offsetof(PSPdispSettings, streaming.sample_rate_wlan), pspdisp_settings_sample_rates));
	gtk_grid_attach(GTK_GRID(sound), pspdisp_ui_component_paragraph_new("While a PSP gets the sound, it appears as a sound output named after it (e.g. \"PSP-3001\"): choose it in the sound settings to play on the PSP."), 0, 3, 2, 1);
	gtk_box_append(GTK_BOX(box), pspdisp_ui_component_groupbox_new("Sound", sound));
	//////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////
	// The control and the presets
	GtkWidget *misc = pspdisp_ui_component_grid_new();
	page->controls = gtk_string_list_new(NULL);
	page->control = gtk_drop_down_new(G_LIST_MODEL(page->controls), NULL);
	g_signal_connect(page->control, "notify::selected", G_CALLBACK(pspdisp_gtk_page_streaming_on_control), ui);
	pspdisp_ui_component_grid_add_row(misc, 0, "Control", page->control);

	page->presets = gtk_string_list_new(NULL);
	page->preset = gtk_drop_down_new(G_LIST_MODEL(page->presets), NULL);
	GtkWidget *apply = gtk_button_new_with_label("Apply");
	g_object_set_data(G_OBJECT(apply), "page", page);
	g_signal_connect(apply, "clicked", G_CALLBACK(pspdisp_gtk_page_streaming_on_apply_preset), ui);
	GtkWidget *preset_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_set_hexpand(page->preset, TRUE);
	gtk_box_append(GTK_BOX(preset_row), page->preset);
	gtk_box_append(GTK_BOX(preset_row), apply);
	pspdisp_ui_component_grid_add_row(misc, 1, "Preset", preset_row);

	gtk_box_append(GTK_BOX(box), pspdisp_ui_component_groupbox_new("Control and presets", misc));
	//////////////////////////////////////////////////////////////////

	g_signal_connect_swapped(scroller, "destroy", G_CALLBACK(g_free), page);
	pspdisp_ui_on_refresh(ui, (PSPdispUIRefresh) pspdisp_gtk_page_streaming_refresh, page);
	pspdisp_ui_on_status(ui, (PSPdispUIRefresh) pspdisp_gtk_page_streaming_status, page);

	// The quality and the update interval are bound once their items exist
	pspdisp_ui_bind(ui, page->quality, PSPDISP_UI_BINDING_NAMES, "Streaming", "Quality", offsetof(PSPdispSettings, streaming.quality), pspdisp_settings_quality_names);
	pspdisp_ui_bind(ui, page->update_interval, PSPDISP_UI_BINDING_NAMES, "Streaming", "UpdateInterval", offsetof(PSPdispSettings, streaming.update_interval), pspdisp_settings_update_interval_names);
	return scroller;
}
