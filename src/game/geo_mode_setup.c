/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00004708_44.asm */
// @rom 0x4708 +0x44 geo_mode_setup

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g1,g2,g0  link g14→g0 bx */
/* abi: u32 arg0=g1, u32 arg1=g2 → void */

void geo_mode_setup(u32 arg0, u32 arg1)
{
    g1 = (uintptr_t)arg0;
    g2 = (uintptr_t)arg1;

    g0 = g14;
    g14 = 0;
    g5 = 0;
    i960_mmio_write_u32(0x98000c, (u32)g14); /* videoctl */;
    do {
        g4 = g5 << 15;
        i960_mmio_write_u32(0x801008, (u32)g4); /* geo_write_start */;
        g5 = g5 + 1;
        i960_mmio_write_u32(0x8000f0, (u32)g14); /* geo_regs */;
    } while (3 >= (signed char)g5);
    i960_st_u32(I960_WORKRAM, 0x202260, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a280, 0, (u32)g14);
    /* bx (g0) */
    return;
}
