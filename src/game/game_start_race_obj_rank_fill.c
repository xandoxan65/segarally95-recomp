/* Object rank helper @ 0x23130 — insertion-sort 0x20b0c0 by pose+0x56.
 * Called from sort_index after seeding identity ranks. Solo count<=1 returns
 * immediately (cmpibge 1,count). Practice seeds count=2 (cam slot + car), so
 * this must run or 0x213b00 stays identity and desert view-base never prefers
 * the car pose.
 *
 * source: disasm/maincpu/maincpu_023130_c0.asm */
// @rom 0x23130 +0xb4 game_start_race_obj_rank_fill

#include "i960_lift.h"
#include "i960_mem.h"

void game_start_race_obj_rank_fill(u32 arg0, u32 arg1, u32 arg2)
{
    u32 count = i960_ld_u32(I960_WORKRAM, 0x213978, 0);
    u32 g0;
    u32 g1 = 1u;
    u32 g3 = 0x0020b0c4u;
    u32 g2;
    u32 g4;
    u32 g5;
    u32 g6;
    u32 g7;
    u32 g13;
    i32 span_ins;
    i32 span_left;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x2313C: cmpibge 1,count → ret when count <= 1. */
    if ((i32)count <= 1)
        return;

    /* Outer: g1 = 1 .. count-1. */
    while ((i32)g1 < (i32)count) {
        g0 = g1 - 1u;
        g13 = i960_ld_u32(I960_WORKRAM, g3, 0); /* 20b0c0[g1] */

        /*
         * @0x23150: cmpibg 0,g0 → store when g0 < 0.
         * Else compare spans; shift while left < insert.
         */
        if ((i32)g0 >= 0) {
            g7 = g0 << 2;
            g5 = i960_ld_u32(I960_WORKRAM, 0x20b0c0u + g7, 0);
            g2 = 0x213980u + (g13 << 2);
            g4 = i960_ld_u32(I960_WORKRAM, g2, 0); /* pose_ins */
            g5 = i960_ld_u32(I960_WORKRAM, 0x213980u + (g5 << 2), 0);
            span_ins = (i32)(signed short)i960_ld_u16(I960_ABS, g4, 0x56);
            span_left = (i32)(signed short)i960_ld_u16(I960_ABS, g5, 0x56);

            /* @0x2317C: cmpibge span_left,span_ins → store when left >= ins. */
            if (span_left < span_ins) {
                g4 = 0x0020b0c0u;
                g6 = g4 + g7;           /* &20b0c0[g0] */
                g7 = g4 + g7 + 4u;      /* &20b0c0[g0+1] */

                for (;;) {
                    /* @0x23194: shift one slot right; g0--. */
                    g4 = i960_ld_u32(I960_WORKRAM, g6, 0);
                    g6 = g6 - 4u;
                    g0 = g0 - 1u;
                    i960_st_u32(I960_WORKRAM, g7, 0, g4);
                    g7 = g7 - 4u;

                    /* @0x231A0/AC: cmpi g0,0 / bl → store when g0 < 0. */
                    if ((i32)g0 < 0)
                        break;

                    /*
                     * @0x231B0–0x231C8: reload spans; cmpibl left,ins →
                     * keep shifting while span_left < span_ins.
                     */
                    g4 = i960_ld_u32(I960_WORKRAM, g6, 0);
                    g5 = i960_ld_u32(I960_WORKRAM, g2, 0); /* pose_ins */
                    g4 = i960_ld_u32(I960_WORKRAM, 0x213980u + (g4 << 2), 0);
                    span_ins = (i32)(signed short)i960_ld_u16(I960_ABS, g5, 0x56);
                    span_left = (i32)(signed short)i960_ld_u16(I960_ABS, g4, 0x56);
                    if (span_left >= span_ins)
                        break;
                }
            }
        }

        /*
         * @0x231CC–0x231E0: bump g1, store insert at 20b0c4[g0],
         * advance g3, loop while g1 < count.
         */
        g1 = g1 + 1u;
        i960_st_u32(I960_WORKRAM, 0x20b0c4u + (g0 << 2), 0, g13);
        g3 = g3 + 4u;
    }
}
