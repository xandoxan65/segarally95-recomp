/* CREDIT label stamp (non-zero credits) @ 0xBFA0.
 * source: disasm/maincpu/maincpu_00bfa0_c0.asm */
// @rom 0xbfa0 +0xb8 attract_credit_stamp_paid

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void attract_credit_stamp_paid(u32 slot, u32 arg1, u32 arg2)
{
    u32 gate;
    u32 r4;

    (void)arg1;
    (void)arg2;

    r4 = slot;
    gate = i960_ld_u32(I960_WORKRAM, 0x202008, 0) & 31u;
    if (gate != 0)
        return;

    if ((i960_ld_u32(I960_WORKRAM, 0x202008, 0) & (1u << 5)) == 0) {
        /* Clear a 16×2 tile rect then return. */
        boot_tile_map_fill(16, 31, 31, 2);
        return;
    }

    boot_tile_map_fill(16, 31, 30, 2);
    g0 = 16;
    g1 = 31;
    g2 = r4;
    g3 = 0;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    g0 = 28;
    g1 = 31;
    g2 = r4;
    g3 = 1;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    g0 = 37; /* 31+6 */
    g1 = 31;
    g2 = r4;
    g3 = 2;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    g0 = 44; /* 31+13 */
    g1 = 31;
    g2 = r4;
    g3 = 3;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
}
