/* Race camera one-shot init @ 0x206E0 — after handler install in cam_boot.
 *
 * Chains unlifted helpers then game_start_race_cam_matrix(g0=0) for the
 * short (CGM/draw_scene) path. Per-frame view is the installed handler
 * (desert @ 0x1FA20), not this init.
 *
 * source: disasm/maincpu/maincpu_0206e0_200.asm */
// @rom 0x206e0 +0x54 game_start_race_cam_init

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_cam_init(u32 arg0, u32 arg1, u32 arg2)
{
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        fprintf(stderr, "lift: race_cam_init\n");
        fflush(stderr);
        logged = 1;
    }

    g0 = 0;
    game_start_race_hud_layers(0, 0, 0);
    game_start_race_hud_row_seed(0, 0, 0);
    g0 = 0;
    /* Matrix short path (g0==0): catalog_draw_setup + draw_scene_dispatch. */
    game_start_race_cam_matrix(0, 0, 0);
    g0 = (u32)-1;
    game_start_race_hud_lap_time((u32)g0, 0, 0);
    g0 = 0;
    game_start_race_hud_position(0, 0, 0);
    g0 = 0;
    game_start_race_hud_speed(0, 0, 0);

    if (i960_ld_u32(I960_WORKRAM, 0x2139d4, 0) == 0u) {
        g0 = 0;
        game_start_race_hud_progress(0, 0, 0);
    }
    /* cmpible 0,timer → call only when timer < 0 (normal seeded −1). */
    if ((i32)i960_ld_u32(I960_WORKRAM, 0x20a560, 0) < 0)
        game_start_race_cgm_seed(0, 0, 0);
}
