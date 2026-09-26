# Sega Rally Championship (1995) — host-compiled lift
#
# Build is CMake. This wrapper keeps `make lift` working on macOS/Linux.
#
#   cmake -B build && cmake --build build
#   cmake --build build --target lift          # compile host (ROMs extracted at run)
#   cmake --build build --target lift-check    # compile only
#
#   make lift        # same as cmake --build build --target lift

ifeq ($(OS),Windows_NT)
  CMAKE ?= cmake
else
  CMAKE ?= cmake
endif
BUILD_DIR ?= build
GAME_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
export SEGAMOD2_ROM_DIR ?= $(GAME_ROOT)/ROMS/srallyc-b
export SEGAMOD2_ROOT ?= $(GAME_ROOT)

PYTHON := python3
ifneq ($(OS),Windows_NT)
  SHELL := /bin/bash
endif

.PHONY: all help lift lift-check lift-main clean configure \
	libmodel2_geo libmodel2_hw libmodel2_tgp libmodel2_snd \
	reference rom-blocks compare quick coverage lift-progress \
	lift-palette \
	lift-boot-viewer lift-boot-practice lift-boot-headless lift-boot-live \
	lift-cgm-decode lift-geo-compare sync-runtime tools-hint

.DEFAULT_GOAL := lift

help:
	@echo "Targets (CMake):"
	@echo "  make / make lift  — compile + link (ROM extract happens at run)"
	@echo "  make lift-check   — compile segamod2 only"
	@echo "  make lift-main    — regenerate lift_main / host glue (needs tools/)"
	@echo "  make rom-blocks   — optional Python extract of out/i960 bins"
	@echo "  make clean        — remove CMake build dir and lib build leftovers"
	@echo ""
	@echo "Direct CMake:"
	@echo "  cmake -B $(BUILD_DIR) && cmake --build $(BUILD_DIR)"

configure:
	$(CMAKE) -S "$(GAME_ROOT)" -B "$(BUILD_DIR)"

lift-check: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target lift-check

lift: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target lift

all: lift

libmodel2_geo libmodel2_hw libmodel2_tgp libmodel2_snd: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target $(@:lib%=%)

rom-blocks reference: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target rom-blocks

lift-boot-viewer lift-boot-practice lift-boot-headless lift-boot-live lift-cgm-decode: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target $@

# Optional: regenerate entry/glue only when tools are cloned.
GEN_LIFT_MAIN := $(firstword \
	$(wildcard $(GAME_ROOT)/tools/tools/decomp/gen_lift_main.py) \
	$(wildcard $(GAME_ROOT)/tools/decomp/gen_lift_main.py))

lift-main:
	@if [ -n "$(GEN_LIFT_MAIN)" ]; then \
	  echo "regenerating lift_main via $(GEN_LIFT_MAIN)"; \
	  if [ -f "$(GAME_ROOT)/tools/tools/decomp/gen_lift_main.py" ]; then \
	    PYTHONPATH="$(GAME_ROOT)/tools" $(PYTHON) -m tools.decomp.gen_lift_main \
	      -o src/lift_main.c --syms-out src/i960_host_syms.c --invoke-out src/i960_host_invoke.c; \
	  else \
	    PYTHONPATH="$(GAME_ROOT)/tools" $(PYTHON) -m decomp.gen_lift_main \
	      -o src/lift_main.c --syms-out src/i960_host_syms.c --invoke-out src/i960_host_invoke.c \
	      2>/dev/null || \
	    PYTHONPATH="$(GAME_ROOT)" $(PYTHON) "$(GEN_LIFT_MAIN)" \
	      -o src/lift_main.c --syms-out src/i960_host_syms.c --invoke-out src/i960_host_invoke.c; \
	  fi; \
	else \
	  echo "using committed lift_main (clone tools to regenerate):"; \
	  echo "  git clone git@github.com-xandoxan65:xandoxan65/segamodel2-tools.git tools"; \
	fi

tools-hint:
	@echo "Optional tools not found. Clone:"
	@echo "  git clone git@github.com-xandoxan65:xandoxan65/segamodel2-tools.git tools"

define REQUIRE_TOOLS
	@if [ ! -d "$(GAME_ROOT)/tools" ]; then \
	  echo "error: tools/ missing — clone segamodel2-tools first:" >&2; \
	  echo "  git clone git@github.com-xandoxan65:xandoxan65/segamodel2-tools.git tools" >&2; \
	  exit 1; \
	fi
endef

OUTPUT      := out/maincpu/maincpu_rebuilt.bin
LIFT_PROGRESS := out/lift/coverage.json
LIFT_BIN    := $(GAME_ROOT)/build/segamod2
LIFT_PALETTE_DUMP := build/lift/palette_state
LIFT_PALETTE_REPORT := $(GAME_ROOT)/out/lift/palette_inspect

$(OUTPUT):
	$(REQUIRE_TOOLS)
	@echo "error: maincpu stitch requires tools + reference ROM; run make rom-blocks first" >&2
	@exit 1

quick compare coverage lift-progress lift-geo-compare:
	$(REQUIRE_TOOLS)
	@echo "error: $@ requires tools/ Python modules; see README" >&2
	@exit 1

lift-palette: lift
	@I960_PALETTE_DUMP=$(LIFT_PALETTE_DUMP) "$(LIFT_BIN)"
	@if [ -d "$(GAME_ROOT)/tools" ]; then \
	  PYTHONPATH="$(GAME_ROOT)/tools" $(PYTHON) -m tools.decomp.lift_palette_inspect \
	    --dump $(LIFT_PALETTE_DUMP) -o $(LIFT_PALETTE_REPORT) 2>/dev/null || \
	  PYTHONPATH="$(GAME_ROOT)/tools" $(PYTHON) -m decomp.lift_palette_inspect \
	    --dump $(LIFT_PALETTE_DUMP) -o $(LIFT_PALETTE_REPORT); \
	else \
	  echo "palette dump written; clone tools/ for lift_palette_inspect"; \
	fi

sync-runtime:
	@if [ -d "$(GAME_ROOT)/tools/runtime" ]; then \
	  rsync -a --delete \
	    --exclude '*/build/' \
	    "$(GAME_ROOT)/tools/runtime/" "$(GAME_ROOT)/lib/model2/"; \
	  echo "synced tools/runtime → lib/model2"; \
	else \
	  echo "tools/runtime missing — clone tools first:"; \
	  echo "  git clone git@github.com-xandoxan65:xandoxan65/segamodel2-tools.git tools"; \
	fi

clean:
	@if [ -d "$(BUILD_DIR)" ]; then $(CMAKE) --build "$(BUILD_DIR)" --target clean; fi
	rm -rf out/maincpu build/lift "$(GAME_ROOT)/build/segamod2" "$(GAME_ROOT)/build/segamod2.exe" $(LIFT_PROGRESS)
	rm -rf lib/model2/geo/build lib/model2/hw/build lib/model2/tgp/build lib/model2/snd/build
