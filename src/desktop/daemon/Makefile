# This file uses the DAEMON_* prefix to avoid conflicts with other Makefile variables when it's included

# The folder of this file, absolute (and not ending with /), as the paths in an included Makefile are relative to where make runs
DAEMON_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

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

# The libraries: the USB host, the sound, the JPEG frames, and the keyboard layout of the desktop (see controller/input/keymap.c)
DAEMON_PACKAGES := glib-2.0 gio-2.0 gusb libpipewire-0.3 libturbojpeg xkbcommon xkbcommon-x11 xcb wayland-client

# Flags for the C compiler, with the library of the display connector compiled in by display/connector.c
DAEMON_CFLAGS   := $(CFLAGS)
DAEMON_CPPFLAGS := $(CPPFLAGS) $(shell pkg-config --cflags $(DAEMON_PACKAGES))
DAEMON_LDFLAGS  := $(LDFLAGS)
DAEMON_LDLIBS   := $(LDLIBS) $(shell pkg-config --libs $(DAEMON_PACKAGES)) -levdi -lm

# The binary of the daemon, the name the desktop app looks for in the PATH to start it (see ui/app.c)
$(BIN_DIR)/pspdisp-daemon: $(DAEMON_DIR)/main.c | $(BIN_DIR) $(DEP_DIR)
	$(CC) $(DAEMON_CPPFLAGS) -MMD -MP -MF $(DEP_DIR)/$(@F).d -MT $@ $(DAEMON_CFLAGS) $(DAEMON_LDFLAGS) $< -o $@ $(DAEMON_LDLIBS)

# Include the dependencies to build the binary, the .d file is written when the binary is compiled
-include $(DEP_DIR)/pspdisp-daemon.d
