/* The race HUD's 2D layer (code/render/hud2d.c): the System 24 style tile maps the HUD routines write
 * (race/hud.h, docs/dreamcast-port.md "The 2D layer and the race HUD") as a list of cells to draw over the 3D view.
 *
 * Scope: what the race uses. All four maps with zero scroll and no window masks (the race sets none); the cells of
 * category 1 (bit 15: over the polygons) in the order maps 3, 2, 1, 0; cells whose character has no visible pixel
 * are left out. */
#ifndef SR_RENDER_HUD2D_H
#define SR_RENDER_HUD2D_H
#include <stdint.h>

#define SR_HUD_W 496 /* the 2D layer's screen, the same window as the 3D view */
#define SR_HUD_H 384
#define SR_HUD_CHARS 16384u

typedef struct {
    int16_t x, y;   /* top left pixel on the 496 x 384 screen */
    uint16_t code;  /* character */
    uint16_t colour; /* palette line: pens colour * 16 .. + 15 */
} sr_hud_cell_t;

/* Mark the characters with at least one non-zero pixel (bit per character, SR_HUD_CHARS bits). */
void sr_hud2d_scan_chars(const uint8_t *chars, uint8_t *visible);

uint32_t sr_hud2d_cells(const uint8_t *tile_map, const uint8_t *visible, sr_hud_cell_t *out, uint32_t max);


/* Pixel (px, py) 0..7 of a character: 0 is transparent. */
static inline uint32_t sr_hud2d_pixel(const uint8_t *chars, uint32_t code, uint32_t px, uint32_t py) {
    /* a row is two little-endian u16 words, pixels 0..3 then 4..7, the leftmost in the top nibble */
    const uint8_t *row = chars + (code & (SR_HUD_CHARS - 1u)) * 32u + py * 4u;
    const uint32_t word = (uint32_t)row[(px >> 2) * 2u] | (uint32_t)row[(px >> 2) * 2u + 1u] << 8;
    return (word >> (12u - 4u * (px & 3u))) & 15u;
}

/* RGB555 as the palette RAM holds it (red in bits 0-4) to ARGB1555 (red in bits 10-14). Black is see-through: the
 * reference overlays the 2D layer on the 3D view with black punched out (src/host/sys24_viewer.c), which is how the
 * progress bar's track and the digits' dark rims show the scene behind them. */
static inline uint16_t sr_hud2d_argb1555(uint16_t c) {
    if(!(c & 0x7fffu)) return 0;
    return (uint16_t)(0x8000u | (c & 31u) << 10 | (c & 0x3e0u) | (c >> 10 & 31u));
}

#endif
