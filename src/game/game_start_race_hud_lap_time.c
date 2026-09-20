/* Race HUD lap-time digits @ 0x1E610 — hud_frame passes g0=0x2020c0;
 * cam_init passes g0=−1 to clear digit caches.
 *
 * g0<0: fill 0x20ab10..0x20ab20 with −1.
 * g0>=0: time_split(0x2020d0[g0]) into a private frame, then draw changed
 * digit slots into batch 0x20ac88.
 *
 * source: disasm/maincpu/maincpu_01e610_1c0.asm */
// @rom 0x1e610 +0x174 game_start_race_hud_lap_time

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

extern void tile_attract_time_split(u32 value, u16 out[4]);

void game_start_race_hud_lap_time(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x50];
    u16 *split;
    u32 y;
    u32 batch;
    u32 time_val;
    u32 digit;
    u32 cached;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_hud_lap_time g0=%d\n", (int)arg0);
        fflush(stderr);
        logged = 1;
    }

    /* @0x1E614: cmpible 0,g0 → draw when g0 >= 0. */
    if ((i32)arg0 < 0) {
        i960_st_u32(I960_WORKRAM, 0x20ab10, 0, (u32)-1);
        i960_st_u32(I960_WORKRAM, 0x20ab14, 0, (u32)-1);
        i960_st_u32(I960_WORKRAM, 0x20ab18, 0, (u32)-1);
        i960_st_u32(I960_WORKRAM, 0x20ab1c, 0, (u32)-1);
        i960_st_u32(I960_WORKRAM, 0x20ab20, 0, (u32)-1);
        return;
    }

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x10u;
    split = (u16 *)(frame + 0x40);

    /* y = 3*g0 + (2139d4==0 ? 10 : 8). */
    if (i960_ld_u32(I960_WORKRAM, 0x2139d4, 0) == 0u)
        y = arg0 * 3u + 10u;
    else
        y = arg0 * 3u + 8u;

    time_val = i960_ld_u32(I960_WORKRAM, 0x2020d0u + arg0 * 4u, 0);
    tile_attract_time_split(time_val, split);

    batch = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);

    /* split[1] at x=3 — minutes-ish field. */
    digit = (u32)split[1];
    cached = i960_ld_u32(I960_WORKRAM, 0x20ab10, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20ab10, 0, digit);
        g0 = 3;
        g1 = y;
        g2 = batch;
        g3 = digit;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    /* split[2] tens at x=6. */
    digit = (u32)split[2] / 10u;
    cached = i960_ld_u32(I960_WORKRAM, 0x20ab14, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20ab14, 0, digit);
        g0 = 6;
        g1 = y;
        g2 = batch;
        g3 = digit;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    /* split[2] ones at x=8. */
    digit = (u32)split[2] % 10u;
    cached = i960_ld_u32(I960_WORKRAM, 0x20ab18, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20ab18, 0, digit);
        g0 = 8;
        g1 = y;
        g2 = batch;
        g3 = digit;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    /* split[3] tens at x=11. */
    digit = (u32)split[3] / 10u;
    cached = i960_ld_u32(I960_WORKRAM, 0x20ab1c, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20ab1c, 0, digit);
        g0 = 11;
        g1 = y;
        g2 = batch;
        g3 = digit;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    /* split[3] ones at x=13. */
    digit = (u32)split[3] % 10u;
    cached = i960_ld_u32(I960_WORKRAM, 0x20ab20, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20ab20, 0, digit);
        g0 = 13;
        g1 = y;
        g2 = batch;
        g3 = digit;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    fp = fp_save;
    sp = sp_save;
}
