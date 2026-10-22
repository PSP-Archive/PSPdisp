/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * app.c - Desktop app: settings and daemon status   *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The core of the desktop app: it edits the settings of PSPdisp (see `settings/settings.c`), which the daemon follows, and shows what the daemon is doing (see `status/status.c`).
 *
 * The widgets are bound to the settings: a change is saved at once, only the key it's about, as the old desktop app saved its settings as they changed.
 * The files are watched, so a change on the disk (e.g. made on the PSP, then saved by the daemon) is shown at once.
 * While the widgets show the settings, `updating` is set, so their handlers don't save what they show.
 */

#pragma once

#include <string.h>
#include <gtk/gtk.h>
#include "../settings/settings.c"
#include "../status/status.c"

#define PSPDISP_UI_RELOAD_DELAY 150 // Time to wait after a change on the disk before loading the settings, in milliseconds, so the changes coming together are loaded once
#define PSPDISP_UI_WLAN_NOTIFICATION_ID "pspdisp-wlan" // ID of the notifications of the WLAN events, a new one replaces the previous one

typedef struct PSPdispUI PSPdispUI;

/**
 * @brief Type of a function pointer called after the settings (or the status) changed, to show them.
 *
 * @param ui The app.
 * @param data The data given when it was added.
 */
typedef void (*PSPdispUIRefresh) (PSPdispUI *ui, gpointer data);

/**
 * @brief A function to call after the settings (or the status) changed, with its data.
 */
typedef struct {
	PSPdispUIRefresh refresh;
	gpointer data;
} PSPdispUIRefresher;

/**
 * @brief What kind of value a widget is bound to.
 */
typedef enum {
	PSPDISP_UI_BINDING_CHECK, // A `bool`, to a `GtkCheckButton`
	PSPDISP_UI_BINDING_SWITCH, // A `bool`, to a `GtkSwitch`
	PSPDISP_UI_BINDING_SPIN, // A `uint32_t`, to a `GtkSpinButton`
	PSPDISP_UI_BINDING_ENTRY, // A `char` array, to a `GtkEntry`
	PSPDISP_UI_BINDING_NAMES, // A `uint32_t` index, to a `GtkDropDown`, saved as a name of a list (e.g. `Medium`)
	PSPDISP_UI_BINDING_VALUES, // A `uint32_t` index, to a `GtkDropDown`, saved as a value of a list (e.g. `22050`)
} PSPdispUIBindingKind;

/**
 * @brief A widget bound to a setting.
 */
typedef struct {
	PSPdispUI *ui; // The app
	GtkWidget *widget; // The widget
	PSPdispUIBindingKind kind; // What kind of value
	const char *group; // The group of the setting in the file (e.g. `Streaming`)
	const char *key; // The key of the setting in the file (e.g. `Quality`)
	size_t offset; // Where the value is in `PSPdispSettings`
	size_t size; // The size of the value, for the strings
	const char *const *names; // The names of a list, for `PSPDISP_UI_BINDING_NAMES`
	const uint32_t *values; // The values of a list, for `PSPDISP_UI_BINDING_VALUES`
} PSPdispUIBinding;

/**
 * @brief The desktop app.
 */
struct PSPdispUI {
	GtkApplication *app; // The application
	GtkWindow *window; // The window (`NULL` until it's created)
	char *settings_dir; // The folder of the settings
	bool default_settings_dir; // If it's the default folder, so the daemon finds it by itself
	PSPdispSettings settings; // The settings, as on the disk
	GPtrArray *presets; // The presets (`PSPdispSettingsPreset *`)
	GPtrArray *controls; // The control files (`PSPdispSettingsControl *`)
	PSPdispStatusClient *status_client; // The connection to the daemon, for its status
	PSPdispStatus status; // The status of the daemon
	bool running; // If the daemon runs
	bool status_read; // If the status was taken once, so the changes are told only after
	bool updating; // Set while the widgets show the settings, so their handlers don't save them
	GPtrArray *bindings; // The widgets bound to the settings (`PSPdispUIBinding *`)
	GArray *refreshers; // Called after the settings were loaded again (`PSPdispUIRefresher`)
	GArray *status_refreshers; // Called after each status of the daemon (`PSPdispUIRefresher`)
	GPtrArray *monitors; // The monitors of the files of the settings (`GFileMonitor *`)
	guint reload_source; // The timeout of the pending reload (0 if none)
};

/**
 * @brief Tell the user an error, in a dialog over the window.
 *
 * @param ui The app.
 * @param message The message (e.g. "The settings can't be saved").
 * @param error The error (`NULL` for none).
 */
void pspdisp_ui_error(PSPdispUI *ui, const char *message, const GError *error) {
	g_warning("%s%s%s", message, error != NULL ? ": " : "", error != NULL ? error->message : "");

	if (ui->window == NULL) return;
	GtkAlertDialog *dialog = gtk_alert_dialog_new("%s", message);
	if (error != NULL)
		gtk_alert_dialog_set_detail(dialog, error->message);
	gtk_alert_dialog_show(dialog, ui->window);
	g_object_unref(dialog);
}

/**
 * @brief Save a setting, as it is in the file (e.g. `true`, `50` or `Medium`).
 *
 * @param ui The app.
 * @param group The group (e.g. `Streaming`).
 * @param key The key (e.g. `Quality`).
 * @param value The value.
 */
void pspdisp_ui_set(PSPdispUI *ui, const char *group, const char *key, const char *value) {
	GError *error = NULL;

	if (!pspdisp_settings_set(ui->settings_dir, group, key, value, &error)) {
		pspdisp_ui_error(ui, "The settings can't be saved", error);
		g_error_free(error);
	}
}

/**
 * @brief Save a change of the streaming settings, only the keys that changed (e.g. a preset applied).
 *
 * @param ui The app.
 * @param after The streaming settings after the change (before is `ui->settings.streaming`).
 */
void pspdisp_ui_set_streaming_settings(PSPdispUI *ui, const PSPdispSettingsStreaming *after) {
	GError *error = NULL;

	if (!pspdisp_settings_update_streaming(ui->settings_dir, &ui->settings.streaming, after, &error)) {
		pspdisp_ui_error(ui, "The settings can't be saved", error);
		g_error_free(error);
	}
}

/**
 * @brief Get the value of a binding in the settings.
 *
 * @param binding The binding.
 * @return A pointer to its value, in the settings of the app.
 */
void *pspdisp_ui_binding_value(PSPdispUIBinding *binding) {
	return (uint8_t *) &binding->ui->settings + binding->offset;
}

/**
 * @brief Show the setting of a binding in its widget, only if it differs (e.g. so the cursor of an entry doesn't move).
 *
 * @param binding The binding.
 */
void pspdisp_ui_binding_show(PSPdispUIBinding *binding) {
	void *value = pspdisp_ui_binding_value(binding);

	switch (binding->kind) {
		case PSPDISP_UI_BINDING_CHECK:
			if (gtk_check_button_get_active(GTK_CHECK_BUTTON(binding->widget)) != *(bool *) value)
				gtk_check_button_set_active(GTK_CHECK_BUTTON(binding->widget), *(bool *) value);
			break;
		case PSPDISP_UI_BINDING_SWITCH:
			if (gtk_switch_get_active(GTK_SWITCH(binding->widget)) != *(bool *) value)
				gtk_switch_set_active(GTK_SWITCH(binding->widget), *(bool *) value);
			break;
		case PSPDISP_UI_BINDING_SPIN:
			if (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(binding->widget)) != (int) *(uint32_t *) value)
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(binding->widget), *(uint32_t *) value);
			break;
		case PSPDISP_UI_BINDING_ENTRY:
			if (strcmp(gtk_editable_get_text(GTK_EDITABLE(binding->widget)), value) != 0)
				gtk_editable_set_text(GTK_EDITABLE(binding->widget), value);
			break;
		case PSPDISP_UI_BINDING_NAMES:
		case PSPDISP_UI_BINDING_VALUES:
			if (gtk_drop_down_get_selected(GTK_DROP_DOWN(binding->widget)) != *(uint32_t *) value)
				gtk_drop_down_set_selected(GTK_DROP_DOWN(binding->widget), *(uint32_t *) value);
			break;
	}
}

/**
 * @brief Save the setting of a binding from its widget, when the user changes it.
 *
 * @param binding The binding.
 */
void pspdisp_ui_binding_save(PSPdispUIBinding *binding) {
	if (binding->ui->updating) return;

	char text[64];
	const char *value = text;

	switch (binding->kind) {
		case PSPDISP_UI_BINDING_CHECK:
			value = gtk_check_button_get_active(GTK_CHECK_BUTTON(binding->widget)) ? "true" : "false";
			break;
		case PSPDISP_UI_BINDING_SWITCH:
			value = gtk_switch_get_active(GTK_SWITCH(binding->widget)) ? "true" : "false";
			break;
		case PSPDISP_UI_BINDING_SPIN:
			g_snprintf(text, sizeof(text), "%d", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(binding->widget)));
			break;
		case PSPDISP_UI_BINDING_ENTRY:
			value = gtk_editable_get_text(GTK_EDITABLE(binding->widget));
			break;
		case PSPDISP_UI_BINDING_NAMES:
			value = binding->names[gtk_drop_down_get_selected(GTK_DROP_DOWN(binding->widget))];
			break;
		case PSPDISP_UI_BINDING_VALUES:
			g_snprintf(text, sizeof(text), "%u", binding->values[gtk_drop_down_get_selected(GTK_DROP_DOWN(binding->widget))]);
			break;
	}

	pspdisp_ui_set(binding->ui, binding->group, binding->key, value);
}

/**
 * @brief Save the setting of a binding, called by the signals of the check buttons, the spin buttons and the entries.
 *
 * @param widget Unused, the widget of the binding.
 * @param binding The binding.
 */
void pspdisp_ui_binding_on_signal(GtkWidget *widget, PSPdispUIBinding *binding) {
	(void) widget;
	pspdisp_ui_binding_save(binding);
}

/**
 * @brief Save the setting of a binding, called by the notifications of the switches and the drop downs.
 *
 * @param widget Unused, the widget of the binding.
 * @param spec Unused, the property that changed.
 * @param binding The binding.
 */
void pspdisp_ui_binding_on_notify(GObject *widget, GParamSpec *spec, PSPdispUIBinding *binding) {
	(void) widget;
	(void) spec;
	pspdisp_ui_binding_save(binding);
}

/**
 * @brief Bind a widget to a setting: it shows the setting, and saves it when the user changes it.
 *
 * @param ui The app.
 * @param widget The widget, of the kind of the binding.
 * @param kind What kind of value.
 * @param group The group of the setting in the file.
 * @param key The key of the setting in the file.
 * @param offset Where the value is in `PSPdispSettings` (e.g. `offsetof(PSPdispSettings, enabled)`).
 * @param list The names or the values of a list, for the drop downs (`NULL` otherwise).
 * @return The widget.
 */
GtkWidget *pspdisp_ui_bind(PSPdispUI *ui, GtkWidget *widget, PSPdispUIBindingKind kind, const char *group, const char *key, size_t offset, const void *list) {
	PSPdispUIBinding *binding = g_new0(PSPdispUIBinding, 1);
	binding->ui = ui;
	binding->widget = widget;
	binding->kind = kind;
	binding->group = group;
	binding->key = key;
	binding->offset = offset;
	binding->names = list;
	binding->values = list;
	g_ptr_array_add(ui->bindings, binding);

	ui->updating = true;
	pspdisp_ui_binding_show(binding);
	ui->updating = false;

	switch (kind) {
		case PSPDISP_UI_BINDING_CHECK: g_signal_connect(widget, "toggled", G_CALLBACK(pspdisp_ui_binding_on_signal), binding); break;
		case PSPDISP_UI_BINDING_SWITCH: g_signal_connect(widget, "notify::active", G_CALLBACK(pspdisp_ui_binding_on_notify), binding); break;
		case PSPDISP_UI_BINDING_SPIN: g_signal_connect(widget, "value-changed", G_CALLBACK(pspdisp_ui_binding_on_signal), binding); break;
		case PSPDISP_UI_BINDING_ENTRY: g_signal_connect(widget, "changed", G_CALLBACK(pspdisp_ui_binding_on_signal), binding); break;
		case PSPDISP_UI_BINDING_NAMES:
		case PSPDISP_UI_BINDING_VALUES: g_signal_connect(widget, "notify::selected", G_CALLBACK(pspdisp_ui_binding_on_notify), binding); break;
	}

	return widget;
}

/**
 * @brief Add a function called after the settings were loaded again, to show them in widgets that are not bound.
 *
 * It's called with `updating` set.
 *
 * @param ui The app.
 * @param refresh The function, also called now.
 * @param data Given to the function.
 */
void pspdisp_ui_on_refresh(PSPdispUI *ui, PSPdispUIRefresh refresh, gpointer data) {
	PSPdispUIRefresher refresher = { refresh, data };
	g_array_append_val(ui->refreshers, refresher);

	ui->updating = true;
	refresh(ui, data);
	ui->updating = false;
}

/**
 * @brief Add a function called after each status of the daemon (about every second while it runs, and when it starts or stops).
 *
 * @param ui The app.
 * @param refresh The function, also called now.
 * @param data Given to the function.
 */
void pspdisp_ui_on_status(PSPdispUI *ui, PSPdispUIRefresh refresh, gpointer data) {
	PSPdispUIRefresher refresher = { refresh, data };
	g_array_append_val(ui->status_refreshers, refresher);
	refresh(ui, data);
}

/**
 * @brief Load the settings, the presets and the control files again, and show them.
 *
 * @param ui The app.
 */
void pspdisp_ui_reload(PSPdispUI *ui) {
	GError *error = NULL;

	if (!pspdisp_settings_load(ui->settings_dir, &ui->settings, &error)) {
		g_warning("%s, the defaults are shown", error->message);
		g_error_free(error);
	}

	g_ptr_array_unref(ui->presets);
	g_ptr_array_unref(ui->controls);
	ui->presets = pspdisp_settings_presets(ui->settings_dir);
	ui->controls = pspdisp_settings_controls(ui->settings_dir);

	ui->updating = true;
	for (guint i = 0; i < ui->bindings->len; i++)
		pspdisp_ui_binding_show(ui->bindings->pdata[i]);
	for (guint i = 0; i < ui->refreshers->len; i++) {
		PSPdispUIRefresher *refresher = &g_array_index(ui->refreshers, PSPdispUIRefresher, i);
		refresher->refresh(ui, refresher->data);
	}
	ui->updating = false;
}

/**
 * @brief Load the settings again after a change on the disk, the timeout planned by `pspdisp_ui_on_file_change()`.
 *
 * @param ui The app.
 * @return Always `G_SOURCE_REMOVE`, it's planned again with the next change.
 */
gboolean pspdisp_ui_on_reload(PSPdispUI *ui) {
	ui->reload_source = 0;
	pspdisp_ui_reload(ui);
	return G_SOURCE_REMOVE;
}

/**
 * @brief Plan a reload of the settings, called by the monitors when a file changes.
 *
 * @param monitor Unused, the monitor.
 * @param file Unused, the file that changed.
 * @param other_file Unused.
 * @param event What changed.
 * @param ui The app.
 */
void pspdisp_ui_on_file_change(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event, PSPdispUI *ui) {
	(void) monitor;
	(void) file;
	(void) other_file;

	// Only the end of the changes (not each write), and the files created or deleted (e.g. a file written then renamed over the old one)
	if (event != G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT && event != G_FILE_MONITOR_EVENT_CREATED && event != G_FILE_MONITOR_EVENT_DELETED) return;
	if (ui->reload_source == 0)
		ui->reload_source = g_timeout_add(PSPDISP_UI_RELOAD_DELAY, (GSourceFunc) pspdisp_ui_on_reload, ui);
}

/**
 * @brief Watch a file or a folder of the settings.
 *
 * @param ui The app.
 * @param path The file or folder.
 * @param directory If it's a folder.
 */
void pspdisp_ui_watch(PSPdispUI *ui, const char *path, bool directory) {
	GFile *file = g_file_new_for_path(path);
	GFileMonitor *monitor = directory ? g_file_monitor_directory(file, G_FILE_MONITOR_NONE, NULL, NULL) : g_file_monitor_file(file, G_FILE_MONITOR_NONE, NULL, NULL);

	if (monitor != NULL) {
		g_signal_connect(monitor, "changed", G_CALLBACK(pspdisp_ui_on_file_change), ui);
		g_ptr_array_add(ui->monitors, monitor);
	}
	g_object_unref(file);
}

/**
 * @brief Send a notification of a WLAN event, as the balloons of the old desktop app.
 *
 * @param ui The app.
 * @param title The title of the notification.
 * @param body The text of the notification.
 */
void pspdisp_ui_notify_wlan_event(PSPdispUI *ui, const char *title, const char *body) {
	GNotification *notification = g_notification_new(title);
	g_notification_set_body(notification, body);
	g_application_send_notification(G_APPLICATION(ui->app), PSPDISP_UI_WLAN_NOTIFICATION_ID, notification);
	g_object_unref(notification);
}

/**
 * @brief Take a status of the daemon, tell its WLAN events, and show it, called about every second while the daemon runs, and when it starts or stops.
 *
 * When the PSP asks to open the menu of PSPdisp (the `OpenMenu` action of a control file), the window is shown.
 *
 * @param status The status, `NULL` when the daemon doesn't run.
 * @param ui The app.
 */
void pspdisp_ui_on_daemon_status(const PSPdispStatus *status, PSPdispUI *ui) {
	PSPdispStatus previous = ui->status;
	bool was_running = ui->running;

	ui->running = status != NULL;
	ui->status = status != NULL ? *status : (PSPdispStatus) {};

	if (ui->status_read) {
		bool wlan_started = ui->running && ui->status.wlan && !(was_running && previous.wlan);
		bool connected = ui->running && ui->status.connected && ui->status.connected_wlan && !(was_running && previous.connected && previous.connected_wlan);
		bool disconnected = was_running && previous.connected && previous.connected_wlan && !(ui->running && ui->status.connected && ui->status.connected_wlan);

		if (wlan_started && ui->settings.notify_server_started) {
			char *body = g_strdup_printf("Listening on port %u, set this computer as the host on the PSP.", ui->status.wlan_port);
			pspdisp_ui_notify_wlan_event(ui, "PSPdisp WLAN server started", body);
			g_free(body);
		}
		if (connected && ui->settings.notify_connected)
			pspdisp_ui_notify_wlan_event(ui, "PSP connected", ui->status.connected_via);
		if (disconnected && ui->settings.notify_disconnected)
			pspdisp_ui_notify_wlan_event(ui, "PSP disconnected", "The connection to the PSP over WLAN ended.");

		if (ui->running && ui->status.open_menu_requests > previous.open_menu_requests && ui->window != NULL)
			gtk_window_present(ui->window);
	}
	ui->status_read = true;

	for (guint i = 0; i < ui->status_refreshers->len; i++) {
		PSPdispUIRefresher *refresher = &g_array_index(ui->status_refreshers, PSPdispUIRefresher, i);
		refresher->refresh(ui, refresher->data);
	}
}

/**
 * @brief Open a folder in the file manager (e.g. the control files), creating it if needed.
 *
 * @param ui The app.
 * @param path The folder.
 */
void pspdisp_ui_open_folder(PSPdispUI *ui, const char *path) {
	pspdisp_settings_make_dir(path, NULL);

	GFile *file = g_file_new_for_path(path);
	GtkFileLauncher *launcher = gtk_file_launcher_new(file);
	gtk_file_launcher_launch(launcher, ui->window, NULL, NULL, NULL);
	g_object_unref(launcher);
	g_object_unref(file);
}

/**
 * @brief Run a program, without waiting for it (e.g. the sound mixer).
 *
 * @param ui The app.
 * @param argv The program and its arguments, the program is found in the `PATH`.
 * @return `true` if it was started.
 */
bool pspdisp_ui_spawn(PSPdispUI *ui, const char *const *argv) {
	GError *error = NULL;

	if (!g_spawn_async(NULL, (char **) argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
		char *message = g_strdup_printf("%s can't be started", argv[0]);
		pspdisp_ui_error(ui, message, error);
		g_free(message);
		g_error_free(error);
		return false;
	}
	return true;
}

/**
 * @brief Start the app: load the settings (installing the defaults the first time), watch them, and read the status of the daemon.
 *
 * @param ui The app.
 * @param app The application.
 * @param settings_dir The folder of the settings, given on the command line (`NULL` for the default one).
 */
void pspdisp_ui_init(PSPdispUI *ui, GtkApplication *app, const char *settings_dir) {
	*ui = (PSPdispUI) {
		.app = app,
		.settings_dir = pspdisp_settings_dir_resolve(settings_dir),
		.default_settings_dir = settings_dir == NULL,
		.presets = g_ptr_array_new(),
		.controls = g_ptr_array_new(),
		.bindings = g_ptr_array_new_with_free_func(g_free),
		.refreshers = g_array_new(FALSE, FALSE, sizeof(PSPdispUIRefresher)),
		.status_refreshers = g_array_new(FALSE, FALSE, sizeof(PSPdispUIRefresher)),
		.monitors = g_ptr_array_new_with_free_func(g_object_unref),
	};

	pspdisp_settings_install_defaults(ui->settings_dir);
	pspdisp_ui_reload(ui);

	char *settings_path = pspdisp_settings_file_path(ui->settings_dir);
	char *presets_dir = pspdisp_settings_presets_dir(ui->settings_dir);
	char *control_dir = pspdisp_settings_control_dir(ui->settings_dir);
	pspdisp_ui_watch(ui, settings_path, false);
	pspdisp_ui_watch(ui, presets_dir, true);
	pspdisp_ui_watch(ui, control_dir, true);
	g_free(control_dir);
	g_free(presets_dir);
	g_free(settings_path);

	ui->status_client = pspdisp_status_client_new((PSPdispStatusHandler) pspdisp_ui_on_daemon_status, ui);
}

/**
 * @brief Make a text of a size in bytes, as the old desktop app (e.g. "21 MiB" or "3 KiB").
 *
 * @param bytes The size.
 * @return The text, free it with `g_free()`.
 */
char *pspdisp_ui_size_text(uint64_t bytes) {
	const uint64_t mebibyte = 1024 * 1024;
	if (bytes >= mebibyte)
		return g_strdup_printf("%" G_GUINT64_FORMAT " MiB", bytes / mebibyte);
	return g_strdup_printf("%" G_GUINT64_FORMAT " KiB", bytes / 1024);
}

/**
 * @brief Find the daemon where the system looks for the programs (see `g_find_program_in_path()`).
 *
 * @return Its path (its name if it's not found, so starting it tells why), free it with `g_free()`.
 */
char *pspdisp_ui_daemon_path() {
	char *found = g_find_program_in_path("pspdisp-daemon");
	return found != NULL ? found : g_strdup("pspdisp-daemon");
}

/**
 * @brief Make the command that starts the daemon, for the messages (with the folder of the settings when it's not the default one).
 *
 * @param ui The app.
 * @return The command, free it with `g_free()`.
 */
char *pspdisp_ui_daemon_command(PSPdispUI *ui) {
	char *path = pspdisp_ui_daemon_path();
	char *command = ui->default_settings_dir ? g_strdup(path) : g_strdup_printf("%s --config-dir \"%s\"", path, ui->settings_dir);
	g_free(path);
	return command;
}

/**
 * @brief Start the daemon, with the folder of the settings of the app.
 *
 * @param ui The app.
 */
void pspdisp_ui_start_daemon(PSPdispUI *ui) {
	char *path = pspdisp_ui_daemon_path();
	const char *argv[] = { path, "--config-dir", ui->settings_dir, NULL };
	pspdisp_ui_spawn(ui, argv);
	g_free(path);
}

/**
 * @brief Stop the daemon.
 *
 * @param ui The app.
 */
void pspdisp_ui_stop_daemon(PSPdispUI *ui) {
	if (!ui->running) return;

	GError *error = NULL;
	if (!pspdisp_status_client_stop(ui->status_client, &error)) {
		pspdisp_ui_error(ui, "The daemon can't be stopped", error);
		g_error_free(error);
	}
}
