# RadioMesh — Zephyr port developer workflow.
#
# Run every target from this directory (RadioMesh/ports/zephyr/).
# Run `make` (or `make help`) for usage. Full guide: DEVELOPING.md.
#
# Target tiers:
#   * setup / build / flash / run  need the Zephyr toolchain (west + SDK + host
#     tools). Activate your west venv first, or pass VENV=/path/to/venv.
#   * boards / monitor / list      are pure serial helpers with NO dependencies;
#     they run under any system python3 (no venv, no pip).

SHELL := /bin/bash
.DEFAULT_GOAL := help

# --- Locations (all derived; nothing machine-specific hardcoded) -----------
HERE   := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
REPO   := $(patsubst %/,%,$(abspath $(HERE)/../..))
TOPDIR := $(patsubst %/,%,$(abspath $(REPO)/..))
BOARD  ?= xiao_esp32s3/esp32s3/procpu
ROLE   ?=
PORT   ?= auto
RMON   := $(HERE)/tools/rmon.py
PATCH  := $(HERE)/patches/0001-gpio_esp32-BIT64-valid-mask.patch
BOOTSTRAP_VENV := $(TOPDIR)/.venv-zephyr

# --- Tools -----------------------------------------------------------------
# `make bootstrap` creates a venv at BOOTSTRAP_VENV; when present it is used
# automatically. Otherwise activate your own venv, or pass VENV=/path. west and
# python are called by absolute path (parse-time `west topdir` must resolve
# without PATH — macOS make is GNU make 3.81); the venv bin is also exported so
# tools west spawns (esptool) resolve.
VENV ?= $(wildcard $(BOOTSTRAP_VENV))
ifneq ($(strip $(VENV)),)
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

.PHONY: help bootstrap setup build flash monitor run boards clean _need_west _need_role

help:
	@echo 'RadioMesh Zephyr examples — run from ports/zephyr/'
	@echo ''
	@echo '  make bootstrap        first-time setup: venv + Zephyr workspace + SDK'
	@echo '  make setup            re-apply driver patch (after every west update)'
	@echo '  make boards           list connected boards and their role'
	@echo '  make build ROLE=tx    build an example (tx|rx)'
	@echo '  make flash ROLE=tx    flash a board (set PORT=/dev/... if more than one)'
	@echo '  make monitor          serial console (Ctrl-C to quit)'
	@echo '  make run ROLE=tx      build + flash + monitor'
	@echo '  make clean            remove build outputs'
	@echo ''
	@echo 'Guide: DEVELOPING.md'

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
bootstrap:
	@command -v python3 >/dev/null 2>&1 || { echo "error: python3 not found"; exit 1; }
	python3 -m venv "$(BOOTSTRAP_VENV)"
	"$(BOOTSTRAP_VENV)/bin/pip" install --quiet --upgrade pip west
	@if [ -e "$(TOPDIR)/.west" ]; then echo "west workspace already initialized — skipping init"; \
	else ( cd "$(TOPDIR)" && "$(BOOTSTRAP_VENV)/bin/west" init -l "$(REPO)" ); fi
	cd "$(TOPDIR)" && "$(BOOTSTRAP_VENV)/bin/west" update
	cd "$(TOPDIR)" && "$(BOOTSTRAP_VENV)/bin/west" zephyr-export
	"$(BOOTSTRAP_VENV)/bin/pip" install --quiet -r "$(TOPDIR)/zephyr/scripts/requirements.txt"
	cd "$(TOPDIR)" && "$(BOOTSTRAP_VENV)/bin/west" packages pip --install
	cd "$(TOPDIR)" && "$(BOOTSTRAP_VENV)/bin/west" sdk install -t xtensa-espressif_esp32s3_zephyr-elf
	@$(MAKE) --no-print-directory setup VENV="$(BOOTSTRAP_VENV)"
	@echo ''
	@echo 'Bootstrap complete. Next: make build ROLE=tx'

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

monitor:
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
