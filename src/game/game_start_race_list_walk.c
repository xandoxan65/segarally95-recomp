/* Race pool-B list walk @ 0x2AB80 — callx each node handler.
 *
 * Starts at *0x20c97c (list-A/B shared link slot after alloc_link). For each
 * node until sentinel 0x20ca10: if +4 == 0, call 0x2AAF0 unlink; else callx +4
 * with g0 = node.
 *
 * source: disasm/maincpu/maincpu_02ab80_80.asm */
// @rom 0x2ab80 +0x40 game_start_race_list_walk

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host_staging.h"

#include "lift_syms.h"

#include <stdio.h>

#define LIST_HEAD   0x20c97cu
#define LIST_SENT   0x20ca10u

void game_start_race_list_walk(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node;
    u32 handler;
    u32 guard = 0;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    node = i960_ld_u32(I960_ABS, LIST_HEAD, 0);

    while (node != LIST_SENT) {
        if (++guard > 128u)
            break;
        handler = i960_ld_u32(I960_ABS, node, 4);
        if (handler == 0u) {
            /*
             * Disasm @ 0x2AB94–0x2AB9C: r4 = +0xc first, then
             * g0 = 0x8(r4) — i.e. next node's +8 (prev link), then unlink.
             */
            node = i960_ld_u32(I960_ABS, node, 0xc);
            g0 = i960_ld_u32(I960_ABS, node, 0x8);
            game_start_race_list_unlink((u32)g0, 0, 0);
        } else {
            /* @0x2ABA4–0x2ABAC: g0 = node; callx (handler); next = +0xc. */
            g0 = node;
            if (!i960_host_staging_call_lifted(handler))
                i960_call_indirect(handler);
            node = i960_ld_u32(I960_ABS, node, 0xc);
        }
    }
}
