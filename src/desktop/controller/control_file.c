/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * control_file.c - PSP as a keyboard and a mouse    *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The control files of the old desktop app: what each PSP button presses on a virtual keyboard and a virtual mouse (see `input.c`).
 *
 * A control file is in the INI format, with a section for each button (e.g. `[CROSS]`) and for each direction of the analog stick (e.g. `[ANALOG_UP]`):
 *   - `Event=Keyboard`, with `Scancode=` the PC scan codes in hexadecimal pressed together (e.g. `38 + 3E` for Alt+F4).
 *   - `Event=Mouse`, with `Button=` `Left`, `Right`, `Middle`, `DoubleClick`, `WheelUp`, `WheelDown`, `MouseMoveUp` (also down, left and right, by `MoveByPixels=`), or `MouseMoveAnalog` (the analog stick moves the mouse).
 *   - `Event=PSPdisp`, with `Action=OpenMenu`, which shows the desktop app.
 *   - `Typematic=1` repeats the event with each frame while the button is held.
 * The `[General]` section has its `Name`, `AnalogDeadZone` (0 to 127), `MapAnalogToDPad` (the analog stick acts as the directional pad), and `EnableOSK` (select opens the on-screen keyboard of the PSP, which types characters).
 * The `[AnalogStickMapping]` section tells how many pixels the mouse moves with each frame, from `Section1` (just out of the dead zone) to `Section5`, and `Maximum` (pushed to the edge).
 *
 * As the old desktop app, a button with a wrong section is ignored, and the rest of the file is still used.
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <glib.h>
#include "../../shared.h"
#include "../settings/settings.c"
#include "./input.c"

#define PSPDISP_CONTROL_FILE_MAX_SCANCODES 8  // Max keys pressed together by a button
#define PSPDISP_CONTROL_FILE_DEAD_ZONE     70 // Default dead zone of the analog stick, as the old desktop app

// The directions of the analog stick, as buttons (as the old desktop app), in 24 bits as the buttons (see `PSPdispButton`)
// They take the bits of the last 4 buttons (volume up and down, screen and note), which the PSP doesn't send in user mode
#define PSPDISP_CONTROL_FILE_ANALOG_UP   0b000100000000000000000000
#define PSPDISP_CONTROL_FILE_ANALOG_RIGHT 0b001000000000000000000000
#define PSPDISP_CONTROL_FILE_ANALOG_DOWN  0b010000000000000000000000
#define PSPDISP_CONTROL_FILE_ANALOG_LEFT  0b100000000000000000000000

#define PSPDISP_CONTROL_FILE_CHARACTER_SHIFT 24 // The on-screen keyboard of the PSP sends the character typed in the highest byte of the buttons

/**
 * @brief The sections of the buttons, with the button each one is about.
 */
const struct {
	const char *section; // The section in the control file
	uint32_t button; // The button bit (`PSPdispButton` or an analog direction)
} pspdisp_control_file_buttons[] = {
	{ "SELECT", PSPDISP_BUTTON_SELECT },
	{ "START", PSPDISP_BUTTON_START },
	{ "DPAD_UP", PSPDISP_BUTTON_UP },
	{ "DPAD_RIGHT", PSPDISP_BUTTON_RIGHT },
	{ "DPAD_DOWN", PSPDISP_BUTTON_DOWN },
	{ "DPAD_LEFT", PSPDISP_BUTTON_LEFT },
	{ "LEFT_TRIGGER", PSPDISP_BUTTON_L },
	{ "RIGHT_TRIGGER", PSPDISP_BUTTON_R },
	{ "TRIANGLE", PSPDISP_BUTTON_TRIANGLE },
	{ "CIRCLE", PSPDISP_BUTTON_CIRCLE },
	{ "CROSS", PSPDISP_BUTTON_CROSS },
	{ "SQUARE", PSPDISP_BUTTON_SQUARE },
	{ "HOME", PSPDISP_BUTTON_HOME },
	{ "ANALOG_UP", PSPDISP_CONTROL_FILE_ANALOG_UP },
	{ "ANALOG_RIGHT", PSPDISP_CONTROL_FILE_ANALOG_RIGHT },
	{ "ANALOG_DOWN", PSPDISP_CONTROL_FILE_ANALOG_DOWN },
	{ "ANALOG_LEFT", PSPDISP_CONTROL_FILE_ANALOG_LEFT },
};

/**
 * @brief What a button does.
 */
typedef enum {
	PSPDISP_CONTROL_FILE_EVENT_KEYBOARD, // Presses keys
	PSPDISP_CONTROL_FILE_EVENT_MOUSE, // Does a mouse action
	PSPDISP_CONTROL_FILE_EVENT_PSPDISP, // Does a PSPdisp action
} PSPdispControlFileEvent;

/**
 * @brief The mouse actions of a button.
 */
typedef enum {
	PSPDISP_CONTROL_FILE_MOUSE_LEFT,
	PSPDISP_CONTROL_FILE_MOUSE_RIGHT,
	PSPDISP_CONTROL_FILE_MOUSE_MIDDLE,
	PSPDISP_CONTROL_FILE_MOUSE_DOUBLE_CLICK,
	PSPDISP_CONTROL_FILE_MOUSE_WHEEL_UP,
	PSPDISP_CONTROL_FILE_MOUSE_WHEEL_DOWN,
	PSPDISP_CONTROL_FILE_MOUSE_MOVE_UP,
	PSPDISP_CONTROL_FILE_MOUSE_MOVE_DOWN,
	PSPDISP_CONTROL_FILE_MOUSE_MOVE_LEFT,
	PSPDISP_CONTROL_FILE_MOUSE_MOVE_RIGHT,
	PSPDISP_CONTROL_FILE_MOUSE_MOVE_ANALOG,
} PSPdispControlFileMouse;

const char *const pspdisp_control_file_mouse_names[] = {
	"Left", "Right", "Middle", "DoubleClick", "WheelUp", "WheelDown", "MouseMoveUp", "MouseMoveDown", "MouseMoveLeft", "MouseMoveRight", "MouseMoveAnalog",
};

/**
 * @brief What a button does, read from its section.
 */
typedef struct {
	uint32_t button; // The button bit
	PSPdispControlFileEvent event; // What it does
	uint16_t scancodes[PSPDISP_CONTROL_FILE_MAX_SCANCODES]; // The keys it presses, for the keyboard
	size_t scancode_count; // How many keys it presses
	PSPdispControlFileMouse mouse; // The mouse action
	int32_t move_by_pixels; // Pixels moved by `MouseMoveUp` and the others
	bool typematic; // If it's repeated with each frame while held
	bool down; // If the button is held
} PSPdispControlFileAction;

/**
 * @brief A control file, loaded, pressing a virtual keyboard and mouse.
 */
typedef struct {
	char name[PSPDISP_COM_NAME_SIZE]; // Its name
	bool map_analog_to_dpad; // If the analog stick acts as the directional pad
	int32_t dead_zone; // Distance from the center under which the analog stick is at rest, from 0 to 127
	bool enable_osk; // If the PSP may open its on-screen keyboard with select
	int32_t analog_mapping[6]; // Pixels moved by the analog stick with each frame, in each of the 5 sections out of the dead zone, then at the maximum
	PSPdispControlFileAction actions[G_N_ELEMENTS(pspdisp_control_file_buttons)]; // What the buttons do
	size_t action_count; // How many buttons do something
	uint32_t last_buttons; // The buttons held at the previous frame (with the analog directions)
	int32_t analog_x; // Pixels the analog stick moves the mouse at this frame, horizontally
	int32_t analog_y; // Pixels the analog stick moves the mouse at this frame, vertically
	bool analog_moved; // If the analog stick moved the mouse at this frame (so the 4 directions only move it once)
	uint32_t open_menu_requests; // Count of the `OpenMenu` actions
	PSPdispInput *input; // The virtual keyboard and mouse
} PSPdispControlFile;

/**
 * @brief Read an integer as the old desktop app (`TIniFile.ReadInteger`): the default when the key is missing, empty or not a number.
 */
int32_t pspdisp_control_file_read_integer(GKeyFile *file, const char *section, const char *key, int32_t fallback) {
	char *value = g_key_file_get_string(file, section, key, NULL);
	char *end = NULL;
	gint64 number = value != NULL ? g_ascii_strtoll(g_strstrip(value), &end, 10) : 0;
	bool valid = value != NULL && value[0] != '\0' && end != NULL && *end == '\0';

	g_free(value);
	return valid ? (int32_t) number : fallback;
}

/**
 * @brief Read what a button does from its section.
 *
 * @param file The control file.
 * @param section The section of the button.
 * @param button The button bit.
 * @param action Where to write what it does.
 * @return `true` if it does something valid.
 */
bool pspdisp_control_file_read_action(GKeyFile *file, const char *section, uint32_t button, PSPdispControlFileAction *action) {
	*action = (PSPdispControlFileAction) { .button = button, .move_by_pixels = 1 };
	action->typematic = pspdisp_control_file_read_integer(file, section, "Typematic", 0) > 0;

	char *event = g_key_file_get_string(file, section, "Event", NULL);
	bool valid = false;

	if (event != NULL)
		g_strstrip(event);

	if (g_strcmp0(event, "Keyboard") == 0) {
		action->event = PSPDISP_CONTROL_FILE_EVENT_KEYBOARD;

		// Keys pressed together, e.g. "38 + 3E" (as the old desktop app, the keys before a wrong one are kept)
		char *scancodes = g_key_file_get_string(file, section, "Scancode", NULL);
		char **parts = g_strsplit(scancodes != NULL ? scancodes : "", "+", -1);

		for (char **part = parts; *part != NULL && action->scancode_count < PSPDISP_CONTROL_FILE_MAX_SCANCODES; part++) {
			char *end = NULL;
			guint64 scancode = g_ascii_strtoull(g_strstrip(*part), &end, 16);

			if ((*part)[0] == '\0' || *end != '\0' || !pspdisp_input_has_key(scancode)) {
				if ((*part)[0] != '\0')
					g_debug("%s(): [%s] has an unknown scan code \"%s\"", __FUNCTION__, section, *part);
				break;
			}
			action->scancodes[action->scancode_count++] = scancode;
		}
		valid = action->scancode_count > 0;

		g_strfreev(parts);
		g_free(scancodes);
	} else if (g_strcmp0(event, "Mouse") == 0) {
		action->event = PSPDISP_CONTROL_FILE_EVENT_MOUSE;

		char *name = g_key_file_get_string(file, section, "Button", NULL);
		int mouse = name != NULL ? pspdisp_settings_find_name(pspdisp_control_file_mouse_names, G_N_ELEMENTS(pspdisp_control_file_mouse_names), g_strstrip(name)) : -1;

		if (mouse >= 0) {
			action->mouse = mouse;
			action->move_by_pixels = pspdisp_control_file_read_integer(file, section, "MoveByPixels", 1);
			valid = true;
		}
		g_free(name);
	} else if (g_strcmp0(event, "PSPdisp") == 0) {
		action->event = PSPDISP_CONTROL_FILE_EVENT_PSPDISP;

		char *name = g_key_file_get_string(file, section, "Action", NULL);
		valid = name != NULL && g_strcmp0(g_strstrip(name), "OpenMenu") == 0;
		g_free(name);
	}

	g_free(event);
	return valid;
}

/**
 * @brief Load a control file, and plug the virtual keyboard and mouse it presses.
 *
 * @param path The control file.
 * @param error Where to tell why it failed (e.g. the file can't be read, or the devices can't be plugged).
 * @return The control file, free it with `pspdisp_control_file_destroy()`, or `NULL` if it failed.
 */
PSPdispControlFile *pspdisp_control_file_new(const char *path, GError **error) {
	GKeyFile *file = pspdisp_settings_load_ini(path, error);
	if (file == NULL) return NULL;

	char *name = g_key_file_get_string(file, "General", "Name", NULL);
	if (name == NULL || name[0] == '\0') {
		g_set_error(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND, "%s has no name ([General] Name)", path);
		g_free(name);
		g_key_file_free(file);
		return NULL;
	}

	PSPdispControlFile *control = g_new0(PSPdispControlFile, 1);
	g_strlcpy(control->name, name, sizeof(control->name));
	g_free(name);

	control->map_analog_to_dpad = pspdisp_control_file_read_integer(file, "General", "MapAnalogToDPad", 0) == 1;
	control->dead_zone = pspdisp_control_file_read_integer(file, "General", "AnalogDeadZone", PSPDISP_CONTROL_FILE_DEAD_ZONE);
	if (control->dead_zone < 0 || control->dead_zone > 127)
		control->dead_zone = PSPDISP_CONTROL_FILE_DEAD_ZONE;
	control->enable_osk = pspdisp_control_file_read_integer(file, "General", "EnableOSK", 0) == 1;

	static const char *const sections[] = { "Section1", "Section2", "Section3", "Section4", "Section5", "Maximum" };
	for (size_t i = 0; i < G_N_ELEMENTS(sections); i++)
		control->analog_mapping[i] = pspdisp_control_file_read_integer(file, "AnalogStickMapping", sections[i], i + 1);

	for (size_t i = 0; i < G_N_ELEMENTS(pspdisp_control_file_buttons); i++) {
		PSPdispControlFileAction *action = &control->actions[control->action_count];
		if (pspdisp_control_file_read_action(file, pspdisp_control_file_buttons[i].section, pspdisp_control_file_buttons[i].button, action))
			control->action_count++;
	}

	g_key_file_free(file);

	control->input = pspdisp_input_new("PSPdisp", error);
	if (control->input == NULL) {
		g_free(control);
		return NULL;
	}

	return control;
}

/**
 * @brief Release what's still pressed, unplug the virtual keyboard and mouse, and free the control file.
 *
 * @param control The control file.
 */
void pspdisp_control_file_destroy(PSPdispControlFile *control) {
	pspdisp_input_destroy(control->input);
	g_free(control);
}

/**
 * @brief Get the pixels the mouse moves for a distance of the analog stick out of the dead zone, from the analog stick mapping.
 *
 * The range out of the dead zone is split into 5 sections, then the maximum, as the old desktop app.
 *
 * @param control The control file.
 * @param distance The distance out of the dead zone, negative to the left or up.
 * @return The pixels, negative to the left or up.
 */
int32_t pspdisp_control_file_analog_pixels(PSPdispControlFile *control, int32_t distance) {
	int32_t sign = distance < 0 ? -1 : 1;
	double range = 128 - control->dead_zone;
	double value = distance * sign;
	size_t section = 5;

	for (size_t i = 0; i < 5; i++) {
		if (value < range * (i + 1) / 5) {
			section = i;
			break;
		}
	}

	return sign * control->analog_mapping[section];
}

/**
 * @brief Turn the analog stick into buttons: the directions of the directional pad, or the analog directions with how far the mouse moves.
 *
 * @param control The control file.
 * @param buttons The buttons, where the directions are added.
 * @param analog_x The analog stick, from the left (0) to the right (255).
 * @param analog_y The analog stick, from the top (0) to the bottom (255).
 */
void pspdisp_control_file_map_analog(PSPdispControlFile *control, uint32_t *buttons, uint8_t analog_x, uint8_t analog_y) {
	int32_t zone = control->dead_zone;
	const int32_t high = 128 + zone; // Above it, the stick is pushed right or down
	const int32_t low = 128 - zone; // Below it, the stick is pushed left or up

	control->analog_x = 0;
	control->analog_y = 0;

	if (control->map_analog_to_dpad) {
		if (analog_x > high)
			*buttons |= PSPDISP_BUTTON_RIGHT;
		else if (analog_x < low)
			*buttons |= PSPDISP_BUTTON_LEFT;
		if (analog_y > high)
			*buttons |= PSPDISP_BUTTON_DOWN;
		else if (analog_y < low)
			*buttons |= PSPDISP_BUTTON_UP;
		return;
	}

	if (analog_x > high) {
		*buttons |= PSPDISP_CONTROL_FILE_ANALOG_RIGHT;
		control->analog_x = pspdisp_control_file_analog_pixels(control, analog_x - high + 1);
	} else if (analog_x < low) {
		*buttons |= PSPDISP_CONTROL_FILE_ANALOG_LEFT;
		control->analog_x = pspdisp_control_file_analog_pixels(control, analog_x - low);
	}

	if (analog_y > high) {
		*buttons |= PSPDISP_CONTROL_FILE_ANALOG_DOWN;
		control->analog_y = pspdisp_control_file_analog_pixels(control, analog_y - high + 1);
	} else if (analog_y < low) {
		*buttons |= PSPDISP_CONTROL_FILE_ANALOG_UP;
		control->analog_y = pspdisp_control_file_analog_pixels(control, analog_y - low);
	}
}

/**
 * @brief Do the action of a button pressed (or held, for a typematic one).
 *
 * @param control The control file.
 * @param action The action.
 */
void pspdisp_control_file_press(PSPdispControlFile *control, PSPdispControlFileAction *action) {
	PSPdispInput *input = control->input;

	switch (action->event) {
		case PSPDISP_CONTROL_FILE_EVENT_KEYBOARD:
			for (size_t i = 0; i < action->scancode_count; i++)
				pspdisp_input_key(input, action->scancodes[i], true);
			break;
		case PSPDISP_CONTROL_FILE_EVENT_PSPDISP:
			control->open_menu_requests++;
			break;
		case PSPDISP_CONTROL_FILE_EVENT_MOUSE:
			switch (action->mouse) {
				case PSPDISP_CONTROL_FILE_MOUSE_LEFT: pspdisp_input_mouse_button(input, PSPDISP_INPUT_MOUSE_LEFT, true); break;
				case PSPDISP_CONTROL_FILE_MOUSE_RIGHT: pspdisp_input_mouse_button(input, PSPDISP_INPUT_MOUSE_RIGHT, true); break;
				case PSPDISP_CONTROL_FILE_MOUSE_MIDDLE: pspdisp_input_mouse_button(input, PSPDISP_INPUT_MOUSE_MIDDLE, true); break;
				case PSPDISP_CONTROL_FILE_MOUSE_DOUBLE_CLICK:
					for (int i = 0; i < 2; i++) {
						pspdisp_input_mouse_button(input, PSPDISP_INPUT_MOUSE_LEFT, true);
						pspdisp_input_mouse_button(input, PSPDISP_INPUT_MOUSE_LEFT, false);
					}
					break;
				case PSPDISP_CONTROL_FILE_MOUSE_WHEEL_UP: pspdisp_input_mouse_wheel(input, 1); break;
				case PSPDISP_CONTROL_FILE_MOUSE_WHEEL_DOWN: pspdisp_input_mouse_wheel(input, -1); break;
				case PSPDISP_CONTROL_FILE_MOUSE_MOVE_UP: pspdisp_input_mouse_move(input, 0, -action->move_by_pixels); break;
				case PSPDISP_CONTROL_FILE_MOUSE_MOVE_DOWN: pspdisp_input_mouse_move(input, 0, action->move_by_pixels); break;
				case PSPDISP_CONTROL_FILE_MOUSE_MOVE_LEFT: pspdisp_input_mouse_move(input, -action->move_by_pixels, 0); break;
				case PSPDISP_CONTROL_FILE_MOUSE_MOVE_RIGHT: pspdisp_input_mouse_move(input, action->move_by_pixels, 0); break;
				case PSPDISP_CONTROL_FILE_MOUSE_MOVE_ANALOG:
					if (!control->analog_moved)
						pspdisp_input_mouse_move(input, control->analog_x, control->analog_y);
					control->analog_moved = true;
					break;
			}
			break;
	}
}

/**
 * @brief Undo the action of a button released: release its keys or its mouse button (the other actions are done once pressed).
 *
 * @param control The control file.
 * @param action The action.
 */
void pspdisp_control_file_release(PSPdispControlFile *control, PSPdispControlFileAction *action) {
	if (action->event == PSPDISP_CONTROL_FILE_EVENT_KEYBOARD) {
		for (size_t i = action->scancode_count; i > 0; i--)
			pspdisp_input_key(control->input, action->scancodes[i - 1], false);
	} else if (action->event == PSPDISP_CONTROL_FILE_EVENT_MOUSE) {
		if (action->mouse == PSPDISP_CONTROL_FILE_MOUSE_LEFT)
			pspdisp_input_mouse_button(control->input, PSPDISP_INPUT_MOUSE_LEFT, false);
		else if (action->mouse == PSPDISP_CONTROL_FILE_MOUSE_RIGHT)
			pspdisp_input_mouse_button(control->input, PSPDISP_INPUT_MOUSE_RIGHT, false);
		else if (action->mouse == PSPDISP_CONTROL_FILE_MOUSE_MIDDLE)
			pspdisp_input_mouse_button(control->input, PSPDISP_INPUT_MOUSE_MIDDLE, false);
	}
}

/**
 * @brief Type a character of the on-screen keyboard of the PSP, as the keys of a US keyboard.
 *
 * The PSP sends 1 for left, 2 for right, 8 for backspace and 10 for enter, the other codes are the characters.
 * As the keys are the ones of a US keyboard, some characters differ with another layout of the desktop (e.g. `z` and `y` on a German one).
 *
 * @param control The control file.
 * @param character The character code.
 */
void pspdisp_control_file_type(PSPdispControlFile *control, uint8_t character) {
	static const char *const unshifted = "1234567890-=qwertyuiop[]asdfghjkl;'`\\zxcvbnm,./ ";
	static const char *const shifted = "!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"~|ZXCVBNM<>? ";
	static const uint16_t scancodes[] = {
		0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
		0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2B,
		0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x39,
	};
	static_assert(G_N_ELEMENTS(scancodes) == 48, "A scan code for each character of the US layout");

	uint16_t scancode = 0;
	bool shift = false;

	if (character == 1)
		scancode = 0xE04B; // Left
	else if (character == 2)
		scancode = 0xE04D; // Right
	else if (character == 8)
		scancode = 0x0E; // Backspace
	else if (character == 10)
		scancode = 0x1C; // Enter
	else if (character >= ' ' && character < 0x7F) {
		const char *found = strchr(unshifted, character);
		shift = found == NULL;
		if (found == NULL)
			found = strchr(shifted, character);
		if (found != NULL)
			scancode = scancodes[found - (shift ? shifted : unshifted)];
	}

	if (scancode == 0) {
		g_debug("%s(): the character %u of the on-screen keyboard has no key", __FUNCTION__, character);
		return;
	}

	if (shift)
		pspdisp_input_key(control->input, 0x2A, true);
	pspdisp_input_key(control->input, scancode, true);
	pspdisp_input_key(control->input, scancode, false);
	if (shift)
		pspdisp_input_key(control->input, 0x2A, false);
}

/**
 * @brief Press what the PSP buttons press, as the old desktop app, called with each response of the PSP.
 *
 * A button pressed does its action, a button released undoes it, and while nothing changes the typematic buttons held do their actions again.
 *
 * @param control The control file.
 * @param buttons The PSP buttons, `PSPdispButton` bits, with a character of the on-screen keyboard in the highest byte.
 * @param analog_x The analog stick, from the left (0) to the right (255).
 * @param analog_y The analog stick, from the top (0) to the bottom (255).
 */
void pspdisp_control_file_update(PSPdispControlFile *control, uint32_t buttons, uint8_t analog_x, uint8_t analog_y) {
	uint8_t character = buttons >> PSPDISP_CONTROL_FILE_CHARACTER_SHIFT;

	if (character != 0) {
		if (character < 0xFF)
			pspdisp_control_file_type(control, character);
		return;
	}

	control->analog_moved = false;
	pspdisp_control_file_map_analog(control, &buttons, analog_x, analog_y);

	for (size_t i = 0; i < control->action_count; i++) {
		PSPdispControlFileAction *action = &control->actions[i];
		bool held = buttons & action->button;
		bool was_held = control->last_buttons & action->button;

		if (buttons != control->last_buttons) {
			if (held && !was_held)
				pspdisp_control_file_press(control, action);
			else if (!held && was_held)
				pspdisp_control_file_release(control, action);
			action->down = held;
		} else if (action->typematic && action->down) {
			pspdisp_control_file_press(control, action);
		}
	}

	control->last_buttons = buttons;
}
