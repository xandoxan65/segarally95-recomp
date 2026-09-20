/* Race camera boot @ 0x1F0D0 — START logo_path one-shot (attract_hud_setup).
 *
 * Picks the staged camera handler from course/mode, installs via
 * game_start_race_cam_bind, seeds a secondary slot via 0x21370, then runs
 * game_start_race_cam_init @ 0x206E0.
 *
 * Handler select (@0x1F0EC–0x1F128):
 *   cmpible 0,timer → chase when timer >= 0
 *   else (timer < 0) && (practice || desert) → 0x5BEA20 (intro spline)
 *   else → 0x5BEDC0 (chase)
 * Then always installs 0x5C0330 via call 0x21370 and calls cam_init.
 *
 * source: disasm/maincpu/maincpu_01f0d0_40.asm + maincpu_01f100_80.asm */
// @rom 0x1f0d0 +0x70 game_start_race_cam_boot

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_cam_boot(u32 arg0, u32 arg1, u32 arg2)
{
    u32 timer;
    u32 mode;
    u32 course;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        fprintf(stderr, "lift: race_cam_boot (START → install camera)\n");
        fflush(stderr);
        logged = 1;
    }

    /* @0x1F0D0–0x1F0E8: tile splash + clear 0x20ab58/5c. */
    tile_map_banks_clear(0, 0, 0);
    scene_list_seed(0, 0, 0);
    /*
     * Disasm `st g14` after bal — return addr ≈0 as float. Host call leaves
     * g14 dirty (e.g. 1.0); desert spline t must start at 0 or the intro pan
     * skips straight to the near-axle endpoint.
     */
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x20ab58, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20ab5c, 0, (u32)g14);
    game_start_race_cgm_batch_seed(0, 0, 0);

    /*
     * cmpible 0, timer → chase when timer >= 0. Phase_0 seeds 0x20a560 = −1,
     * so the normal START path takes the practice/desert select below.
     */
    timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    if ((i32)timer < 0) {
        mode = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
        course = i960_host_race_course_index();
        if (mode == 1u || course == 0u)
            g0 = 0x005bea20u; /* desert cam @ ROM 0x1FA20 */
        else
            g0 = 0x005bedc0u; /* chase @ ROM 0x1FDC0 */
        game_start_race_cam_bind((u32)g0, 0, 0);
    } else {
        g0 = 0x005bedc0u;
        game_start_race_cam_bind((u32)g0, 0, 0);
    }

    g0 = 0x005c0330u;
    game_start_race_geo_prg_slot((u32)g0, 0, 0);
    game_start_race_cam_init(0, 0, 0);
}
