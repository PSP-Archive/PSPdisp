/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * input.c - Virtual keyboard and mouse of platform  *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * A virtual keyboard and a virtual mouse that the computer sees as plugged devices, pressed by the control files (see `control_file.c`).
 *
 * The keys are PC scan codes (set 1, with 0xE0 before the extended keys, e.g. 0xE048 for up), as in the control files of the old desktop app.
 * They tell the place of a key on the keyboard, whatever its layout (e.g. 0x10 is the key of Q on a QWERTY keyboard, and of A on an AZERTY one).
 *
 * Its implementation varies with the platform, chosen at compile time: uinput on Linux (`input/uinput.c`).
 */

#pragma once

#include <stdint.h>
#include <glib.h>

/**
 * @brief Buttons of the virtual mouse.
 */
typedef enum {
	PSPDISP_INPUT_MOUSE_LEFT,   // Left button
	PSPDISP_INPUT_MOUSE_RIGHT,  // Right button
	PSPDISP_INPUT_MOUSE_MIDDLE, // Middle button (the wheel pressed)
} PSPdispInputMouseButton;

/**
 * @brief Virtual keyboard and mouse.
 *
 * Its platform implementation (included below, e.g. `input/uinput.c`) declares this struct with the data it needs, and defines `PSPDISP_INPUT_NAME`.
 */
typedef struct PSPdispInput PSPdispInput;

/**
 * @brief Plug a virtual keyboard and a virtual mouse into the computer, implemented by the platform.
 *
 * @param name The name the computer shows for them, before " keyboard" and " mouse".
 * @param error Where to tell why it failed, ready to be shown to the user (e.g. the permission it needs).
 * @return The devices, unplug them with `pspdisp_input_destroy()`, or `NULL` if it failed.
 */
PSPdispInput *pspdisp_input_new(const char *name, GError **error);

/**
 * @brief Unplug the virtual keyboard and mouse, implemented by the platform.
 *
 * The keys and buttons still pressed are released first.
 *
 * @param input The devices to destroy.
 */
void pspdisp_input_destroy(PSPdispInput *input);

/**
 * @brief Tell if a scan code is a key of the virtual keyboard, implemented by the platform.
 *
 * @param scancode The scan code (e.g. 0x1E, or 0xE048 for an extended key).
 * @return `true` if it can be pressed.
 */
bool pspdisp_input_has_key(uint16_t scancode);

/**
 * @brief Get the scan code of a key pressed on the keyboard of the computer, to write it in a control file, implemented by the platform.
 *
 * @param keycode The hardware key code given by the toolkit (e.g. `GdkEvent` key code).
 * @return The scan code (e.g. 0x1E, or 0xE048 for an extended key), or 0 if it has none.
 */
uint16_t pspdisp_input_scancode_of_key(uint32_t keycode);

/**
 * @brief Press or release a key of the virtual keyboard, implemented by the platform.
 *
 * @param input The devices.
 * @param scancode The scan code of the key.
 * @param pressed If it's pressed (`true`) or released (`false`).
 * @return `true` if the computer took it.
 */
bool pspdisp_input_key(PSPdispInput *input, uint16_t scancode, bool pressed);

/**
 * @brief Type a character, as the keyboard layout of the computer writes it, implemented by the platform.
 *
 * @param input The devices.
 * @param character The character, a Unicode code point.
 * @return `true` if the computer took it, `false` if no key types it.
 */
bool pspdisp_input_type(PSPdispInput *input, uint32_t character);

/**
 * @brief Press or release a button of the virtual mouse, implemented by the platform.
 *
 * @param input The devices.
 * @param button The button.
 * @param pressed If it's pressed (`true`) or released (`false`).
 * @return `true` if the computer took it.
 */
bool pspdisp_input_mouse_button(PSPdispInput *input, PSPdispInputMouseButton button, bool pressed);

/**
 * @brief Move the virtual mouse, implemented by the platform.
 *
 * @param input The devices.
 * @param x Pixels to the right (negative to the left).
 * @param y Pixels down (negative up).
 * @return `true` if the computer took it.
 */
bool pspdisp_input_mouse_move(PSPdispInput *input, int32_t x, int32_t y);

/**
 * @brief Turn the wheel of the virtual mouse, implemented by the platform.
 *
 * @param input The devices.
 * @param steps Steps away from the user, scrolling up (negative towards the user, scrolling down).
 * @return `true` if the computer took it.
 */
bool pspdisp_input_mouse_wheel(PSPdispInput *input, int32_t steps);

// The virtual keyboard and mouse of this platform, it declares `PSPdispInput`, implements its functions and defines `PSPDISP_INPUT_NAME`
#if defined(__linux__)
	#include "./input/uinput.c"
#else
	#error "Unsupported platform: there's no virtual keyboard and mouse for it yet"
#endif
