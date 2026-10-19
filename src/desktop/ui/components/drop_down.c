/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * drop_down.c - The items of a drop down            *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The items of a drop down made from a string list, set again when they change (e.g. the presets), keeping the drop down on the item the setting has.
 */

#pragma once

#include <string.h>
#include <gtk/gtk.h>

/**
 * @brief Set the items of a drop down if they changed, then select an item.
 *
 * @param list The string list of the drop down.
 * @param drop_down The drop down.
 * @param items The items, ended by `NULL`.
 * @param selected The item to select.
 */
void pspdisp_ui_component_drop_down_set_items(GtkStringList *list, GtkWidget *drop_down, const char *const *items, guint selected) {
    guint count = g_list_model_get_n_items(G_LIST_MODEL(list));
    bool same = count == g_strv_length((char **) items);

    for (guint i = 0; same && i < count; i++)
        same = strcmp(gtk_string_list_get_string(list, i), items[i]) == 0;
    if (!same)
        gtk_string_list_splice(list, 0, count, items);

    if (gtk_drop_down_get_selected(GTK_DROP_DOWN(drop_down)) != selected)
        gtk_drop_down_set_selected(GTK_DROP_DOWN(drop_down), selected);
}
