/* Race HUD tachometer digits @ 0x1E500 — per-frame from hud_frame.
 *
 * ROM: ld 0x20b0b0 → lda 0x3b(g4) → divi 60, clamp 99. 0x20b0b0 is the
 * lap-span integer (slot0/1/3), not engine speed, so that formula parks
 * at 00. Analog needle math @ 0x3A5A0 is 60×60×|0x20b0b4| / 1.885
 * (same float the speedo uses). Digit path uses that product as `raw`
 * then the ROM /60 + 0x3b rounding.
 *
 * Draws/erases tens and ones into batch 0x20ac7c at (0x20aebc, 0x20aec0)
 * when 0x20aeb8 changes.
 *
 * source: disasm/maincpu/maincpu_01e500_c0.asm */
// @rom 0x1e500 +0xbc game_start_race_hud_tach

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_hud_tach(u32 arg0, u32 arg1, u32 arg2)
{
    u32 raw;
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
        fprintf(stderr, "lift: race_hud_tach\n");
        fflush(stderr);
        logged = 1;
    }

    /* @0x3A5A0: 60*60*|20b0b4|; @0x1E50C: lda 0x3b then /60. */
    {
        double mag = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0));

        if (mag < 0.0)
            mag = -mag;
        mag *= i960_u32_to_f64(0x42700000u);
        mag *= i960_u32_to_f64(0x42700000u);
        raw = (u32)mag;
    }
    v = (i32)((raw + 0x3bu) / 60u);
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
