/* View scale setup thunk @ 0x26FD8 (bal target from draw_scene_dispatch / game_inner_dispatch). */
// @rom 0x26fd8 +0x18 scene_view_scale

#include "i960_lift.h"
#include "i960_mem.h"

void scene_view_scale(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;

    i960_st_u32(I960_WORKRAM, 0x20b1b0, 0, (u32)(arg0 << 7));
}
