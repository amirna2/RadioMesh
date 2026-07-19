# RadioMesh — Zephyr port developer workflow (the dev + CI contract).
#
#   make setup                      one-time / post-`west update`: verify tools, (re-)apply patches
#   make build ROLE=tx|rx           build an example into an out-of-tree build dir
#   make flash ROLE=tx|rx [PORT=]   flash a board (PORT=auto detects a single board)
#   make monitor [PORT=]            reconnecting serial console (Ctrl-C to stop)
#   make run ROLE=tx|rx [PORT=]     build + flash + monitor
#   make boards                     list connected boards and the role each is running
#   make clean [ROLE=tx|rx]         remove build dir(s)
#
# Requirements: an activated west venv (so `west` is on PATH) plus the Zephyr SDK
# and host tools (cmake/ninja/dtc/gperf). `make setup` checks these. If you would
# rather not activate the venv, pass VENV=/path/to/venv. See DEVELOPING.md.

SHELL := /bin/bash
.DEFAULT_GOAL := help

# --- Locations (all derived; nothing machine-specific hardcoded) -----------
HERE  := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BOARD ?= xiao_esp32s3/esp32s3/procpu
ROLE  ?=
PORT  ?= auto
RMON  := $(HERE)/tools/rmon.py
PATCH := $(HERE)/patches/0001-gpio_esp32-BIT64-valid-mask.patch

# --- Tools -----------------------------------------------------------------
# VENV=/path selects a venv. We reference west/python by ABSOLUTE path (so the
# parse-time `west topdir` resolves without relying on PATH — macOS `make` is
# GNU make 3.81, which doesn't export PATH to parse-time $(shell)), AND export
# the venv bin onto PATH so tools west shells out to at build time (esptool,
# cmake) resolve too. Without VENV, activate your venv first.
ifdef VENV
  VBIN := $(VENV)/bin/
  export PATH := $(VENV)/bin:$(PATH)
else
  VBIN :=
endif
WEST   := $(VBIN)west
PYTHON := $(VBIN)python3

# west workspace topdir + Zephyr base, discovered from the workspace itself.
WORKSPACE   := $(shell $(WEST) topdir 2>/dev/null)
ZEPHYR_BASE := $(WORKSPACE)/zephyr
BUILD_DIR    = $(WORKSPACE)/.build-$(ROLE)   # deferred: depends on ROLE per target

VALID_ROLES := tx rx

.PHONY: help setup build flash monitor run boards clean _need_west _need_role

help:
	@echo "RadioMesh Zephyr workflow:"
	@grep -E '^#   make ' $(firstword $(MAKEFILE_LIST)) | sed 's/^#   /  /'

# --- Guards ----------------------------------------------------------------
_need_west:
	@command -v $(WEST) >/dev/null 2>&1 || { \
	  echo "error: 'west' not found. Activate your Zephyr venv, or pass VENV=/path/to/venv."; exit 1; }
	@[ -n "$(WORKSPACE)" ] || { \
	  echo "error: not in a west workspace ('west topdir' failed). See DEVELOPING.md (west init -l)."; exit 1; }

_need_role:
	@echo "$(VALID_ROLES)" | grep -qw "$(ROLE)" || { \
	  echo "error: ROLE must be one of [$(VALID_ROLES)] (got '$(ROLE)'). e.g. make build ROLE=tx"; exit 1; }

# --- Targets ---------------------------------------------------------------
setup: _need_west
	@echo "west workspace : $(WORKSPACE)"
	@echo "zephyr base    : $(ZEPHYR_BASE)"
	@if grep -q 'BIT64(pin)' "$(ZEPHYR_BASE)/drivers/gpio/gpio_esp32.c" 2>/dev/null; then \
	  echo "gpio_esp32 patch: already applied"; \
	elif git -C "$(ZEPHYR_BASE)" apply --check "$(PATCH)" >/dev/null 2>&1; then \
	  git -C "$(ZEPHYR_BASE)" apply "$(PATCH)" && echo "gpio_esp32 patch: applied"; \
	else \
	  echo "gpio_esp32 patch: FAILED to apply cleanly against $(ZEPHYR_BASE)"; exit 1; \
	fi
	@for t in cmake ninja dtc gperf; do \
	  command -v $$t >/dev/null 2>&1 || echo "warning: host tool '$$t' not found on PATH"; \
	done
	@echo "setup OK."

build: _need_west _need_role
	$(WEST) build -b $(BOARD) $(HERE)/examples/$(ROLE) -d $(BUILD_DIR) -p auto

flash: _need_west _need_role
	@port=$$($(PYTHON) $(RMON) resolve --port "$(PORT)") || exit 1; \
	echo "[flash] $(ROLE) -> $$port"; \
	$(WEST) flash -d $(BUILD_DIR) --esp-device $$port

monitor: _need_west
	@$(PYTHON) $(RMON) monitor --port "$(PORT)"

run:
	@$(MAKE) build ROLE=$(ROLE)
	@$(MAKE) flash ROLE=$(ROLE) PORT=$(PORT)
	@$(MAKE) monitor PORT=$(PORT)

boards:
	@$(PYTHON) $(RMON) boards

clean: _need_west
ifeq ($(ROLE),)
	rm -rf $(WORKSPACE)/.build-tx $(WORKSPACE)/.build-rx
else
	rm -rf $(WORKSPACE)/.build-$(ROLE)
endif
