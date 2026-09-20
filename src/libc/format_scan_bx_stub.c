/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_0005cd38_14.asm */
// @rom 0x5cd38 +0x14 format_scan_bx_stub

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g0,g2,g1  link g14→g1 bx */
/* abi: u32 arg0=g0, u32 arg1=g2 → void */

void format_scan_bx_stub(u32 arg0, u32 arg1)
{
    g0 = (uintptr_t)arg0;
    g2 = (uintptr_t)arg1;

    g1 = g14;
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x2197b0, 0, (u32)arg0);
    /* bx (g1) */
    return;
}
