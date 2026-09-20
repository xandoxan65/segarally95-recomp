/* D6 tracker count apply @ 0x31768 (call from game_d6_tracker_init). */
// @rom 0x31768 +0x44 game_d6_count_apply

#include "i960_lift.h"
#include "i960_mem.h"

void game_d6_count_apply(u32 arg0, u32 arg1, u32 arg2)
{
    u32 stored;
    u32 delta;

    (void)arg1;
    (void)arg2;

    stored = i960_ld_u32(I960_WORKRAM, 0x20d800, 0);
    if (arg0 > stored) {
        delta = arg0 - stored;
        i960_st_u32(I960_WORKRAM, 0x20d7f0, 0, arg0);
        i960_st_u32(I960_WORKRAM, 0x20d7f4, 0, delta);
        return;
    }
    if (arg0 != 0) {
        i960_st_u32(I960_WORKRAM, 0x20d7f0, 0, stored);
        i960_st_u32(I960_WORKRAM, 0x20d7f4, 0, 0);
        return;
    }
    i960_st_u32(I960_WORKRAM, 0x20d7f0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20d7f4, 0, 0);
}
