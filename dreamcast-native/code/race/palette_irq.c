/* The palette interrupt's per-frame work: see include/race/palette_irq.h. Ported from the lifted originals
 * (src/game/geo_palette_*.c). From the countdown (absolute tick 147) to the end of the race only modes 0 and 4
 * occur; the others (1, 2: snapshots of the GEO colour tables, 6: their restore, 7: the colour base upload) run at
 * boot and after the race and are reported, not ported. */
#include "race/palette_irq.h"
#include "sr_mem.h"
#include <stdio.h>

static void not_ported(uint32_t mode) {
    static uint32_t seen;
    if(seen & (1u << (mode & 31u))) return;
    seen |= 1u << (mode & 31u);
    printf("sr: palette irq mode %lu is not ported yet\n", (unsigned long)mode);
}

void sr_palette_mode0_promote(void) {
    uint32_t i;
    sr_st32(0x213834u, 0);
    sr_st32(0x213838u, 0);
    for(i = 0; i < 4; i++) {
        const uint32_t w = sr_ld32(0x213840u + i * 4);
        if(i == 0) sr_st32(0x213840u, 0);
        sr_st32(0x213850u + i * 4, w);
    }
}

void sr_palette_fade_step(void) {
    const uint32_t scale = sr_ld32(0x213854u), target = sr_ld32(0x213858u);
    const int32_t step = (int32_t)sr_ld32(0x21385cu);
    uint32_t mode = sr_ld32(0x213850u), next_scale = scale;
    if(scale == target || step == 0) {
        sr_st32(0x213850u, 0);
        sr_st32(0x213854u, scale);
        return;
    }
    {
        const int32_t next = (int32_t)scale + step;
        if(step > 0) next_scale = (int32_t)target <= next ? target : (uint32_t)next;
        else next_scale = (int32_t)target >= next ? target : (uint32_t)next;
    }
    sr_st32(0x213850u, mode);
    sr_st32(0x213854u, next_scale);
}

void sr_palette_fade_commit(void) {
    /* the copies go to the GEO's colour tables (0x01810000..), hardware here: only the cursor walk is RAM */
    uint32_t cursor = sr_ld32(0x213834u), remain = sr_ld32(0x213838u);
    if(remain == 0) return;
    do {
        cursor = sr_ld32(0x213834u) + 1;
        remain = sr_ld32(0x213838u) - 1;
        sr_st32(0x213834u, cursor);
        sr_st32(0x213838u, remain);
    } while(remain != 0);
}

static uint32_t lerp_wash(uint32_t packed, uint32_t scale, uint32_t mask) {
    const uint32_t chan = packed & mask;
    return chan + ((scale * (mask - chan)) >> 8);
}

void sr_palette_mode4_fade(void) {
    uint32_t cursor, scale, base, r12 = 0, r11, r13, r9, r8, r7, r6, r5, r4;
    const uint32_t mask = 0x00ff00ffu;
    sr_palette_fade_commit();
    cursor = sr_ld32(0x213834u), scale = sr_ld32(0x213854u), r13 = sr_ld32(0x213838u);
    base = cursor << 7, r11 = cursor;
    r9 = 0x20d830u + base, r8 = 0x20e830u + base, r7 = 0x20f830u + base;
    r6 = 0x210830u + base, r5 = 0x211830u + base, r4 = 0x212830u + base;
    if((int32_t)cursor <= 31) {
        do {
            uint32_t g13;
            for(g13 = 0; (int32_t)g13 <= 31; g13++) {
                const uint32_t w0 = sr_ld32(r9), w1 = sr_ld32(r8), w2 = sr_ld32(r7);
                r9 += 4, r8 += 4, r7 += 4;
                sr_st32(r6, lerp_wash(w0, scale, mask));
                sr_st32(r5, lerp_wash(w1, scale, mask));
                sr_st32(r4, lerp_wash(w2, scale, mask));
                r6 += 4, r5 += 4, r4 += 4;
            }
            r12++, r11++, r13++;
            if((int32_t)r12 > 15) break;
        } while((int32_t)31 >= (int32_t)r11);
    }
    sr_st32(0x213838u, r13);
    if((int32_t)31 >= (int32_t)cursor) return;
    /* the backdrop pen (0x01800000, palette RAM: hardware here) and the cursor rewind */
    sr_st32(0x213834u, 0);
    sr_palette_fade_step();
}

void sr_palette_irq_frame(void) {
    uint32_t mode;
    {
        const uint32_t tick = (sr_ld16(0x213862u) + 2u) & 0xffu;
        sr_st16(0x213862u, tick);
        sr_st16(0x213860u, tick);
    }
    mode = sr_ld32(0x213850u);
    if(mode == 0) {
        sr_palette_mode0_promote();
        mode = sr_ld32(0x213850u);
    }
    if(mode == 0) return;
    if(mode == 4) {
        sr_palette_mode4_fade();
        return;
    }
    not_ported(mode);
}

uint32_t sr_palette_wash(void) {
    const uint32_t scale = sr_ld32(0x213854u);
    return sr_ld32(0x213850u) == 4 ? (scale > 0x100u ? 0x100u : scale) : 0;
}
