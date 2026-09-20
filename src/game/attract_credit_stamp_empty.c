/* CREDIT label stamp (empty credits) @ 0xBEE0.
 * source: disasm/maincpu/maincpu_00bee0_c0.asm */
// @rom 0xbee0 +0xc0 attract_credit_stamp_empty

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void attract_credit_stamp_empty(u32 slot, u32 arg1, u32 arg2)
{
    u32 gate;
    u32 r4;

    (void)arg1;
    (void)arg2;

    r4 = slot;
    gate = i960_ld_u32(I960_WORKRAM, 0x202008, 0) & 31u;
    if (gate != 0)
        return;

    if ((i960_ld_u32(I960_WORKRAM, 0x202008, 0) & (1u << 5)) != 0) {
        g0 = 21;
        g1 = 31;
        g2 = r4;
        g3 = 0;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        g0 = 33; /* 31+2 */
        g1 = 31;
        g2 = r4;
        g3 = 2;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        g0 = 40; /* 31+9 */
        g1 = 31;
        g2 = r4;
        g3 = 3;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        return;
    }

    g0 = 21;
    g1 = 31;
    g2 = r4;
    g3 = 0;
    g4 = 0;
    erase_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    g0 = 33;
    g1 = 31;
    g2 = r4;
    g3 = 2;
    g4 = 0;
    erase_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    g0 = 40;
    g1 = 31;
    g2 = r4;
    g3 = 3;
    g4 = 0;
    erase_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
}
