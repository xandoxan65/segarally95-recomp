/* Comm board slot scan gate @ 0xFE40 (inner_2 precondition). */
// @rom 0xfe40 +0x220 comm_attract_slot_scan

#include "i960_lift.h"
#include "i960_mem.h"

u32 comm_attract_slot_scan(u32 arg0, u32 arg1, u32 arg2)
{
    u32 board_type;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    board_type = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    if (board_type != 0u && board_type != 3u)
        return 0;

    /* Single-cabinet / absent-board path @ 0x10058. */
    i960_st_u32(I960_WORKRAM, 0x20a7c4, 0, 1u);
    g0 = 1;
    return 1;
}
