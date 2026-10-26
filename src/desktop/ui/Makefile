# This file uses the UI_* prefix to avoid conflicts with other Makefile variables when it's included

# The folder of this file, absolute (and not ending with /), as the paths in an included Makefile are relative to where make runs
UI_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# CC has a default of make (cc), so check that it was given (on the command line, in the environment or in a Makefile)
ifneq ($(filter default undefined,$(origin CC)),)
$(error CC is not set, give the C compiler (e.g. make CC=gcc), or run make from the root of the repository)
endif
ifeq ($(strip $(CC)),)
$(error CC is empty, give the C compiler (e.g. make CC=gcc))
endif

ifeq ($(strip $(BIN_DIR)),)
$(error BIN_DIR is not set, give the folder of the binaries (e.g. make BIN_DIR=build), or run make from the root of the repository)
endif
ifeq ($(strip $(DEP_DIR)),)
$(error DEP_DIR is not set, give the folder of the .d files (e.g. make DEP_DIR=build/deps), or run make from the root of the repository)
endif

# The libraries: GTK, and the keyboard layout of the desktop (see controller/input/keymap.c)
UI_PACKAGES := gtk4 xkbcommon xkbcommon-x11 xcb wayland-client

# Flags for the C compiler
UI_CFLAGS   := $(CFLAGS)
UI_CPPFLAGS := $(CPPFLAGS) $(shell pkg-config --cflags $(UI_PACKAGES))
UI_LDFLAGS  := $(LDFLAGS)
UI_LDLIBS   := $(LDLIBS) $(shell pkg-config --libs $(UI_PACKAGES))

# The binary of the app
$(BIN_DIR)/pspdisp-ui: $(UI_DIR)/main.c | $(BIN_DIR) $(DEP_DIR)
	$(CC) $(UI_CPPFLAGS) -MMD -MP -MF $(DEP_DIR)/$(@F).d -MT $@ $(UI_CFLAGS) $(UI_LDFLAGS) $< -o $@ $(UI_LDLIBS)

# The icon of the app
$(BIN_DIR)/com.pspdisp.ui.svg: $(UI_DIR)/com.pspdisp.ui.svg
	install -Dm644 $< $@

# The desktop entry of the app
$(BIN_DIR)/com.pspdisp.ui.desktop: $(UI_DIR)/com.pspdisp.ui.desktop
	install -Dm644 $< $@

# Include the dependencies to build the binary, the .d file is written when the binary is compiled
-include $(DEP_DIR)/pspdisp-ui.d
