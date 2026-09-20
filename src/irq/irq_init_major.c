/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00001490_60.asm */
// @rom 0x1490 +0x60 irq_init_major

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* call site: caller 0x15e0 */
/* call site: caller 0x1834 */

/* pointers: g5=void * */

void irq_init_major(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    g5 = 0x5e9ac;
    i960_mmio_write_u32(0xf00008, (u32)g5); /* timers */;
    i960_st_u32(I960_WORKRAM, 0x202004, 0, (u32)g14);
    g4 = i960_ld_u32(I960_WORKRAM, 0x202000, 0);
    g4 = g4 - 1;
    i960_st_u32(I960_WORKRAM, 0x202000, 0, (u32)g4);
    g4 = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
    g4 = g4 + 1;
    i960_st_u32(I960_WORKRAM, 0x202008, 0, (u32)g4);
    game_io_coin_sense(0, 0, 0);
    game_coin_frame(0, 0, 0);
    i960_call_rom(0x9410);
    boot_tile_splash_frame(0, 0, 0);
    geo_palette_irq_frame(0, 0, 0);
    i960_call_rom(0x2df0);
    return;
    /* data .long 0x00000000 */
}
