/* The race HUD's 2D layer: see include/render/hud2d.h. */
#include "render/hud2d.h"
#include "sr_alloc.h"

void sr_hud2d_scan_chars(const uint8_t *chars, uint8_t *visible) {
    uint32_t c, i;
    sr_fill(visible, 0, SR_HUD_CHARS / 8u);
    for(c = 0; c < SR_HUD_CHARS; c++)
        for(i = 0; i < 32u; i++)
            if(chars[c * 32u + i]) {
                visible[c >> 3] |= (uint8_t)(1u << (c & 7u));
                break;
            }
}

uint32_t sr_hud2d_cells(const uint8_t *tile_map, const uint8_t *visible, sr_hud_cell_t *out, uint32_t max) {
    uint32_t n = 0;
    int map;
    for(map = 3; map >= 0; map--) {
        const uint8_t *cells = tile_map + (uint32_t)map * 0x2000u;
        uint32_t x, y;
        for(y = 0; y < SR_HUD_H / 8u; y++)
            for(x = 0; x < SR_HUD_W / 8u; x++) {
                const uint32_t cell = (uint32_t)cells[(y * 64u + x) * 2u] | (uint32_t)cells[(y * 64u + x) * 2u + 1u] << 8;
                const uint32_t code = cell & 0x3fffu;
                if(!(cell & 0x8000u) || !(visible[code >> 3] & (1u << (code & 7u)))) continue;
                if(n == max) return n;
                out[n].x = (int16_t)(x * 8u);
                out[n].y = (int16_t)(y * 8u);
                out[n].code = (uint16_t)code;
                out[n].colour = (uint16_t)((cell >> 7) & 0xffu);
                n++;
            }
    }
    return n;
}
