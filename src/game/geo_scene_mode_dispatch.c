/* Mode-4 scene submode dispatch @ 0x5830 (staged workram 0x5A4830). */
// @rom 0x5830 +0x60 geo_scene_mode_dispatch

#include "i960_lift.h"
#include "i960_mem.h"
#include "placement_catalog_feed.h"

#include "lift_syms.h"

#include <stdlib.h>

void geo_scene_mode_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 submode;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    submode = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    if (submode == 0) {
        i960_st_u32(I960_WORKRAM, 0x20a2ec, 0, 10);
        submode++;
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, submode);
        geo_scene_palette_gate((u32)g0, (u32)g1, (u32)g2);
        return;
    }
    if (submode == 1) {
        geo_scene_string_prep((u32)g0, (u32)g1, (u32)g2);
        return;
    }
    if (submode == 2) {
        tile_map_banks_clear((u32)g0, (u32)g1, (u32)g2);
        {
            u32 slot = i960_ld_u32(I960_WORKRAM, 0x20a2ec, 0) & 15u;
            u32 handler = i960_ld_u32(I960_WORKRAM, 0x5a7120, slot << 2);
            g0 = 1;
            if (handler)
                i960_call_indirect(handler);
        }
        submode++;
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, submode);
        return;
    }
    if (submode == 3) {
        geo_draw_frame_entry((u32)g0, (u32)g1, (u32)g2);
        {
            u32 slot = i960_ld_u32(I960_WORKRAM, 0x20a2ec, 0) & 15u;
            u32 handler = i960_ld_u32(I960_WORKRAM, 0x5a7120, slot << 2);
            g0 = 0;
            if (handler)
                i960_call_indirect(handler);
            else {
                u32 cursor = i960_ld_u32(I960_WORKRAM, 0x20a2ec, 0);
                u32 batch = placement_catalog_feed_env_batch();
                g0 = cursor;
                g1 = 0;
                g2 = cursor;
                g0 = draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
                i960_st_u32(I960_WORKRAM, 0x20a2ec, 0, cursor + batch);
            }
        }
        if (g0 == 1) {
            tile_map_banks_clear((u32)g0, (u32)g1, (u32)g2);
            i960_st_u32(I960_WORKRAM, 0x20209c, 0, 1);
        }
        return;
    }
}
