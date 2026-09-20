/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00033e18_14.asm */
// @rom 0x33e18 +0x14 game_staging_bx

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g0,g2,g1  link g14→g1 bx */
/* abi: u32 arg0=g0, u32 arg1=g2 → void */

void game_staging_bx(u32 arg0, u32 arg1)
{
    g0 = (uintptr_t)arg0;
    g2 = (uintptr_t)arg1;

    g1 = g14;
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x213864, 0, (u32)arg0);
    /* bx (g1) */
    return;
}
