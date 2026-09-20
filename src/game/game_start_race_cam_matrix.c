/* Race cam matrix / CGM helper @ 0x20390 — called from cam_init with g0=0.
 *
 * g0==0 (cam_init short path): catalog_draw_setup START-adjacent CGM +
 * draw_scene_dispatch — Sys24 overlay, not the 3D course path.
 * g0!=0: long path @ 0x203d0 (game_start_race_cam_matrix_long).
 *
 * 3D course/cars use the same attract interface: race_frame →
 * geo_attract_course_view_bind → catalog_span → copro_submit → object_pen.
 *
 * source: /tmp/dasm_20390b/maincpu_020390_280.asm */
// @rom 0x20390 +0x40 game_start_race_cam_matrix

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_cam_matrix(u32 arg0, u32 arg1, u32 arg2)
{
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_cam_matrix g0=%u\n", (unsigned)arg0);
        fflush(stderr);
        logged = 1;
    }

    /* @0x20390: cmpibne 0,g0 → long path @ 0x203d0. */
    if (arg0 != 0u) {
        game_start_race_cam_matrix_long(arg0, arg1, arg2);
        return;
    }

    /* Short path: CGM @ 0x21f5c44 then draw slot 0x20ac78. */
    g0 = 2;
    g1 = 31u + 3u;
    g2 = 0x021f5c44u;
    g3 = 0;
    g4 = 0;
    catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

    g2 = i960_ld_u32(I960_WORKRAM, 0x20ac78, 0);
    g0 = 10;
    g1 = 31u + 13u;
    g3 = 0;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
}
