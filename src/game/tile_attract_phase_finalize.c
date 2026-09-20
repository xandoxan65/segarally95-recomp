/* Attract phase 5 @ 0xCBF0 — final script + promote main mode. */
// @rom 0xcbf0 +0x50 tile_attract_phase_finalize

#include "i960_lift.h"
#include "i960_mem.h"

void tile_attract_phase_finalize(u32 arg0, u32 arg1, u32 arg2)
{
    u32 counter;
    u32 script;
    u32 main_mode;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    g0 = 15;
    g1 = 17;
    i960_call_rom(0x26e18);

    counter = i960_ld_u32(I960_WORKRAM, 0x20a520, 0);
    if (counter > 0)
        i960_st_u32(I960_WORKRAM, 0x20a520, 0, counter - 1);

    if (counter != 1)
        return;

    script = i960_ld_u32(I960_WORKRAM, 0x20a524, 0);
    if (script != 0) {
        g0 = script;
        i960_call_rom(0x27130);
    }

    main_mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x202098, 0, main_mode + 1);
}
