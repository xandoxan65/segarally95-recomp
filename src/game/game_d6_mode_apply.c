/* D6 tracker mode apply @ 0x316C8 (bal from game_d6_tracker_init). */
// @rom 0x316c8 +0x94 game_d6_mode_apply

#include "i960_lift.h"
#include "i960_mem.h"

void game_d6_mode_apply(u32 arg0, u32 arg1, u32 arg2)
{
    u32 idx;
    u32 code;
    u32 limit;

    (void)arg2;

    idx = arg0 - 1u;
    i960_st_u32(I960_WORKRAM, 0x20d7f8, 0, arg0);

    if (idx < 5u) {
        /* Jump table @ 0x5D06EC: idx 0..4 → codes; idx≥5 → default 0. */
        static const u8 k_table_code[5] = { 15u, 3u, 12u, 10u, 5u };
        code = k_table_code[idx];
    } else {
        code = 0u;
    }
    i960_st_u8(I960_WORKRAM, 0x20d7fc, 0, (u8)code);

    limit = arg1;
    if ((i32)limit < 0)
        limit = (u32)(0u - limit);
    if (limit > 40u)
        limit = 40u;
    i960_st_u32(I960_WORKRAM, 0x20d800, 0, limit);
}
