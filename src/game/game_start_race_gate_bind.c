/* Race start-gate handler install @ 0x22768 — pick staged callx target
 * into 0x20afa0 from practice (0x5c0d60) vs championship (0x5c0fa0),
 * clear 0x20afa4, return via saved link in g0 (bx (g0) leaf).
 *
 * source: disasm/maincpu/maincpu_022768_80.asm */
// @rom 0x22768 +0x44 game_start_race_gate_bind

#include "i960_lift.h"
#include "i960_mem.h"

void game_start_race_gate_bind(u32 arg0, u32 arg1, u32 arg2)
{
    u32 link = (u32)g14;
    u32 choice;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x2276C: mov 0,g14 before stores that use g14 as zero. */
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x20afa4, 0, 0u);
    choice = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
    if (choice == 0u)
        i960_st_u32(I960_WORKRAM, 0x20afa0, 0, 0x005c0d60u);
    else
        i960_st_u32(I960_WORKRAM, 0x20afa0, 0, 0x005c0fa0u);
    /* bx (g0) with g0 = saved link — host call returns normally. */
    g0 = link;
    g14 = 0;
}
