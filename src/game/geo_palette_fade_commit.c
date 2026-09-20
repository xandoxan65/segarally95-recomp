/* Palette irq fade commit @ 0x330A0 — fade workbufs → colorxlat when 0x213838≠0. */
// @rom 0x330a0 +0xd0 geo_palette_fade_commit

#include "i960_lift.h"
#include "i960_mem.h"

static void copy_quad(u32 src, u32 dest)
{
    i960_st_u32(I960_ABS, dest, 0, i960_ld_u32(I960_ABS, src, 0));
    i960_st_u32(I960_ABS, dest, 4, i960_ld_u32(I960_ABS, src, 4));
    i960_st_u32(I960_ABS, dest, 8, i960_ld_u32(I960_ABS, src, 8));
    i960_st_u32(I960_ABS, dest, 12, i960_ld_u32(I960_ABS, src, 12));
}

/*
 * Called at the head of modes 3/4/5. When 0x213838==0, no-op (first fade frame).
 * Else copies 0x210830/11830/12830 → colorxlat banks and walks 0x213834/838.
 */
void geo_palette_fade_commit(u32 arg0, u32 arg1, u32 arg2)
{
    u32 cursor;
    u32 remain;
    u32 r6;
    u32 g5;
    u32 r5;
    u32 r4;
    u32 g13;
    u32 g3;
    u32 g2;
    u32 g1;
    u32 g0;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
    remain = i960_ld_u32(I960_WORKRAM, 0x213838, 0);
    g5 = cursor << 9;
    r6 = cursor << 3;
    r5 = 0x01810000u + g5;
    r4 = 0x01814000u + g5;
    g13 = 0x01818000u + g5;

    /* @0x330D4: be when remain==0 */
    if (remain == 0u)
        return;

    do {
        /* @0x330D8–0x33134 */
        g5 = r6 << 4;
        g3 = 0x00212830u + g5;
        g2 = 0x00211830u + g5;
        g1 = 0x00210830u + g5;
        g0 = 0u;
        do {
            copy_quad(g1, r5);
            copy_quad(g2, r4);
            copy_quad(g3, g13);
            r5 += 16u;
            r4 += 16u;
            g13 += 16u;
            g3 += 16u;
            g2 += 16u;
            g1 += 16u;
            r6++;
            g0++;
        } while ((i32)g0 <= 7);

        cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
        remain = i960_ld_u32(I960_WORKRAM, 0x213838, 0);
        r5 += 0x180u;
        r4 += 0x180u;
        g13 += 0x180u;
        cursor++;
        remain--;
        i960_st_u32(I960_WORKRAM, 0x213834, 0, cursor);
        i960_st_u32(I960_WORKRAM, 0x213838, 0, remain);
    } while (remain != 0u);
}
