/* Attract logo CGM submit @ 0x32790 — sega_mini @ 0x72130 + moji4 @ 0x72224.
 * source: disasm/maincpu/maincpu_032790_200.asm */
// @rom 0x32790 +0x160 attract_logo_cgm_submit

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "lift_syms.h"
#include "model2_rom.h"

void attract_logo_cgm_submit(u32 arg0, u32 arg1, u32 arg2)
{
    u32 course;
    u32 idx;
    double scale_a;
    double scale_b;
    double scale_c;
    double prod_a;
    double prod_b;
    i32 out_a;
    i32 out_b;
    u32 colorbase;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    course = i960_host_race_course_index();
    idx = course + (course << 1); /* course * 3 */

    scale_a = i960_u32_to_f64(model2_workram_mirror_u32(0x5d1694u + (idx << 2)));
    prod_a = scale_a * 400.0;
    scale_b = i960_u32_to_f64(model2_workram_mirror_u32(0x5d1698u + (idx << 2)));
    prod_b = scale_b * 400.0;
    scale_c = i960_u32_to_f64(model2_workram_mirror_u32(0x5d1690u + (idx << 2)));
    out_a = (i32)(prod_a / scale_c); /* cvtzri */
    out_b = (i32)(prod_b / scale_c);

    i960_st_u32(I960_WORKRAM, 0x20d818, 0, i960_f64_to_u32(scale_c));
    i960_st_u32(I960_WORKRAM, 0x20d810, 0, (u32)out_a);
    i960_st_u32(I960_WORKRAM, 0x20d814, 0, (u32)out_b);

    /* bal 0x29b48 — return colorbase counter @ 0x20c950 */
    colorbase = i960_ld_u32(I960_WORKRAM, 0x20c950, 0);
    g0 = colorbase;
    i960_st_u32(I960_WORKRAM, 0x20d824, 0, colorbase << 7);

    g0 = 0;
    g1 = 0;
    g2 = 0x00072130u;
    g3 = 0;
    g4 = 4;
    catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

    g0 = 0;
    g1 = 0;
    g2 = 0x00072130u;
    g3 = 0;
    g4 = 4;
    catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

    g3 = i960_host_race_course_index() & 3u;
    g0 = 31u + 13u; /* 44 */
    g1 = 30;
    g2 = 0x00072224u;
    g4 = 4;
    g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
    i960_st_u32(I960_WORKRAM, 0x20d81c, 0, (u32)g0);

    attract_logo_cgm_scale_clear(0, 0, 0);
    attract_logo_tile_stripe(0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20d820, 0, (u32)g14);
}
