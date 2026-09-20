/* BAL helper @ 0x2AB48 — return *0x20c97c in g0.
 *
 * Race sub_reset uses bal (not call); host invoke still lands here. Result is
 * stored to 0x20ab64 (draw/list head used later in the race frame path).
 *
 * source: disasm/maincpu/maincpu_02ab48_80.asm */
// @rom 0x2ab48 +0x14 game_start_race_list_head_fetch

#include "i960_lift.h"
#include "i960_mem.h"

u32 game_start_race_list_head_fetch(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x2AB50: ld 0x20c97c,g0; bx return. */
    g0 = i960_ld_u32(I960_WORKRAM, 0x20c97c, 0);
    return (u32)g0;
}
