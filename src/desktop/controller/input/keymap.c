/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * keymap.c - Keyboard layout of the desktop         *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * The keyboard layout of the desktop, to type characters on the uinput keyboard (see `pspdisp_input_type()`).
 *
 * A uinput keyboard only presses keys, and the desktop turns them into characters with its layout, so a character is typed with the key (and the modifiers) that writes it in that layout.
 * The layout comes from the display server, as any app gets it: the keymap of the keyboard of the Wayland seat, or the keymap and the active layout of the core keyboard of X11.
 * libxkbcommon then finds the key of a character in it.
 *
 * On Wayland, only the window with the keyboard focus is told which layout is active, so with several layouts the first one is used.
 * On X11, the active layout is read each time, but a change of the layouts themselves is only seen by a new virtual keyboard.
 */

#pragma once

#include <poll.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/input-event-codes.h>
#include <glib.h>
#include <wayland-client.h>
#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>

#define PSPDISP_INPUT_KEYMAP_MAX_KEYS 3 // Keys pressed together to type a character: Shift, AltGr and the key
#define PSPDISP_INPUT_KEYMAP_EVDEV_OFFSET 8 // The key codes of XKB are the key codes of Linux plus 8

/**
 * @brief Type of a function pointer to tell if a key can be pressed, as the virtual keyboard may not have every key of the layout.
 *
 * @param code The key code of Linux.
 * @return `true` if it can be pressed.
 */
typedef bool (*PSPdispInputKeymapUsable) (uint16_t code);

/**
 * @brief The keyboard layout of the desktop, from Wayland or X11.
 */
typedef struct {
	struct xkb_context *context; // The context of libxkbcommon
	struct xkb_keymap *keymap; // The keymap of the desktop (`NULL` until the compositor sends it)
	xkb_layout_index_t layout; // The layout of the keymap that types
	struct wl_display *display; // The Wayland connection (`NULL` on X11)
	struct wl_registry *registry; // The globals of Wayland
	struct wl_seat *seat; // The seat
	struct wl_keyboard *keyboard; // The keyboard of the seat, which gets the keymap
	xcb_connection_t *xcb; // The X11 connection (`NULL` on Wayland)
	int32_t device; // The core keyboard of X11
} PSPdispInputKeymap;

/**
 * @brief Load the keymap sent by the compositor, which sends it again when it changes.
 *
 * @param data The layout.
 * @param keyboard Unused, the keyboard of the seat.
 * @param format The format of the keymap.
 * @param fd The file of the keymap, closed here.
 * @param size The size of the keymap in bytes.
 */
void pspdisp_input_keymap_on_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {
	(void) keyboard;
	PSPdispInputKeymap *keymap = data;
	char *text = format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 ? mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0) : MAP_FAILED;

	if (text != MAP_FAILED) {
		struct xkb_keymap *loaded = xkb_keymap_new_from_buffer(keymap->context, text, strnlen(text, size), XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
		munmap(text, size);

		if (loaded != NULL) {
			xkb_keymap_unref(keymap->keymap);
			keymap->keymap = loaded;
		}
	}
	close(fd);
}

/**
 * @brief Ignore the keyboard focus, the layout doesn't need it.
 *
 * @param data Unused, the layout.
 * @param keyboard Unused, the keyboard of the seat.
 * @param serial Unused, the serial of the event.
 * @param surface Unused, the surface with the focus.
 * @param keys Unused, the keys pressed.
 */
void pspdisp_input_keymap_on_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
	(void) data;
	(void) keyboard;
	(void) serial;
	(void) surface;
	(void) keys;
}

/**
 * @brief Ignore the loss of the keyboard focus, the layout doesn't need it.
 *
 * @param data Unused, the layout.
 * @param keyboard Unused, the keyboard of the seat.
 * @param serial Unused, the serial of the event.
 * @param surface Unused, the surface that had the focus.
 */
void pspdisp_input_keymap_on_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {
	(void) data;
	(void) keyboard;
	(void) serial;
	(void) surface;
}

/**
 * @brief Ignore the keys of the seat, the layout doesn't need them.
 *
 * @param data Unused, the layout.
 * @param keyboard Unused, the keyboard of the seat.
 * @param serial Unused, the serial of the event.
 * @param time Unused, the time of the key.
 * @param key Unused, the key.
 * @param state Unused, if it was pressed or released.
 */
void pspdisp_input_keymap_on_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
	(void) data;
	(void) keyboard;
	(void) serial;
	(void) time;
	(void) key;
	(void) state;
}

/**
 * @brief Ignore the modifiers of the seat, only sent to the window with the keyboard focus.
 *
 * @param data Unused, the layout.
 * @param keyboard Unused, the keyboard of the seat.
 * @param serial Unused, the serial of the event.
 * @param depressed Unused, the modifiers held.
 * @param latched Unused, the modifiers latched.
 * @param locked Unused, the modifiers locked.
 * @param group Unused, the active layout.
 */
void pspdisp_input_keymap_on_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) {
	(void) data;
	(void) keyboard;
	(void) serial;
	(void) depressed;
	(void) latched;
	(void) locked;
	(void) group;
}

// The events of the keyboard of the seat (version 1)
const struct wl_keyboard_listener pspdisp_input_keymap_keyboard_listener = {
	.keymap = pspdisp_input_keymap_on_keymap,
	.enter = pspdisp_input_keymap_on_enter,
	.leave = pspdisp_input_keymap_on_leave,
	.key = pspdisp_input_keymap_on_key,
	.modifiers = pspdisp_input_keymap_on_modifiers,
};

/**
 * @brief Get the keyboard of the seat when it has one, for its keymap.
 *
 * @param data The layout.
 * @param seat The seat.
 * @param capabilities The devices of the seat (`WL_SEAT_CAPABILITY_*`).
 */
void pspdisp_input_keymap_on_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
	PSPdispInputKeymap *keymap = data;

	if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && keymap->keyboard == NULL) {
		keymap->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(keymap->keyboard, &pspdisp_input_keymap_keyboard_listener, keymap);
	}
}

// The events of the seat (version 1)
const struct wl_seat_listener pspdisp_input_keymap_seat_listener = {
	.capabilities = pspdisp_input_keymap_on_capabilities,
};

/**
 * @brief Bind the first seat of the compositor.
 *
 * @param data The layout.
 * @param registry The globals of Wayland.
 * @param name The name of the global.
 * @param interface The interface of the global.
 * @param version Unused, the version of the interface.
 */
void pspdisp_input_keymap_on_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
	(void) version;
	PSPdispInputKeymap *keymap = data;

	if (strcmp(interface, wl_seat_interface.name) == 0 && keymap->seat == NULL) {
		keymap->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
		wl_seat_add_listener(keymap->seat, &pspdisp_input_keymap_seat_listener, keymap);
	}
}

/**
 * @brief Ignore the globals removed, the seat stays as long as the session.
 *
 * @param data Unused, the layout.
 * @param registry Unused, the globals of Wayland.
 * @param name Unused, the name of the global.
 */
void pspdisp_input_keymap_on_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
	(void) data;
	(void) registry;
	(void) name;
}

// The events of the globals of Wayland
const struct wl_registry_listener pspdisp_input_keymap_registry_listener = {
	.global = pspdisp_input_keymap_on_global,
	.global_remove = pspdisp_input_keymap_on_global_remove,
};

/**
 * @brief Free the layout, and close its connection to the display server.
 *
 * @param keymap The layout.
 */
void pspdisp_input_keymap_destroy(PSPdispInputKeymap *keymap) {
	if (keymap->keyboard != NULL)
		wl_keyboard_destroy(keymap->keyboard);
	if (keymap->seat != NULL)
		wl_seat_destroy(keymap->seat);
	if (keymap->registry != NULL)
		wl_registry_destroy(keymap->registry);
	if (keymap->display != NULL)
		wl_display_disconnect(keymap->display);
	if (keymap->xcb != NULL)
		xcb_disconnect(keymap->xcb);

	xkb_keymap_unref(keymap->keymap);
	xkb_context_unref(keymap->context);
	g_free(keymap);
}

/**
 * @brief Get the keyboard layout of the desktop, from Wayland, or from X11 without Wayland.
 *
 * @return The layout, free it with `pspdisp_input_keymap_destroy()`, or `NULL` if it failed.
 */
PSPdispInputKeymap *pspdisp_input_keymap_new() {
	PSPdispInputKeymap *keymap = g_new0(PSPdispInputKeymap, 1);
	keymap->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	// Wayland first: in a Wayland session, X11 is only Xwayland
	if (keymap->context == NULL) {
		// No layout without libxkbcommon
	} else if (g_getenv("WAYLAND_DISPLAY") != NULL && (keymap->display = wl_display_connect(NULL)) != NULL) {
		keymap->registry = wl_display_get_registry(keymap->display);
		wl_registry_add_listener(keymap->registry, &pspdisp_input_keymap_registry_listener, keymap);

		// The globals, then the devices of the seat, then the keymap of its keyboard
		for (int i = 0; i < 4 && keymap->keymap == NULL; i++)
			wl_display_roundtrip(keymap->display);
	} else if (g_getenv("DISPLAY") != NULL) {
		keymap->xcb = xcb_connect(NULL, NULL);

		bool xkb = xcb_connection_has_error(keymap->xcb) == 0 && xkb_x11_setup_xkb_extension(keymap->xcb, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION, XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, NULL, NULL, NULL, NULL);
		keymap->device = xkb ? xkb_x11_get_core_keyboard_device_id(keymap->xcb) : -1;
		if (keymap->device >= 0)
			keymap->keymap = xkb_x11_keymap_new_from_device(keymap->context, keymap->xcb, keymap->device, XKB_KEYMAP_COMPILE_NO_FLAGS);
	}

	if (keymap->keymap == NULL) {
		pspdisp_input_keymap_destroy(keymap);
		return NULL;
	}
	return keymap;
}

/**
 * @brief Take the changes of the layout: a new keymap sent by the compositor (Wayland), or the active layout (X11).
 *
 * @param keymap The layout.
 */
void pspdisp_input_keymap_update(PSPdispInputKeymap *keymap) {
	if (keymap->display != NULL && wl_display_get_error(keymap->display) == 0) {
		// The events already received, without waiting for more
		while (wl_display_prepare_read(keymap->display) != 0) {
			if (wl_display_dispatch_pending(keymap->display) < 0) return;
		}
		wl_display_flush(keymap->display);

		struct pollfd fd = { .fd = wl_display_get_fd(keymap->display), .events = POLLIN };
		if (poll(&fd, 1, 0) > 0)
			wl_display_read_events(keymap->display);
		else
			wl_display_cancel_read(keymap->display);
		wl_display_dispatch_pending(keymap->display);
	} else if (keymap->xcb != NULL) {
		struct xkb_state *state = xkb_x11_state_new_from_device(keymap->keymap, keymap->xcb, keymap->device);

		if (state != NULL) {
			keymap->layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
			xkb_state_unref(state);
		}
	}
}

/**
 * @brief Get the mask of a modifier of the keymap.
 *
 * @param keymap The keymap.
 * @param name The name of the modifier (e.g. `XKB_MOD_NAME_SHIFT`).
 * @return The mask, or 0 if the keymap has no such modifier.
 */
xkb_mod_mask_t pspdisp_input_keymap_mod_mask(struct xkb_keymap *keymap, const char *name) {
	xkb_mod_index_t index = xkb_keymap_mod_get_index(keymap, name);
	return index != XKB_MOD_INVALID ? (xkb_mod_mask_t) 1 << index : 0;
}

/**
 * @brief Find the key of a keysym in the layout.
 *
 * @param keymap The layout.
 * @param keysym The keysym.
 * @param modifiers The modifiers that may be held for it (none for a modifier key).
 * @param usable Tells if a key can be pressed.
 * @param level Where to write the level of the key that types it.
 * @param mask Where to write the modifiers that type it.
 * @return The key code of XKB, or `XKB_KEYCODE_INVALID` if no key types it.
 */
xkb_keycode_t pspdisp_input_keymap_find_keysym(PSPdispInputKeymap *keymap, xkb_keysym_t keysym, xkb_mod_mask_t modifiers, PSPdispInputKeymapUsable usable, xkb_level_index_t *level, xkb_mod_mask_t *mask) {
	xkb_keycode_t found = XKB_KEYCODE_INVALID;
	const xkb_keycode_t max = xkb_keymap_max_keycode(keymap->keymap);

	for (xkb_keycode_t keycode = xkb_keymap_min_keycode(keymap->keymap); keycode <= max; keycode++) {
		xkb_layout_index_t layouts = xkb_keymap_num_layouts_for_key(keymap->keymap, keycode);
		if (layouts == 0 || !usable(keycode - PSPDISP_INPUT_KEYMAP_EVDEV_OFFSET)) continue;

		const xkb_layout_index_t layout = keymap->layout < layouts ? keymap->layout : 0;
		const xkb_level_index_t levels = xkb_keymap_num_levels_for_key(keymap->keymap, keycode, layout);

		// The lowest level wins, e.g. a character without Shift rather than with it
		for (xkb_level_index_t i = 0; i < levels && (found == XKB_KEYCODE_INVALID || i < *level); i++) {
			const xkb_keysym_t *syms;
			if (xkb_keymap_key_get_syms_by_level(keymap->keymap, keycode, layout, i, &syms) != 1 || syms[0] != keysym) continue;

			xkb_mod_mask_t masks[16];
			const size_t count = xkb_keymap_key_get_mods_for_level(keymap->keymap, keycode, layout, i, masks, G_N_ELEMENTS(masks));

			for (size_t j = 0; j < count; j++) {
				if (masks[j] & ~modifiers) continue;

				found = keycode;
				*level = i;
				*mask = masks[j];
				break;
			}
		}
	}

	return found;
}

/**
 * @brief Get the key code of Linux of a modifier key of the layout.
 *
 * @param keymap The layout.
 * @param keysym The keysym of the modifier (e.g. `XKB_KEY_Shift_L`).
 * @param usable Tells if a key can be pressed.
 * @param fallback The key code of Linux when the layout has no such key.
 * @return The key code of Linux.
 */
uint16_t pspdisp_input_keymap_modifier_key(PSPdispInputKeymap *keymap, xkb_keysym_t keysym, PSPdispInputKeymapUsable usable, uint16_t fallback) {
	xkb_level_index_t level;
	xkb_mod_mask_t mask;
	xkb_keycode_t keycode = pspdisp_input_keymap_find_keysym(keymap, keysym, 0, usable, &level, &mask);
	return keycode != XKB_KEYCODE_INVALID ? keycode - PSPDISP_INPUT_KEYMAP_EVDEV_OFFSET : fallback;
}

/**
 * @brief Find the keys that type a character in the layout: the modifiers it needs (Shift, AltGr), then its key.
 *
 * @param keymap The layout.
 * @param character The character, a Unicode code point.
 * @param usable Tells if a key can be pressed, so the character is typed with the keys that can be.
 * @param keys Where to write the key codes of Linux to press together, in order, `PSPDISP_INPUT_KEYMAP_MAX_KEYS` at most.
 * @return The number of keys, or 0 if no key types it.
 */
size_t pspdisp_input_keymap_find(PSPdispInputKeymap *keymap, uint32_t character, PSPdispInputKeymapUsable usable, uint16_t *keys) {
	pspdisp_input_keymap_update(keymap);

	const xkb_keysym_t keysym = xkb_utf32_to_keysym(character);
	if (keysym == XKB_KEY_NoSymbol) return 0;

	// Shift, and the level 3 of xkeyboard-config (AltGr), on Mod5: the only modifiers that type characters
	const xkb_mod_mask_t shift = pspdisp_input_keymap_mod_mask(keymap->keymap, XKB_MOD_NAME_SHIFT);
	const xkb_mod_mask_t level3 = pspdisp_input_keymap_mod_mask(keymap->keymap, "Mod5") | pspdisp_input_keymap_mod_mask(keymap->keymap, "LevelThree");

	xkb_level_index_t level;
	xkb_mod_mask_t mask;
	const xkb_keycode_t keycode = pspdisp_input_keymap_find_keysym(keymap, keysym, shift | level3, usable, &level, &mask);
	if (keycode == XKB_KEYCODE_INVALID) return 0;

	size_t count = 0;
	if (mask & shift)
		keys[count++] = pspdisp_input_keymap_modifier_key(keymap, XKB_KEY_Shift_L, usable, KEY_LEFTSHIFT);
	if (mask & level3)
		keys[count++] = pspdisp_input_keymap_modifier_key(keymap, XKB_KEY_ISO_Level3_Shift, usable, KEY_RIGHTALT);
	keys[count++] = keycode - PSPDISP_INPUT_KEYMAP_EVDEV_OFFSET;
	return count;
}
