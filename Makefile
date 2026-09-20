# Sega Rally Championship (1995) — host-compiled lift
#
#   make lift        # extract ROM blocks, then compile + link segamod2 (default)
#   make lift-main   # regenerate lift_main / host glue (requires tools/)
#   make clean

SHELL := /bin/bash
.DELETE_ON_ERROR:
.PRECIOUS: src/lift_main.c src/i960_host_syms.c src/i960_host_invoke.c include/lift_syms.h

GAME_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
export SEGAMOD2_ROM_DIR ?= $(GAME_ROOT)/ROMS/srallyc-b
export SEGAMOD2_ROOT ?= $(GAME_ROOT)

PYTHON      := python3
IMAGE_CFG   := maincpu.image.yaml
REFERENCE   := out/i960/maincpu_deinterleaved.bin
MAIN_DATA   := out/i960/main_data_deinterleaved.bin
OUTPUT      := out/maincpu/maincpu_rebuilt.bin
REPORT      := out/maincpu/maincpu_image.json
COMPARE_RPT := out/maincpu/compare.json

# --- Lift compile (host syntax / link check) ---
LIFT_CC       ?= cc
LIFT_PNG_CFLAGS := $(shell pkg-config --exists libpng 2>/dev/null && pkg-config --cflags libpng)
LIFT_PNG_LIBS   := $(shell pkg-config --exists libpng 2>/dev/null && pkg-config --libs libpng || echo -lpng -lz)
LIFT_SDL_CFLAGS := $(shell pkg-config --exists sdl2 2>/dev/null && pkg-config --cflags sdl2) $(if $(shell pkg-config --exists sdl2 2>/dev/null && echo yes),-DI960_HOST_HAVE_SDL,)
LIFT_SDL_LIBS   := $(shell pkg-config --exists sdl2 2>/dev/null && pkg-config --libs sdl2)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  LIFT_GL_CFLAGS := $(if $(shell pkg-config --exists sdl2 2>/dev/null && echo yes),-DI960_HOST_HAVE_GL,)
  LIFT_GL_LIBS   := $(if $(shell pkg-config --exists sdl2 2>/dev/null && echo yes),-framework OpenGL,)
else
  LIFT_GL_CFLAGS := $(if $(shell pkg-config --exists gl 2>/dev/null && echo yes),-DI960_HOST_HAVE_GL $(shell pkg-config --cflags gl),)
  LIFT_GL_LIBS   := $(if $(shell pkg-config --exists gl 2>/dev/null && echo yes),$(shell pkg-config --libs gl),-lGL)
endif

LIFT_CFLAGS   := -std=c99 -Wall -Wextra -Wno-unused-parameter \
	-Isrc -Iinclude \
	-Ilib/model2/include \
	-Ilib/model2/host \
	-Ilib/model2/geo/include \
	-Ilib/model2/hw/include \
	-Ilib/model2/tgp/include \
	-Ilib/model2/snd/include \
	$(LIFT_PNG_CFLAGS) $(LIFT_SDL_CFLAGS) $(LIFT_GL_CFLAGS)
LIFT_LDFLAGS  := $(LIFT_PNG_LIBS) $(LIFT_SDL_LIBS) $(LIFT_GL_LIBS) -lpthread -lm

LIFT_BUILD    := build/lift
LIFT_BIN      := $(GAME_ROOT)/build/segamod2

# Portable runtime (lib/model2/host) + game-specific generated glue (src/)
LIFT_HOST_RUNTIME := \
	lib/model2/host/i960_regs.c \
	lib/model2/host/i960_mem.c \
	lib/model2/host/model2_rom.c \
	lib/model2/host/i960_host.c \
	lib/model2/host/i960_host_scene.c \
	lib/model2/host/i960_host_staging.c \
	lib/model2/host/i960_host_staging_dispatch.c \
	lib/model2/host/i960_host_dispatch.c \
	lib/model2/host/lift_stubs.c
LIFT_GAME_GLUE := \
	src/i960_host_syms.c \
	src/i960_host_invoke.c \
	src/lift_main.c

LIFT_GAME     := $(wildcard src/game/*.c)
LIFT_HOST     := $(wildcard src/host/*.c)
LIFT_GEO_DIR  := lib/model2/geo
LIFT_GEO_LIB  := $(LIFT_GEO_DIR)/build/libmodel2_geo.a
LIFT_HW_DIR   := lib/model2/hw
LIFT_HW_LIB   := $(LIFT_HW_DIR)/build/libmodel2_hw.a
LIFT_TGP_DIR  := lib/model2/tgp
LIFT_TGP_LIB  := $(LIFT_TGP_DIR)/build/libmodel2_tgp.a
LIFT_SND_DIR  := lib/model2/snd
LIFT_SND_LIB  := $(LIFT_SND_DIR)/build/libmodel2_snd.a

LIFT_OBJS     := $(patsubst lib/model2/host/%.c,$(LIFT_BUILD)/runtime/%.o,$(LIFT_HOST_RUNTIME)) \
                 $(patsubst src/%.c,$(LIFT_BUILD)/%.o,$(LIFT_GAME_GLUE)) \
                 $(patsubst src/libc/%.c,$(LIFT_BUILD)/libc/%.o,$(wildcard src/libc/*.c)) \
                 $(patsubst src/boot/%.c,$(LIFT_BUILD)/boot/%.o,$(wildcard src/boot/*.c)) \
                 $(patsubst src/irq/%.c,$(LIFT_BUILD)/irq/%.o,$(wildcard src/irq/*.c)) \
                 $(patsubst src/game/%.c,$(LIFT_BUILD)/game/%.o,$(LIFT_GAME)) \
                 $(patsubst src/host/%.c,$(LIFT_BUILD)/host/%.o,$(LIFT_HOST))
LIFT_PROGRESS := out/lift/coverage.json

GEN_LIFT_MAIN := $(firstword \
	$(wildcard $(GAME_ROOT)/tools/tools/decomp/gen_lift_main.py) \
	$(wildcard $(GAME_ROOT)/tools/decomp/gen_lift_main.py))

.PHONY: all help lift lift-check lift-main clean \
	libmodel2_geo libmodel2_hw libmodel2_tgp libmodel2_snd \
	reference rom-blocks compare quick coverage lift-progress \
	lift-palette lift-palette-viewer lift-viewer \
	lift-boot-viewer lift-boot-practice lift-boot-headless lift-boot-live \
	lift-cgm-decode lift-geo-compare sync-runtime tools-hint

.DEFAULT_GOAL := lift

LIFT_PALETTE_DUMP := build/lift/palette_state
LIFT_PALETTE_REPORT := $(GAME_ROOT)/out/lift/palette_inspect
LIFT_PALETTE_CACHE := $(GAME_ROOT)/out/textures/palette_cache/desert

help:
	@echo "Targets:"
	@echo "  make / make lift  — extract ROM blocks, compile + link ($(LIFT_BIN))"
	@echo "  make lift-check   — compile lifted C objects + libs"
	@echo "  make lift-main    — regenerate lift_main / host glue (needs tools/)"
	@echo "  make libmodel2_*  — stand-alone Model 2 libs under lib/model2/"
	@echo "  make sync-runtime — rsync tools/runtime → lib/model2 (if tools present)"
	@echo "  make rom-blocks   — extract ROM blocks (needs ROMs; no tools/)"
	@echo "  make compare      — byte-compare rebuilt image (needs tools/)"
	@echo "  make coverage     — lift coverage report (needs tools/)"
	@echo "  make lift-viewer  — track viewer (needs ROMs + libpng/sdl2)"
	@echo "  make clean        — remove build/lift/ and lib build dirs"

# Compile-only check. Default `make` / `lift` also extracts ROM blocks.
lift-check: $(LIFT_OBJS) libmodel2_geo libmodel2_hw libmodel2_tgp libmodel2_snd
	@echo "lift-check: $(words $(LIFT_OBJS)) object files OK (+ $(notdir $(LIFT_GEO_LIB)) + $(notdir $(LIFT_HW_LIB)) + $(notdir $(LIFT_TGP_LIB)) + $(notdir $(LIFT_SND_LIB)))"

lift: lift-check $(LIFT_BIN) $(REFERENCE) $(MAIN_DATA)

libmodel2_geo:
	$(MAKE) -C $(LIFT_GEO_DIR) \
	  CC="$(LIFT_CC)" \
	  CFLAGS="-std=c99 -Wall -Wextra -Wno-unused-parameter -O2 $(LIFT_PNG_CFLAGS) $(LIFT_SDL_CFLAGS) $(LIFT_GL_CFLAGS)"

libmodel2_hw:
	$(MAKE) -C $(LIFT_HW_DIR) \
	  CC="$(LIFT_CC)" \
	  CFLAGS="-std=c99 -Wall -Wextra -Wno-unused-parameter -O2"

libmodel2_tgp:
	$(MAKE) -C $(LIFT_TGP_DIR) \
	  CC="$(LIFT_CC)" \
	  CFLAGS="-std=c99 -Wall -Wextra -Wno-unused-parameter -O2"

libmodel2_snd:
	$(MAKE) -C $(LIFT_SND_DIR) \
	  CC="$(LIFT_CC)" \
	  CFLAGS="-std=c99 -Wall -Wextra -Wno-unused-parameter -O2"

# Optional: regenerate entry/glue only when tools are cloned.
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

$(LIFT_BIN): $(LIFT_OBJS) libmodel2_geo libmodel2_hw libmodel2_tgp libmodel2_snd
	@mkdir -p $(dir $@)
	$(LIFT_CC) -o $@ $(LIFT_OBJS) $(LIFT_GEO_LIB) $(LIFT_HW_LIB) $(LIFT_TGP_LIB) $(LIFT_SND_LIB) $(LIFT_LDFLAGS)

$(LIFT_BUILD)/runtime/%.o: lib/model2/host/%.c
	@mkdir -p $(dir $@)
	$(LIFT_CC) $(LIFT_CFLAGS) -c -o $@ $<

$(LIFT_BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(LIFT_CC) $(LIFT_CFLAGS) -c -o $@ $<

$(LIFT_BUILD)/libc/%.o: src/libc/%.c
	@mkdir -p $(dir $@)
	$(LIFT_CC) $(LIFT_CFLAGS) -c -o $@ $<

$(LIFT_BUILD)/boot/%.o: src/boot/%.c
	@mkdir -p $(dir $@)
	$(LIFT_CC) $(LIFT_CFLAGS) -c -o $@ $<

$(LIFT_BUILD)/irq/%.o: src/irq/%.c
	@mkdir -p $(dir $@)
	$(LIFT_CC) $(LIFT_CFLAGS) -c -o $@ $<

$(LIFT_BUILD)/game/%.o: src/game/%.c
	@mkdir -p $(dir $@)
	$(LIFT_CC) $(LIFT_CFLAGS) -c -o $@ $<

$(LIFT_BUILD)/host/%.o: src/host/%.c
	@mkdir -p $(dir $@)
	$(LIFT_CC) $(LIFT_CFLAGS) -c -o $@ $<

# --- Tools-gated targets ---
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

# maincpu pair + main_data pairs consumed by extract_rom_blocks.
ROM_BLOCK_SRCS := \
	$(SEGAMOD2_ROM_DIR)/epr-17888b.12 \
	$(SEGAMOD2_ROM_DIR)/epr-17889b.13 \
	$(SEGAMOD2_ROM_DIR)/mpr-17746.10 \
	$(SEGAMOD2_ROM_DIR)/mpr-17747.11 \
	$(SEGAMOD2_ROM_DIR)/mpr-17744.8 \
	$(SEGAMOD2_ROM_DIR)/mpr-17745.9 \
	$(SEGAMOD2_ROM_DIR)/mpr-17884.6 \
	$(SEGAMOD2_ROM_DIR)/mpr-17885.7
# Game-tree extractor (stdlib only). tools/ is optional for lift-main / compare / etc.
EXTRACT_ROM_BLOCKS := $(GAME_ROOT)/scripts/extract_rom_blocks.py
# One stamp: GNU make 3.81 has no grouped targets, and the script writes both bins.
ROM_EXTRACT_STAMP := out/i960/.rom-blocks.stamp

reference rom-blocks: $(REFERENCE) $(MAIN_DATA)

$(ROM_EXTRACT_STAMP): $(ROM_BLOCK_SRCS) $(EXTRACT_ROM_BLOCKS)
	@test -d "$(SEGAMOD2_ROM_DIR)" || { \
	  echo "error: ROM dir missing: $(SEGAMOD2_ROM_DIR)" >&2; \
	  echo "  place MAME srallyc-b dumps under ROMS/srallyc-b/ (see ROMS/README.md)" >&2; \
	  exit 1; \
	}
	@mkdir -p out/i960
	$(PYTHON) "$(EXTRACT_ROM_BLOCKS)" --rom-dir "$(SEGAMOD2_ROM_DIR)" --out-dir out/i960
	@touch $@

$(REFERENCE) $(MAIN_DATA): $(ROM_EXTRACT_STAMP)
	@test -s $@ || { rm -f $(ROM_EXTRACT_STAMP); $(MAKE) $(ROM_EXTRACT_STAMP); }

all: $(OUTPUT)

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

lift-viewer: lift
	@cd $(GAME_ROOT) && SEGAMOD2_ROOT=$(GAME_ROOT) \
	  "$(LIFT_BIN)" --viewer track --course desert --out $(GAME_ROOT)/out \
	  --palette-dump $(LIFT_PALETTE_DUMP)

lift-palette-viewer: lift-viewer

lift-boot-viewer: lift
	@cd $(GAME_ROOT) && SEGAMOD2_ROOT=$(GAME_ROOT) \
	  "$(LIFT_BIN)" --viewer boot \
	  --palette-dump build/lift/boot_copyright

lift-boot-practice: lift
	@cd $(GAME_ROOT) && SEGAMOD2_ROOT=$(GAME_ROOT) \
	  "$(LIFT_BIN)" --viewer boot --practice \
	  --palette-dump build/lift/boot_copyright

lift-boot-headless: lift
	@cd $(GAME_ROOT) && "$(LIFT_BIN)" --viewer boot --headless \
	  --palette-dump build/lift/boot_copyright

lift-boot-live: lift-boot-viewer

LIFT_CGM_DECODE_DUMP := build/lift/cgm_decode
LIFT_CGM_SPLASH_VADDR := 0x2879db0

lift-cgm-decode: lift
	@cd $(GAME_ROOT) && "$(LIFT_BIN)" --decode-cgm $(LIFT_CGM_SPLASH_VADDR) \
	  --palette-dump $(LIFT_CGM_DECODE_DUMP)

# Sync portable runtime from tools SoT → this game tree.
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
	rm -rf out/maincpu build/lift build/segamod2 $(LIFT_PROGRESS)
	$(MAKE) -C $(LIFT_GEO_DIR) clean
	$(MAKE) -C $(LIFT_HW_DIR) clean
	$(MAKE) -C $(LIFT_TGP_DIR) clean
	$(MAKE) -C $(LIFT_SND_DIR) clean
