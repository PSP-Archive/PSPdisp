/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * linux.c - What the daemon needs from Linux        *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The system of the daemon on Linux (see `system.c`): the signals stop it, each thread has its own nice value, and root is the administrator.
 */

#pragma once

#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <glib.h>
#include <glib-unix.h>
#include "../system.c" // Its declarations (when `system.c` includes this file, it already has them and this include is skipped)

#define PSPDISP_DAEMON_SYSTEM_HIGHER_PRIORITY -5 // Nice value of the daemon with a higher priority (0 is the one of the other programs, lower is higher)

/**
 * @brief Call a handler on the main thread on SIGINT (Ctrl+C) and SIGTERM.
 *
 * @param handler Stops the daemon, it returns `G_SOURCE_CONTINUE`.
 * @param data Given to the handler.
 */
void pspdisp_daemon_system_watch_stop(GSourceFunc handler, gpointer data) {
	g_unix_signal_add(SIGINT, handler, data);
	g_unix_signal_add(SIGTERM, handler, data);
}

/**
 * @brief Set the nice value of all the threads of the daemon.
 *
 * On Linux each thread has its own nice value, so each one is set (the threads created after inherit it).
 * Raising it needs the `CAP_SYS_NICE` capability or a `RLIMIT_NICE` limit allowing it, which the user usually doesn't have, so a warning tells when it's refused.
 *
 * @param higher If the priority is higher than the one of the other programs.
 */
void pspdisp_daemon_system_set_priority(bool higher) {
	const int nice = higher ? PSPDISP_DAEMON_SYSTEM_HIGHER_PRIORITY : 0;
	GDir *tasks = g_dir_open("/proc/self/task", 0, NULL);
	const char *task;
	int failure = 0; // The error of the first thread refused (0 if none), told once for all of them

	while (tasks != NULL && (task = g_dir_read_name(tasks)) != NULL) {
		if (setpriority(PRIO_PROCESS, g_ascii_strtoull(task, NULL, 10), nice) != 0 && failure == 0)
			failure = errno;
	}
	if (tasks != NULL)
		g_dir_close(tasks);
	else if (setpriority(PRIO_PROCESS, 0, nice) != 0)
		failure = errno;

	if (failure == EACCES || failure == EPERM)
		g_warning("%s(): can't raise the priority to nice %d without CAP_SYS_NICE or a nice limit allowing it (see limits.conf), the \"Higher process priority\" setting has no effect", __FUNCTION__, nice);
	else if (failure != 0)
		g_warning("%s(): failed to set the priority to nice %d: %s", __FUNCTION__, nice, g_strerror(failure));
}

/**
 * @brief Get the ID of the process of the daemon.
 *
 * @return The process ID.
 */
int64_t pspdisp_daemon_system_pid() {
	return getpid();
}

/**
 * @brief Tell if the daemon runs as root.
 *
 * @return `true` if its effective user ID is 0.
 */
bool pspdisp_daemon_system_runs_as_administrator() {
	return geteuid() == 0;
}
