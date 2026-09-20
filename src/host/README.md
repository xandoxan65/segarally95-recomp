# Host harness shims (not lifted game code)

C in this directory is **host-only**: it substitutes ROM functions the lifter has not
emitted yet, or wires harness helpers (catalog FIFO feed, env-driven placement batches).

It must **not** be treated as lifted maincpu code. Progress reports and byte-match
validation use `src/game/`, `src/boot/`, `src/libc/`, and `src/irq/` only.

## Track viewer (unified path)

**Single entry point** — lifted palette + viewer asset export:

```bash
cd decomp && make lift-viewer
# or:
./build/lift/segamod2 --viewer track --course desert --out ../out
```

Pipeline (`lift_track_viewer.c`):

1. **Lifted C** — `geo_renderer_init` (`geo_palette_lut_upload` @ `0x3C80`, `geo_lumaram_init` @ `0x4350`)
2. **Optional** — `--geo-frames N` runs lifted `geo_draw_frame_entry` with track placement feed
3. **C dump** — `model2_palette_state_dump` → `build/lift/palette_state`
4. **Export** — skipped until mesh/palette_cache bake is lifted in C (no host-script bridge)

Flags:

| Flag | Effect |
|------|--------|
| `--viewer track` | Enable track viewer mode |
| `--course desert` | Course id |
| `--out DIR` | Repo `out/` root (default `SEGAMOD2_ROOT/out`) |
| `--palette-dump DIR` | Palette snapshot dir (default `build/lift/palette_state`) |
| `--palette-only` | Skip PNG/OBJ export (palette dump only) |
| `--geo-frames N` | Run N lifted draw frames with track placement feed |
| `--live` | SDL window: refresh sys24 framebuffer each frame (boot viewer) |
| `--practice` | Boot viewer: skip attract/menus → desert practice START (Delta AT) |
| `--help` | Usage |

Legacy env modes were removed; use `--viewer track` or `--viewer boot` only.

## System-24 tile framebuffer (`sys24_tile.c`)

Port of MAME `segas24_tile_device` (see `third_party/mame/src/mame/sega/segaic24.cpp`).
Char pixels use `sys24_gfx.c` — MAME `gfx_element::decode` / `char_layout` with
`layout_xormask` 8 (LE), matching `set_gfx(..., NATIVE_ENDIAN_VALUE_LE_BE(8,0), ...)`.
Reads lifted `model2_tile_map` / `model2_tile_char` and composites a 496×384 RGB
framebuffer when `model2_palette_state_dump` runs (`sys24_framebuffer.png` + `.rgba`
in the dump dir).

Boot preview: `make lift-boot-viewer` runs full lifted cold boot with **SDL live preview**
by default (close window or Escape to exit). Writes `sys24_framebuffer.png` on exit via
`sys24_tile.c`. Use `make lift-boot-headless` for fast PNG-only runs (dispatch cap).

### Live SDL preview (`sys24_viewer.c`)

Requires SDL2 at build time (`pkg-config sdl2`). Refreshes the tile framebuffer each
`post_reset_dispatch` frame while the lift runs:

```bash
cd decomp && make lift-boot-viewer
# skip attract / championship+course+car select → desert practice START (Delta AT):
make lift-boot-practice
# or:
./build/lift/segamod2 --viewer boot --practice
# headless PNG only:
make lift-boot-headless
# or:
./build/lift/segamod2 --viewer boot --palette-dump build/lift/boot_copyright
./build/lift/segamod2 --viewer boot --headless --palette-dump build/lift/boot_copyright
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
| `track_viewer_export.c` | Asset export stub (C bake not lifted yet) |
| `placement_catalog_feed.c` | Catalog-row geo FIFO pushes |

Palette RAM dumps come from **running `segamod2` with `--viewer`**.
Set `SEGAMOD2_ROOT` to the segamod2 repo root when not using Makefile targets.
