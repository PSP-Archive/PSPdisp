/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * uinput.c - Keyboard and mouse using uinput        *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * Plugs a virtual keyboard and a virtual mouse through uinput (`/dev/uinput`), two devices as a real keyboard and a real mouse.
 *
 * The key codes of Linux were made from the PC scan codes, so the scan codes from 0x01 to 0x58 are the same numbers, and only the extended keys need a table.
 * The keyboard doesn't repeat the keys held: the desktop does it, as with a real keyboard.
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
#include "../input.c" // Its declarations (when `input.c` includes this file, it already has them and this include is skipped)

#define PSPDISP_INPUT_NAME "uinput" // User friendly name of this virtual keyboard and mouse

#define PSPDISP_INPUT_UINPUT_LAST_BASIC 0x58 // Last scan code without 0xE0 that has the same number as its key code (F12)

/**
 * @brief Virtual keyboard and mouse using uinput, created by `pspdisp_input_new()`.
 */
struct PSPdispInput {
    int keyboard; // The uinput keyboard
    int mouse; // The uinput mouse
    uint8_t pressed_keys[(KEY_MAX + 1) / 8]; // The keys pressed (one bit per key code), released when unplugged
    uint8_t pressed_buttons; // The mouse buttons pressed (one bit per `PSPdispInputMouseButton`), released when unplugged
};

/**
 * @brief The extended keys (scan codes after 0xE0), with their key codes.
 */
static const struct {
    uint16_t scancode; // The scan code, with its 0xE0
    uint16_t code; // The key code
} pspdisp_input_uinput_extended_keys[] = {
    { 0xE010, KEY_PREVIOUSSONG }, { 0xE019, KEY_NEXTSONG }, { 0xE01C, KEY_KPENTER }, { 0xE01D, KEY_RIGHTCTRL },
    { 0xE020, KEY_MUTE }, { 0xE021, KEY_CALC }, { 0xE022, KEY_PLAYPAUSE }, { 0xE024, KEY_STOPCD },
    { 0xE02E, KEY_VOLUMEDOWN }, { 0xE030, KEY_VOLUMEUP }, { 0xE032, KEY_HOMEPAGE }, { 0xE035, KEY_KPSLASH },
    { 0xE037, KEY_SYSRQ }, { 0xE038, KEY_RIGHTALT }, { 0xE046, KEY_PAUSE }, { 0xE047, KEY_HOME },
    { 0xE048, KEY_UP }, { 0xE049, KEY_PAGEUP }, { 0xE04B, KEY_LEFT }, { 0xE04D, KEY_RIGHT },
    { 0xE04F, KEY_END }, { 0xE050, KEY_DOWN }, { 0xE051, KEY_PAGEDOWN }, { 0xE052, KEY_INSERT },
    { 0xE053, KEY_DELETE }, { 0xE05B, KEY_LEFTMETA }, { 0xE05C, KEY_RIGHTMETA }, { 0xE05D, KEY_COMPOSE },
    { 0xE05E, KEY_POWER }, { 0xE05F, KEY_SLEEP }, { 0xE063, KEY_WAKEUP }, { 0xE065, KEY_SEARCH },
    { 0xE066, KEY_BOOKMARKS }, { 0xE067, KEY_REFRESH }, { 0xE068, KEY_STOP }, { 0xE069, KEY_FORWARD },
    { 0xE06A, KEY_BACK }, { 0xE06B, KEY_COMPUTER }, { 0xE06C, KEY_MAIL }, { 0xE06D, KEY_MEDIA },
};

/**
 * @brief The key codes of the mouse buttons, by `PSPdispInputMouseButton`.
 */
static const uint16_t pspdisp_input_uinput_buttons[] = {
    [PSPDISP_INPUT_MOUSE_LEFT] = BTN_LEFT,
    [PSPDISP_INPUT_MOUSE_RIGHT] = BTN_RIGHT,
    [PSPDISP_INPUT_MOUSE_MIDDLE] = BTN_MIDDLE,
};

/**
 * @brief Get the key code of a scan code.
 *
 * @param scancode The scan code.
 * @return The key code, or 0 (`KEY_RESERVED`) if it's not a key of the virtual keyboard.
 */
uint16_t pspdisp_input_uinput_code(uint16_t scancode) {
    if (scancode >= KEY_ESC && scancode <= PSPDISP_INPUT_UINPUT_LAST_BASIC && scancode != 0x54 && scancode != 0x55) return scancode; // 0x54 and 0x55 are not keys of a PC keyboard

    for (size_t i = 0; i < G_N_ELEMENTS(pspdisp_input_uinput_extended_keys); i++) {
        if (pspdisp_input_uinput_extended_keys[i].scancode == scancode)
            return pspdisp_input_uinput_extended_keys[i].code;
    }

    return KEY_RESERVED;
}

bool pspdisp_input_has_key(uint16_t scancode) {
    return pspdisp_input_uinput_code(scancode) != KEY_RESERVED;
}

/**
 * @brief Get the scan code of a key pressed, from its hardware key code, which is its Linux key code plus 8 (on X11 and on Wayland).
 */
uint16_t pspdisp_input_scancode_of_key(uint32_t keycode) {
    if (keycode < 8) return 0;
    uint32_t code = keycode - 8;

    if (code <= PSPDISP_INPUT_UINPUT_LAST_BASIC)
        return pspdisp_input_has_key(code) ? code : 0;
    for (size_t i = 0; i < G_N_ELEMENTS(pspdisp_input_uinput_extended_keys); i++) {
        if (pspdisp_input_uinput_extended_keys[i].code == code)
            return pspdisp_input_uinput_extended_keys[i].scancode;
    }
    return 0;
}

/**
 * @brief Write input events to a uinput device, then a synchronization, so the computer takes them together.
 *
 * @param fd The uinput device.
 * @param events The events, with room for one more (the synchronization).
 * @param count The number of events.
 * @return `true` if they were written.
 */
bool pspdisp_input_uinput_write(int fd, struct input_event *events, size_t count) {
    events[count++] = (struct input_event) { .type = EV_SYN, .code = SYN_REPORT, .value = 0 };

    if (write(fd, events, count * sizeof(struct input_event)) != (ssize_t) (count * sizeof(struct input_event))) {
        g_warning("%s(): failed to send input events: %s", __FUNCTION__, g_strerror(errno));
        return false;
    }

    return true;
}

/**
 * @brief Create one uinput device.
 *
 * @param name The name the computer shows for it.
 * @param setup Enables its events (e.g. its keys), `false` on failure.
 * @param error Where to tell why it failed.
 * @return The device, or -1 if it failed.
 */
int pspdisp_input_uinput_create(const char *name, bool (*setup) (int fd), GError **error) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);

    if (fd < 0) {
        int code = errno;

        if (code == EACCES || code == EPERM)
            g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(code), "no access to /dev/uinput, install the udev rule (sudo make install) or join the input group");
        else if (code == ENOENT || code == ENODEV)
            g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(code), "/dev/uinput is missing, load the uinput kernel module (modprobe uinput)");
        else
            g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(code), "failed to open /dev/uinput: %s", g_strerror(code));
        return -1;
    }

    struct uinput_setup device = { .id = { .bustype = BUS_VIRTUAL } };
    g_strlcpy(device.name, name, sizeof(device.name));

    if (!setup(fd) || ioctl(fd, UI_DEV_SETUP, &device) != 0 || ioctl(fd, UI_DEV_CREATE) != 0) {
        int code = errno;
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(code), "failed to create %s with uinput: %s", name, g_strerror(code));
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * @brief Enable the keys of the virtual keyboard: all the keys with a scan code.
 */
bool pspdisp_input_uinput_setup_keyboard(int fd) {
    bool done = ioctl(fd, UI_SET_EVBIT, EV_KEY) == 0;

    for (uint16_t scancode = KEY_ESC; done && scancode <= PSPDISP_INPUT_UINPUT_LAST_BASIC; scancode++) {
        if (pspdisp_input_has_key(scancode))
            done = ioctl(fd, UI_SET_KEYBIT, scancode) == 0;
    }
    for (size_t i = 0; done && i < G_N_ELEMENTS(pspdisp_input_uinput_extended_keys); i++)
        done = ioctl(fd, UI_SET_KEYBIT, pspdisp_input_uinput_extended_keys[i].code) == 0;

    return done;
}

/**
 * @brief Enable the buttons, the motion and the wheel of the virtual mouse.
 */
bool pspdisp_input_uinput_setup_mouse(int fd) {
    bool done = ioctl(fd, UI_SET_EVBIT, EV_KEY) == 0 && ioctl(fd, UI_SET_EVBIT, EV_REL) == 0;

    for (size_t i = 0; done && i < G_N_ELEMENTS(pspdisp_input_uinput_buttons); i++)
        done = ioctl(fd, UI_SET_KEYBIT, pspdisp_input_uinput_buttons[i]) == 0;

    return done && ioctl(fd, UI_SET_RELBIT, REL_X) == 0 && ioctl(fd, UI_SET_RELBIT, REL_Y) == 0 && ioctl(fd, UI_SET_RELBIT, REL_WHEEL) == 0 && ioctl(fd, UI_SET_RELBIT, REL_WHEEL_HI_RES) == 0;
}

/**
 * @brief Plug a virtual keyboard and a virtual mouse through uinput.
 *
 * @param name The name the computer shows for them, before " keyboard" and " mouse".
 * @param error Where to tell why it failed.
 * @return The devices, or `NULL` if it failed (e.g. no access to `/dev/uinput`).
 */
PSPdispInput *pspdisp_input_new(const char *name, GError **error) {
    char *keyboard_name = g_strdup_printf("%s keyboard", name);
    char *mouse_name = g_strdup_printf("%s mouse", name);
    int keyboard = pspdisp_input_uinput_create(keyboard_name, pspdisp_input_uinput_setup_keyboard, error);
    int mouse = keyboard >= 0 ? pspdisp_input_uinput_create(mouse_name, pspdisp_input_uinput_setup_mouse, error) : -1;

    g_free(keyboard_name);
    g_free(mouse_name);

    if (mouse < 0) {
        if (keyboard >= 0) {
            ioctl(keyboard, UI_DEV_DESTROY);
            close(keyboard);
        }
        return NULL;
    }

    PSPdispInput *input = g_new0(PSPdispInput, 1);
    input->keyboard = keyboard;
    input->mouse = mouse;
    return input;
}

/**
 * @brief Release what's still pressed, then unplug the virtual keyboard and mouse.
 *
 * @param input The devices to destroy.
 */
void pspdisp_input_destroy(PSPdispInput *input) {
    for (uint32_t code = 0; code <= KEY_MAX; code++) {
        if (input->pressed_keys[code / 8] & (1 << (code % 8))) {
            struct input_event events[2] = { { .type = EV_KEY, .code = code, .value = 0 } };
            pspdisp_input_uinput_write(input->keyboard, events, 1);
        }
    }
    for (size_t i = 0; i < G_N_ELEMENTS(pspdisp_input_uinput_buttons); i++) {
        if (input->pressed_buttons & (1 << i))
            pspdisp_input_mouse_button(input, i, false);
    }

    ioctl(input->keyboard, UI_DEV_DESTROY);
    ioctl(input->mouse, UI_DEV_DESTROY);
    close(input->keyboard);
    close(input->mouse);
    g_free(input);
}

/**
 * @brief Press or release a key of the virtual keyboard.
 *
 * A key pressed again (e.g. by a typematic button) is not sent again, as the desktop already repeats it.
 */
bool pspdisp_input_key(PSPdispInput *input, uint16_t scancode, bool pressed) {
    uint16_t code = pspdisp_input_uinput_code(scancode);
    if (code == KEY_RESERVED) return false;

    bool was_pressed = input->pressed_keys[code / 8] & (1 << (code % 8));
    if (pressed == was_pressed) return true;

    if (pressed)
        input->pressed_keys[code / 8] |= 1 << (code % 8);
    else
        input->pressed_keys[code / 8] &= ~(1 << (code % 8));

    struct input_event events[2] = { { .type = EV_KEY, .code = code, .value = pressed } };
    return pspdisp_input_uinput_write(input->keyboard, events, 1);
}

bool pspdisp_input_mouse_button(PSPdispInput *input, PSPdispInputMouseButton button, bool pressed) {
    if (pressed)
        input->pressed_buttons |= 1 << button;
    else
        input->pressed_buttons &= ~(1 << button);

    struct input_event events[2] = { { .type = EV_KEY, .code = pspdisp_input_uinput_buttons[button], .value = pressed } };
    return pspdisp_input_uinput_write(input->mouse, events, 1);
}

bool pspdisp_input_mouse_move(PSPdispInput *input, int32_t x, int32_t y) {
    struct input_event events[3] = {};
    size_t count = 0;

    if (x != 0)
        events[count++] = (struct input_event) { .type = EV_REL, .code = REL_X, .value = x };
    if (y != 0)
        events[count++] = (struct input_event) { .type = EV_REL, .code = REL_Y, .value = y };

    return count == 0 || pspdisp_input_uinput_write(input->mouse, events, count);
}

bool pspdisp_input_mouse_wheel(PSPdispInput *input, int32_t steps) {
    // The steps, and the same in the high resolution unit (120 per step), as a real wheel sends both
    struct input_event events[3] = {
        { .type = EV_REL, .code = REL_WHEEL, .value = steps },
        { .type = EV_REL, .code = REL_WHEEL_HI_RES, .value = steps * 120 },
    };

    return steps == 0 || pspdisp_input_uinput_write(input->mouse, events, 2);
}
