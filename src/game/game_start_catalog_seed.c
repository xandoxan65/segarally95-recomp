/* Catalog seed helper @ 0x144F0 — called from game_start_display_setup.
 * Draws CGM @ 0x208E9E4 (layer 4), stores batch + offset 0x3c(arg0).
 * source: disasm/maincpu/maincpu_0144f0_40.asm */
// @rom 0x144f0 +0x38 game_start_catalog_seed

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

void game_start_catalog_seed(u32 arg0, u32 arg1, u32 arg2)
{
    u32 batch;

    (void)arg1;
    (void)arg2;

    g0 = 0;
    g1 = 0;
    g2 = 0x0208e9e4u;
    g3 = 0;
    g4 = 4;
    batch = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
    i960_st_u32(I960_WORKRAM, 0x20a8b8, 0, arg0 + 0x3cu);
    i960_st_u32(I960_WORKRAM, 0x20a8b4, 0, batch);
}
