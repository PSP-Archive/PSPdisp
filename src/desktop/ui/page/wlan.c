/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * wlan.c - "WLAN" page of the desktop app           *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

#pragma once

#include <gtk/gtk.h>
#include "../app.c"
#include "../components/groupbox.c"
#include "../components/page.c"
#include "../components/paragraph.c"

#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>

/**
 * @brief The widgets of the "WLAN" page that aren't bound to a setting.
 */
typedef struct {
	GtkWidget *addresses; // The IP addresses of the computer
	GtkWidget *server; // What the WLAN server is doing
	GtkWidget *password; // The password entry
} PSPdispGtkPageWLAN;

/**
 * @brief List the IPv4 addresses of the computer (the PSP only knows IPv4), without the loopback.
 *
 * @return The addresses, one per line with their interface (e.g. "192.168.1.20 (wlan0)"), free it with `g_free()`.
 */
char *pspdisp_gtk_page_wlan_addresses() {
	GString *text = g_string_new(NULL);

	struct ifaddrs *interfaces = NULL;

	if (getifaddrs(&interfaces) == 0) {
		for (struct ifaddrs *interface = interfaces; interface != NULL; interface = interface->ifa_next) {
			char address[NI_MAXHOST];

			if (interface->ifa_addr == NULL || interface->ifa_addr->sa_family != AF_INET || (interface->ifa_flags & IFF_LOOPBACK)) continue;
			if (getnameinfo(interface->ifa_addr, sizeof(struct sockaddr_in), address, sizeof(address), NULL, 0, NI_NUMERICHOST) != 0) continue;

			g_string_append_printf(text, "%s%s (%s)", text->len > 0 ? "\n" : "", address, interface->ifa_name);
		}
		freeifaddrs(interfaces);
	}

	if (text->len == 0)
		g_string_append(text, "No network address found, is the computer connected to the network?");
	return g_string_free(text, FALSE);
}

/**
 * @brief Show the IP addresses again (e.g. after joining a network).
 *
 * @param button Unused, the button.
 * @param page The page.
 */
void pspdisp_gtk_page_wlan_on_refresh_addresses(GtkButton *button, PSPdispGtkPageWLAN *page) {
	(void) button;
	char *addresses = pspdisp_gtk_page_wlan_addresses();
	gtk_label_set_text(GTK_LABEL(page->addresses), addresses);
	g_free(addresses);
}

/**
 * @brief Remove the password, as "No password" of the old desktop app.
 *
 * @param button Unused, the button.
 * @param page The page.
 */
void pspdisp_gtk_page_wlan_on_no_password(GtkButton *button, PSPdispGtkPageWLAN *page) {
	(void) button;
	gtk_editable_set_text(GTK_EDITABLE(page->password), "");
}

/**
 * @brief Show what the WLAN server is doing, called about every second.
 *
 * @param ui The app.
 * @param page The page.
 */
void pspdisp_gtk_page_wlan_status(PSPdispUI *ui, PSPdispGtkPageWLAN *page) {
	const PSPdispStatus *status = &ui->status;
	char *text;

	if (!ui->running)
		text = g_strdup("The daemon isn't running.");
	else if (!ui->settings.enabled || !ui->settings.use_wlan)
		text = g_strdup("The WLAN mode is not enabled (see the \"Streaming\" page).");
	else if (status->wlan)
		text = g_strdup_printf("The WLAN server listens on port %u%s.", status->wlan_port, ui->settings.wlan_password[0] == '\0' ? ", without a password: anyone on the network can connect" : "");
	else if (status->wlan_error[0] != '\0')
		text = g_strdup_printf("The WLAN server can't start: %s.", status->wlan_error);
	else
		text = g_strdup("The WLAN server is starting...");

	gtk_label_set_text(GTK_LABEL(page->server), text);
	g_free(text);
}

/**
 * @brief Make the group of the notifications: the WLAN events that show one.
 *
 * @param ui The app.
 * @return The group box.
 */
GtkWidget *pspdisp_gtk_page_wlan_notifications_new(PSPdispUI *ui) {
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 24);

	GtkWidget *server_checkbox = pspdisp_ui_bind(ui, gtk_check_button_new_with_label("Server has been started"), PSPDISP_UI_BINDING_CHECK, "WLAN", "NotifyServerStarted", offsetof(PSPdispSettings, notify_server_started), NULL);
	GtkWidget *connected_checkbox = pspdisp_ui_bind(ui, gtk_check_button_new_with_label("Client connected"), PSPDISP_UI_BINDING_CHECK, "WLAN", "NotifyConnected", offsetof(PSPdispSettings, notify_connected), NULL);
	GtkWidget *disconnected_checkbox = pspdisp_ui_bind(ui, gtk_check_button_new_with_label("Client disconnected"), PSPDISP_UI_BINDING_CHECK, "WLAN", "NotifyDisconnected", offsetof(PSPdispSettings, notify_disconnected), NULL);

	gtk_grid_attach(GTK_GRID(grid), server_checkbox, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), connected_checkbox, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), disconnected_checkbox, 0, 1, 1, 1);

	return pspdisp_ui_component_groupbox_new("Show notification on those events", grid);
}

/**
 * @brief Make the group of the password: its entry, and a button to remove it.
 *
 * @param ui The app.
 * @param page The page, whose password entry is made.
 * @return The group box.
 */
GtkWidget *pspdisp_gtk_page_wlan_password_new(PSPdispUI *ui, PSPdispGtkPageWLAN *page) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	GtkWidget *entry = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
	gtk_entry_set_max_length(GTK_ENTRY(entry), PSPDISP_SETTINGS_PASSWORD_SIZE);
	gtk_entry_set_invisible_char(GTK_ENTRY(entry), L'*');
	gtk_widget_set_hexpand(entry, TRUE);
	page->password = pspdisp_ui_bind(ui, entry, PSPDISP_UI_BINDING_ENTRY, "WLAN", "Password", offsetof(PSPdispSettings, wlan_password), NULL);

	GtkWidget *no_password = gtk_button_new_with_label("No password");
	g_signal_connect(no_password, "clicked", G_CALLBACK(pspdisp_gtk_page_wlan_on_no_password), page);

	gtk_box_append(GTK_BOX(row), gtk_label_new("Password (max 32 chars)"));
	gtk_box_append(GTK_BOX(row), entry);
	gtk_box_append(GTK_BOX(row), no_password);

	GtkWidget *paragraph = pspdisp_ui_component_paragraph_new(
		"The PSP sends it when it connects, set the same one in the \"WLAN Settings\" of the PSP app. "
		"It's sent in clear over the network, so it only keeps away the PSPs of others.");
	gtk_widget_add_css_class(paragraph, "dim-label");

	gtk_box_append(GTK_BOX(box), row);
	gtk_box_append(GTK_BOX(box), paragraph);
	return pspdisp_ui_component_groupbox_new("Password", box);
}

/**
 * @brief Make the group of the port: the port offset of the PSP, as the instance number of the old desktop app.
 *
 * @param ui The app.
 * @return The group box.
 */
GtkWidget *pspdisp_gtk_page_wlan_port_new(PSPdispUI *ui) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	GtkWidget *offset = pspdisp_ui_bind(ui, gtk_spin_button_new_with_range(0, PSPDISP_SETTINGS_MAX_PORT_OFFSET, 1), PSPDISP_UI_BINDING_SPIN, "WLAN", "PortOffset", offsetof(PSPdispSettings, wlan_port_offset), NULL);

	gtk_box_append(GTK_BOX(row), gtk_label_new("Port offset"));
	gtk_box_append(GTK_BOX(row), offset);

	GtkWidget *paragraph = pspdisp_ui_component_paragraph_new(
		"The server listens on the port 17584 plus this offset. "
		"Set the same \"Port offset (instance)\" in the \"WLAN Host IP Settings\" of the PSP app.");
	gtk_widget_add_css_class(paragraph, "dim-label");

	gtk_box_append(GTK_BOX(box), row);
	gtk_box_append(GTK_BOX(box), paragraph);
	return pspdisp_ui_component_groupbox_new("Port", box);
}

/**
 * @brief Make the group of the IP addresses: the addresses of this computer to set on the PSP, and what the WLAN server is doing.
 *
 * @param ui The app.
 * @param page The page, whose labels are made.
 * @return The group box.
 */
GtkWidget *pspdisp_gtk_page_wlan_ip_address_new(PSPdispUI *ui, PSPdispGtkPageWLAN *page) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	GtkWidget *paragraph = pspdisp_ui_component_paragraph_new(
		"Set one of these addresses of this PC as a \"Host IP address\" in the \"WLAN Host IP Settings\" of the PSP app, "
		"then start the WLAN mode on the PSP.");

	char *addresses = pspdisp_gtk_page_wlan_addresses();
	page->addresses = gtk_label_new(addresses);
	gtk_label_set_xalign(GTK_LABEL(page->addresses), 0.0f);
	gtk_label_set_selectable(GTK_LABEL(page->addresses), TRUE);
	gtk_widget_add_css_class(page->addresses, "monospace");
	g_free(addresses);

	page->server = pspdisp_ui_component_paragraph_new("");

	GtkWidget *refresh = gtk_button_new_with_label("Refresh");
	gtk_widget_set_halign(refresh, GTK_ALIGN_START);
	g_signal_connect(refresh, "clicked", G_CALLBACK(pspdisp_gtk_page_wlan_on_refresh_addresses), page);

	gtk_box_append(GTK_BOX(box), paragraph);
	gtk_box_append(GTK_BOX(box), page->addresses);
	gtk_box_append(GTK_BOX(box), refresh);
	gtk_box_append(GTK_BOX(box), page->server);

	pspdisp_ui_on_status(ui, (PSPdispUIRefresh) pspdisp_gtk_page_wlan_status, page);
	return pspdisp_ui_component_groupbox_new("IP address", box);
}

/**
 * @brief Make the "WLAN" page: the notifications, the password, the port and the IP addresses of the WLAN mode.
 *
 * @param ui The app.
 * @return The page.
 */
GtkWidget *pspdisp_gtk_page_wlan_new(PSPdispUI *ui) {
	PSPdispGtkPageWLAN *page = g_new0(PSPdispGtkPageWLAN, 1);
	GtkWidget *box;
	GtkWidget *scroller = pspdisp_ui_component_page_new(&box);

	gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_wlan_notifications_new(ui));
	gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_wlan_password_new(ui, page));
	gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_wlan_port_new(ui));
	gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_wlan_ip_address_new(ui, page));

	g_signal_connect_swapped(scroller, "destroy", G_CALLBACK(g_free), page);
	return scroller;
}
