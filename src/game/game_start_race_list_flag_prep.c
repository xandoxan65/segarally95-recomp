/* Race list-flag / scratch prep @ 0x1C2A0 — race_frame peer of prg_budget.
 *
 * When 0x202230 == 1: mask low 6 bits of 0x213be9 stride-0x58 bytes for
 * 0x2139c8 count, then force 0x2139f0 list flags to 1.
 * Else: index 0x5bb200 from g0 (0x20aac0)/10000, walk 0x2139f0 vs 0x5bb290
 * thresholds and write masked object bytes + list flags.
 *
 * Private host frame for stq/ld at 0x40(fp).
 *
 * source: disasm/maincpu/maincpu_01c2a0_1c0.asm */
// @rom 0x1c2a0 +0x1b0 game_start_race_list_flag_prep

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

void game_start_race_list_flag_prep(u32 arg0, u32 arg1, u32 arg2)
{
    u32 g0_in = arg0 != 0u ? arg0 : (u32)g0;
    u32 g4;
    u32 g3;
    u32 g1;
    u32 g2;
    u32 g5;
    u32 g6;
    u32 r4;
    u32 r5;
    u32 r6;
    u32 r7;
    u32 r8;
    u32 r9;
    u32 r10;
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x50];
    static int logged;

    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 16u;
    g14 = 0;

    g4 = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
    g1 = 0x2139f0u;
    g3 = i960_ld_u32(I960_WORKRAM, 0x2139f8, 0);

    if (g4 == 1u) {
        /* @0x1C2C0–0x1C31C: mask 0x213be9[..] then force list flags = 1. */
        g6 = i960_ld_u32(I960_WORKRAM, 0x2139c8, 0);
        g2 = 1u;
        if (!((u32)1 >= g6)) {
            g5 = 0x213be9u;
            r8 = 0x3fu;
            do {
                g4 = i960_ld_u8(I960_WORKRAM, g5, 0);
                g2 += 1u;
                g4 = g4 & r8;
                i960_st_u8(I960_WORKRAM, g5, 0, (u8)g4);
                g5 += 0x58u;
            } while (g2 < g6);
        }
        if (g3 != 0u) {
            g6 = g1 + 8u;
            g5 = g1 + 4u;
            r8 = 1u;
            do {
                g6 += 16u;
                g4 = i960_ld_u32(I960_WORKRAM, g6, 0);
                i960_st_u32(I960_WORKRAM, g5, 0, r8);
                g5 += 16u;
            } while (g4 != 0u);
        }
        goto done;
    }

    /* @0x1C320–0x1C440: g0 = clamp((g0/10000 − 21) >> 1, 0..8). */
    r8 = 0x2710u;
    g0_in = (u32)((i32)g0_in / (i32)r8);
    g0_in = g0_in - 21u;
    g0_in = (u32)((i32)g0_in >> 1);
    if ((i32)g0_in < 0)
        g0_in = 0u;
    else if ((i32)g0_in > 7)
        g0_in = 8u;

    {
        u32 w0 = i960_ld_u32(I960_WORKRAM, 0x5bb200, g0_in << 4);
        u32 w1 = i960_ld_u32(I960_WORKRAM, 0x5bb200, (g0_in << 4) + 4u);
        u32 w2 = i960_ld_u32(I960_WORKRAM, 0x5bb200, (g0_in << 4) + 8u);
        u32 w3 = i960_ld_u32(I960_WORKRAM, 0x5bb200, (g0_in << 4) + 12u);

        *(u32 *)(fp + 0x40) = w0;
        *(u32 *)(fp + 0x44) = w1;
        *(u32 *)(fp + 0x48) = w2;
        *(u32 *)(fp + 0x4c) = w3;
    }

    g2 = 0u;
    if (g3 == 0u)
        goto done;

    g13 = g1 + 8u;
    r4 = g1 + 4u;
    r5 = g1 + 12u;
    g1 = 0u;
    g3 = 0x5bb290u;
    r9 = 1u;
    r10 = 0x3fu;

    for (;;) {
        /* @0x1C380: ldl (g13) → r6:r7 (obj ptr + float). */
        r6 = i960_ld_u32(I960_WORKRAM, g13, 0);
        r7 = i960_ld_u32(I960_WORKRAM, g13, 4);
        g4 = i960_ld_u32(I960_WORKRAM, g3, 0);
        /* cmpr r7,g4 — advance 5bb290 cursor while float > table. */
        if (i960_u32_to_f64(r7) > i960_u32_to_f64(g4)) {
            g5 = 0x5bb290u + (g2 << 2);
            g6 = i960_ld_u32(I960_WORKRAM, r5, 0);
            g5 += 4u;
            for (;;) {
                g4 = i960_ld_u32(I960_WORKRAM, g5, 0);
                if (!(i960_u32_to_f64(g6) > i960_u32_to_f64(g4)))
                    break;
                g1 += 4u;
                g3 += 4u;
                g2 += 1u;
                g5 += 4u;
            }
        }

        g4 = *(u32 *)(fp + 0x40 + g1);
        if (g4 == 0u) {
            do {
                g1 += 4u;
                g4 = *(u32 *)(fp + 0x40 + g1);
                g3 += 4u;
                g2 += 1u;
            } while (g4 == 0u);
        }

        g0_in = r6 + 0x51u;
        /* Host frame slot — keep full fp+offset (do not truncate to u32). */
        g4 = i960_ld_u8(I960_WORKRAM, g0_in, 0);
        g5 = *(u32 *)(fp + 0x40 + g1);
        g6 = g2 << 6;
        g4 = g4 & r10;
        g5 -= 1u;
        g4 = g4 | g6;
        *(u32 *)(fp + 0x40 + g1) = g5;
        i960_st_u8(I960_WORKRAM, g0_in, 0, (u8)g4);

        /* cmpi g2,2 then bg → skip bit7 test. */
        if (!((i32)g2 > 2)) {
            g4 = i960_ld_u32(I960_WORKRAM, g13, 0);
            g4 = i960_ld_u8(I960_WORKRAM, g4, 0x50);
            if ((g4 & (1u << 7)) != 0u)
                i960_st_u32(I960_WORKRAM, r4, 0, r9);
            else
                i960_st_u32(I960_WORKRAM, r4, 0, (u32)g14);
        } else {
            i960_st_u32(I960_WORKRAM, r4, 0, (u32)g14);
        }

        g13 += 16u;
        g4 = i960_ld_u32(I960_WORKRAM, g13, 0);
        r4 += 16u;
        r5 += 16u;
        if (g4 == 0u)
            break;
    }

done:
    if (!logged) {
        fprintf(stderr, "lift: race_list_flag_prep mode230=%u g0_in=%u\n",
                (unsigned)i960_ld_u32(I960_WORKRAM, 0x202230, 0),
                (unsigned)g0_in);
        fflush(stderr);
        logged = 1;
    }
    fp = fp_save;
    sp = sp_save;
}
