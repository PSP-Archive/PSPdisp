#ifndef PSPDISP_DESKTOP_UI_PAGE_SOUND
#define PSPDISP_DESKTOP_UI_PAGE_SOUND

#include <gtk/gtk.h>
#include "../groupbox.c"

// Audio input device component
static GtkWidget *pspdisp_gtk_page_sound_device_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *paragraph = gtk_label_new(
        "Choose the audio recording device that should be used for "
        "capturing sounds.");
    gtk_label_set_wrap(GTK_LABEL(paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(paragraph), 50);

    // TODO: fill with the recording devices (PipeWire / PulseAudio sources?)
    GtkWidget *device = gtk_drop_down_new_from_strings((const char *[]){ NULL });
    gtk_widget_set_hexpand(device, TRUE);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(row), device);
    gtk_box_append(GTK_BOX(row), gtk_button_new_with_label("Refresh"));

    GtkWidget *hint = gtk_label_new("* = recommended setting");
    gtk_label_set_xalign(GTK_LABEL(hint), 0.0f);

    gtk_box_append(GTK_BOX(box), paragraph);
    gtk_box_append(GTK_BOX(box), row);
    gtk_box_append(GTK_BOX(box), hint);

    return pspdisp_gtk_groupbox_new("Audio input device", box);
}

// Instructions component
static GtkWidget *pspdisp_gtk_page_sound_instructions_new() {
    // TODO: the original text is about Windows mixers, rewrite it for Linux
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *xp_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(xp_title), "<b>Windows 2000 / XP</b>");
    gtk_label_set_xalign(GTK_LABEL(xp_title), 0.0f);

    GtkWidget *xp_paragraph = gtk_label_new(
        "Choose your main sound card from the list above, then enable "
        "'Stereo Mix' or 'What you hear' in the recording mixer.");
    gtk_label_set_wrap(GTK_LABEL(xp_paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(xp_paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(xp_paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(xp_paragraph), 50);

    GtkWidget *vista_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(vista_title), "<b>Windows Vista / 7</b>");
    gtk_label_set_xalign(GTK_LABEL(vista_title), 0.0f);
    gtk_widget_set_margin_top(vista_title, 8);

    GtkWidget *vista_paragraph = gtk_label_new(
        "If possible, enable 'Stereo Mix' in the recording settings. Then "
        "refresh the list above and choose the newly enabled input. Otherwise "
        "choose 'Software Loopback Device' which (almost) always works.");
    gtk_label_set_wrap(GTK_LABEL(vista_paragraph), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(vista_paragraph), PANGO_WRAP_WORD);
    gtk_label_set_xalign(GTK_LABEL(vista_paragraph), 0.0f);
    gtk_label_set_max_width_chars(GTK_LABEL(vista_paragraph), 50);

    GtkWidget *open_mixer = gtk_button_new_with_label("Open recording mixer / recording settings");
    gtk_widget_set_margin_top(open_mixer, 8);

    gtk_box_append(GTK_BOX(box), xp_title);
    gtk_box_append(GTK_BOX(box), xp_paragraph);
    gtk_box_append(GTK_BOX(box), vista_title);
    gtk_box_append(GTK_BOX(box), vista_paragraph);
    gtk_box_append(GTK_BOX(box), open_mixer);

    return pspdisp_gtk_groupbox_new("Instructions", box);
}

// "Sound" page component
static GtkWidget *pspdisp_gtk_page_sound_new() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);

    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_sound_device_new());
    gtk_box_append(GTK_BOX(box), pspdisp_gtk_page_sound_instructions_new());
    return box;
}

#endif
