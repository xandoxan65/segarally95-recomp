/* Inner-2 frame wait tick @ 0xF9D0 (animate path @ 0x107E0). */
// @rom 0xf9d0 +0x30 comm_attract_countdown_tick

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void comm_attract_countdown_tick(u32 arg0, u32 arg1, u32 arg2)
{
    u32 board_type;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    board_type = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    if (board_type == 3u)
        return;

    if (i960_ld_u16(I960_ABS, 0x01d00020u, 0) == 0)
        return;

    if ((i960_ld_u8(I960_WORKRAM, 0x202024, 0) & 8u) != 0)
        return;

    comm_scene_catalog_init(2, 0, 0);
}
