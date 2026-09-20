/* Attract CREDIT HUD per-frame @ 0xF5F0 (inner jump table slot 12).
 * source: disasm/maincpu/maincpu_00f5f0_30.asm */
// @rom 0xf5f0 +0x30 attract_credit_hud_frame

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void attract_credit_hud_frame(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (i960_ld_u32(I960_WORKRAM, 0x20a550, 0) != 2u)
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);

    g0 = i960_ld_u32(I960_WORKRAM, 0x20a540, 0);
    g1 = 1;
    i960_call_rom(0x1afb0);
    comm_scene_catalog_init(0, 0, 0);
}
