/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00031650_8.asm */
// @rom 0x31650 +0x8 game_subsys_init_stub

#include "i960_lift.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* call site: caller 0x3168 */
/* call site: caller 0x3168 */
/* call site: caller 0x3168 */

void game_subsys_init_stub(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    i960_call_rom(0x323a0);
    return;
}
