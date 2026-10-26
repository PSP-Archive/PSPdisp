VERSION != git describe --tags

# The folder of this file, absolute (and not ending with /), as the sub-makes run from other folders (-C)
DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

BIN_DIR = $(DIR)/build
DEP_DIR = $(BIN_DIR)/dep

.PHONY: all

# Note: "make" will run only the first entry, which must be all files
all: $(BIN_DIR)/pspdisp-daemon $(BIN_DIR)/pspdisp-ui $(BIN_DIR)/com.pspdisp.ui.svg $(BIN_DIR)/com.pspdisp.ui.desktop

# The folders of the build, created before a program is built in them (each program has them as order-only prerequisites)
$(BIN_DIR) $(DEP_DIR):
	mkdir -p $@

$(BIN_DIR)/pspdisp-daemon $(BIN_DIR)/pspdisp-ui $(BIN_DIR)/com.pspdisp.ui.svg $(BIN_DIR)/com.pspdisp.ui.desktop: $(BIN_DIR) $(DEP_DIR)
	$(MAKE) -C $(DIR)/src/desktop $@ VERSION=$(VERSION) BIN_DIR=$(BIN_DIR) DEP_DIR=$(DEP_DIR)

