/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00001758_28.asm */
// @rom 0x1758 +0x28 game_irq_wr_stub

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g1,g2,g0  link g14→g0 bx */
/* abi: u32 arg0=g1, u32 arg1=g2 → u32 g0 */

u32 game_irq_wr_stub(u32 arg0, u32 arg1)
{
    g1 = (uintptr_t)arg0;
    g2 = (uintptr_t)arg1;

    g0 = g14;
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x20200c, 0, (u32)g14);
    g4 = i960_ld_u32(I960_WORKRAM, 0x20200c, 0);
    i960_mmio_write_u32(0xe80004, (u32)g4); /* irq_enable */;
    /* bx (g0) */
    return g0;
}
