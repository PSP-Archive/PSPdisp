/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * controller.c - PSP as a gamepad of the computer   *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The virtual controller: what the buttons and the analog stick of the PSP drive on the computer, a virtual gamepad, or a virtual keyboard and mouse pressed as a control file says (see `control_file.c`).
 *
 * The PSP buttons go to the Xbox buttons at the same place, as most games know the Xbox layout: cross is A (bottom), circle is B (right), square is X (left) and triangle is Y (top).
 * L and R are the shoulder buttons, select and start are back and start, the directional pad is the directional pad, and the analog stick is the left stick.
 * The analog stick of the PSP rests a bit off its center, by a different amount on each PSP, so a dead zone around the center reads as centered (see `PSPDISP_CONTROLLER_DEAD_ZONE`).
 *
 * The virtual gamepad itself varies with the platform (see `gamepad.c`), as the virtual keyboard and mouse (see `input.c`).
 */

#pragma once

#include <math.h>
#include <stdint.h>
#include <glib.h>
#include "../../shared.h"
#include "./gamepad.c"
#include "./control_file.c"

#define PSPDISP_CONTROLLER_NAME      "PSPdisp virtual gamepad" // Name the computer shows for the gamepad
#define PSPDISP_CONTROLLER_CENTER    127.5                     // Center of the analog stick of the PSP, on each axis (from 0 to 255)
#define PSPDISP_CONTROLLER_DEAD_ZONE 32.0                      // Distance from the center under which the analog stick reads as centered, from 0 to 127.5 (a quarter of the way, as the dead zone XInput advises)

/**
 * @brief The PSP buttons, with the gamepad button each one drives.
 */
static const struct {
    PSPdispButton psp; // The PSP button
    PSPdispGamepadButton gamepad; // The gamepad button
} pspdisp_controller_buttons[] = {
    { PSPDISP_BUTTON_CROSS,    PSPDISP_GAMEPAD_BUTTON_SOUTH },
    { PSPDISP_BUTTON_CIRCLE,   PSPDISP_GAMEPAD_BUTTON_EAST },
    { PSPDISP_BUTTON_SQUARE,   PSPDISP_GAMEPAD_BUTTON_WEST },
    { PSPDISP_BUTTON_TRIANGLE, PSPDISP_GAMEPAD_BUTTON_NORTH },
    { PSPDISP_BUTTON_L,        PSPDISP_GAMEPAD_BUTTON_LEFT_SHOULDER },
    { PSPDISP_BUTTON_R,        PSPDISP_GAMEPAD_BUTTON_RIGHT_SHOULDER },
    { PSPDISP_BUTTON_SELECT,   PSPDISP_GAMEPAD_BUTTON_BACK },
    { PSPDISP_BUTTON_START,    PSPDISP_GAMEPAD_BUTTON_START },
    { PSPDISP_BUTTON_UP,       PSPDISP_GAMEPAD_BUTTON_DPAD_UP },
    { PSPDISP_BUTTON_DOWN,     PSPDISP_GAMEPAD_BUTTON_DPAD_DOWN },
    { PSPDISP_BUTTON_LEFT,     PSPDISP_GAMEPAD_BUTTON_DPAD_LEFT },
    { PSPDISP_BUTTON_RIGHT,    PSPDISP_GAMEPAD_BUTTON_DPAD_RIGHT },
};

/**
 * @brief The virtual controller, driven by the PSP: a gamepad, or a keyboard and a mouse pressed as a control file says.
 */
typedef struct {
    PSPdispGamepad *gamepad; // The virtual gamepad of the platform (`NULL` with a control file)
    PSPdispControlFile *control_file; // The control file, with its virtual keyboard and mouse (`NULL` with the gamepad)
} PSPdispController;

/**
 * @brief Plug the virtual gamepad into the computer, at rest.
 *
 * @param error Where to tell why it failed, ready to be shown to the user (e.g. the permission it needs).
 * @return The controller, free it with `pspdisp_controller_destroy()`, or `NULL` if the gamepad can't be created.
 */
PSPdispController *pspdisp_controller_new(GError **error) {
    PSPdispGamepad *gamepad = pspdisp_gamepad_new(PSPDISP_CONTROLLER_NAME, error);
    if (gamepad == NULL) return NULL;

    PSPdispController *controller = g_new0(PSPdispController, 1);
    controller->gamepad = gamepad;
    return controller;
}

/**
 * @brief Load a control file, and plug the virtual keyboard and mouse it presses into the computer.
 *
 * @param path The control file.
 * @param error Where to tell why it failed, ready to be shown to the user (e.g. the permission it needs).
 * @return The controller, free it with `pspdisp_controller_destroy()`, or `NULL` if it failed.
 */
PSPdispController *pspdisp_controller_new_control_file(const char *path, GError **error) {
    PSPdispControlFile *control_file = pspdisp_control_file_new(path, error);
    if (control_file == NULL) return NULL;

    PSPdispController *controller = g_new0(PSPdispController, 1);
    controller->control_file = control_file;
    return controller;
}

/**
 * @brief Unplug the virtual devices and free the controller.
 *
 * @param controller The controller to destroy.
 */
void pspdisp_controller_destroy(PSPdispController *controller) {
    if (controller->gamepad != NULL)
        pspdisp_gamepad_destroy(controller->gamepad);
    if (controller->control_file != NULL)
        pspdisp_control_file_destroy(controller->control_file);
    g_free(controller);
}

/**
 * @brief Tell if the PSP may open its on-screen keyboard, to type with it (`EnableOSK` of the control file).
 *
 * @param controller The controller.
 * @return `true` with a control file that enables it.
 */
bool pspdisp_controller_enables_osk(PSPdispController *controller) {
    return controller->control_file != NULL && controller->control_file->enable_osk;
}

/**
 * @brief Get how many times the PSP asked to open the menu of PSPdisp (the `OpenMenu` action of a control file).
 *
 * @param controller The controller.
 * @return The count, since the controller was created.
 */
uint32_t pspdisp_controller_open_menu_requests(PSPdispController *controller) {
    return controller->control_file != NULL ? controller->control_file->open_menu_requests : 0;
}

/**
 * @brief Turn the state of the PSP controls into the state of the gamepad.
 *
 * The dead zone is round, and the stick is scaled from its edge to the edge of the analog stick, so the whole range of the gamepad is still reached.
 * A stick pushed into a corner is brought back to the edge of the circle, as on an Xbox controller.
 *
 * @param buttons The pressed buttons of the PSP, `PSPdispButton` bits.
 * @param analog_x The analog stick of the PSP, from the left (0) to the right (255).
 * @param analog_y The analog stick of the PSP, from the top (0) to the bottom (255).
 * @return The state of the gamepad.
 */
PSPdispGamepadState pspdisp_controller_map(uint32_t buttons, uint8_t analog_x, uint8_t analog_y) {
    PSPdispGamepadState state = {0};

    for (size_t i = 0; i < G_N_ELEMENTS(pspdisp_controller_buttons); i++)
        if (buttons & pspdisp_controller_buttons[i].psp)
            state.buttons |= pspdisp_controller_buttons[i].gamepad;

    double x = analog_x - PSPDISP_CONTROLLER_CENTER;
    double y = analog_y - PSPDISP_CONTROLLER_CENTER;
    double distance = sqrt(x * x + y * y);

    if (distance > PSPDISP_CONTROLLER_DEAD_ZONE) {
        double scale = MIN((distance - PSPDISP_CONTROLLER_DEAD_ZONE) / (PSPDISP_CONTROLLER_CENTER - PSPDISP_CONTROLLER_DEAD_ZONE), 1.0) / distance; // To a distance from 0 to 1
        state.left_x = (int16_t) lround(x * scale * 32767);
        state.left_y = (int16_t) lround(y * scale * 32767);
    }

    return state;
}

/**
 * @brief Drive the virtual devices with the state of the PSP controls.
 *
 * Only what changed is sent to the computer, so it can be called with each response of the PSP.
 *
 * @param controller The controller.
 * @param buttons The pressed buttons of the PSP, `PSPdispButton` bits, with a character of the on-screen keyboard in the highest byte.
 * @param analog_x The analog stick of the PSP, from the left (0) to the right (255).
 * @param analog_y The analog stick of the PSP, from the top (0) to the bottom (255).
 * @return `true` if the computer took it.
 */
bool pspdisp_controller_update(PSPdispController *controller, uint32_t buttons, uint8_t analog_x, uint8_t analog_y) {
    if (controller->control_file != NULL) {
        pspdisp_control_file_update(controller->control_file, buttons, analog_x, analog_y);
        return true;
    }

    PSPdispGamepadState state = pspdisp_controller_map(buttons, analog_x, analog_y);
    return pspdisp_gamepad_update(controller->gamepad, &state);
}
