/* Comm-attract inner mode 8 @ 0xF7D0 — real attract loop close.
 * source: decomp/disasm/maincpu/maincpu_00f7d0_1a0.asm/
 * // @rom 0xf7d0 +0x30 (trampoline) / slice continues into inner_0 */
// @rom 0xf7d0 +0x30 comm_attract_inner_8

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include <stdio.h>

/*
 * Disasm:
 *   lda 0x5ae800,g14   ; LEA — g0 ← 0x5ae800 (ROM mirror @ 0xF800 = ret)
 *   mov 0,g14
 *   st  g14,0x20209c   ; inner mode ← 0
 *   0x20a780++
 *   bx  (g0)           ; execute ret stub; return to dispatcher
 *
 * Closing the carousel: next board_dispatch indexes inner_0 again.
 */
void comm_attract_inner_8(u32 arg0, u32 arg1, u32 arg2)
{
    u32 ctr;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    ctr = i960_ld_u32(I960_WORKRAM, 0x20a780, 0);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a780, 0, ctr + 1u);
    lift_log( "lift: inner8 loop-close → inner 0 (cycle %u)\n",
            (unsigned)(ctr + 1u));
}
