/* Race HUD countdown digits @ 0x1E500 — per-frame from hud_frame.
 *
 * ld 0x20b0b0, lda 0x3b(g4), divi by 60, clamp to 99. 0x20b0b0 is the
 * lap-span frame count (seeded from the course bonus table, ticked in
 * lap_tick, refilled at checkpoints). Displayed seconds are
 * (span + 59) / 60. Speed lives in 0x20b0b4 and is the speedo, not this.
 *
 * Draws/erases tens and ones into batch 0x20ac7c at (0x20aebc, 0x20aec0)
 * when 0x20aeb8 changes.
 *
 * source: disasm/maincpu/maincpu_01e500_c0.asm */
// @rom 0x1e500 +0xbc game_start_race_hud_tach

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_hud_tach(u32 arg0, u32 arg1, u32 arg2)
{
    i32 v;
    u32 slot_base;
    u32 x;
    u32 y;
    u32 batch;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_hud_tach\n");
        fflush(stderr);
        logged = 1;
    }

    /* @0x1E500: ld 0x20b0b0; addo 60; lda 0x3b; divi; cmpible r4,99. */
    {
        i32 span = (i32)i960_ld_u32(I960_WORKRAM, 0x20b0b0, 0);

        v = (span + 0x3b) / 60;
    }
    if (v > 0x63)
        v = 0x63;

    if ((u32)v == i960_ld_u32(I960_WORKRAM, 0x20aeb8, 0))
        return;

    i960_st_u32(I960_WORKRAM, 0x20aeb8, 0, (u32)v);

    /* r5 = 10 unless aeb4==0 and v<=5. */
    if (i960_ld_u32(I960_WORKRAM, 0x20aeb4, 0) != 0u || v <= 5)
        slot_base = 10u;
    else
        slot_base = 0u;

    x = i960_ld_u32(I960_WORKRAM, 0x20aebc, 0);
    y = i960_ld_u32(I960_WORKRAM, 0x20aec0, 0);
    batch = i960_ld_u32(I960_WORKRAM, 0x20ac7c, 0);

    /* Tens: erase when v<=9, else draw (v/10)%10. */
    if (v <= 9) {
        g0 = x;
        g1 = y;
        g2 = batch;
        g3 = 0;
        g4 = 0;
        erase_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    } else {
        g0 = x;
        g1 = y;
        g2 = batch;
        g3 = slot_base + (u32)((v / 10) % 10);
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    /* Ones at x+4. */
    g0 = x + 4u;
    g1 = y;
    g2 = batch;
    g3 = slot_base + (u32)(v % 10);
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
}
