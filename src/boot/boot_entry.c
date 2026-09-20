/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00000310_20.asm */
// @rom 0x310 +0x20 boot_entry

#include "i960_host.h"
#include "i960_lift.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* call site: caller 0x330 */
/* call site: caller 0x340 */
/* call site: caller 0x350 */

/* pointers: fp=u32 *, g4=u32, g5=void * */

extern void maincpu_reset_entry(u32 arg0, u32 arg1, u32 arg2);

void boot_entry(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    sp = sp + 16;
    g5 = 0x186a0;
    *(u32 *)(fp + 0x40) = (u32)g5;
    if (!i960_host_skip_hw_timer) {
    do {
        g4 = *(u32 *)(fp + 0x40);
    } while ((unsigned char)g4 != 0);
    }
    maincpu_reset_entry(g0, g1, g2);
    return;
}
