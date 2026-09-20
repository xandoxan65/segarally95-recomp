/* Race lap/progress tick @ 0x1C8B8 — bal from slot3 after gate_walk.
 * mov g14,g1; mov 0,g14; if span<0 then span−=1; bump 0x2020bc / 0x2020b8;
 * increment words at 0x20afb0[ac98] and 0x2020d0[c0]; bx (g1).
 * source: disasm/maincpu/maincpu_01c8b8_90.asm */
// @rom 0x1c8b8 +0x88 game_start_race_lap_tick

#include "i960_lift.h"
#include "i960_mem.h"

void game_start_race_lap_tick(u32 arg0, u32 arg1, u32 arg2)
{
    u32 span;
    u32 bc;
    u32 b8;
    u32 ac98;
    u32 c0;
    u32 ea;
    u32 val;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    g14 = 0;
    span = i960_ld_u32(I960_WORKRAM, 0x20b0b0, 0);
    /* @0x1C8C8: cmpibge 0,span → skip; else span -= 1. */
    if ((i32)span < 0)
        span = span - 1u;
    bc = i960_ld_u32(I960_WORKRAM, 0x2020bc, 0);
    b8 = i960_ld_u32(I960_WORKRAM, 0x2020b8, 0);
    ac98 = i960_ld_u32(I960_WORKRAM, 0x20ac98, 0);
    c0 = i960_ld_u32(I960_WORKRAM, 0x2020c0, 0);
    i960_st_u32(I960_WORKRAM, 0x20b0b0, 0, span);
    i960_st_u32(I960_WORKRAM, 0x2020bc, 0, bc + 1u);
    i960_st_u32(I960_WORKRAM, 0x2020b8, 0, b8 + 1u);

    ea = 0x20afb0u + (ac98 << 2);
    val = i960_ld_u32(I960_WORKRAM, ea, 0);
    i960_st_u32(I960_WORKRAM, ea, 0, val + 1u);

    ea = 0x2020d0u + (c0 << 2);
    val = i960_ld_u32(I960_WORKRAM, ea, 0);
    i960_st_u32(I960_WORKRAM, ea, 0, val + 1u);
}
