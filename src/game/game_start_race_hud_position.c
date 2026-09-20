/* Race HUD position / car-count digits @ 0x1ECC0 — hud_frame passes
 * g0 = 0x2020c8+1. g0==0 seeds glyph pair; g0!=0 updates when caches differ.
 *
 * source: disasm/maincpu/maincpu_01ecc0_f0.asm */
// @rom 0x1ecc0 +0xe4 game_start_race_hud_position

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_hud_position(u32 arg0, u32 arg1, u32 arg2)
{
    u32 y_base;
    u32 batch;
    u32 count;
    u32 slot;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        fprintf(stderr, "lift: race_hud_position g0=%u\n", (unsigned)arg0);
        fflush(stderr);
        logged = 1;
    }

    y_base = (i960_ld_u32(I960_WORKRAM, 0x2139d4, 0) == 0u) ? 3u : 1u;
    batch = i960_ld_u32(I960_WORKRAM, 0x20ac80, 0);

    if (arg0 == 0u) {
        /* Seed "P"/"/" style pair; invalidate caches. */
        g0 = 31u + 17u; /* 48 */
        g1 = y_base;
        g2 = batch;
        g3 = 0;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        g0 = 31u + 23u; /* 54 */
        g1 = y_base + 2u;
        g2 = batch;
        g3 = 1;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        i960_st_u32(I960_WORKRAM, 0x20ab3c, 0, (u32)-1);
        i960_st_u32(I960_WORKRAM, 0x20ab40, 0, (u32)-1);
        return;
    }

    count = i960_ld_u32(I960_WORKRAM, 0x213978, 0);
    if (i960_ld_u32(I960_WORKRAM, 0x20ab3c, 0) != count) {
        i960_st_u32(I960_WORKRAM, 0x20ab3c, 0, count);
        slot = count;
        if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) == 0u)
            slot = 15u;
        g0 = 31u + 25u; /* 56 */
        g1 = y_base + 4u;
        g2 = batch;
        g3 = slot + 1u;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    if (i960_ld_u32(I960_WORKRAM, 0x20ab40, 0) != arg0) {
        i960_st_u32(I960_WORKRAM, 0x20ab40, 0, arg0);
        g0 = 31u + 15u; /* 46 */
        g1 = y_base + 2u;
        g2 = batch;
        g3 = arg0 + 17u;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }
}
