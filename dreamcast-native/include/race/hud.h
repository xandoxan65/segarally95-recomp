/* The race HUD (code/race/hud.c): the per-tick HUD frame (0x20740) and the 2D layer writes it makes.
 *
 * The HUD is Model 2's System 24 tile layer: text records ("scenes") are blitted as 16-bit cells into the tile map
 * at 0x01000000 (64 KiB; 64 x 64 cells of 8 x 8 pixels per layer map, row stride 0x80 bytes), and only the digits
 * that change are redrawn. The tile map is not one of the RAM images the race tick otherwise keeps (sr_mem.h), so
 * it is its own buffer, sr_tile_map, which the caller provides (the Dreamcast loads its race-start contents from
 * the pack; the differential checks point it at the host's tile map). NULL drops the writes.
 *
 * The tachometer needle is a 3D GEO object (catalog row 0x2865130) the HUD frame places through the TGP (0x203D0);
 * its GEO words go through the course hooks (race/course_hooks.h) like the course catalogs. */
#ifndef SR_RACE_HUD_H
#define SR_RACE_HUD_H
#include <stdint.h>

#define SR_TILE_MAP_BASE 0x01000000u
#define SR_TILE_MAP_SIZE 0x10000u

extern uint8_t *sr_tile_map;
/* Set when a write changes the tile map (the renderer rebuilds its cell list then and clears it). */
extern int sr_tile_map_dirty;
/* Which rows (bit = row 0..63) of each of the four cell maps a write changed since the renderer last looked. */
extern uint64_t sr_tile_rows_dirty[4];

/* The tach needle as the last HUD frame placed it (sr_hud_cam_matrix): the TGP matrix its GEO object 0x801a7a is
 * drawn with, in the HUD's own GEO window (focus 128 on the 496 x 384 screen, centre (248, 192), view +y up), and
 * the frame counter 0x202008 of that tick (unchanged: the needle was not drawn). */
extern float sr_hud_needle_matrix[12];
extern uint32_t sr_hud_needle_tick;

/* draw_scene_dispatch (0x29C10) / erase_scene_dispatch (0x29D60): scene record `glyph` of batch `batch` (g3 = glyph,
 * g4 = flags) at cell (x, y); flags bit 0 the L1 map (0x01004000) instead of L2, bit 1 the second page (+0x2000),
 * bit 3 sets cell bit 14. Return batch + glyph, or -1 out of range (the original then prints an error line). */
uint32_t sr_draw_scene(uint32_t x, uint32_t y, uint32_t batch, uint32_t glyph, uint32_t flags);
uint32_t sr_erase_scene(uint32_t x, uint32_t y, uint32_t batch, uint32_t glyph, uint32_t flags);
/* boot_tile_map_fill (0x27260): zero a rectangle of L2 cells. */
void sr_tile_map_fill(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
/* tile_map_banks_clear (0x269D0): zero the L2 and L1 maps (8 KiB each). */
void sr_tile_banks_clear(void);
/* tile_attract_time_split (0x27C68): ticks -> hours, minutes, seconds, hundredths (60 ticks a second). */
void sr_time_split(uint32_t value, uint16_t out[4]);

void sr_hud_layers(uint32_t g0);      /* 0x1E1D0: 0 = the static labels, else the total time digits */
void sr_hud_tach(void);               /* 0x1E500: the time-left digits (0x20b0b0) */
void sr_hud_cam_matrix(void);         /* 0x203D0: the START label slot and the tach needle (g0 != 0 path) */
void sr_hud_lap_time(uint32_t lap);   /* 0x1E610: lap `lap`'s time digits; lap < 0 clears the digit caches */
void sr_hud_position(uint32_t pos);   /* 0x1ECC0: 0 = the labels, else position `pos` of the car count */
void sr_hud_speed(uint32_t g0);       /* 0x1DF40: 0 = the label, else the km/h digits */
void sr_hud_progress(uint32_t g0);    /* 0x1EDB0: 0 = the label, else the START..GOAL bar */
void sr_hud_frame(void);              /* 0x20740: the per-tick HUD (g0 = 1 paths) */

#endif
