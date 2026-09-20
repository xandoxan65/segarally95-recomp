/* Clear inner-dispatch workram flags @ 0xF818 (bal from game_inner_dispatch). */
// @rom 0xf818 +0x1c game_inner_flags_clear

#include "i960_lift.h"
#include "i960_mem.h"

void game_inner_flags_clear(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    g0 = g14;
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x20a7dc, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x214354, 0, (u32)g14);
    /* bx (g0) */
}
