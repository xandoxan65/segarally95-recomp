/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00003650_54.asm */
// @rom 0x3650 +0x54 game_mode_apply

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"
#include "model2_rom.h"
#include "model2_nvram.h"

#include <stdio.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

void game_mode_apply(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode;
    u8 flags;
    int host_test;

    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    geo_vsync_wait(0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x202000, 0, 1);
    i960_st_u32(I960_WORKRAM, 0x2020a0, 0, arg0);
    game_io_poll((u32)g0, (u32)g1, (u32)g2);

    mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
    if (mode == 4)
        return;

    /*
     * ROM @ 0x3680: TEST (0x202064 bit2) or START edge (0x202074 bit0) → mode 4
     * (geo_scene_mode_dispatch = operator menu: MEMORY/INPUT/SOUND/…).
     * Gate on TEST only — START bit0 races attract credit-start (F6D0 → mode 3).
     *
     * ROM stores g14 into 0x20209c (submode); callers keep g14==0. Host g14 is
     * often polluted, so force submode 0. Also clear the same-press TEST/START
     * edges: geo_scene_string_prep treats those as “confirm”, which would skip
     * the menu and run MEMORY TEST (tile bank clear) immediately.
     *
     * Host F2: model2_io_request_test_menu() one-shot if the IN0 edge is missed.
     */
    flags = (u8)i960_ld_u8(I960_WORKRAM, 0x202064, 0);
    host_test = model2_io_consume_test_menu_request();
    if (((flags >> 2) & 1) || host_test) {
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x202098, 0, 4);
        i960_st_u8(I960_WORKRAM, 0x202064, 0,
                   (u8)(i960_ld_u8(I960_WORKRAM, 0x202064, 0) & (u8)~0x04u));
        i960_st_u32(I960_WORKRAM, 0x202074, 0,
                    i960_ld_u32(I960_WORKRAM, 0x202074, 0) & ~1u);
        /* Fresh tile banks — attract may have left transparent/geo-punch maps. */
        tile_map_banks_clear(0, 0, 0);
        model2_nvram_mark_dirty();
        fprintf(stderr, "lift: TEST → mode 4 (operator menu)\n");
    }
}
