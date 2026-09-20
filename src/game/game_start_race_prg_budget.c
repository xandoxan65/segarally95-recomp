/* PRG draw budget seed @ 0x1C450 — race_frame before course_view_bind.
 *
 * Selects the per-course halfword table (0x5dc9a0 / 0x5dc9c0) by 0x2020a4,
 * indexes with 0x2140c8/10, optionally adds the 0x5dc9e0 row when the desert
 * zoom flag 0x2139d8 is clear, then folds 0x2139f0 list counters and mode
 * 0x202230 into a capped budget stored at 0x214344 (also 0x20aac4 / 0x214348).
 *
 * Without this leaf, 0x214344 stays 0 so catalog_span remain ≤ 0 after the
 * head catalog bumps 0x20b940 — no track landscape / start objects.
 *
 * source: disasm/maincpu/maincpu_01c450_280.asm */
// @rom 0x1c450 +0x280 game_start_race_prg_budget

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"

#include "lift_syms.h"

#include <stdio.h>

static i32 ldos_abs(u32 base, u32 byte_off)
{
    u16 half = i960_ld_u16(I960_WORKRAM, base, byte_off);

    return (i32)(signed short)half;
}

void game_start_race_prg_budget(u32 arg0, u32 arg1, u32 arg2)
{
    u32 g4;
    u32 g5;
    u32 g6;
    u32 g7;
    u32 g13;
    i32 r4;
    i32 r6;
    u32 g1;
    u32 g2;
    u32 g3;
    u32 r5;
    u32 r7;
    u32 r8;
    u32 r9;
    u32 g0_cursor;
    u32 g2_cursor;
    u32 g6_cursor;
    u32 g7_cursor;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    g14 = 0;

    /* @0x1C450–0x1C480: table pick by 0x2020a4. */
    g4 = i960_ld_u32(I960_WORKRAM, 0x2020a4, 0);
    if (g4 == 0u) {
        g4 = i960_host_race_course_index();
        g5 = i960_ld_u32(I960_WORKRAM, 0x5dc9a0, g4 << 2);
    } else {
        g4 = i960_host_race_course_index();
        g5 = i960_ld_u32(I960_WORKRAM, 0x5dc9c0, g4 << 2);
    }

    /* @0x1C480–0x1C4BC: halfword at (2140c8/10)*2; optional 5dc9e0 addend. */
    g4 = i960_ld_u32(I960_WORKRAM, 0x2140c8, 0);
    g4 = (u32)((i32)g4 / 10);
    g13 = i960_ld_u32(I960_WORKRAM, 0x2139d8, 0);
    g6 = g4 << 1;
    r6 = ldos_abs(g5, g6);
    if (g13 == 0u) {
        g4 = i960_host_race_course_index();
        g5 = i960_ld_u32(I960_WORKRAM, 0x5dc9e0, g4 << 2);
        r4 = ldos_abs(g5, g6);
    } else {
        r4 = 0;
    }

    /* @0x1C4C0–0x1C4D0: g3 seed from zoom flag. */
    g1 = 0;
    g3 = (g13 != 0u) ? 1u : 0u;

    /* @0x1C4D4–0x1C588: walk 0x2139f0 list when 0x2139f8 != 0. */
    g4 = i960_ld_u32(I960_WORKRAM, 0x2139f8, 0);
    g7_cursor = 0x2139f0u;
    r5 = i960_ld_u32(I960_WORKRAM, 0x213970, 0);
    if (g4 != 0u) {
        g2_cursor = g7_cursor + 8u;
        g6_cursor = g7_cursor + 4u;
        g0_cursor = g7_cursor + 12u;
        r7 = 2u;
        for (;;) {
            g4 = i960_ld_u32(I960_WORKRAM, g6_cursor, 0);
            if (g4 != 0u) {
                u32 g5_add = 1u;

                if (g13 == 0u) {
                    g4 = i960_ld_u32(I960_WORKRAM, g7_cursor, 0);
                    if (g4 == 3u)
                        g5_add = 2u;
                }
                r9 = i960_ld_u32(I960_WORKRAM, g0_cursor, 0);
                /* cmprl (g0) as float vs double 400.0 (0x40790000:0). */
                if (i960_u32_to_f64(r9) > 400.0) {
                    i960_st_u32(I960_WORKRAM, g6_cursor, 0, (u32)g14);
                } else {
                    g4 = i960_ld_u32(I960_WORKRAM, g0_cursor, 0);
                    if (i960_u32_to_f64(g4) > i960_u32_to_f64(r5)) {
                        i960_st_u32(I960_WORKRAM, g6_cursor, 0, r7);
                        g1 += g5_add;
                    } else {
                        r9 = 1u;
                        g3 += g5_add;
                        i960_st_u32(I960_WORKRAM, g6_cursor, 0, r9);
                    }
                }
            }
            g2_cursor += 16u;
            g4 = i960_ld_u32(I960_WORKRAM, g2_cursor, 0);
            g6_cursor += 16u;
            g0_cursor += 16u;
            g7_cursor += 16u;
            if (g4 == 0u)
                break;
        }
    }

    /* @0x1C588–0x1C5DC: mode 0x202230 → g5 / g6 weights. */
    g7 = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
    if (g7 == 0u) {
        if (g3 == 0u) {
            g5 = 0u;
        } else {
            if (g13 != 0u)
                g5 = 0x2eeu;
            else
                g5 = 0x177u;
            r8 = 0x177u;
            g4 = g3 * r8;
            /* lda 0xfffffe89(g4)[g5] → g5 + g4 - 0x177. */
            g5 = g5 + g4 + 0xfffffe89u;
        }
        /* lda (g1)[g1*4],g4; shlo 4 → g6 = g1*5 << 4. */
        g4 = g1 + g1 * 4u;
        g6 = g4 << 4;
    } else {
        r9 = 0x2eeu;
        g5 = g3 * r9;
        g4 = g1 + g1 * 4u;
        g6 = g4 << 5;
    }

    /* @0x1C5DC–0x1C62C: sum + optional desert close-up cap 0xaf0. */
    g4 = (u32)((i32)r6 + (i32)r4);
    g4 = g5 + g4;
    g2 = g6 + g4;
    i960_st_u32(I960_WORKRAM, 0x20aac8, 0, g1);
    i960_st_u32(I960_WORKRAM, 0x214348, 0, (u32)r4);
    g1 = 0xed8u; /* 3800 */
    if (g7 == 0u && g13 != 0u) {
        g4 = i960_host_race_course_index();
        if (g4 == 0u) {
            g4 = i960_ld_u32(I960_WORKRAM, 0x2140c4, 0);
            /* cmpible g4,-5 / cmpibl 9,g4 — af0 when -4 ≤ g4 ≤ 9. */
            if ((i32)g4 > -5 && (i32)g4 <= 9)
                g1 = 0xaf0u; /* 2800 */
        }
    }

    /* @0x1C62C–0x1C6C4: over-cap trim; re-walk list to shrink g3. */
    if (!((u32)g2 < g1)) {
        g2 = g2 - g5;
        g4 = (g13 != 0u) ? 1u : 2u;
        if ((i32)g3 >= (i32)g4)
            g6 = (g13 != 0u) ? 1u : 2u;
        else
            g6 = g3;
        g3 = g6;
        g4 = i960_ld_u32(I960_WORKRAM, 0x2139f8, 0);
        g7_cursor = 0x2139f0u;
        if (g4 != 0u) {
            g0_cursor = g7_cursor + 8u;
            g5 = g7_cursor + 4u;
            r9 = 2u;
            for (;;) {
                g4 = i960_ld_u32(I960_WORKRAM, g5, 0);
                if (g4 == 1u) {
                    if (g6 == 0u)
                        i960_st_u32(I960_WORKRAM, g5, 0, r9);
                    else
                        g6 -= 1u;
                }
                g0_cursor += 16u;
                g4 = i960_ld_u32(I960_WORKRAM, g0_cursor, 0);
                g5 += 16u;
                if (g4 == 0u)
                    break;
            }
        }
        r8 = 0x2eeu;
        g4 = g3 * r8;
        g2 = g2 + g4;
        if (!((u32)g2 <= g1)) {
            /* subo g1,g2,g4; subo g4,r6,r6 → r6 -= (g2 − g1). */
            g4 = g2 - g1;
            r6 = (i32)r6 - (i32)g4;
        }
    }

    i960_st_u32(I960_WORKRAM, 0x20aac4, 0, g3);
    i960_st_u32(I960_WORKRAM, 0x214344, 0, (u32)r6);

    if (!logged) {
        fprintf(stderr,
                "lift: race_prg_budget 214344=%u 20aac4=%u 214348=%d zoom=%u\n",
                (unsigned)(u32)r6, (unsigned)g3, (int)r4, (unsigned)g13);
        fflush(stderr);
        logged = 1;
    }
}
