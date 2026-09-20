/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00001780_80.asm */
// @rom 0x1780 +0x80 irq_enable_dispatch

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

/* pointers: g0=void *, g14=void *, g5=void * */

void irq_enable_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    void * arg0_p = (void *)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    g14 = (uintptr_t)(model2_workram + 0xa0800);
    g0 = g14;
    g14 = 0;
    i960_mmio_write_u32(0xe80000, (u32)g14); /* irq_request */;
    g5 = 21;
    i960_st_u32(I960_WORKRAM, 0x20200c, 0, (u32)g5);
    g4 = i960_ld_u32(I960_WORKRAM, 0x20200c, 0);
    i960_mmio_write_u32(0xe80004, (u32)g4); /* irq_enable */;
    g5 = 0x61a80;
    i960_mmio_write_u32(0xf00000, (u32)g5); /* timers */;
    i960_mmio_write_u32(0xf00004, (u32)g5); /* timers */;
    g5 = 0 - 1;
    i960_mmio_write_u32(0xf00008, (u32)g5); /* timers */;
    i960_mmio_write_u32(0xf0000c, (u32)g5); /* timers */;
    i960_st_u32(I960_WORKRAM, 0x202000, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x202008, 0, (u32)g14);
    g5 = 1;
    i960_st_u32(I960_WORKRAM, 0x202004, 0, (u32)g5);
    /* bx (g0) */
    return;
}
