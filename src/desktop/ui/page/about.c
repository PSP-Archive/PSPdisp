/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * about.c - "About" page of the desktop app         *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <gtk/gtk.h>
#include "../app.c"
#include "../components/groupbox.c"
#include "../components/page.c"

/**
 * @brief The labels of the status of the "About" page, updated about every second.
 */
typedef struct {
	GtkWidget *connection; // How the PSP is connected
	GtkWidget *fps; // Frames per second
	GtkWidget *frame_size; // Average frame size
	GtkWidget *total_frames; // Total frames sent
	GtkWidget *total_sent; // Total amount sent
} PSPdispGtkPageAbout;

/**
 * @brief Make the title of the page: the name of PSPdisp, and its copyrights.
 *
 * @return The label of the title.
 */
GtkWidget *pspdisp_gtk_page_about_title_new() {
	GtkWidget *label = gtk_label_new(NULL);
	// The name in big bold letters, with the Pango markup
	gtk_label_set_markup(GTK_LABEL(label), "<span size='xx-large' weight='bold'>PSPdisp</span> © 2008-2015 JJS, © 2026 Arthur Neto");
	gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
	return label;
}

/**
 * @brief Make the version of PSPdisp, as the build gives it.
 *
 * @return The label of the version.
 */
GtkWidget *pspdisp_gtk_page_about_version_new() {
	GtkWidget *label = gtk_label_new("Version " PSPDISP_VERSION);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
	return label;
}

/**
 * @brief Make the links to the home pages: the GitHub repository of PSPdisp, and the home page of the old desktop app.
 *
 * @return The box of the links.
 */
GtkWidget *pspdisp_gtk_page_about_links_new() {
	GtkWidget *github_link = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(github_link), "Visit the PSPdisp GitHub repository <a href=\"https://github.com/net-0/PSPdisp\">PSPdisp</a>");
	gtk_label_set_xalign(GTK_LABEL(github_link), 0.0f);
	gtk_widget_set_margin_top(github_link, 16);

	GtkWidget *jjs_link = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(jjs_link), "Visit the original JJS homepage @ <a href=\"http://www.jjs.at/redirect/pspdisp/v0.6\">jjs.at</a>");
	gtk_label_set_xalign(GTK_LABEL(jjs_link), 0.0f);
	gtk_widget_set_margin_top(jjs_link, 16);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_append(GTK_BOX(box), github_link);
	gtk_box_append(GTK_BOX(box), jjs_link);
	return box;
}

/**
 * @brief Show the status of the daemon, called about every second.
 *
 * @param ui The app.
 * @param page The page.
 */
void pspdisp_gtk_page_about_status(PSPdispUI *ui, PSPdispGtkPageAbout *page) {
	const PSPdispStatus *status = &ui->status;
	bool connected = ui->running && status->connected;

	gtk_label_set_text(GTK_LABEL(page->connection), !ui->running ? "The daemon isn't running" : connected ? status->connected_via : "No PSP connected");

	char *fps = g_strdup_printf("%u", connected ? status->frames_per_second : 0);
	char *frame_size = pspdisp_ui_size_text(connected && status->frames_per_second > 0 ? status->bytes_per_second / status->frames_per_second : 0);
	char *total_frames = g_strdup_printf("%" G_GUINT64_FORMAT, connected ? status->total_frames : 0);
	char *total_sent = pspdisp_ui_size_text(connected ? status->total_bytes : 0);

	gtk_label_set_text(GTK_LABEL(page->fps), fps);
	gtk_label_set_text(GTK_LABEL(page->frame_size), frame_size);
	gtk_label_set_text(GTK_LABEL(page->total_frames), total_frames);
	gtk_label_set_text(GTK_LABEL(page->total_sent), total_sent);

	g_free(total_sent);
	g_free(total_frames);
	g_free(frame_size);
	g_free(fps);
}

/**
 * @brief Add a row to the status: its name, and its value.
 *
 * @param grid The grid of the status.
 * @param row The row.
 * @param name The name of the row.
 * @return The label of the value, to update.
 */
GtkWidget *pspdisp_gtk_page_about_status_row(GtkWidget *grid, int row, const char *name) {
	GtkWidget *key = gtk_label_new(name);
	gtk_label_set_xalign(GTK_LABEL(key), 0.0f);

	GtkWidget *value = gtk_label_new("0");
	gtk_label_set_xalign(GTK_LABEL(value), 0.0f);

	gtk_grid_attach(GTK_GRID(grid), key,   0, row, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), value, 1, row, 1, 1);
	return value;
}

/**
 * @brief Make the status of the page: how the PSP is connected and how fast it streams, as the daemon tells it.
 *
 * The totals are the ones of the PSP connected.
 *
 * @param ui The app.
 * @param page The page, whose labels of the status are made.
 * @return The group box of the status.
 */
GtkWidget *pspdisp_gtk_page_about_status_new(PSPdispUI *ui, PSPdispGtkPageAbout *page) {
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 32);

	page->connection = pspdisp_gtk_page_about_status_row(grid, 0, "Connection");
	page->fps = pspdisp_gtk_page_about_status_row(grid, 1, "Frames per second");
	page->frame_size = pspdisp_gtk_page_about_status_row(grid, 2, "Average frame size");
	page->total_frames = pspdisp_gtk_page_about_status_row(grid, 3, "Total frames sent");
	page->total_sent = pspdisp_gtk_page_about_status_row(grid, 4, "Total amount sent");

	pspdisp_ui_on_status(ui, (PSPdispUIRefresh) pspdisp_gtk_page_about_status, page);
	return pspdisp_ui_component_groupbox_new("Status", grid);
}

/**
 * @brief Make the "About" page: the name and the version of PSPdisp, its links, and the status of the daemon.
 *
 * @param ui The app.
 * @return The page.
 */
GtkWidget *pspdisp_gtk_page_about_new(PSPdispUI *ui) {
	PSPdispGtkPageAbout *page = g_new0(PSPdispGtkPageAbout, 1);
	GtkWidget *box;
	GtkWidget *scroller = pspdisp_ui_component_page_new(&box);

	GtkWidget *title = pspdisp_gtk_page_about_title_new();
	gtk_widget_set_halign(title, GTK_ALIGN_START);

	GtkWidget *version = pspdisp_gtk_page_about_version_new();
	gtk_widget_set_halign(version, GTK_ALIGN_START);

	GtkWidget *links = pspdisp_gtk_page_about_links_new();
	gtk_widget_set_halign(links, GTK_ALIGN_START);

	GtkWidget *status = pspdisp_gtk_page_about_status_new(ui, page);
	gtk_widget_set_halign(status, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top(status, 24);

	gtk_box_append(GTK_BOX(box), title);
	gtk_box_append(GTK_BOX(box), version);
	gtk_box_append(GTK_BOX(box), links);
	gtk_box_append(GTK_BOX(box), status);

	g_signal_connect_swapped(scroller, "destroy", G_CALLBACK(g_free), page);
	return scroller;
}
