/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_0003ec78_24.asm */
// @rom 0x3ec78 +0x24 geo_fifo_preset

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g1,g2,g0  link g14→g0 bx */
/* abi: u32 arg0=g1, u32 arg1=g2 → u32 g0 */

/* pointers: g4=void * */

u32 geo_fifo_preset(u32 arg0, u32 arg1)
{
    g1 = (uintptr_t)arg0;
    g2 = (uintptr_t)arg1;

    g0 = g14;
    g14 = 0;
    /* Disasm: lda 0x800100,g4 — GEO texture_ram base, not a host pointer. */
    g4 = 0x800100u;
    i960_st_u32(I960_WORKRAM, 0x214374, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x214370, 0, (u32)g4);
    /* bx (g0) — host C callers ignore; ROM bal path restores via invoke. */
    return g0;
}
