/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * system.c - What the daemon needs from the system  *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The parts of the daemon that depend on the operating system: how it's asked to stop, the priority of its threads, its process ID, and if it runs as an administrator.
 *
 * Each platform implements the functions below in its own file, included at compile time (e.g. `system/linux.c`).
 */

#pragma once

#include <stdint.h>
#include <glib.h>

/**
 * @brief Call a handler on the main thread each time the user asks the daemon to stop (e.g. with Ctrl+C in its terminal), for as long as it runs.
 *
 * It's called once, on the main thread.
 *
 * @param handler Stops the daemon, it returns `G_SOURCE_CONTINUE`.
 * @param data Given to the handler.
 */
void pspdisp_daemon_system_watch_stop(GSourceFunc handler, gpointer data);

/**
 * @brief Set the priority of all the threads of the daemon, higher than the one of the other programs or the same.
 *
 * The threads created after it get the same priority.
 * When the user isn't allowed to raise it, it stays as it was.
 *
 * @param higher If the priority is higher than the one of the other programs.
 */
void pspdisp_daemon_system_set_priority(bool higher);

/**
 * @brief Get the ID of the process of the daemon.
 *
 * @return The process ID.
 */
int64_t pspdisp_daemon_system_pid();

/**
 * @brief Tell if the daemon runs as an administrator, who can write in the folders of the other users.
 *
 * @return `true` if it runs as an administrator.
 */
bool pspdisp_daemon_system_runs_as_administrator();

// The implementation of this platform
#if defined(__linux__)
    #include "./system/linux.c"
#else
    #error "Unsupported platform"
#endif
