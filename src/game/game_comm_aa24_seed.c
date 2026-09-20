/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_0001a7b8_18.asm */
// @rom 0x1a7b8 +0x18 game_comm_aa24_seed

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g1,g2,g0  link g14→g0 bx */
/* abi: u32 arg0=g1, u32 arg1=g2 → u32 g0 */

u32 game_comm_aa24_seed(u32 arg0, u32 arg1)
{
    g1 = (uintptr_t)arg0;
    g2 = (uintptr_t)arg1;

    g0 = g14;
    g14 = 0;
    g4 = 1;
    i960_st_u32(I960_WORKRAM, 0x20aa24, 0, (u32)g4);
    /* bx (g0) */
    return g0;
}
