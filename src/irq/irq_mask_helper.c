/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00001560_f0.asm */
// @rom 0x1560 +0xf0 irq_mask_helper

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r4 */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* call site: caller 0x194c */
/* call site: caller 0x1960 */
/* call site: caller 0x1974 */

void irq_mask_helper(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    g4 = i960_ld_u32(I960_WORKRAM, 0x20200c, 0);
    /* lift: andnot g0, g4, g4 @ 0x1568 */
    i960_st_u32(I960_WORKRAM, 0x20200c, 0, (u32)g4);
    g4 = i960_ld_u32(I960_WORKRAM, 0x20200c, 0);
    r4 = (uintptr_t)arg0;
}
