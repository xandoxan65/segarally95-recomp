/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: disasm/maincpu/maincpu_0036b0_70.asm */
// @rom 0x36b0 +0x64 game_seed_globals

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"
#include "model2_nvram.h"

void game_seed_globals(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    g4 = 1;
    i960_st_u32(I960_WORKRAM, 0x213840, 0, (u32)g4);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x202098, 0, (u32)g14);
    /* ROM @ 0x36B0 does not touch 0x202019 (COUNTRY); prior lift wrongly cleared it. */
    i960_st_u32(I960_WORKRAM, 0x20a558, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x214350, 0, (u32)g4);
    g4 = 0x1469;
    i960_st_u32(I960_WORKRAM, 0x20a55c, 0, (u32)g4);
    i960_st_u32(I960_WORKRAM, 0x20a530, 0, (u32)g14);
    g4 = 1;
    i960_st_u32(I960_WORKRAM, 0x20a560, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a564, 0, (u32)g4);
    geo_fifo_preset((u32)g1, (u32)g2);
    geo_workram_slot_clear((u32)g0, (u32)g1, (u32)g2);
    geo_view_params((u32)g0, (u32)g1, (u32)g2);

    model2_nvram_apply_options();
}
