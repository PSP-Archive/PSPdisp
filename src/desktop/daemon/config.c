/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * config.c - Fixed configuration of the daemon      *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * What the daemon can't take from the settings (see `settings/settings.c` for the settings, edited by the desktop app).
 */

#pragma once

#include <stdint.h>
#include <glib.h>
#include "../../shared.h"

// TODO: improve the communication protocol to include a handshake with the PSP model infos
// The old protocol doesn't tell which PSP is connected, so the identification of its virtual display and audio is fixed (see `pspdisp_psp_name()`)
#define PSPDISP_DAEMON_PSP_MODEL         PSPDISP_MODEL_03G            // PSP model given to the virtual display, and with the region to its name
#define PSPDISP_DAEMON_PSP_REGION        PSPDISP_REGION_NORTH_AMERICA // PSP region given to the virtual display, and with the model to its name
#define PSPDISP_DAEMON_PSP_SERIAL_NUMBER 1                            // Serial number given to the virtual display, not 0

/**
 * @brief Names of the display devices, there's only the virtual display.
 */
const char *const pspdisp_daemon_display_devices[] = {
	"PSPdisp virtual display",
};

static_assert(G_N_ELEMENTS(pspdisp_daemon_display_devices) <= PSPDISP_COM_MAX_DISPLAY_DEVICES, "The PSP keeps at most PSPDISP_COM_MAX_DISPLAY_DEVICES display device names");
