/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_029958_c.asm */
// @rom 0x29958 +0xc cgm_thunk_walk

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host_invoke.h"
#include "i960_host_staging.h"

/* convention: kind=leaf_bx  args g1,g2,g0  link g14→g0 bx */
/* abi: u32 arg0=g1, u32 arg1=g2 → u32 g0 */

u32 cgm_thunk_walk(u32 arg0, u32 arg1)
{
    uintptr_t link;

    g1 = (uintptr_t)arg0;
    g2 = (uintptr_t)arg1;
    link = g14;
    if (link == 0)
        link = i960_ld_u32(I960_WORKRAM, 0x5c8964, 0);
    g0 = link;
    g14 = 0;
    if (g0 != 0) {
        u32 handler_rom = i960_host_resolve_call_target((u32)g0);

        if (!i960_host_invoke_lifted(handler_rom))
            i960_call_indirect(g0);
    }
    return (u32)g0;
}
