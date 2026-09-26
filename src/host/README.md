# Host harness shims (not lifted game code)

C in this directory is **host-only**: it substitutes ROM functions the lifter has not
emitted yet, or wires harness helpers (catalog FIFO feed, env-driven placement batches).

It must **not** be treated as lifted maincpu code. Progress reports and byte-match
validation use `src/game/`, `src/boot/`, `src/libc/`, and `src/irq/` only.

## Boot viewer

Default `segamod2` (no arguments) is the SDL cold-boot viewer. `--harness` is the short dispatch trace. No palette snapshot is written unless `--palette-dump` or `I960_PALETTE_DUMP` is set.

```bash
cmake --build build --target lift-boot-viewer
# or:
./build/segamod2
```

`--practice` skips attract/menus to desert practice START. `--headless` runs with no SDL window.

## System-24 tile framebuffer (`sys24_tile.c`)

Port of MAME `segas24_tile_device` (see `third_party/mame/src/mame/sega/segaic24.cpp`).
Char pixels use `sys24_gfx.c` — MAME `gfx_element::decode` / `char_layout` with
`layout_xormask` 8 (LE), matching `set_gfx(..., NATIVE_ENDIAN_VALUE_LE_BE(8,0), ...)`.
Reads lifted `model2_tile_map` / `model2_tile_char` and composites a 496×384 RGB
framebuffer when `model2_palette_state_dump` runs (`sys24_framebuffer.png` + `.rgba`
in the dump dir).

Boot preview: `make lift-boot-viewer` runs full lifted cold boot with **SDL live preview**
by default (close window or Escape to exit). A palette snapshot is written only with
`--palette-dump DIR`. Use `make lift-boot-headless` for a run with no SDL window.

### Live SDL preview (`sys24_viewer.c`)

Requires SDL2 at build time (`pkg-config sdl2`). Refreshes the tile framebuffer each
`post_reset_dispatch` frame while the lift runs:

```bash
cd decomp && make lift-boot-viewer
# skip attract / championship+course+car select → desert practice START (Delta AT):
make lift-boot-practice
# or:
./build/segamod2 --practice
# headless, no palette snapshot:
make lift-boot-headless
# or:
./build/segamod2 --headless
# palette snapshot only when asked:
./build/segamod2 --palette-dump build/lift/boot_copyright
```

Close the window or press Escape to halt. With SDL2 + OpenGL, the viewer composites
sys24 tiles and a host geo mesh decoded from `prg_fifo` / `copro` (lifted GEO
display-list path). Title bar shows `geo=<verts>/<tris>`.

Offline FIFO decode:

```bash
./build/lift/segamod2 --decode-geo-fifo prg.bin --geo-summary out.json
```

## Other harness modules

| Module | Role |
|--------|------|
| `lift_cli.c` | CLI parsing for `segamod2` |
| `placement_catalog_feed.c` | Catalog-row geo FIFO pushes |

Palette RAM dumps are written only with `--palette-dump DIR` or `I960_PALETTE_DUMP`.
Set `SEGAMOD2_ROOT` to the segamod2 repo root when not using Makefile targets.
