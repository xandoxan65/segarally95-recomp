/* Attract HUD / logo setup @ 0x1B940 (callx target 0x5BA940).
 * When 0x20AABC==0 runs course/CGM init then attract_logo_dispatch(0).
 * source: disasm/maincpu/maincpu_01b940_280.asm */
// @rom 0x1b940 +0x260 attract_hud_setup

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "lift_syms.h"
#include "model2_geo.h"
#include "model2_rom.h"

void attract_hud_setup(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;
    u32 course;
    u32 countdown;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x1B940: ld 0x20aabc; cmpibe 0 → logo_path @ 0x1BA70 */
    gate = i960_ld_u32(I960_WORKRAM, 0x20aabc, 0);
    if (gate == 0)
        goto logo_path;

    /*
     * Championship scene slot [2] lands here after course_select (no car
     * select on table 0x5BA820). Drop the course-select spinner mesh so
     * START/CGM is not under two pens stacked at the origin.
     */
    model2_geo_clear();

    /* @0x1B94C–0x1B978: clear banks, palram gate, scene_list_seed. */
    i960_st_u8(I960_ABS, 0x0181c000u, 0, (u8)g14);
    tile_map_banks_clear(0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x213840, 0, 6);
    i960_st_u32(I960_WORKRAM, 0x213850, 0, (u32)g14);
    g0 = 0xe166;
    tile_attract_palram_gate((u32)g0);
    scene_list_seed(0, 0, 0);

    /*
     * @0x1B984: cmpibl 4,g4,0x1ba00 — same COBR sense as catalog @ 0x29FE4
     * (branch when src1 < src2). Splash path when 4 < course; else course
     * banner + title CGM.
     */
    course = i960_host_race_course_index();
    if (4 < (i32)course)
        goto splash_catalog;

    {
        u32 slot;

        /* @0x1B988–0x1B9AC: course CGM from 0x5BA910[course], g4=4 (setup only). */
        g0 = 0;
        g1 = 0;
        g3 = 0;
        g2 = model2_workram_mirror_u32(0x5ba910u + (course << 2));
        g4 = 4;
        slot = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

        /*
         * @0x1B9B0–0x1B9C8: title line CGM @ 0x20F8B40 at (0,18), g4=0.
         * catalog_draw_setup auto-draws (draw_mode<=3) on L2 + bit15 —
         * priority text above the L1 course banner.
         */
        g0 = 0;
        g1 = 18;
        g2 = 0x020f8b40u;
        g3 = 0;
        g4 = 0;
        catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

        /* @0x1B9CC–0x1B9E0: course banner row0 at y=0, L1 (g4=1). */
        g0 = 0;
        g1 = 0;
        g2 = slot;
        g3 = 0;
        g4 = 1;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        /* @0x1B9E4–0x1B9F8: course banner row1 at y=18, L1, g3=1. */
        g0 = 0;
        g1 = 18;
        g2 = slot;
        g3 = 1;
        g4 = 1;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }
    goto after_catalogs;

splash_catalog:
    /* @0x1BA00–0x1BA18: fallback splash CGM. */
    g0 = 0;
    g1 = 0;
    g2 = 0x02879db0u;
    g3 = 0;
    g4 = 0;
    catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

after_catalogs:
    /* @0x1BA1C–0x1BA6C: texture bank + countdown seed, then ret. */
    course = i960_host_race_course_index();
    g0 = model2_workram_mirror_u32(0x5dccf0u + (course << 2));
    g1 = 1;
    i960_st_u32(I960_WORKRAM, 0x214350, 0, (u32)g0);
    texture_bank_select((u32)g0, (u32)g1, 0);
    /*
     * @0x1BA40–0x1BA5C: st g14 → step; mov 4,r8; setbit8 → r9; stl r8 →
     * 0x213840 (mode=4, scale=0x100). Target untouched. step=0 is a hold:
     * fade_step clears mode after mode4 applies the wash so logo_path can
     * promote the fade-in stq.
     */
    i960_st_u32(I960_WORKRAM, 0x21384c, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20aab8, 0, 15u << 3);
    i960_st_u32(I960_WORKRAM, 0x213840, 0, 4u);
    i960_st_u32(I960_WORKRAM, 0x213844, 0, 1u << 8);
    i960_st_u8(I960_WORKRAM, 0x202049, 0, (u8)g14);
    return;

logo_path:
    /* Drop course/car select mesh before race init advances to race_frame. */
    model2_geo_clear();
    /*
     * @0x1BA70–0x1BA94 (desert/course START hold):
     *   ld g5=0x20aab8; st g5-1; cmpibge 0,g5 → race init when countdown <= 0
     *   else ld 0x20227c; cmpibe 0 → race init when tex idle
     *   else ret — hold splash while countdown > 0 and upload busy.
     * Splash path seeds 0x20aab8 = shlo 3,15 (=120) @ 0x1BA4C–0x1BA54.
     * 0x20227c is texture_bank_select state; pump @ 0x39AC clears it when done
     * (multi-frame on HW via 0x202004 yield @ 0x3A50).
     */
    countdown = i960_ld_u32(I960_WORKRAM, 0x20aab8, 0);
    i960_st_u32(I960_WORKRAM, 0x20aab8, 0, countdown - 1u);
    if ((i32)countdown > 0) {
        if (i960_ld_u32(I960_WORKRAM, 0x20227c, 0) != 0)
            return;
    }

    /* @0x1BA98–0x1BB9C: full attract init then bottom logo CGMs. */
    i960_st_u32(I960_WORKRAM, 0x213840, 0, 4);
    i960_st_u32(I960_WORKRAM, 0x213844, 0, 1u << 8);
    i960_st_u32(I960_WORKRAM, 0x213848, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x21384c, 0, (u32)(0u - 8u));
    i960_call_rom(0x31650);
    g0 = 3;
    g1 = 15;
    i960_call_rom(0x316c8); /* bal */
    g0 = 10;
    i960_call_rom(0x31768); /* bal */
    i960_st_u32(I960_WORKRAM, 0x2140c4, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x2140c8, 0, (u32)g14);
    game_start_race_cam_boot(0, 0, 0);
    game_start_race_sub_reset(0, 0, 0);
    /* @0x1BAE0: course list seed — same init attract uses before catalog_span. */
    comm_attract_course_init(0, 0, 0);
    /* @0x3DE18: clear record spans (same as attract catalog_seed). */
    comm_attract_catalog_seed(0, 0, 0);
    cgm_scratch_pool_reset(0, 0, 0);
    cgm_scratch_pool_alloc_link(0, 0, 0);

    {
        u8 b91 = i960_ld_u8(I960_WORKRAM, 0x213b91, 0);
        u8 b90 = i960_ld_u8(I960_WORKRAM, 0x213b90, 0);

        i960_st_u32(I960_WORKRAM, 0x2139c8, 0, 1);
        i960_st_u32(I960_WORKRAM, 0x2140cc, 0, (u32)g0);
        i960_st_u8(I960_WORKRAM, 0x213b91, 0, (u8)(b91 & 0xc3u));
        i960_st_u8(I960_WORKRAM, 0x213b90, 0, (u8)(b90 & 0x7fu));
        /* stl r4:r5 with r4=cam, r5=0 → 0x2140d0 / 0x2140d4. */
        i960_st_u32(I960_WORKRAM, 0x2140d0, 0, (u32)g0);
        i960_st_u32(I960_WORKRAM, 0x2140d4, 0, 0);
    }

    game_start_race_cam_object_init((u32)g0, 0, 0);
    game_start_race_obj_seed(0, 0, 0);
    /* @0x1BB50: desert installs list node + centroid (same CGM pool as attract). */
    game_start_race_logo_extra(0, 0, 0);
    g0 = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    game_start_race_obj_bind((u32)g0, 0, 0);

    i960_st_u32(I960_WORKRAM, 0x20aab0, 0, 15u << 4);
    i960_st_u32(I960_WORKRAM, 0x20aac0, 0, (u32)g14);
    i960_st_u8(I960_ABS, 0x0181c000u, 0, 0xff);
    g0 = 0;
    attract_logo_dispatch(0, 0, 0);
    {
        u32 frame = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0);

        i960_st_u32(I960_WORKRAM, 0x2020ac, 0, frame + 1u);
    }
}
