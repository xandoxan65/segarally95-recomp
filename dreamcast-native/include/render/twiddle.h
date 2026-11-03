/* PVR twiddled texel order, as the texture unit reads TWIDDLED textures (and as KOS pvr_txr_load_ex writes them):
 * within a square of side s = min(w, h), bit i of y goes to bit 2i and bit i of x to bit 2i + 1; a rectangle is a
 * row (w > h) or column (h > w) of such squares stored one after another. */
#ifndef SR_RENDER_TWIDDLE_H
#define SR_RENDER_TWIDDLE_H
#include <stdint.h>

static inline uint32_t sr_twiddle_index(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    const uint32_t s = w < h ? w : h;
    const uint32_t block = w > h ? x / s : y / s;
    uint32_t i = 0, bit;
    x %= s;
    y %= s;
    for(bit = 0; (1u << bit) < s; bit++) {
        i |= ((y >> bit) & 1u) << (2 * bit);
        i |= ((x >> bit) & 1u) << (2 * bit + 1);
    }
    return block * s * s + i;
}

#endif
