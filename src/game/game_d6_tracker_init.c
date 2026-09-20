/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_000323a0_6c.asm */
// @rom 0x323a0 +0x6c game_d6_tracker_init

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* call site: caller 0x31650 */

/* pointers: g5=u32 *, g6=u32 * */

void game_d6_tracker_init(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    g4 = 0;
    g6 = (uintptr_t)(model2_crx_ram + 0xd600);
    g5 = (uintptr_t)(model2_crx_ram + 0xd6f0);
    g7 = 31 + 28;
    do {
        *(u32 *)g5 = (u32)g4;
        g4 = g4 + 1;
        *(u32 *)g6 = (u32)g14;
        g6 = g6 + 0x4;
        g5 = g5 + 4;
    } while (g4 <= g7);
    i960_st_u32(I960_WORKRAM, 0x20d7e0, 0, (u32)g14);
    g7 = 31 + 29;
    i960_st_u32(I960_WORKRAM, 0x20d7e4, 0, (u32)g7);
    i960_st_u32(I960_WORKRAM, 0x20d7ec, 0, (u32)g14);
    g0 = 3;
    i960_st_u32(I960_WORKRAM, 0x20d7e8, 0, (u32)g14);
    g1 = 31 + 9;
    i960_call_rom(0x316c8);
    g0 = 31 + 29;
    i960_call_rom(0x31768);
}
