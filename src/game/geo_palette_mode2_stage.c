/* Palette irq modes 1/2 @ 0x331D8 — snapshot colorxlat → CRX buffers; arm mode 7. */
// @rom 0x331d8 +0xf8 geo_palette_mode2_stage

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
 * Mode 1 @ 0x331D8 clears cursors and sets mode=2, then falls into this body.
 * Mode 2 @ 0x331F4 copies colorxlat banks @ 0x01810000+ into CRX work windows
 * @ 0x0020D830/E830/F830 (opposite of mode 6). When cursor > 31, arms mode 7.
 * Safe: does not overwrite colorxlat.
 */
void geo_palette_mode2_stage(u32 from_mode1, u32 arg1, u32 arg2)
{
    u32 cursor;
    u32 r7;
    u32 r8;
    u32 g5;
    u32 r5;
    u32 r4;
    u32 g13;
    u32 g3;
    u32 base;
    u32 g0;
    u32 g1;
    u32 g2;

    (void)arg1;
    (void)arg2;

    if (from_mode1 != 0u) {
        /* @0x331D8–0x331EC */
        i960_st_u32(I960_WORKRAM, 0x213834, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x213838, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x213850, 0, 2u);
    }

    cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
    r8 = 0u;
    r7 = cursor << 3;

    /* @0x33204: cmpibl 31, cursor → skip when cursor > 31 */
    if ((i32)31 < (i32)cursor)
        goto finish;

    do {
        /* @0x33208–0x3324C: colorxlat → CRX */
        g5 = r7 << 4;
        cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
        r5 = 0x0020f830u + g5;
        r4 = 0x0020e830u + g5;
        g13 = 0x0020d830u + g5;
        base = 0x01800000u + (cursor << 9);
        g2 = base + 0x18000u;
        g1 = base + 0x14000u;
        g0 = base + 0x10000u;

        g3 = 0u;
        do {
            /* @0x33254–0x33290: 8× ldq colorxlat / stq CRX; r7++ each */
            copy_quad(g0, g13);
            copy_quad(g1, r4);
            copy_quad(g2, r5);
            g2 += 16u;
            g1 += 16u;
            g0 += 16u;
            r5 += 16u;
            r4 += 16u;
            g13 += 16u;
            r7++;
            g3++;
        } while ((i32)g3 <= 7);

        cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
        r8++;
        cursor++;
        i960_st_u32(I960_WORKRAM, 0x213834, 0, cursor);
        /* @0x332A4: cmpibl 15, r8 → finish; else continue while cursor <= 31 */
        if ((i32)15 < (i32)r8)
            break;
    } while ((i32)31 >= (i32)cursor);

finish:
    cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
    /* @0x332B4: cmpibge 31, cursor → ret; else arm mode 7 */
    if ((i32)31 >= (i32)cursor)
        return;

    i960_st_u32(I960_WORKRAM, 0x213834, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x213850, 0, 7u);
}
