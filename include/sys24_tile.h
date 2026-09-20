/* Standalone System-24 tile chip (segas24_tile_device) for the lift harness.
 *
 * Ported from MAME src/mame/sega/segaic24.cpp (BSD-3-Clause, Olivier Galibert).
 * No MAME device_t / tilemap_t dependency — binds to model2_tile_* host RAM.
 */
#ifndef SYS24_TILE_H
#define SYS24_TILE_H

#include "i960_lift.h"

#define SYS24_FB_WIDTH        496
#define SYS24_FB_HEIGHT       384
#define SYS24_PIXMAP_WIDTH    512
#define SYS24_PIXMAP_HEIGHT   512
#define SYS24_TILE_MASK_M2    0x3fffu

typedef struct sys24_tile_state sys24_tile_state_t;

sys24_tile_state_t *sys24_tile_create(u16 tile_mask);
void sys24_tile_destroy(sys24_tile_state_t *st);

/* Point at CPU tile map / char RAM (model2_rom byte buffers, little-endian u16). */
void sys24_tile_bind(sys24_tile_state_t *st, const u8 *tile_map, const u8 *char_ram);

/* Rebuild internal 512×512 layer pixmaps from bound RAM.
 * MAME segaic24 only mark_tile_dirty on VRAM writes — scroll alone must not
 * rebuild. Prefer sys24_tile_ensure_refreshed from the draw path. */
void sys24_tile_refresh(sys24_tile_state_t *st);

/* Refresh only when tile map / char bytes changed since last rebuild. */
void sys24_tile_ensure_refreshed(sys24_tile_state_t *st);

/* Model-2 RGB path: composite Sys24 layers per MAME model2_v / segaic24
 * draw_common (win mask + ctrl&0x6000). Ranking (bank @ 0x01002000) shows only
 * when scene_hud_alt has set ctrl bit14 via 0x20b91c → tile_ram[0x5004]. */
void sys24_tile_draw_frame_rgb32(sys24_tile_state_t *st, u32 *bitmap, const u8 *palram,
                                 u32 clear_color);

/*
 * Same as draw_frame but selectable passes (MAME model2_v.cpp screen_update):
 *   BOTTOM   — even layers under the polygon framebuffer
 *   PRIORITY — odd layers over the polygon framebuffer
 *   ALL      — both (legacy single-buffer path)
 */
#define SYS24_PASS_BOTTOM    1u
#define SYS24_PASS_PRIORITY  2u
#define SYS24_PASS_ALL       3u

void sys24_tile_draw_layers_rgb32(sys24_tile_state_t *st, u32 *bitmap, const u8 *palram,
                                  u32 clear_color, unsigned pass);

#endif
