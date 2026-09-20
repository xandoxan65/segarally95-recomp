/* Practice path after mode-select @ 0x1C140 — table slot 6.
 * Seeds choice=1, then advances 20209c → slot 7.
 * source: disasm/maincpu bytes @ 0x1c140..0x1c198 */
// @rom 0x1c140 +0x60 game_start_practice_enter

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_practice_enter(u32 arg0, u32 arg1, u32 arg2)
{
    u32 sub;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_st_u32(I960_WORKRAM, 0x2020b8, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x202230, 0, 1u);
    i960_st_u32(I960_WORKRAM, 0x2020ac, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20aab4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x213840, 0, 6u);

    /* Same clear bals as champ_enter (@ 0x1C7E8 / 0x1E898). */
    i960_st_u32(I960_WORKRAM, 0x20aea8, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20af90, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20af98, 0, 0);

    i960_st_u32(I960_WORKRAM, 0x2020c8, 0, 15u);
    sub = i960_ld_u32(I960_WORKRAM, 0x20209c, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, sub);
    fprintf(stderr, "lift: practice_enter → submode %u\n", (unsigned)sub);
}
