# This file uses the DESKTOP_* prefix to avoid conflicts with other Makefile variables when it's included

# The folder of this file, absolute (and not ending with /), as the paths in an included Makefile are relative to where make runs
DESKTOP_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

ifeq ($(strip $(BIN_DIR)),)
$(error BIN_DIR is not set, give the folder of the binaries (e.g. make BIN_DIR=build), or run make from the root of the repository)
endif
ifeq ($(strip $(DEP_DIR)),)
$(error DEP_DIR is not set, give the folder of the .d files (e.g. make DEP_DIR=build/deps), or run make from the root of the repository)
endif
ifeq ($(strip $(VERSION)),)
$(error VERSION is not set, give the version of the programs (e.g. make VERSION=0.7.0), or run make from the root of the repository)
endif

# C compiler
CC := gcc

# Flags for the C compiler, the ones of the user last, so they win (e.g. `CFLAGS=-O0 make`), with the version for the programs that include settings.c
CFLAGS   := -std=gnu23 -Wall -Wextra -Wno-pragma-once-outside-header $(CFLAGS)
CPPFLAGS := $(CPPFLAGS) -DPSPDISP_VERSION='"$(VERSION)"'
LDFLAGS  := $(LDFLAGS)
LDLIBS   := $(LDLIBS)

# The programs
include $(DESKTOP_DIR)/daemon/Makefile
include $(DESKTOP_DIR)/ui/Makefile
