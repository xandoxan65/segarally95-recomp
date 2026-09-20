/* Palette irq mode 4 @ 0x335B4 — fade CRX→workbufs toward 0xff00ff; arm fade_step. */
// @rom 0x335b4 +0x18c geo_palette_mode4_fade

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

static u32 lerp_wash(u32 packed, u32 scale, u32 mask)
{
    u32 chan = packed & mask;
    u32 delta = mask - chan;

    return chan + ((scale * delta) >> 8);
}

/*
 * Attract queues mode 4 with scale 0x100 → 0, step -8 (fade-in from washed).
 * Requires mode 1/2 CRX snapshot @ 0x20D830 (game_seed_globals queues mode 1).
 * Steady state after scale→0 matches boot colorxlat.
 */
void geo_palette_mode4_fade(u32 arg0, u32 arg1, u32 arg2)
{
    u32 cursor;
    u32 r12;
    u32 scale;
    u32 mask;
    u32 r13;
    u32 base;
    u32 r11;
    u32 r9;
    u32 r8;
    u32 r7;
    u32 r6;
    u32 r5;
    u32 r4;
    u32 g13;
    u32 w0;
    u32 w1;
    u32 w2;
    u8 b0;
    u8 b1;
    u16 seed_lo;
    u32 g3;
    u32 g1;
    u32 g2;
    u32 out;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    geo_palette_fade_commit(0, 0, 0);

    cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
    r12 = 0u;
    scale = i960_ld_u32(I960_WORKRAM, 0x213854, 0);
    mask = 0x00ff00ffu;
    r13 = i960_ld_u32(I960_WORKRAM, 0x213838, 0);
    base = cursor << 7;
    r11 = cursor;

    r9 = 0x0020d830u + base;
    r8 = 0x0020e830u + base;
    r7 = 0x0020f830u + base;
    r6 = 0x00210830u + base;
    r5 = 0x00211830u + base;
    r4 = 0x00212830u + base;

    if ((i32)cursor > 31)
        goto after_banks;

    do {
        g13 = 0u;
        do {
            /* @0x33620–0x33694 */
            w0 = i960_ld_u32(I960_ABS, r9, 0);
            w1 = i960_ld_u32(I960_ABS, r8, 0);
            r9 += 4u;
            w2 = i960_ld_u32(I960_ABS, r7, 0);
            r8 += 4u;
            r7 += 4u;

            i960_st_u32(I960_ABS, r6, 0, lerp_wash(w0, scale, mask));
            r6 += 4u;
            i960_st_u32(I960_ABS, r5, 0, lerp_wash(w1, scale, mask));
            r5 += 4u;
            i960_st_u32(I960_ABS, r4, 0, lerp_wash(w2, scale, mask));
            r4 += 4u;
            g13++;
        } while ((i32)g13 <= 31);

        r12++;
        r11++;
        r13++;
        if ((i32)r12 > 15)
            break;
    } while ((i32)31 >= (i32)r11);

after_banks:
    i960_st_u32(I960_WORKRAM, 0x213838, 0, r13);

    if ((i32)31 >= (i32)cursor)
        return;

    b0 = i960_ld_u8(I960_WORKRAM, 0x213830, 0);
    seed_lo = i960_ld_u16(I960_WORKRAM, 0x213830, 0);
    b1 = i960_ld_u8(I960_WORKRAM, 0x213831, 0);
    g3 = b0 & 31u;
    g1 = (seed_lo >> 5) & 31u;
    g2 = (b1 >> 2) & 31u;
    /*
     * @0x33700: st g14,0x213834 — bx-entered handlers treat g14 as 0 (irq
     * prologue / host ABI). Storing a live link leaves cursor >> 31 so later
     * frames skip bank rebuild and colorxlat stays at the scale=0x100 wash.
     */
    i960_st_u32(I960_WORKRAM, 0x213834, 0, 0);
    out = ((g2 + (((31u - g2) * scale) >> 8)) << 10)
        | (((g1 + (((31u - g1) * scale) >> 8)) << 5) | 0x8000u)
        | (g3 + (((31u - g3) * scale) >> 8));
    i960_st_u16(I960_ABS, 0x01800000u, 0, (u16)out);
    geo_palette_fade_step(0, 0, 0);
}
