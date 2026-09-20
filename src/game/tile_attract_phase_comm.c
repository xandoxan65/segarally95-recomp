/* Attract phase 0 @ 0xC5D0 — comm board check script + C640 tail from 0xC644. */
// @rom 0xc5d0 +0x80 tile_attract_phase_comm

#include "tile_attract_shared.h"
#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void tile_attract_phase_comm(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_st_u32(I960_WORKRAM, 0x213840, 0, 6);
    i960_st_u32(I960_WORKRAM, 0x20227c, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x213850, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a280, 0, (u32)g14);

    boot_tile_map_init(0, 0, 0);
    /* @0xC5F8: setbit 15,0,g0 → 0x8000 before tile_attract_palram_gate. */
    g0 = 0x8000u;
    tile_attract_palram_gate((u32)g0);

    g0 = 15;
    g1 = 10;
    tile_cursor_seed((u32)g0, (u32)g1);
    boot_tile_script_run(0x005ab530u);

    i960_st_u8(I960_ROM, 0x1a14000, 0, (u8)g14);
    i960_st_u32(I960_WORKRAM, 0x20a558, 0, 1);
    i960_st_u8(I960_ROM, 0x1a14002, 0, (u8)g14);
    i960_st_u32(I960_WORKRAM, 0x20a564, 0, 4);

    /* @ 0xC640: bal 0xC4D8 then fall into 0xC644 walk + case dispatch. */
    tile_attract_flags_prologue();
    tile_attract_index_walk();

    g0 = 15;
    g1 = 19;
    tile_cursor_seed((u32)g0, (u32)g1);
    scene_view_scale(0, 0, 0);

    tile_attract_text_cases();
}
