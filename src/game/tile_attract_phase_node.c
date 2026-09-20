/* Attract phase 3 @ 0xC990 — dynamic script pointer @ 0x20A524. */
// @rom 0xc990 +0x50 tile_attract_phase_node

#include "tile_attract_shared.h"
#include "i960_lift.h"
#include "i960_mem.h"

void tile_attract_phase_node(u32 arg0, u32 arg1, u32 arg2)
{
    u32 counter;
    u32 script;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    g0 = 15;
    g1 = 17;
    i960_call_rom(0x26e18);

    script = i960_ld_u32(I960_WORKRAM, 0x20a524, 0);
    if (script != 0) {
        g0 = script;
        i960_call_rom(0x27130);
    }

    tile_attract_flags_prologue();

    counter = i960_ld_u32(I960_WORKRAM, 0x20a520, 0);
    if (counter > 0)
        i960_st_u32(I960_WORKRAM, 0x20a520, 0, counter - 1);
}
