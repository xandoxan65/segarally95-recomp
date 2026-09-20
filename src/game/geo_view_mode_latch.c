/* Mode latch @ 0x34CE0 — updates 0x2142C8 / counters from signed float g0.
 * Callers: geo_view_scene_frame @ 0x36DDC (also 0x34348).
 * source: disasm/maincpu/maincpu_034ce0_15c.asm */
// @rom 0x34ce0 +0x15c geo_view_mode_latch

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

void geo_view_mode_latch(u32 arg0, u32 arg1, u32 arg2)
{
    float x;
    u32 count;
    u32 flag;

    (void)arg1;
    (void)arg2;

    x = (float)i960_u32_to_f64(arg0);

    /* bl if x < 0; also if mode==2 → clear counters */
    if (x < 0.f || i960_ld_u32(I960_WORKRAM, 0x214120, 0) == 2u) {
        i960_st_u32(I960_WORKRAM, 0x213870, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x21396c, 0, 0);
        return;
    }

    if (x == 0.f) {
        count = i960_ld_u32(I960_WORKRAM, 0x213870, 0) + 1u;
        flag = i960_ld_u32(I960_WORKRAM, 0x21396c, 0);
        i960_st_u32(I960_WORKRAM, 0x213870, 0, count);
        if (flag != 0u) {
            i960_st_u32(I960_WORKRAM, 0x21396c, 0, 0);
            i960_st_u32(I960_WORKRAM, 0x2142c8, 0, 3u);
        }
        return;
    }

    /* x != 0: tile_cursor_seed(2, 46); then branch on counter */
    tile_cursor_seed(2u, 31u + 15u);
    count = i960_ld_u32(I960_WORKRAM, 0x213870, 0);
    if ((i32)count >= 30) {
        if (i960_ld_u32(I960_WORKRAM, 0x21396c, 0) == 0u
            && i960_ld_u32(I960_WORKRAM, 0x2142c8, 0) == 0u
            && i960_ld_u32(I960_WORKRAM, 0x2142d4, 0) == 0u) {
            i960_st_u32(I960_WORKRAM, 0x2142c8, 0, 2u);
            i960_st_u32(I960_WORKRAM, 0x2142d0, 0, 5u);
            i960_st_u32(I960_WORKRAM, 0x21396c, 0, 1u);
        }
        i960_st_u32(I960_WORKRAM, 0x213870, 0, 0);
        return;
    }

    if (i960_ld_u32(I960_WORKRAM, 0x2142c8, 0) == 0u) {
        i960_st_u32(I960_WORKRAM, 0x2142c8, 0, 1u);
        i960_st_u32(I960_WORKRAM, 0x2142cc, 0, i960_ld_u32(I960_WORKRAM, 0x213964, 0));
        i960_st_u32(I960_WORKRAM, 0x2142d0, 0, i960_ld_u32(I960_WORKRAM, 0x213968, 0));
    }
    /* 0x3f9eb851eb851eb8 ≈ 0.03 */
    if (x > 0.03f) {
        tile_texture_descriptor_apply(0x47u, 0x7fu, 0);
        tile_texture_descriptor_apply(14u, 0x7fu, 0);
    } else {
        tile_texture_descriptor_apply(0x49u, 0x7fu, 0);
    }
    i960_st_u32(I960_WORKRAM, 0x213870, 0, 0);
}
