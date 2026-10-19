/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * settings.c - Desktop app and daemon settings      *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The settings of PSPdisp, shared by the desktop app (which edits them) and the daemon (which streams with them, and saves what the PSP changes).
 *
 * They live in a folder, `~/.config/pspdisp` by default (see `pspdisp_settings_dir_resolve()`), as key files (the INI format, as the old desktop app):
 *   - `pspdisp.conf`, the settings (see `PSPdispSettings`).
 *   - The `.preset` files of `presets`, named sets of streaming settings, chosen from the desktop app or on the PSP (see `pspdisp_settings_presets()`).
 *   - The `.control` files of `control`, what the PSP buttons press on the keyboard and the mouse, in the format of the old desktop app (see `controller/control_file.c`).
 *
 * Both programs may write the settings at the same time, so a write only changes the keys it's about, in the file as it is on the disk, under a lock (see `pspdisp_settings_edit()`).
 * Both run as the user, so the files are the user's ones.
 */

#pragma once

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "../../shared.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>

// PSPDISP_VERSION: version of the desktop app and the daemon (given by the build)
#ifndef PSPDISP_VERSION
	#error "PSPDISP_VERSION must be defined by the build, e.g. -DPSPDISP_VERSION='\"0.7.0-dev\"'"
#endif

#define PSPDISP_SETTINGS_FILE_NAME   "pspdisp.conf" // The settings, in the folder of the settings
#define PSPDISP_SETTINGS_PRESETS_DIR "presets"      // The presets (`*.preset`), in the folder of the settings
#define PSPDISP_SETTINGS_CONTROL_DIR "control"      // The control files (`*.control`), in the folder of the settings
#define PSPDISP_SETTINGS_LOCK_NAME   ".lock"        // Locked while a program writes the settings, in the folder of the settings

#define PSPDISP_SETTINGS_PASSWORD_SIZE 32 // Max characters of the WLAN password, as the PSP sends it
#define PSPDISP_SETTINGS_MAX_PORT_OFFSET 9 // Max port offset of the WLAN mode, as the PSP sets it

#define PSPDISP_SETTINGS_CONTROL_GAMEPAD "Gamepad" // Control: a virtual gamepad (see `controller.c`)
#define PSPDISP_SETTINGS_CONTROL_NONE    "None"    // Control: nothing

/**
 * @brief The streaming settings: the ones the PSP shows and changes on its "PC Settings" menu, and that a preset sets.
 */
typedef struct {
	PSPdispComQuality quality; // JPEG quality level of the images
	bool uncompressed; // If the images are sent uncompressed instead of JPEG
	PSPdispComUpdateInterval update_interval; // Pause after each frame, without sound
	bool image; // If the display is streamed, otherwise only the sound
	bool sound; // If the sound is streamed
	PSPdispComSampleRate sample_rate_usb; // Sample rate of the sound over USB
	PSPdispComSampleRate sample_rate_wlan; // Sample rate of the sound over WLAN, which carries less
	char control[PSPDISP_COM_NAME_SIZE]; // What the PSP controls drive: `PSPDISP_SETTINGS_CONTROL_GAMEPAD`, `PSPDISP_SETTINGS_CONTROL_NONE`, or the name of a control file
} PSPdispSettingsStreaming;

/**
 * @brief The settings of PSPdisp, as in `pspdisp.conf`.
 *
 * It holds no pointer, so it can be copied and compared (see `pspdisp_settings_equal()`).
 */
typedef struct {
	bool enabled; // If the daemon serves the PSPs ("Enabled" of the old desktop app)
	bool use_usb; // If the PSPs plugged in USB mode are served
	bool use_wlan; // If the PSPs connecting in WLAN mode are served
	bool higher_priority; // If the daemon runs with a higher priority than the other programs, so the streaming stutters less ("Higher process priority" of the old desktop app)
	PSPdispSettingsStreaming streaming; // The streaming settings
	uint32_t jpeg_quality[PSPDISP_COM_QUALITY_HIGHEST + 1]; // JPEG quality (1 to 100) of each quality level
	uint32_t update_interval[PSPDISP_COM_UPDATE_INTERVAL_LONGEST + 1]; // Pause in milliseconds of each update interval (the continuous one is always 0)
	char wlan_password[PSPDISP_SETTINGS_PASSWORD_SIZE + 1]; // Password set on the PSP, for the WLAN mode (`""` for none)
	uint32_t wlan_port_offset; // Port offset set on the PSP, for the WLAN mode (0 to `PSPDISP_SETTINGS_MAX_PORT_OFFSET`)
	bool notify_server_started; // If the desktop app notifies when the WLAN server starts
	bool notify_connected; // If the desktop app notifies when a PSP connects over WLAN
	bool notify_disconnected; // If the desktop app notifies when a PSP disconnects from WLAN
} PSPdispSettings;

/**
 * @brief A preset file, a named set of streaming settings.
 */
typedef struct {
	char *name; // Its name, as the PSP shows it
	char *file_name; // Its file name, in the presets folder (e.g. `balanced.preset`)
} PSPdispSettingsPreset;

/**
 * @brief A control file, telling what the PSP buttons press on the keyboard and the mouse.
 */
typedef struct {
	char *name; // Its name, as the PSP shows it
	char *path; // Its path
} PSPdispSettingsControl;

/**
 * @brief Type of a function pointer to edit the settings as a key file (see `pspdisp_settings_edit()`).
 *
 * @param file The key file, as on the disk (empty when there's no file yet).
 * @param data The data given to `pspdisp_settings_edit()`.
 */
typedef void (*PSPdispSettingsEditor) (GKeyFile *file, gpointer data);

// Names of the settings values in the files, by their value in the protocol
const char *const pspdisp_settings_quality_names[] = { "Lowest", "Low", "Medium", "High", "Highest" };
const char *const pspdisp_settings_update_interval_names[] = { "Continuous", "Shortest", "Short", "Medium", "Long", "Longest" };
const uint32_t pspdisp_settings_sample_rates[] = { 11025, 22050, 44100 };

static_assert(G_N_ELEMENTS(pspdisp_settings_quality_names) == PSPDISP_COM_QUALITY_HIGHEST + 1, "A name for each quality level");
static_assert(G_N_ELEMENTS(pspdisp_settings_update_interval_names) == PSPDISP_COM_UPDATE_INTERVAL_LONGEST + 1, "A name for each update interval");
static_assert(G_N_ELEMENTS(pspdisp_settings_sample_rates) == PSPDISP_COM_SAMPLE_RATE_44100 + 1, "A value for each sample rate");

// The default control files, as the old desktop app installed them
const char pspdisp_settings_default_mouse_control[] = {
	#embed "./defaults/mouse.control"
	, 0
};
const char pspdisp_settings_default_fps_control[] = {
	#embed "./defaults/fps.control"
	, 0
};
const char pspdisp_settings_default_re4_control[] = {
	#embed "./defaults/re4.control"
	, 0
};
const char pspdisp_settings_default_control_template[] = {
	#embed "./defaults/control.template"
	, 0
};

/**
 * @brief The default control files, written when the control folder is created.
 */
const struct {
	const char *file_name; // Its file name
	const char *content; // Its content
} pspdisp_settings_default_controls[] = {
	{ "mouse.control", pspdisp_settings_default_mouse_control },
	{ "fps.control", pspdisp_settings_default_fps_control },
	{ "re4.control", pspdisp_settings_default_re4_control },
	{ "control.template", pspdisp_settings_default_control_template }, // Not listed, as it's not a `.control` file
};

/**
 * @brief The default presets, written when the presets folder is created.
 *
 * With sound, the sample rate also gives the frame rate, because every frame carries a chunk of audio (about 10 frames per second at 11025 Hz, 20 at 22050 Hz and 33 at 44100 Hz).
 */
const struct {
	const char *file_name; // Its file name
	const char *name; // Its name
	PSPdispComQuality quality;
	bool uncompressed;
	PSPdispComUpdateInterval update_interval;
	bool sound;
	PSPdispComSampleRate sample_rate_usb;
	PSPdispComSampleRate sample_rate_wlan;
} pspdisp_settings_default_presets[] = {
	{ "1-balanced.preset", "Balanced", PSPDISP_COM_QUALITY_MEDIUM, false, PSPDISP_COM_UPDATE_INTERVAL_CONTINUOUS, true, PSPDISP_COM_SAMPLE_RATE_22050, PSPDISP_COM_SAMPLE_RATE_11025 },
	{ "2-sharp.preset", "Sharp image", PSPDISP_COM_QUALITY_HIGHEST, false, PSPDISP_COM_UPDATE_INTERVAL_CONTINUOUS, true, PSPDISP_COM_SAMPLE_RATE_22050, PSPDISP_COM_SAMPLE_RATE_11025 },
	{ "3-smooth.preset", "Smooth motion", PSPDISP_COM_QUALITY_LOW, false, PSPDISP_COM_UPDATE_INTERVAL_CONTINUOUS, true, PSPDISP_COM_SAMPLE_RATE_44100, PSPDISP_COM_SAMPLE_RATE_11025 },
	{ "4-uncompressed.preset", "Uncompressed, no sound", PSPDISP_COM_QUALITY_HIGHEST, true, PSPDISP_COM_UPDATE_INTERVAL_CONTINUOUS, false, PSPDISP_COM_SAMPLE_RATE_22050, PSPDISP_COM_SAMPLE_RATE_11025 },
	{ "5-low-bandwidth.preset", "Low bandwidth", PSPDISP_COM_QUALITY_LOWEST, false, PSPDISP_COM_UPDATE_INTERVAL_MEDIUM, true, PSPDISP_COM_SAMPLE_RATE_11025, PSPDISP_COM_SAMPLE_RATE_11025 },
};

/**
 * @brief Set the default streaming settings.
 *
 * @param streaming Where to set them.
 */
void pspdisp_settings_streaming_defaults(PSPdispSettingsStreaming *streaming) {
	*streaming = (PSPdispSettingsStreaming) {
		.quality = PSPDISP_COM_QUALITY_MEDIUM,
		.update_interval = PSPDISP_COM_UPDATE_INTERVAL_CONTINUOUS,
		.image = true,
		.sound = true,
		.sample_rate_usb = PSPDISP_COM_SAMPLE_RATE_22050,
		.sample_rate_wlan = PSPDISP_COM_SAMPLE_RATE_11025,
	};
	g_strlcpy(streaming->control, PSPDISP_SETTINGS_CONTROL_GAMEPAD, sizeof(streaming->control));
}

/**
 * @brief Set the default settings, as the old desktop app (USB and WLAN, no password).
 *
 * @param settings Where to set them.
 */
void pspdisp_settings_defaults(PSPdispSettings *settings) {
	*settings = (PSPdispSettings) {
		.enabled = true,
		.use_usb = true,
		.use_wlan = true,
		.higher_priority = true,
		.jpeg_quality = { [PSPDISP_COM_QUALITY_LOWEST] = 10, [PSPDISP_COM_QUALITY_LOW] = 20, [PSPDISP_COM_QUALITY_MEDIUM] = 30, [PSPDISP_COM_QUALITY_HIGH] = 50, [PSPDISP_COM_QUALITY_HIGHEST] = 80 },
		.update_interval = { [PSPDISP_COM_UPDATE_INTERVAL_SHORTEST] = 30, [PSPDISP_COM_UPDATE_INTERVAL_SHORT] = 40, [PSPDISP_COM_UPDATE_INTERVAL_MEDIUM] = 50, [PSPDISP_COM_UPDATE_INTERVAL_LONG] = 100, [PSPDISP_COM_UPDATE_INTERVAL_LONGEST] = 200 },
		.notify_server_started = true,
		.notify_connected = true,
		.notify_disconnected = true,
	};
	pspdisp_settings_streaming_defaults(&settings->streaming);
}

/**
 * @brief Tell if two streaming settings are the same.
 */
bool pspdisp_settings_streaming_equal(const PSPdispSettingsStreaming *a, const PSPdispSettingsStreaming *b) {
	return a->quality == b->quality && a->uncompressed == b->uncompressed && a->update_interval == b->update_interval
		&& a->image == b->image && a->sound == b->sound && a->sample_rate_usb == b->sample_rate_usb && a->sample_rate_wlan == b->sample_rate_wlan
		&& strcmp(a->control, b->control) == 0;
}

/**
 * @brief Tell if two settings are the same.
 */
bool pspdisp_settings_equal(const PSPdispSettings *a, const PSPdispSettings *b) {
	return a->enabled == b->enabled && a->use_usb == b->use_usb && a->use_wlan == b->use_wlan && a->higher_priority == b->higher_priority && pspdisp_settings_streaming_equal(&a->streaming, &b->streaming)
		&& memcmp(a->jpeg_quality, b->jpeg_quality, sizeof(a->jpeg_quality)) == 0 && memcmp(a->update_interval, b->update_interval, sizeof(a->update_interval)) == 0
		&& strcmp(a->wlan_password, b->wlan_password) == 0 && a->wlan_port_offset == b->wlan_port_offset
		&& a->notify_server_started == b->notify_server_started && a->notify_connected == b->notify_connected && a->notify_disconnected == b->notify_disconnected;
}

/**
 * @brief Get the sample rate in Hz of a sample rate setting.
 */
uint32_t pspdisp_settings_sample_rate_hz(PSPdispComSampleRate sample_rate) {
	return pspdisp_settings_sample_rates[MIN(sample_rate, PSPDISP_COM_SAMPLE_RATE_44100)];
}

/**
 * @brief Find a name in a list of names, ignoring the case.
 *
 * @return Its index, or -1 if it's not there.
 */
int pspdisp_settings_find_name(const char *const *names, size_t count, const char *name) {
	for (size_t i = 0; name != NULL && i < count; i++) {
		if (g_ascii_strcasecmp(names[i], name) == 0) return i;
	}
	return -1;
}

/**
 * @brief Find a value in a list of values.
 *
 * @return Its index, or -1 if it's not there.
 */
int pspdisp_settings_find_value(const uint32_t *values, size_t count, uint32_t value) {
	for (size_t i = 0; i < count; i++) {
		if (values[i] == value) return i;
	}
	return -1;
}

/**
 * @brief Read a boolean of a key file, if it's there and valid.
 *
 * @param value Where to write it, untouched otherwise.
 */
void pspdisp_settings_read_boolean(GKeyFile *file, const char *group, const char *key, bool *value) {
	GError *error = NULL;
	gboolean read = g_key_file_get_boolean(file, group, key, &error);

	if (error == NULL)
		*value = read;
	g_clear_error(&error);
}

/**
 * @brief Read an integer of a key file, if it's there and in a range.
 *
 * @param value Where to write it, untouched otherwise.
 */
void pspdisp_settings_read_integer(GKeyFile *file, const char *group, const char *key, uint32_t minimum, uint32_t maximum, uint32_t *value) {
	GError *error = NULL;
	gint64 read = g_key_file_get_int64(file, group, key, &error);

	if (error == NULL && read >= minimum && read <= maximum)
		*value = read;
	g_clear_error(&error);
}

/**
 * @brief Read a string of a key file into a buffer, if it's there and fits.
 *
 * @param value Where to write it, untouched otherwise.
 * @param size The size of `value` in bytes.
 */
void pspdisp_settings_read_string(GKeyFile *file, const char *group, const char *key, char *value, size_t size) {
	char *read = g_key_file_get_string(file, group, key, NULL);

	if (read != NULL && strlen(read) < size)
		g_strlcpy(value, read, size);
	g_free(read);
}

/**
 * @brief Read a name of a list (e.g. a quality level), if it's there and in the list.
 *
 * @param value Where to write its index, untouched otherwise.
 */
void pspdisp_settings_read_name(GKeyFile *file, const char *group, const char *key, const char *const *names, size_t count, uint32_t *value) {
	char *read = g_key_file_get_string(file, group, key, NULL);
	int index = pspdisp_settings_find_name(names, count, read);

	if (index >= 0)
		*value = index;
	g_free(read);
}

/**
 * @brief Read a value of a list (e.g. a sample rate in Hz), if it's there and in the list.
 *
 * @param value Where to write its index, untouched otherwise.
 */
void pspdisp_settings_read_value(GKeyFile *file, const char *group, const char *key, const uint32_t *values, size_t count, uint32_t *value) {
	uint32_t read = UINT32_MAX;
	pspdisp_settings_read_integer(file, group, key, 0, UINT32_MAX - 1, &read);

	int index = pspdisp_settings_find_value(values, count, read);
	if (index >= 0)
		*value = index;
}

/**
 * @brief Read the streaming settings of a group of a key file, over the ones given.
 *
 * Only the keys there and valid are read, so a preset can set only some settings.
 *
 * @param file The key file.
 * @param group The group (`Streaming` in the settings and in the presets).
 * @param streaming The settings, changed by the keys read.
 */
void pspdisp_settings_read_streaming(GKeyFile *file, const char *group, PSPdispSettingsStreaming *streaming) {
	uint32_t value;

	value = streaming->quality;
	pspdisp_settings_read_name(file, group, "Quality", pspdisp_settings_quality_names, G_N_ELEMENTS(pspdisp_settings_quality_names), &value);
	streaming->quality = value;

	value = streaming->update_interval;
	pspdisp_settings_read_name(file, group, "UpdateInterval", pspdisp_settings_update_interval_names, G_N_ELEMENTS(pspdisp_settings_update_interval_names), &value);
	streaming->update_interval = value;

	value = streaming->sample_rate_usb;
	pspdisp_settings_read_value(file, group, "SampleRateUSB", pspdisp_settings_sample_rates, G_N_ELEMENTS(pspdisp_settings_sample_rates), &value);
	streaming->sample_rate_usb = value;

	value = streaming->sample_rate_wlan;
	pspdisp_settings_read_value(file, group, "SampleRateWLAN", pspdisp_settings_sample_rates, G_N_ELEMENTS(pspdisp_settings_sample_rates), &value);
	streaming->sample_rate_wlan = value;

	pspdisp_settings_read_boolean(file, group, "Uncompressed", &streaming->uncompressed);
	pspdisp_settings_read_boolean(file, group, "Image", &streaming->image);
	pspdisp_settings_read_boolean(file, group, "Sound", &streaming->sound);
	pspdisp_settings_read_string(file, group, "Control", streaming->control, sizeof(streaming->control));
}

/**
 * @brief Write the streaming settings into a group of a key file.
 *
 * @param file The key file.
 * @param group The group (`Streaming` in the settings and in the presets).
 * @param streaming The settings.
 */
void pspdisp_settings_write_streaming(GKeyFile *file, const char *group, const PSPdispSettingsStreaming *streaming) {
	g_key_file_set_string(file, group, "Quality", pspdisp_settings_quality_names[MIN(streaming->quality, PSPDISP_COM_QUALITY_HIGHEST)]);
	g_key_file_set_boolean(file, group, "Uncompressed", streaming->uncompressed);
	g_key_file_set_string(file, group, "UpdateInterval", pspdisp_settings_update_interval_names[MIN(streaming->update_interval, PSPDISP_COM_UPDATE_INTERVAL_LONGEST)]);
	g_key_file_set_boolean(file, group, "Image", streaming->image);
	g_key_file_set_boolean(file, group, "Sound", streaming->sound);
	g_key_file_set_integer(file, group, "SampleRateUSB", pspdisp_settings_sample_rate_hz(streaming->sample_rate_usb));
	g_key_file_set_integer(file, group, "SampleRateWLAN", pspdisp_settings_sample_rate_hz(streaming->sample_rate_wlan));
	g_key_file_set_string(file, group, "Control", streaming->control);
}

/**
 * @brief Read the settings of a key file, over the ones given (e.g. the defaults).
 *
 * @param file The key file.
 * @param settings The settings, changed by the keys read.
 */
void pspdisp_settings_read(GKeyFile *file, PSPdispSettings *settings) {
	pspdisp_settings_read_boolean(file, "General", "Enabled", &settings->enabled);
	pspdisp_settings_read_boolean(file, "General", "USB", &settings->use_usb);
	pspdisp_settings_read_boolean(file, "General", "WLAN", &settings->use_wlan);
	pspdisp_settings_read_boolean(file, "General", "HigherPriority", &settings->higher_priority);

	pspdisp_settings_read_streaming(file, "Streaming", &settings->streaming);

	for (size_t i = 0; i < G_N_ELEMENTS(settings->jpeg_quality); i++)
		pspdisp_settings_read_integer(file, "Quality", pspdisp_settings_quality_names[i], 1, 100, &settings->jpeg_quality[i]);
	for (size_t i = PSPDISP_COM_UPDATE_INTERVAL_SHORTEST; i < G_N_ELEMENTS(settings->update_interval); i++)
		pspdisp_settings_read_integer(file, "UpdateInterval", pspdisp_settings_update_interval_names[i], 0, 10000, &settings->update_interval[i]);

	pspdisp_settings_read_string(file, "WLAN", "Password", settings->wlan_password, sizeof(settings->wlan_password));
	pspdisp_settings_read_integer(file, "WLAN", "PortOffset", 0, PSPDISP_SETTINGS_MAX_PORT_OFFSET, &settings->wlan_port_offset);
	pspdisp_settings_read_boolean(file, "WLAN", "NotifyServerStarted", &settings->notify_server_started);
	pspdisp_settings_read_boolean(file, "WLAN", "NotifyConnected", &settings->notify_connected);
	pspdisp_settings_read_boolean(file, "WLAN", "NotifyDisconnected", &settings->notify_disconnected);
}

/**
 * @brief Write all the settings into a key file.
 *
 * @param file The key file.
 * @param settings The settings.
 */
void pspdisp_settings_write(GKeyFile *file, const PSPdispSettings *settings) {
	g_key_file_set_boolean(file, "General", "Enabled", settings->enabled);
	g_key_file_set_boolean(file, "General", "USB", settings->use_usb);
	g_key_file_set_boolean(file, "General", "WLAN", settings->use_wlan);
	g_key_file_set_boolean(file, "General", "HigherPriority", settings->higher_priority);

	pspdisp_settings_write_streaming(file, "Streaming", &settings->streaming);

	for (size_t i = 0; i < G_N_ELEMENTS(settings->jpeg_quality); i++)
		g_key_file_set_integer(file, "Quality", pspdisp_settings_quality_names[i], settings->jpeg_quality[i]);
	for (size_t i = PSPDISP_COM_UPDATE_INTERVAL_SHORTEST; i < G_N_ELEMENTS(settings->update_interval); i++)
		g_key_file_set_integer(file, "UpdateInterval", pspdisp_settings_update_interval_names[i], settings->update_interval[i]);

	g_key_file_set_string(file, "WLAN", "Password", settings->wlan_password);
	g_key_file_set_integer(file, "WLAN", "PortOffset", settings->wlan_port_offset);
	g_key_file_set_boolean(file, "WLAN", "NotifyServerStarted", settings->notify_server_started);
	g_key_file_set_boolean(file, "WLAN", "NotifyConnected", settings->notify_connected);
	g_key_file_set_boolean(file, "WLAN", "NotifyDisconnected", settings->notify_disconnected);
}

/**
 * @brief Get the path of the settings file.
 *
 * @param dir The folder of the settings.
 * @return The path, free it with `g_free()`.
 */
char *pspdisp_settings_file_path(const char *dir) {
	return g_build_filename(dir, PSPDISP_SETTINGS_FILE_NAME, NULL);
}

/**
 * @brief Get the path of the presets folder.
 *
 * @param dir The folder of the settings.
 * @return The path, free it with `g_free()`.
 */
char *pspdisp_settings_presets_dir(const char *dir) {
	return g_build_filename(dir, PSPDISP_SETTINGS_PRESETS_DIR, NULL);
}

/**
 * @brief Get the path of the control files folder.
 *
 * @param dir The folder of the settings.
 * @return The path, free it with `g_free()`.
 */
char *pspdisp_settings_control_dir(const char *dir) {
	return g_build_filename(dir, PSPDISP_SETTINGS_CONTROL_DIR, NULL);
}

/**
 * @brief Create a folder and its parents.
 *
 * @param path The folder to create.
 * @param created Where to write if it was created now (`NULL` to ignore it).
 * @return `true` if it exists now.
 */
bool pspdisp_settings_make_dir(const char *path, bool *created) {
	bool existed = g_file_test(path, G_FILE_TEST_IS_DIR);

	if (created != NULL)
		*created = false;
	if (existed) return true;

	if (g_mkdir_with_parents(path, 0755) != 0) {
		g_warning("%s(): failed to create %s: %s", __FUNCTION__, path, g_strerror(errno));
		return false;
	}

	if (created != NULL)
		*created = true;
	return true;
}

/**
 * @brief Write a whole file atomically.
 *
 * @param path The file.
 * @param content The content.
 * @param length The size of the content in bytes.
 * @param error Where to tell why it failed.
 * @return `true` if it was written.
 */
bool pspdisp_settings_write_file(const char *path, const char *content, gsize length, GError **error) {
	return g_file_set_contents_full(path, content, length, G_FILE_SET_CONTENTS_CONSISTENT, 0644, error);
}

/**
 * @brief Install the default presets and control files, the first time.
 *
 * They're written when their folder is created, so the files the user deletes don't come back.
 *
 * @param dir The folder of the settings.
 */
void pspdisp_settings_install_defaults(const char *dir) {
	bool created = false;

	pspdisp_settings_make_dir(dir, NULL);

	char *presets_dir = pspdisp_settings_presets_dir(dir);
	if (pspdisp_settings_make_dir(presets_dir, &created) && created) {
		for (size_t i = 0; i < G_N_ELEMENTS(pspdisp_settings_default_presets); i++) {
			GKeyFile *file = g_key_file_new();
			PSPdispSettingsStreaming streaming;

			pspdisp_settings_streaming_defaults(&streaming);
			streaming.quality = pspdisp_settings_default_presets[i].quality;
			streaming.uncompressed = pspdisp_settings_default_presets[i].uncompressed;
			streaming.update_interval = pspdisp_settings_default_presets[i].update_interval;
			streaming.sound = pspdisp_settings_default_presets[i].sound;
			streaming.sample_rate_usb = pspdisp_settings_default_presets[i].sample_rate_usb;
			streaming.sample_rate_wlan = pspdisp_settings_default_presets[i].sample_rate_wlan;

			g_key_file_set_string(file, "General", "Name", pspdisp_settings_default_presets[i].name);
			pspdisp_settings_write_streaming(file, "Streaming", &streaming);
			g_key_file_remove_key(file, "Streaming", "Control", NULL); // A preset doesn't change the control by default

			gsize length;
			char *content = g_key_file_to_data(file, &length, NULL);
			char *path = g_build_filename(presets_dir, pspdisp_settings_default_presets[i].file_name, NULL);
			GError *error = NULL;

			if (!pspdisp_settings_write_file(path, content, length, &error)) {
				g_warning("%s(): failed to write the preset %s: %s", __FUNCTION__, path, error->message);
				g_error_free(error);
			}

			g_free(path);
			g_free(content);
			g_key_file_free(file);
		}
	}
	g_free(presets_dir);

	char *control_dir = pspdisp_settings_control_dir(dir);
	if (pspdisp_settings_make_dir(control_dir, &created) && created) {
		for (size_t i = 0; i < G_N_ELEMENTS(pspdisp_settings_default_controls); i++) {
			char *path = g_build_filename(control_dir, pspdisp_settings_default_controls[i].file_name, NULL);
			GError *error = NULL;

			if (!pspdisp_settings_write_file(path, pspdisp_settings_default_controls[i].content, strlen(pspdisp_settings_default_controls[i].content), &error)) {
				g_warning("%s(): failed to write the control file %s: %s", __FUNCTION__, path, error->message);
				g_error_free(error);
			}
			g_free(path);
		}
	}
	g_free(control_dir);
}

/**
 * @brief Load the settings, the defaults for what's not in the file (or all of them without a file).
 *
 * @param dir The folder of the settings.
 * @param settings Where to write the settings.
 * @param error Where to tell why the file can't be read (the defaults are still written then).
 * @return `true` if the file was read, or doesn't exist yet.
 */
bool pspdisp_settings_load(const char *dir, PSPdispSettings *settings, GError **error) {
	char *path = pspdisp_settings_file_path(dir);
	GKeyFile *file = g_key_file_new();
	GError *failure = NULL;

	pspdisp_settings_defaults(settings);

	bool loaded = g_key_file_load_from_file(file, path, G_KEY_FILE_NONE, &failure);
	if (loaded)
		pspdisp_settings_read(file, settings);

	bool missing = !loaded && g_error_matches(failure, G_FILE_ERROR, G_FILE_ERROR_NOENT);
	if (!loaded && !missing)
		g_propagate_prefixed_error(error, failure, "failed to read %s: ", path);
	else
		g_clear_error(&failure);

	g_key_file_free(file);
	g_free(path);
	return loaded || missing;
}

/**
 * @brief Edit the settings file: read it as it is on the disk, let the editor change it, and write it back if it changed.
 *
 * The desktop app and the daemon may write it at the same time, so it's done under a lock, and the editor only changes the keys it's about.
 * The comments of the file are kept.
 *
 * @param dir The folder of the settings.
 * @param editor Changes the key file.
 * @param data Given to the editor.
 * @param error Where to tell why it failed.
 * @return `true` if the file has the change now.
 */
bool pspdisp_settings_edit(const char *dir, PSPdispSettingsEditor editor, gpointer data, GError **error) {
	if (!pspdisp_settings_make_dir(dir, NULL)) {
		g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "failed to create %s", dir);
		return false;
	}

	char *lock_path = g_build_filename(dir, PSPDISP_SETTINGS_LOCK_NAME, NULL);
	int lock = open(lock_path, O_RDWR | O_CREAT | O_CLOEXEC, 0644);

	if (lock >= 0)
		flock(lock, LOCK_EX);
	g_free(lock_path);

	char *path = pspdisp_settings_file_path(dir);
	GKeyFile *file = g_key_file_new();
	char *before = NULL;
	GError *failure = NULL;

	if (g_key_file_load_from_file(file, path, G_KEY_FILE_KEEP_COMMENTS, &failure)) {
		before = g_key_file_to_data(file, NULL, NULL);
	} else {
		// A new file (or one that can't be read, which is replaced)
		if (!g_error_matches(failure, G_FILE_ERROR, G_FILE_ERROR_NOENT))
			g_warning("%s(): %s is unreadable, it's written again: %s", __FUNCTION__, path, failure->message);
		g_clear_error(&failure);
		g_key_file_free(file);
		file = g_key_file_new();
		g_key_file_set_comment(file, NULL, NULL, " PSPdisp settings, shared by the desktop app and the daemon (see src/desktop/settings/settings.c)", NULL);
	}

	editor(file, data);

	gsize length;
	char *after = g_key_file_to_data(file, &length, NULL);
	bool done = g_strcmp0(before, after) == 0 || pspdisp_settings_write_file(path, after, length, error); // Unchanged, nothing written

	g_free(after);
	g_free(before);
	g_key_file_free(file);
	g_free(path);

	if (lock >= 0)
		close(lock); // Also unlocks
	return done;
}

/**
 * @brief A key of the settings to set, for `pspdisp_settings_edit_key()`.
 */
typedef struct {
	const char *group;
	const char *key;
	const char *value;
} PSPdispSettingsKey;

/**
 * @brief Set a key, the editor of `pspdisp_settings_set()`.
 */
void pspdisp_settings_edit_key(GKeyFile *file, PSPdispSettingsKey *key) {
	g_key_file_set_value(file, key->group, key->key, key->value);
}

/**
 * @brief Set one key of the settings file, as a string (e.g. `true`, `50` or `Medium`).
 *
 * @param dir The folder of the settings.
 * @param group The group (e.g. `Streaming`).
 * @param key The key (e.g. `Quality`).
 * @param value The value, as in the file.
 * @param error Where to tell why it failed.
 * @return `true` if the file has it now.
 */
bool pspdisp_settings_set(const char *dir, const char *group, const char *key, const char *value, GError **error) {
	PSPdispSettingsKey edit = { group, key, value };
	return pspdisp_settings_edit(dir, (PSPdispSettingsEditor) pspdisp_settings_edit_key, &edit, error);
}

/**
 * @brief Streaming settings before and after a change, for `pspdisp_settings_edit_streaming_changes()`.
 */
typedef struct {
	const PSPdispSettingsStreaming *before;
	const PSPdispSettingsStreaming *after;
} PSPdispSettingsStreamingChange;

/**
 * @brief Write the streaming settings that changed, the editor of `pspdisp_settings_update_streaming()`.
 */
void pspdisp_settings_edit_streaming_changes(GKeyFile *file, PSPdispSettingsStreamingChange *change) {
	GKeyFile *before = g_key_file_new();
	GKeyFile *after = g_key_file_new();
	gsize count = 0;

	pspdisp_settings_write_streaming(before, "Streaming", change->before);
	pspdisp_settings_write_streaming(after, "Streaming", change->after);

	char **keys = g_key_file_get_keys(after, "Streaming", &count, NULL);
	for (gsize i = 0; i < count; i++) {
		char *old_value = g_key_file_get_value(before, "Streaming", keys[i], NULL);
		char *new_value = g_key_file_get_value(after, "Streaming", keys[i], NULL);

		if (g_strcmp0(old_value, new_value) != 0)
			g_key_file_set_value(file, "Streaming", keys[i], new_value);

		g_free(old_value);
		g_free(new_value);
	}

	g_strfreev(keys);
	g_key_file_free(after);
	g_key_file_free(before);
}

/**
 * @brief Save only the streaming settings that changed, e.g. on the PSP, so the ones changed meanwhile by the desktop app stay.
 *
 * @param dir The folder of the settings.
 * @param before The streaming settings before the change.
 * @param after The streaming settings after the change.
 * @param error Where to tell why it failed.
 * @return `true` if the file has the change now.
 */
bool pspdisp_settings_update_streaming(const char *dir, const PSPdispSettingsStreaming *before, const PSPdispSettingsStreaming *after, GError **error) {
	PSPdispSettingsStreamingChange change = { before, after };
	return pspdisp_settings_edit(dir, (PSPdispSettingsEditor) pspdisp_settings_edit_streaming_changes, &change, error);
}

/**
 * @brief Write the streaming settings, the editor of `pspdisp_settings_set_streaming()`.
 */
void pspdisp_settings_edit_streaming(GKeyFile *file, const PSPdispSettingsStreaming *streaming) {
	pspdisp_settings_write_streaming(file, "Streaming", streaming);
}

/**
 * @brief Set the streaming settings in the settings file, e.g. after they changed on the PSP.
 *
 * @param dir The folder of the settings.
 * @param streaming The streaming settings.
 * @param error Where to tell why it failed.
 * @return `true` if the file has them now.
 */
bool pspdisp_settings_set_streaming(const char *dir, const PSPdispSettingsStreaming *streaming, GError **error) {
	return pspdisp_settings_edit(dir, (PSPdispSettingsEditor) pspdisp_settings_edit_streaming, (gpointer) streaming, error);
}

/**
 * @brief Load a key file written by hand in the INI format of the old desktop app (e.g. a control file).
 *
 * GLib doesn't take its `;` comments nor its Windows line ends, so they're removed first.
 *
 * @param path The file.
 * @param error Where to tell why it failed.
 * @return The key file, free it with `g_key_file_free()`, or `NULL` if it failed.
 */
GKeyFile *pspdisp_settings_load_ini(const char *path, GError **error) {
	char *content = NULL;
	if (!g_file_get_contents(path, &content, NULL, error)) return NULL;

	GString *cleaned = g_string_new(NULL);
	char **lines = g_strsplit(content, "\n", -1);

	for (char **line = lines; *line != NULL; line++) {
		g_strchomp(*line); // Also the '\r' of the Windows line ends
		if (g_strchug(*line)[0] == ';') continue; // A comment
		g_string_append(cleaned, *line);
		g_string_append_c(cleaned, '\n');
	}

	GKeyFile *file = g_key_file_new();
	bool loaded = g_key_file_load_from_data(file, cleaned->str, cleaned->len, G_KEY_FILE_NONE, error);

	g_strfreev(lines);
	g_string_free(cleaned, TRUE);
	g_free(content);

	if (!loaded) {
		g_key_file_free(file);
		return NULL;
	}
	return file;
}

/**
 * @brief Free a preset (for `g_ptr_array_new_with_free_func()`).
 */
void pspdisp_settings_preset_free(PSPdispSettingsPreset *preset) {
	g_free(preset->name);
	g_free(preset->file_name);
	g_free(preset);
}

/**
 * @brief Free a control file (for `g_ptr_array_new_with_free_func()`).
 */
void pspdisp_settings_control_free(PSPdispSettingsControl *control) {
	g_free(control->name);
	g_free(control->path);
	g_free(control);
}

/**
 * @brief Compare two presets or two control files by their file names, for sorting them as the old desktop app (so `1-...` comes before `2-...`).
 */
int pspdisp_settings_compare_file_names(const void *a, const void *b) {
	const PSPdispSettingsPreset *preset_a = *(const PSPdispSettingsPreset *const *) a;
	const PSPdispSettingsPreset *preset_b = *(const PSPdispSettingsPreset *const *) b;
	return g_strcmp0(preset_a->file_name, preset_b->file_name);
}

/**
 * @brief List the files of a folder with an extension that have a name (in `[General] Name`).
 *
 * @param path The folder.
 * @param extension The extension (e.g. `.preset`).
 * @param with_path If the full path of each file is kept, otherwise only its file name.
 * @return The files as `PSPdispSettingsPreset` (which has the same layout as `PSPdispSettingsControl`), sorted by file name.
 */
GPtrArray *pspdisp_settings_list_named_files(const char *path, const char *extension, bool with_path) {
	GPtrArray *files = g_ptr_array_new_with_free_func((GDestroyNotify) pspdisp_settings_preset_free);
	GDir *dir = g_dir_open(path, 0, NULL);
	const char *entry;

	static_assert(offsetof(PSPdispSettingsPreset, name) == offsetof(PSPdispSettingsControl, name) && offsetof(PSPdispSettingsPreset, file_name) == offsetof(PSPdispSettingsControl, path), "Presets and control files are listed alike");

	while (dir != NULL && (entry = g_dir_read_name(dir)) != NULL) {
		if (!g_str_has_suffix(entry, extension)) continue;

		char *file_path = g_build_filename(path, entry, NULL);
		GKeyFile *file = pspdisp_settings_load_ini(file_path, NULL);
		char *name = file != NULL ? g_key_file_get_string(file, "General", "Name", NULL) : NULL;

		if (name != NULL && name[0] != '\0') {
			PSPdispSettingsPreset *named = g_new0(PSPdispSettingsPreset, 1);
			named->name = g_strndup(name, PSPDISP_COM_NAME_SIZE - 1); // As the PSP shows it
			named->file_name = with_path ? g_strdup(file_path) : g_strdup(entry);
			g_ptr_array_add(files, named);
		}

		g_free(name);
		if (file != NULL)
			g_key_file_free(file);
		g_free(file_path);
	}

	if (dir != NULL)
		g_dir_close(dir);
	g_ptr_array_sort(files, pspdisp_settings_compare_file_names);
	return files;
}

/**
 * @brief List the presets, sorted by file name.
 *
 * @param dir The folder of the settings.
 * @return The presets (`PSPdispSettingsPreset *`), free them with `g_ptr_array_unref()`.
 */
GPtrArray *pspdisp_settings_presets(const char *dir) {
	char *path = pspdisp_settings_presets_dir(dir);
	GPtrArray *presets = pspdisp_settings_list_named_files(path, ".preset", false);
	g_free(path);
	return presets;
}

/**
 * @brief List the control files, sorted by file name.
 *
 * @param dir The folder of the settings.
 * @return The control files (`PSPdispSettingsControl *`), free them with `g_ptr_array_unref()`.
 */
GPtrArray *pspdisp_settings_controls(const char *dir) {
	char *path = pspdisp_settings_control_dir(dir);
	GPtrArray *controls = pspdisp_settings_list_named_files(path, ".control", true);
	g_free(path);
	return controls;
}

/**
 * @brief Read a preset over streaming settings: only the settings it has change.
 *
 * @param dir The folder of the settings.
 * @param file_name The file name of the preset, in the presets folder.
 * @param streaming The streaming settings, changed by the preset.
 * @param error Where to tell why it failed.
 * @return `true` if the preset was read.
 */
bool pspdisp_settings_preset_load(const char *dir, const char *file_name, PSPdispSettingsStreaming *streaming, GError **error) {
	char *presets_dir = pspdisp_settings_presets_dir(dir);
	char *path = g_build_filename(presets_dir, file_name, NULL);
	GKeyFile *file = pspdisp_settings_load_ini(path, error);

	if (file != NULL) {
		pspdisp_settings_read_streaming(file, "Streaming", streaming);
		g_key_file_free(file);
	}

	g_free(path);
	g_free(presets_dir);
	return file != NULL;
}

/**
 * @brief Save streaming settings as a preset, replacing the file if it exists.
 *
 * @param dir The folder of the settings.
 * @param file_name The file name of the preset, in the presets folder (`.preset` is added if missing).
 * @param name The name of the preset.
 * @param streaming The streaming settings.
 * @param error Where to tell why it failed.
 * @return `true` if it was saved.
 */
bool pspdisp_settings_preset_save(const char *dir, const char *file_name, const char *name, const PSPdispSettingsStreaming *streaming, GError **error) {
	char *presets_dir = pspdisp_settings_presets_dir(dir);
	char *full_name = g_str_has_suffix(file_name, ".preset") ? g_strdup(file_name) : g_strconcat(file_name, ".preset", NULL);
	char *path = g_build_filename(presets_dir, full_name, NULL);
	GKeyFile *file = g_key_file_new();

	g_key_file_set_string(file, "General", "Name", name);
	pspdisp_settings_write_streaming(file, "Streaming", streaming);

	gsize length;
	char *content = g_key_file_to_data(file, &length, NULL);
	bool saved = pspdisp_settings_make_dir(presets_dir, NULL) && pspdisp_settings_write_file(path, content, length, error);

	g_free(content);
	g_key_file_free(file);
	g_free(path);
	g_free(full_name);
	g_free(presets_dir);
	return saved;
}

/**
 * @brief Delete a preset.
 *
 * @param dir The folder of the settings.
 * @param file_name The file name of the preset, in the presets folder.
 * @param error Where to tell why it failed.
 * @return `true` if it was deleted.
 */
bool pspdisp_settings_preset_delete(const char *dir, const char *file_name, GError **error) {
	char *presets_dir = pspdisp_settings_presets_dir(dir);
	char *path = g_build_filename(presets_dir, file_name, NULL);
	bool deleted = g_remove(path) == 0;

	if (!deleted)
		g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno), "failed to delete %s: %s", path, g_strerror(errno));

	g_free(path);
	g_free(presets_dir);
	return deleted;
}

/**
 * @brief Find the folder of the settings.
 *
 * @param option The folder given on the command line (`NULL` for none).
 * @return The folder, free it with `g_free()`.
 */
char *pspdisp_settings_dir_resolve(const char *option) {
	if (option != NULL)
		return g_canonicalize_filename(option, NULL);

	return g_build_filename(g_get_user_config_dir(), "pspdisp", NULL);
}
