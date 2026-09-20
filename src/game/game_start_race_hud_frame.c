/* Race per-frame HUD/matrix refresh @ 0x20740 — callx target via trampoline
 * 0x21330 (staged as 0x5c0330). Same helper chain as cam_init @ 0x206E0 but
 * with g0=1 on the draw paths and tach @ 0x1e500 instead of hud_row_seed.
 *
 * source: disasm/maincpu/maincpu_020740_200.asm */
// @rom 0x20740 +0x4c game_start_race_hud_frame
// @rom 0x21330 +0x8 game_start_race_geo_prg_thunk
// @rom 0x21340 +0x28 game_start_race_geo_prg_hud_go

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_hud_frame(u32 arg0, u32 arg1, u32 arg2)
{
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        fprintf(stderr, "lift: race_hud_frame (per-frame g0=1 path)\n");
        fflush(stderr);
        logged = 1;
    }

    g0 = 1;
    game_start_race_hud_layers(1, 0, 0);
    game_start_race_hud_tach(0, 0, 0);
    g0 = 1;
    game_start_race_cam_matrix(1, 0, 0);
    g0 = i960_ld_u32(I960_WORKRAM, 0x2020c0, 0);
    game_start_race_hud_lap_time((u32)g0, 0, 0);
    g0 = i960_ld_u32(I960_WORKRAM, 0x2020c8, 0) + 1u;
    game_start_race_hud_position((u32)g0, 0, 0);
    g0 = 1;
    game_start_race_hud_speed(1, 0, 0);
    if (i960_ld_u32(I960_WORKRAM, 0x2139d4, 0) == 0u) {
        g0 = 1;
        game_start_race_hud_progress(1, 0, 0);
    }
}

void game_start_race_geo_prg_thunk(u32 arg0, u32 arg1, u32 arg2)
{
    game_start_race_hud_frame(arg0, arg1, arg2);
}

/* Post-countdown PRG @ 0x21340 (staged 0x5c0340). Hud then optional 0x21430. */
void game_start_race_geo_prg_hud_go(u32 arg0, u32 arg1, u32 arg2)
{
    u32 timer;
    static int logged;

    if (!logged) {
        fprintf(stderr, "lift: race_geo_prg_hud_go (0x21340)\n");
        fflush(stderr);
        logged = 1;
    }

    game_start_race_hud_frame(arg0, arg1, arg2);
    timer = i960_ld_u32(I960_WORKRAM, 0x2020bc, 0);
    if (timer > 0xb4u && i960_ld_u32(I960_WORKRAM, 0x2139d4, 0) == 0u)
        i960_call_rom(0x21430);
}
