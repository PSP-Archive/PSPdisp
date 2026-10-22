/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * main.c - Desktop app of PSPdisp                   *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The desktop app: the settings of PSPdisp, as the options window and the tray menu of the old desktop app, and what the daemon is doing (see `app.c`).
 *
 * It runs as the user, as the daemon, sharing the folder of the settings (the default one of the user, or the one given with `--config-dir`, as the daemon).
 */

#include <gtk/gtk.h>
#include "./app.c"
#include "./page/streaming.c"
#include "./page/about.c"
#include "./page/wlan.c"
#include "./page/quality.c"
#include "./page/update_interval.c"
#include "./page/control.c"
#include "./page/presets.c"

#define PSPDISP_UI_APP_ID "com.pspdisp.desktop" // ID of the app, also the name of its icon (`com.pspdisp.desktop.svg`) and of its desktop entry (`com.pspdisp.desktop.desktop`)

PSPdispUI pspdisp_gtk_ui; // The app
char *pspdisp_gtk_option_config_dir = NULL; // `--config-dir`

/**
 * @brief The options of the command line.
 */
GOptionEntry pspdisp_gtk_options[] = {
	{ "config-dir", 'c', 0, G_OPTION_ARG_FILENAME, &pspdisp_gtk_option_config_dir, "Folder of the settings, shared with the daemon (default: ${XDG_CONFIG_HOME}/pspdisp)", "DIR" },
	G_OPTION_ENTRY_NULL
};

/**
 * @brief Make and show the window of the desktop app, called by GTK when the app starts, or show it again when it's started a second time.
 *
 * Its pages are the tabs of the options window of the old desktop app.
 *
 * @param app The application.
 * @param user_data Unused.
 */
void pspdisp_gtk_application_activate(GtkApplication *app, gpointer user_data) {
	(void) user_data;
	PSPdispUI *ui = &pspdisp_gtk_ui;

	// A second start shows the window already there
	if (ui->window != NULL) {
		gtk_window_present(ui->window);
		return;
	}

	pspdisp_ui_init(ui, app, pspdisp_gtk_option_config_dir);

	GtkWidget *window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "PSPdisp");
	gtk_window_set_icon_name(GTK_WINDOW(window), PSPDISP_UI_APP_ID); // From the icon theme (hicolor), where `make install` puts it
	gtk_window_set_default_size(GTK_WINDOW(window), 760, 640);
	ui->window = GTK_WINDOW(window);

	// Notebook with the tabs
	// Not scrollable: a drag over its scroll arrows (e.g. of a text selected in a label) scrolls the pages until the last one, as GTK never stops it (see the TODO)
	GtkWidget *notebook = gtk_notebook_new();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pspdisp_gtk_page_streaming_new(ui), gtk_label_new("Streaming"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pspdisp_gtk_page_about_new(ui), gtk_label_new("About"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pspdisp_gtk_page_wlan_new(ui), gtk_label_new("WLAN"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pspdisp_gtk_page_quality_new(ui), gtk_label_new("Quality"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pspdisp_gtk_page_update_interval_new(ui), gtk_label_new("Update interval"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pspdisp_gtk_page_control_new(ui), gtk_label_new("Control"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pspdisp_gtk_page_presets_new(ui), gtk_label_new("Presets"));
	gtk_window_set_child(GTK_WINDOW(window), notebook);

	// The settings are saved as they change, so there's no "Save" button
	gtk_window_present(GTK_WINDOW(window));
}

/**
 * @brief Run the desktop app until its window is closed.
 *
 * @param argc The number of arguments of the command line.
 * @param argv The arguments of the command line (see `pspdisp_gtk_options`).
 * @return The exit status of the app, 0 when it ended well.
 */
int pspdisp_gtk_application_run(int argc, char **argv) {
	GtkApplication *app = gtk_application_new(PSPDISP_UI_APP_ID, G_APPLICATION_DEFAULT_FLAGS);
	g_application_add_main_option_entries(G_APPLICATION(app), pspdisp_gtk_options); // Parsed into their variables before "activate"
	g_application_set_option_context_description(G_APPLICATION(app), "Environment variables:\n" PSPDISP_SETTINGS_ENVIRONMENT_HELP);
	g_signal_connect(app, "activate", G_CALLBACK(pspdisp_gtk_application_activate), NULL);
	int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	return status;
}

/**
 * @brief Start the desktop app.
 *
 * @param argc The number of arguments of the command line.
 * @param argv The arguments of the command line.
 * @return The exit status of the app (see `pspdisp_gtk_application_run()`).
 */
int main(int argc, char **argv) {
	return pspdisp_gtk_application_run(argc, argv);
}
