/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00032fa0_30.asm */
// @rom 0x32fa0 +0x30 game_staging_init

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* call site: caller 0x3164 */
/* call site: caller 0x3164 */
/* call site: caller 0x3164 */

/* pointers: g0=void * */

void game_staging_init(u32 arg0, u32 arg1, u32 arg2)
{
    void * arg0_p = (void *)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    i960_st_u16(I960_WORKRAM, 0x213860, 0, (u16)g14);
    i960_st_u16(I960_WORKRAM, 0x213862, 0, (u16)g14);
    i960_st_u32(I960_WORKRAM, 0x213840, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x213850, 0, (u32)g14);
    g0 = (uintptr_t)i960_vaddr_ptr(0x12d687);
    i960_call_rom(0x33e18);
    return;
}
