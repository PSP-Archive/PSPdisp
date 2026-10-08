/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * gamepad.c - Virtual gamepad of the platform       *
 *                                                   *
 * Copyright © 2026 Net-0                            *
 *****************************************************/

/*
 * A virtual gamepad that the computer sees as a plugged controller, with the layout of an Xbox controller, so the games know it.
 *
 * Its implementation varies with the platform, chosen at compile time: uinput on Linux (`gamepad/uinput.c`).
 */

#pragma once

#include <stdint.h>
#include <glib.h>

/**
 * @brief Buttons of the virtual gamepad, combined with bitwise OR, named after their place (as the Xbox buttons A, B, X and Y aren't at the same place as the Nintendo ones).
 */
typedef enum: uint16_t {
    PSPDISP_GAMEPAD_BUTTON_SOUTH          = 1 << 0,  // Bottom face button, A on Xbox, cross on PlayStation
    PSPDISP_GAMEPAD_BUTTON_EAST           = 1 << 1,  // Right face button, B on Xbox, circle on PlayStation
    PSPDISP_GAMEPAD_BUTTON_WEST           = 1 << 2,  // Left face button, X on Xbox, square on PlayStation
    PSPDISP_GAMEPAD_BUTTON_NORTH          = 1 << 3,  // Top face button, Y on Xbox, triangle on PlayStation
    PSPDISP_GAMEPAD_BUTTON_LEFT_SHOULDER  = 1 << 4,  // Left shoulder button, LB on Xbox
    PSPDISP_GAMEPAD_BUTTON_RIGHT_SHOULDER = 1 << 5,  // Right shoulder button, RB on Xbox
    PSPDISP_GAMEPAD_BUTTON_BACK           = 1 << 6,  // Back on Xbox, select on PlayStation
    PSPDISP_GAMEPAD_BUTTON_START          = 1 << 7,  // Start
    PSPDISP_GAMEPAD_BUTTON_DPAD_UP        = 1 << 8,  // Up of the directional pad
    PSPDISP_GAMEPAD_BUTTON_DPAD_DOWN      = 1 << 9,  // Down of the directional pad
    PSPDISP_GAMEPAD_BUTTON_DPAD_LEFT      = 1 << 10, // Left of the directional pad
    PSPDISP_GAMEPAD_BUTTON_DPAD_RIGHT     = 1 << 11, // Right of the directional pad
} PSPdispGamepadButton;

/**
 * @brief State of the virtual gamepad.
 *
 * The parts an Xbox controller has but the PSP doesn't (the right stick, the triggers, the guide button and the stick buttons) stay at rest.
 */
typedef struct {
    uint16_t buttons; // The pressed buttons, `PSPdispGamepadButton` bits
    int16_t left_x; // Left stick, from the left (-32768) to the right (32767), 0 when centered
    int16_t left_y; // Left stick, from the top (-32768) to the bottom (32767), 0 when centered
} PSPdispGamepadState;

/**
 * @brief Virtual gamepad.
 *
 * Its platform implementation (included below, e.g. `gamepad/uinput.c`) declares this struct with the data it needs, and defines `PSPDISP_GAMEPAD_NAME`.
 */
typedef struct PSPdispGamepad PSPdispGamepad;

/**
 * @brief Plug a virtual gamepad into the computer, at rest, implemented by the platform.
 *
 * @param name The name the computer shows for it.
 * @param error Where to tell why it failed, ready to be shown to the user (e.g. the permission it needs).
 * @return The gamepad, unplug it with `pspdisp_gamepad_destroy()`, or `NULL` if it failed.
 */
PSPdispGamepad *pspdisp_gamepad_new(const char *name, GError **error);

/**
 * @brief Set the state of the virtual gamepad, implemented by the platform.
 *
 * Only what changed since the previous state is sent to the computer, so it can be called with the same state again and again.
 *
 * @param gamepad The gamepad.
 * @param state The new state.
 * @return `true` if the computer took it.
 */
bool pspdisp_gamepad_update(PSPdispGamepad *gamepad, const PSPdispGamepadState *state);

/**
 * @brief Unplug the virtual gamepad and free it, implemented by the platform.
 *
 * @param gamepad The gamepad to destroy.
 */
void pspdisp_gamepad_destroy(PSPdispGamepad *gamepad);

// The virtual gamepad of this platform, it declares `PSPdispGamepad`, implements its functions and defines `PSPDISP_GAMEPAD_NAME`
#if defined(__linux__)
    #include "./gamepad/uinput.c"
#else
    // TODO: impl it
    #error "Unsupported platform: there's no virtual gamepad for it yet"
#endif
