/* Race-init CGM seed @ 0x1DD30 — called from cam_init when timer < 0.
 *
 * Always runs catalog_draw_setup on CGM @ 0x02888754 (layer g4=4), which
 * hits cgm_leading_colorbase → palram_colorbase_upload. When 0x2139d4 == 0
 * continues with Sys24 digit HUD (practice/champ tables + time_split).
 *
 * source: /tmp/dasm_1dd30/maincpu_01dd30_200.asm */
// @rom 0x1dd30 +0x1f0 game_start_race_cgm_seed

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

extern void tile_attract_time_split(u32 value, u16 out[4]);
extern u32 draw_scene_dispatch(u32 arg0, u32 arg1, u32 arg2);

void game_start_race_cgm_seed(u32 arg0, u32 arg1, u32 arg2)
{
    static int logged;
    u32 batch;
    u32 table;
    u32 course;
    u32 pass;
    u32 row_x;
    u32 cursor;
    u16 split[4];
    uintptr_t fp_save = fp;
    u8 frame[0x50];

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_cgm_seed (catalog @ 0x2888754)\n");
        fflush(stderr);
        logged = 1;
    }

    /* @0x1DD34–0x1DD4C: catalog_draw_setup(0, 0, 0x2888754), g3=0, g4=4. */
    g0 = 0;
    g1 = 0;
    g2 = 0x02888754u;
    g3 = 0;
    g4 = 4;
    batch = catalog_draw_setup(0, 0, 0x02888754u);
    i960_st_u32(I960_WORKRAM, 0x20aeb0, 0, batch);

    /* @0x1DD60: cmpibe 0,0x2139d4 → continue HUD; else ret. */
    if (i960_ld_u32(I960_WORKRAM, 0x2139d4, 0) != 0u)
        return;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    /*
     * Practice (0x202230==1): table = 0x1d00484 + course*0x194.
     * Else champ/network tables; desert course==3 early-outs.
     */
    if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) == 1u) {
        course = i960_host_race_course_index();
        table = 0x01d00484u + course * 0x194u;
    } else {
        if (i960_ld_u32(I960_WORKRAM, 0x2020b4, 0) == 0u)
            table = 0x01d002f0u;
        else
            table = 0x01d00ad4u;
        course = i960_host_race_course_index();
        if (course == 3u) {
            fp = fp_save;
            return;
        }
    }

    batch = i960_ld_u32(I960_WORKRAM, 0x20aeb0, 0);
    row_x = 12u;
    cursor = table;

    /* Two passes (r6=0,1): label row then digit rows from time_split fields. */
    for (pass = 0; pass < 2u; pass++) {
        u32 time_val;
        u32 d10;
        u32 d1;

        g0 = 48u; /* addo 31,17 */
        g1 = 11u;
        g2 = batch;
        g3 = 17u;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        time_val = i960_ld_u32(I960_ABS, cursor, 0xd4);
        tile_attract_time_split(time_val, split);
        memcpy(frame + 0x40, split, sizeof(split));

        g0 = 47u; /* addo 31,16 */
        g1 = row_x;
        g2 = batch;
        g3 = 10u + pass;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        /* Tens digit of field @ +0x42 when non-zero. */
        d10 = (u32)split[1] / 10u;
        if (d10 != 0u) {
            g0 = 48u;
            g1 = row_x;
            g2 = batch;
            g3 = d10;
            g4 = 0;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        }
        d1 = (u32)split[1] % 10u;
        g0 = 49u;
        g1 = row_x;
        g2 = batch;
        g3 = d1;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        g0 = 50u;
        g1 = row_x;
        g2 = batch;
        g3 = 13u;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        d10 = (u32)split[2] / 10u;
        g0 = 51u;
        g1 = row_x;
        g2 = batch;
        g3 = d10;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        d1 = (u32)split[2] % 10u;
        g0 = 52u;
        g1 = row_x;
        g2 = batch;
        g3 = d1;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        g0 = 53u;
        g1 = row_x;
        g2 = batch;
        g3 = 14u;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        d10 = (u32)split[3] / 10u;
        g0 = 54u;
        g1 = row_x;
        g2 = batch;
        g3 = d10;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        d1 = (u32)split[3] % 10u;
        g0 = 55u;
        g1 = row_x;
        g2 = batch;
        g3 = d1;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        /*
         * @0x1DF0C: call 0x1dc00 — string digit walk for label glyphs.
         * g2 = lda 0xd8(table) — EA of the C string, not a load.
         */
        g0 = 57u;
        g1 = row_x;
        g2 = cursor + 0xd8u;
        game_start_race_hud_string_draw((u32)g0, (u32)g1, (u32)g2);

        row_x += 1u;
        cursor += 12u;
    }

    fp = fp_save;
}
