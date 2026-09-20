/* Semantic C from MAME disasm @ 0x33dc8 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_033dc8_80.asm */
// @rom 0x33dc8 +0x40 geo_rng_next

#include "i960_lift.h"
#include "i960_mem.h"

/*
 * LCG step used by geo_attract_object_extra (bal @ 0x31D10 / 0x31F48).
 *   seed = seed * 0x41C64E6D + 0x3039
 *   return (seed >> 16) % 0x7FFF
 * Seed cell @ 0x213864. Returns via bx(link); value in g0.
 */
void geo_rng_next(void)
{
    uintptr_t link = g14;
    u32 seed = i960_ld_u32(I960_WORKRAM, 0x213864, 0);

    g14 = 0;
    seed = seed * 0x41c64e6du + 0x3039u;
    i960_st_u32(I960_WORKRAM, 0x213864, 0, seed);
    g0 = (seed >> 16) % 0x7fffu;
    g14 = link;
}
