/* Championship / practice select icon + countdown draw @ 0x14530.
 * Catalog batch @ 0x20a8b4 (from 0x208e9e4); timer @ 0x20a8b8.
 * source: disasm/maincpu/maincpu_014530_130.asm */
// @rom 0x14530 +0x130 game_start_select_icons

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

void game_start_select_icons(u32 arg0, u32 arg1, u32 arg2)
{
    u32 timer;
    u32 x0 = arg0;
    u32 y0 = arg1;
    u32 batch;
    u32 y;
    u32 digits;
    u32 glyph;

    (void)arg2;

    timer = i960_ld_u32(I960_WORKRAM, 0x20a8b8, 0);
    batch = i960_ld_u32(I960_WORKRAM, 0x20a8b4, 0);
    y = y0 + 1u;

    /* @0x14544: timer ≤ 59 → erase digit slots, then decrement. */
    if (timer <= 59u) {
        g0 = x0 + 1u;
        g1 = y;
        g2 = batch;
        g3 = 0;
        g4 = 0;
        erase_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        g0 = x0 + 5u;
        g1 = y;
        g2 = batch;
        g3 = 0;
        g4 = 0;
        erase_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        goto dec_timer;
    }

    /* @0x14588: timer > 59. */
    if (timer == 60u) {
        g0 = 0x7eu;
        g1 = 0x50u;
        i960_call_rom(0x261b0);
    }

    /*
     * @0x145A8: timer ≤ 59 → selection glyphs (unreachable here);
     * timer > 59 → countdown digits from (timer − 60).
     */
    if (timer <= 59u) {
        g0 = x0 + 1u;
        g1 = y;
        g2 = batch;
        g3 = 0;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        g0 = x0 + 5u;
        g1 = y;
        g2 = batch;
        g3 = 0;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    } else {
        digits = timer - 60u;
        glyph = digits / 1000u;
        g0 = x0 + 1u;
        g1 = y;
        g2 = batch;
        g3 = glyph;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        glyph = (digits % 1000u) / 100u;
        g0 = x0 + 5u;
        g1 = y;
        g2 = batch;
        g3 = glyph;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

dec_timer:
    timer = i960_ld_u32(I960_WORKRAM, 0x20a8b8, 0);
    if (timer != 0u)
        timer -= 1u;
    i960_st_u32(I960_WORKRAM, 0x20a8b8, 0, timer);
}
