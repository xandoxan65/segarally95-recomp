/* Attract-mode tile text dispatch @ 0xC640 (jump table @ 0x5AB68C). */
// @rom 0xc640 +0xe0 tile_attract_text_dispatch

#include "tile_attract_shared.h"
#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void tile_attract_text_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @ 0xC640: bal 0xC4D8 */
    tile_attract_flags_prologue();
    tile_attract_index_walk();

    g0 = 15;
    g1 = 19;
    tile_cursor_seed((u32)g0, (u32)g1);
    scene_view_scale(0, 0, 0);

    tile_attract_text_cases();
}
