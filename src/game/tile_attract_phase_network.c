/* Attract phase 2 @ 0xC840 — network check script @ 0x5AB7E0. */
// @rom 0xc840 +0x50 tile_attract_phase_network

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void tile_attract_phase_network(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    gate = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
    if ((gate & 0x14u) != 0)
        return;

    tile_cursor_seed(15, 17);
    boot_tile_script_run(0x005ab7e0u);

    /* @ 0xC868: string-length blanker @ 0x271D8 (not printf format). */
    boot_tile_script_format(0x005ab7e0u, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a564, 0, 4);
}
