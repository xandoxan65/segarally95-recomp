/* Palette irq mode 6 @ 0x332D0 — commit work buffers → palram banks; arm mode 7. */
// @rom 0x332d0 +0xf8 geo_palette_mode6_commit

#include "i960_lift.h"
#include "i960_mem.h"

static void copy_quad(u32 src, u32 dest)
{
    i960_st_u32(I960_ABS, dest, 0, i960_ld_u32(I960_ABS, src, 0));
    i960_st_u32(I960_ABS, dest, 4, i960_ld_u32(I960_ABS, src, 4));
    i960_st_u32(I960_ABS, dest, 8, i960_ld_u32(I960_ABS, src, 8));
    i960_st_u32(I960_ABS, dest, 12, i960_ld_u32(I960_ABS, src, 12));
}

void geo_palette_mode6_commit(u32 arg0, u32 arg1, u32 arg2)
{
    u32 cursor;
    u32 r6;
    u32 r7;
    u32 g5;
    u32 r5;
    u32 r4;
    u32 g13;
    u32 g3;
    u32 base;
    u32 g0;
    u32 g1;
    u32 g2;
    u16 seed;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
    r7 = 0u;
    r6 = cursor << 3;

    /* @0x332E0: cmpibl 31, cursor → skip body when cursor > 31 */
    if ((i32)31 < (i32)cursor)
        goto finish;

    do {
        /* @0x332E4–0x33328 */
        g5 = r6 << 4;
        cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
        /* Immediates @ 0x332F0–0x33300 are 0x0020xxxx (CRX / work window). */
        r5 = 0x0020f830u + g5;
        r4 = 0x0020e830u + g5;
        g13 = 0x0020d830u + g5;
        base = 0x01800000u + (cursor << 9);
        g2 = base + 0x18000u;
        g1 = base + 0x14000u;
        g0 = base + 0x10000u;

        g3 = 0u;
        do {
            /* @0x33330–0x3336C: 8× ldq/stq triples; r6++ each */
            copy_quad(g13, g0);
            copy_quad(r4, g1);
            copy_quad(r5, g2);
            g2 += 16u;
            g1 += 16u;
            g0 += 16u;
            r5 += 16u;
            r4 += 16u;
            g13 += 16u;
            r6++;
            g3++;
        } while ((i32)g3 <= 7);

        cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
        r7++;
        cursor++;
        i960_st_u32(I960_WORKRAM, 0x213834, 0, cursor);
        /* @0x33380: cmpi r7,15; bg → finish; else if cursor <= 31 continue */
        if ((i32)r7 > 15)
            break;
    } while ((i32)31 >= (i32)cursor);

finish:
    cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
    /* @0x3339C: cmpibge 31, cursor → ret without arming mode 7 when cursor <= 31 */
    if ((i32)31 >= (i32)cursor)
        return;

    seed = i960_ld_u16(I960_WORKRAM, 0x213830, 0);
    i960_st_u32(I960_WORKRAM, 0x213834, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x213850, 0, 7u);
    i960_st_u16(I960_ABS, 0x01800000u, 0, seed);
}
