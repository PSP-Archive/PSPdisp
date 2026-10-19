/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * snapshot.c - Settings the daemon streams with     *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The settings the daemon streams with, from the folder of the settings (see `settings/settings.c`), reloaded when they change on the disk.
 *
 * The main thread watches the files, and loads them into a snapshot, which never changes once published.
 * The other threads take the latest snapshot when they need it (see `pspdisp_daemon_snapshot_store_get()`), e.g. the session before each frame.
 * A snapshot holds the settings, and the presets and the control files, as the PSP lists them.
 */

#pragma once

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include "../../shared.h"
#include "../settings/settings.c"

#define PSPDISP_DAEMON_SNAPSHOT_DELAY 200 // Time to wait after a change on the disk before loading the settings, in milliseconds, so the changes coming together are loaded once
#define PSPDISP_DAEMON_SNAPSHOT_FIXED_CONTROLS 2 // Controls listed before the control files: the gamepad and none

/**
 * @brief The settings at a point in time, never changed once published.
 */
typedef struct {
	gatomicrefcount refs; // References, it's freed with the last one
	uint64_t generation; // Increments with each snapshot published
	uint64_t controls_generation; // Increments when the control files change on the disk, so the control file in use is loaded again
	gint64 loaded_at; // When it began to be read from the disk, in microseconds (monotonic), so a snapshot read before a save isn't taken as newer than it
	PSPdispSettings settings; // The settings
	GPtrArray *presets; // The presets (`PSPdispSettingsPreset *`), at most `PSPDISP_COM_MAX_PRESETS`, as the PSP lists them
	GPtrArray *controls; // The control files (`PSPdispSettingsControl *`), as the PSP lists them after the gamepad and none
} PSPdispDaemonSnapshot;

/**
 * @brief Type of a function pointer called on the main thread when a new snapshot is published.
 *
 * @param snapshot The new snapshot.
 * @param data The data given with the function.
 */
typedef void (*PSPdispDaemonSnapshotHandler) (PSPdispDaemonSnapshot *snapshot, gpointer data);

/**
 * @brief The snapshots of a folder of the settings: the latest one, loaded again when the files change on the disk.
 */
typedef struct {
	char *dir; // The folder of the settings
	GMutex mutex; // Guards `current`
	PSPdispDaemonSnapshot *current; // The latest snapshot
	PSPdispDaemonSnapshotHandler handler; // Called when a snapshot is published, after the first one
	gpointer handler_data; // Given to `handler`
	GPtrArray *monitors; // The monitors of the files (`GFileMonitor *`), only used on the main thread like the fields below
	GFileMonitor *control_monitor; // The monitor of the folder of the control files, one of `monitors` (`NULL` if it can't be watched)
	guint reload; // The timeout of the pending reload (0 if none)
	bool controls_changed; // If the control files changed since the last reload
} PSPdispDaemonSnapshotStore;

/**
 * @brief Release a reference to a snapshot, from any thread.
 *
 * @param snapshot The snapshot.
 */
void pspdisp_daemon_snapshot_unref(PSPdispDaemonSnapshot *snapshot) {
	if (!g_atomic_ref_count_dec(&snapshot->refs)) return;

	g_ptr_array_unref(snapshot->presets);
	g_ptr_array_unref(snapshot->controls);
	g_free(snapshot);
}

/**
 * @brief Tell if two lists of presets or of control files have the same names and files.
 *
 * @param a A list (`PSPdispSettingsPreset *` or `PSPdispSettingsControl *`).
 * @param b The other list.
 * @return `true` if they're the same.
 */
bool pspdisp_daemon_snapshot_lists_equal(GPtrArray *a, GPtrArray *b) {
	if (a->len != b->len) return false;

	for (guint i = 0; i < a->len; i++) {
		const PSPdispSettingsPreset *item_a = a->pdata[i];
		const PSPdispSettingsPreset *item_b = b->pdata[i];
		if (strcmp(item_a->name, item_b->name) != 0 || strcmp(item_a->file_name, item_b->file_name) != 0) return false;
	}
	return true;
}

/**
 * @brief Find the index of a control as the PSP lists it: the gamepad, none, then the control files.
 *
 * @param snapshot The snapshot.
 * @param name The name of the control.
 * @param path Where to write the path of the control file (`NULL` for the gamepad and none, or to ignore it).
 * @return The index, or -1 if there's no such control.
 */
int pspdisp_daemon_snapshot_find_control(PSPdispDaemonSnapshot *snapshot, const char *name, const char **path) {
	if (path != NULL)
		*path = NULL;
	if (strcmp(name, PSPDISP_SETTINGS_CONTROL_GAMEPAD) == 0) return 0;
	if (strcmp(name, PSPDISP_SETTINGS_CONTROL_NONE) == 0) return 1;

	for (guint i = 0; i < snapshot->controls->len; i++) {
		const PSPdispSettingsControl *control = snapshot->controls->pdata[i];

		if (strcmp(control->name, name) == 0) {
			if (path != NULL)
				*path = control->path;
			return PSPDISP_DAEMON_SNAPSHOT_FIXED_CONTROLS + i;
		}
	}
	return -1;
}

/**
 * @brief Get the name of a control from its index, as the PSP lists them.
 *
 * @param snapshot The snapshot.
 * @param index The index.
 * @return The name, or `NULL` if there's no such control.
 */
const char *pspdisp_daemon_snapshot_control_name(PSPdispDaemonSnapshot *snapshot, uint32_t index) {
	if (index == 0) return PSPDISP_SETTINGS_CONTROL_GAMEPAD;
	if (index == 1) return PSPDISP_SETTINGS_CONTROL_NONE;

	const uint32_t file_index = index - PSPDISP_DAEMON_SNAPSHOT_FIXED_CONTROLS; // Index in the control files
	if (file_index >= snapshot->controls->len) return NULL;

	return ((PSPdispSettingsControl *) snapshot->controls->pdata[file_index])->name;
}

/**
 * @brief Load the settings, the presets and the control files of a folder into a new snapshot.
 *
 * @param dir The folder of the settings.
 * @param previous The latest snapshot (`NULL` for the first one).
 * @param controls_changed If the control files changed since `previous`.
 * @return The snapshot, with one reference.
 */
PSPdispDaemonSnapshot *pspdisp_daemon_snapshot_load(const char *dir, PSPdispDaemonSnapshot *previous, bool controls_changed) {
	PSPdispDaemonSnapshot *snapshot = g_new0(PSPdispDaemonSnapshot, 1);
	GError *error = NULL;

	g_atomic_ref_count_init(&snapshot->refs);
	snapshot->loaded_at = g_get_monotonic_time();
	snapshot->generation = previous != NULL ? previous->generation + 1 : 1;
	snapshot->controls_generation = previous != NULL ? previous->controls_generation + controls_changed : 1;

	if (!pspdisp_settings_load(dir, &snapshot->settings, &error)) {
		g_warning("%s(): %s, the defaults are used", __FUNCTION__, error->message);
		g_error_free(error);
	}

	// As many as the PSP keeps
	snapshot->presets = pspdisp_settings_presets(dir);
	if (snapshot->presets->len > PSPDISP_COM_MAX_PRESETS)
		g_ptr_array_set_size(snapshot->presets, PSPDISP_COM_MAX_PRESETS);

	const guint max_control_files = PSPDISP_COM_MAX_CONTROLS - PSPDISP_DAEMON_SNAPSHOT_FIXED_CONTROLS; // The PSP lists the gamepad and none before them
	snapshot->controls = pspdisp_settings_controls(dir);
	if (snapshot->controls->len > max_control_files)
		g_ptr_array_set_size(snapshot->controls, max_control_files);

	return snapshot;
}

/**
 * @brief Take a reference to the latest snapshot, from any thread.
 *
 * @param store The snapshots.
 * @return The snapshot, release it with `pspdisp_daemon_snapshot_unref()`.
 */
PSPdispDaemonSnapshot *pspdisp_daemon_snapshot_store_get(PSPdispDaemonSnapshotStore *store) {
	g_mutex_lock(&store->mutex);
	PSPdispDaemonSnapshot *snapshot = store->current;
	g_atomic_ref_count_inc(&snapshot->refs);
	g_mutex_unlock(&store->mutex);
	return snapshot;
}

/**
 * @brief Publish a snapshot, so it's the latest one, then tell the handler.
 *
 * @param store The snapshots.
 * @param snapshot The snapshot, its reference is taken.
 */
void pspdisp_daemon_snapshot_store_publish(PSPdispDaemonSnapshotStore *store, PSPdispDaemonSnapshot *snapshot) {
	g_mutex_lock(&store->mutex);
	PSPdispDaemonSnapshot *previous = store->current;
	store->current = snapshot;
	g_mutex_unlock(&store->mutex);

	if (previous != NULL)
		pspdisp_daemon_snapshot_unref(previous);
	if (store->handler != NULL)
		store->handler(snapshot, store->handler_data);
}

/**
 * @brief Load the settings again, after a change on the disk, and publish them if they changed.
 *
 * @param store The snapshots.
 * @return Always `G_SOURCE_REMOVE`, it's called once per change.
 */
gboolean pspdisp_daemon_snapshot_store_on_reload(PSPdispDaemonSnapshotStore *store) {
	PSPdispDaemonSnapshot *previous = pspdisp_daemon_snapshot_store_get(store);
	PSPdispDaemonSnapshot *snapshot = pspdisp_daemon_snapshot_load(store->dir, previous, store->controls_changed);
	bool changed = store->controls_changed || !pspdisp_settings_equal(&previous->settings, &snapshot->settings)
	            || !pspdisp_daemon_snapshot_lists_equal(previous->presets, snapshot->presets) || !pspdisp_daemon_snapshot_lists_equal(previous->controls, snapshot->controls);

	store->reload = 0;
	store->controls_changed = false;
	pspdisp_daemon_snapshot_unref(previous);

	if (changed) {
		g_debug("Settings reloaded from %s", store->dir);
		pspdisp_daemon_snapshot_store_publish(store, snapshot);
	} else {
		pspdisp_daemon_snapshot_unref(snapshot);
	}

	return G_SOURCE_REMOVE;
}

/**
 * @brief Plan a reload of the settings, called by the monitors when a file changes.
 *
 * @param monitor The monitor.
 * @param file The file that changed.
 * @param other_file Unused.
 * @param event What changed.
 * @param store The snapshots.
 */
void pspdisp_daemon_snapshot_store_on_change(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event, PSPdispDaemonSnapshotStore *store) {
	(void) file;
	(void) other_file;

	// Only the end of the changes (not each write), and the files created or deleted (e.g. a file written then renamed over the old one)
	if (event != G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT && event != G_FILE_MONITOR_EVENT_CREATED && event != G_FILE_MONITOR_EVENT_DELETED) return;

	if (monitor == store->control_monitor)
		store->controls_changed = true;
	if (store->reload == 0)
		store->reload = g_timeout_add(PSPDISP_DAEMON_SNAPSHOT_DELAY, G_SOURCE_FUNC(pspdisp_daemon_snapshot_store_on_reload), store);
}

/**
 * @brief Watch a file or a folder of the settings.
 *
 * @param store The snapshots.
 * @param path The file or folder.
 * @param directory If it's a folder.
 * @return The monitor, owned by the snapshots, or `NULL` if it failed.
 */
GFileMonitor *pspdisp_daemon_snapshot_store_watch(PSPdispDaemonSnapshotStore *store, const char *path, bool directory) {
	GFile *file = g_file_new_for_path(path);
	GError *error = NULL;
	GFileMonitor *monitor = directory ? g_file_monitor_directory(file, G_FILE_MONITOR_NONE, NULL, &error) : g_file_monitor_file(file, G_FILE_MONITOR_NONE, NULL, &error);

	if (monitor != NULL) {
		g_signal_connect(monitor, "changed", G_CALLBACK(pspdisp_daemon_snapshot_store_on_change), store);
		g_ptr_array_add(store->monitors, monitor);
	} else {
		g_warning("%s(): failed to watch %s, its changes need a restart of the daemon: %s", __FUNCTION__, path, error->message);
		g_error_free(error);
	}

	g_object_unref(file);
	return monitor;
}

/**
 * @brief Load the first snapshot of a folder of the settings, and watch its files, on the main thread.
 *
 * The default presets and control files are installed the first time.
 *
 * @param dir The folder of the settings.
 * @param handler Called on the main thread when a new snapshot is published, after this first one.
 * @param data Given to the handler.
 * @return The snapshots, free them with `pspdisp_daemon_snapshot_store_destroy()`.
 */
PSPdispDaemonSnapshotStore *pspdisp_daemon_snapshot_store_new(const char *dir, PSPdispDaemonSnapshotHandler handler, gpointer data) {
	PSPdispDaemonSnapshotStore *store = g_new0(PSPdispDaemonSnapshotStore, 1);
	store->dir = g_strdup(dir);
	g_mutex_init(&store->mutex);

	pspdisp_settings_install_defaults(dir);
	pspdisp_daemon_snapshot_store_publish(store, pspdisp_daemon_snapshot_load(dir, NULL, false));
	store->handler = handler;
	store->handler_data = data;

	char *settings_path = pspdisp_settings_file_path(dir);
	char *presets_dir = pspdisp_settings_presets_dir(dir);
	char *control_dir = pspdisp_settings_control_dir(dir);

	store->monitors = g_ptr_array_new_with_free_func(g_object_unref);
	pspdisp_daemon_snapshot_store_watch(store, settings_path, false);
	pspdisp_daemon_snapshot_store_watch(store, presets_dir, true);
	store->control_monitor = pspdisp_daemon_snapshot_store_watch(store, control_dir, true);

	g_free(control_dir);
	g_free(presets_dir);
	g_free(settings_path);
	return store;
}

/**
 * @brief Stop watching the files, release the latest snapshot and free the snapshots, once no other thread uses them.
 *
 * @param store The snapshots.
 */
void pspdisp_daemon_snapshot_store_destroy(PSPdispDaemonSnapshotStore *store) {
	if (store->reload != 0)
		g_source_remove(store->reload);

	// GIO may keep a monitor a bit longer, so it must not call back with the freed snapshots
	for (guint i = 0; i < store->monitors->len; i++) {
		g_file_monitor_cancel(store->monitors->pdata[i]);
		g_signal_handlers_disconnect_by_data(store->monitors->pdata[i], store);
	}
	g_ptr_array_unref(store->monitors);
	pspdisp_daemon_snapshot_unref(store->current);
	g_mutex_clear(&store->mutex);
	g_free(store->dir);
	g_free(store);
}
