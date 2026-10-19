/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * uinput.c - Virtual gamepad using uinput (Linux)   *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * Plugs the virtual gamepad through uinput (`/dev/uinput`), as the `xpad` kernel driver shows an Xbox 360 controller: the same identity, the same buttons and the same axes.
 * So the programs map it as they map a real one (e.g. SDL, from its mapping of that controller), with nothing to configure.
 *
 * The program numbers the buttons and the axes in the order of their codes, which the mappings rely on, so all of them are there, even those the PSP doesn't have.
 * As `xpad`, the left face button (X on Xbox) is `BTN_X` and the top one (Y) is `BTN_Y`, even though the kernel also names those codes `BTN_NORTH` and `BTN_WEST`.
 *
 * Writing to `/dev/uinput` needs root, or the `input` group on some distributions.
 */

#pragma once

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>
#include "../gamepad.c" // Its declarations (when `gamepad.c` includes this file, it already has them and this include is skipped)

#define PSPDISP_GAMEPAD_NAME "uinput" // User friendly name of this virtual gamepad

#define PSPDISP_GAMEPAD_UINPUT_VENDOR_ID  0x045E // Microsoft
#define PSPDISP_GAMEPAD_UINPUT_PRODUCT_ID 0x028E // Xbox 360 controller
#define PSPDISP_GAMEPAD_UINPUT_VERSION    0x0114 // As `xpad` gives it for an Xbox 360 controller

/**
 * @brief Virtual gamepad using uinput, created by `pspdisp_gamepad_new()`.
 */
struct PSPdispGamepad {
	int fd; // The uinput device, its gamepad exists while it's open
	PSPdispGamepadState state; // The state the computer has, to send only what changes
};

/**
 * @brief The buttons of `xpad` for an Xbox 360 controller, with the gamepad button each one shows (0 for one the PSP doesn't have).
 */
const struct {
	uint16_t code; // The key code
	PSPdispGamepadButton button; // The button of the gamepad
} pspdisp_gamepad_uinput_buttons[] = {
	{ BTN_A,      PSPDISP_GAMEPAD_BUTTON_SOUTH },
	{ BTN_B,      PSPDISP_GAMEPAD_BUTTON_EAST },
	{ BTN_X,      PSPDISP_GAMEPAD_BUTTON_WEST },
	{ BTN_Y,      PSPDISP_GAMEPAD_BUTTON_NORTH },
	{ BTN_TL,     PSPDISP_GAMEPAD_BUTTON_LEFT_SHOULDER },
	{ BTN_TR,     PSPDISP_GAMEPAD_BUTTON_RIGHT_SHOULDER },
	{ BTN_SELECT, PSPDISP_GAMEPAD_BUTTON_BACK },
	{ BTN_START,  PSPDISP_GAMEPAD_BUTTON_START },
	{ BTN_MODE,   0 }, // The guide button
	{ BTN_THUMBL, 0 }, // The left stick button
	{ BTN_THUMBR, 0 }, // The right stick button
};

/**
 * @brief The axes of `xpad` for an Xbox 360 controller, with their ranges.
 */
const struct uinput_abs_setup pspdisp_gamepad_uinput_axes[] = {
	{ .code = ABS_X,     .absinfo = { .minimum = -32768, .maximum = 32767, .fuzz = 16, .flat = 128 } }, // Left stick, the analog stick of the PSP
	{ .code = ABS_Y,     .absinfo = { .minimum = -32768, .maximum = 32767, .fuzz = 16, .flat = 128 } },
	{ .code = ABS_Z,     .absinfo = { .minimum = 0, .maximum = 255 } }, // Left trigger
	{ .code = ABS_RX,    .absinfo = { .minimum = -32768, .maximum = 32767, .fuzz = 16, .flat = 128 } }, // Right stick
	{ .code = ABS_RY,    .absinfo = { .minimum = -32768, .maximum = 32767, .fuzz = 16, .flat = 128 } },
	{ .code = ABS_RZ,    .absinfo = { .minimum = 0, .maximum = 255 } }, // Right trigger
	{ .code = ABS_HAT0X, .absinfo = { .minimum = -1, .maximum = 1 } }, // Directional pad, from the left (-1) to the right (1)
	{ .code = ABS_HAT0Y, .absinfo = { .minimum = -1, .maximum = 1 } }, // Directional pad, from the top (-1) to the bottom (1)
};

/**
 * @brief Get the position of the directional pad on one axis, as the hat of `xpad`.
 *
 * @param buttons The pressed buttons, `PSPdispGamepadButton` bits.
 * @param negative The button towards -1 (left or up).
 * @param positive The button towards 1 (right or down).
 * @return -1, 0 or 1 (0 when both or none are pressed).
 */
int32_t pspdisp_gamepad_uinput_hat(uint16_t buttons, PSPdispGamepadButton negative, PSPdispGamepadButton positive) {
	return ((buttons & positive) != 0) - ((buttons & negative) != 0);
}

/**
 * @brief Plug a virtual Xbox 360 controller through uinput.
 *
 * @param name The name the computer shows for it.
 * @param error Where to tell why it failed.
 * @return The gamepad, or `NULL` if it failed.
 */
PSPdispGamepad *pspdisp_gamepad_new(const char *name, GError **error) {
	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);

	if (fd < 0) {
		int code = errno;

		if (code == EACCES || code == EPERM)
			g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(code), "no access to /dev/uinput, install the udev rule (sudo make install) or join the input group");
		else if (code == ENOENT || code == ENODEV)
			g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(code), "/dev/uinput is missing, load the uinput kernel module (modprobe uinput)");
		else
			g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(code), "failed to open /dev/uinput: %s", g_strerror(code));
		return NULL;
	}

	bool done = ioctl(fd, UI_SET_EVBIT, EV_KEY) == 0 && ioctl(fd, UI_SET_EVBIT, EV_ABS) == 0;

	for (size_t i = 0; done && i < G_N_ELEMENTS(pspdisp_gamepad_uinput_buttons); i++)
		done = ioctl(fd, UI_SET_KEYBIT, pspdisp_gamepad_uinput_buttons[i].code) == 0;
	for (size_t i = 0; done && i < G_N_ELEMENTS(pspdisp_gamepad_uinput_axes); i++)
		done = ioctl(fd, UI_ABS_SETUP, &pspdisp_gamepad_uinput_axes[i]) == 0;

	struct uinput_setup setup = {
		.id = { .bustype = BUS_USB, .vendor = PSPDISP_GAMEPAD_UINPUT_VENDOR_ID, .product = PSPDISP_GAMEPAD_UINPUT_PRODUCT_ID, .version = PSPDISP_GAMEPAD_UINPUT_VERSION },
	};
	g_strlcpy(setup.name, name, sizeof(setup.name));

	done = done && ioctl(fd, UI_DEV_SETUP, &setup) == 0 && ioctl(fd, UI_DEV_CREATE) == 0;

	if (!done) {
		int code = errno;
		g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(code), "failed to create the gamepad with uinput: %s", g_strerror(code));
		close(fd);
		return NULL;
	}

	PSPdispGamepad *gamepad = g_new0(PSPdispGamepad, 1);
	gamepad->fd = fd;
	return gamepad;
}

/**
 * @brief Send what changed in the state of the gamepad, as input events ended by a synchronization.
 *
 * @param gamepad The gamepad.
 * @param state The new state.
 * @return `true` if the events were written.
 */
bool pspdisp_gamepad_update(PSPdispGamepad *gamepad, const PSPdispGamepadState *state) {
	const PSPdispGamepadState *old = &gamepad->state;
	struct input_event events[G_N_ELEMENTS(pspdisp_gamepad_uinput_buttons) + 4 + 1] = {0}; // The buttons, both hat axes, both stick axes, and the synchronization
	size_t count = 0;

	for (size_t i = 0; i < G_N_ELEMENTS(pspdisp_gamepad_uinput_buttons); i++) {
		PSPdispGamepadButton button = pspdisp_gamepad_uinput_buttons[i].button;
		bool pressed = (state->buttons & button) != 0;

		if (button != 0 && pressed != ((old->buttons & button) != 0))
			events[count++] = (struct input_event) { .type = EV_KEY, .code = pspdisp_gamepad_uinput_buttons[i].code, .value = pressed };
	}

	int32_t hat_x = pspdisp_gamepad_uinput_hat(state->buttons, PSPDISP_GAMEPAD_BUTTON_DPAD_LEFT, PSPDISP_GAMEPAD_BUTTON_DPAD_RIGHT);
	int32_t hat_y = pspdisp_gamepad_uinput_hat(state->buttons, PSPDISP_GAMEPAD_BUTTON_DPAD_UP, PSPDISP_GAMEPAD_BUTTON_DPAD_DOWN);

	if (hat_x != pspdisp_gamepad_uinput_hat(old->buttons, PSPDISP_GAMEPAD_BUTTON_DPAD_LEFT, PSPDISP_GAMEPAD_BUTTON_DPAD_RIGHT))
		events[count++] = (struct input_event) { .type = EV_ABS, .code = ABS_HAT0X, .value = hat_x };
	if (hat_y != pspdisp_gamepad_uinput_hat(old->buttons, PSPDISP_GAMEPAD_BUTTON_DPAD_UP, PSPDISP_GAMEPAD_BUTTON_DPAD_DOWN))
		events[count++] = (struct input_event) { .type = EV_ABS, .code = ABS_HAT0Y, .value = hat_y };
	if (state->left_x != old->left_x)
		events[count++] = (struct input_event) { .type = EV_ABS, .code = ABS_X, .value = state->left_x };
	if (state->left_y != old->left_y)
		events[count++] = (struct input_event) { .type = EV_ABS, .code = ABS_Y, .value = state->left_y };

	if (count == 0) return true;

	events[count++] = (struct input_event) { .type = EV_SYN, .code = SYN_REPORT, .value = 0 };

	if (write(gamepad->fd, events, count * sizeof(struct input_event)) != (ssize_t) (count * sizeof(struct input_event))) {
		g_warning("%s(): failed to send the state of the gamepad: %s", __FUNCTION__, g_strerror(errno));
		return false;
	}

	gamepad->state = *state;
	return true;
}

/**
 * @brief Unplug the virtual gamepad and free it.
 *
 * @param gamepad The gamepad to destroy.
 */
void pspdisp_gamepad_destroy(PSPdispGamepad *gamepad) {
	ioctl(gamepad->fd, UI_DEV_DESTROY);
	close(gamepad->fd);
	g_free(gamepad);
}
