/* Race HUD layer draws @ 0x1E1D0 — cam_init g0=0; hud_frame g0=1.
 *
 * g0==0: four static draw_scene overlays on 0x20ac88 + digit-cache clear.
 * g0!=0 (@0x1E28C): lake special-case erase/draw, else time_split(0x2020b8)
 * into private frame and update changed lap-time digits (caches @ 0x20aaf0).
 *
 * source: disasm/maincpu/maincpu_01e1d0_c0.asm + maincpu_01e28c_200.asm */
// @rom 0x1e1d0 +0x2a0 game_start_race_hud_layers

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "i960_host.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

extern void tile_attract_time_split(u32 value, u16 out[4]);

static void hud_layers_long(u32 y_base)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x50];
    u16 *split;
    u32 y;
    u32 batch;
    u32 digit;
    u32 cached;
    u32 course;
    u32 mode;
    u32 flags;

    course = i960_host_race_course_index();
    mode = i960_ld_u32(I960_WORKRAM, 0x202230, 0);

    /* Lake + champ: optional erase/draw then return. */
    if (course == 3u && mode == 0u) {
        flags = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
        flags = (flags & ~31u) & 0xffu;
        if (flags == 0u)
            return;
        flags = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
        g0 = 0;
        g1 = y_base + 2u;
        g2 = i960_ld_u32(I960_WORKRAM, 0x20ac94, 0);
        g3 = 0;
        g4 = 0;
        if ((flags & (1u << 5)) != 0u)
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        else
            erase_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        return;
    }

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    /* Caller already did addo 16,sp — keep a private frame for 0x40(fp). */
    sp = sp + 0x10u;
    split = (u16 *)(frame + 0x40);
    y = y_base + 2u;

    tile_attract_time_split(i960_ld_u32(I960_WORKRAM, 0x2020b8, 0), split);
    batch = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);

    digit = (u32)split[1] / 10u;
    cached = i960_ld_u32(I960_WORKRAM, 0x20ab04, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20ab04, 0, digit);
        g0 = 1;
        g1 = y;
        g2 = batch;
        g3 = digit;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    digit = (u32)split[1] % 10u;
    cached = i960_ld_u32(I960_WORKRAM, 0x20aaf0, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20aaf0, 0, digit);
        g0 = 3;
        g1 = y;
        g2 = batch;
        g3 = digit;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    digit = (u32)split[2] / 10u;
    cached = i960_ld_u32(I960_WORKRAM, 0x20aaf4, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20aaf4, 0, digit);
        g0 = 6;
        g1 = y;
        g2 = batch;
        g3 = digit;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    digit = (u32)split[2] % 10u;
    cached = i960_ld_u32(I960_WORKRAM, 0x20aaf8, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20aaf8, 0, digit);
        g0 = 8;
        g1 = y;
        g2 = batch;
        g3 = digit;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    digit = (u32)split[3] / 10u;
    cached = i960_ld_u32(I960_WORKRAM, 0x20aafc, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20aafc, 0, digit);
        g0 = 11;
        g1 = y;
        g2 = batch;
        g3 = digit;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    digit = (u32)split[3] % 10u;
    cached = i960_ld_u32(I960_WORKRAM, 0x20ab00, 0);
    if (cached != digit) {
        i960_st_u32(I960_WORKRAM, 0x20ab00, 0, digit);
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

void game_start_race_hud_layers(u32 arg0, u32 arg1, u32 arg2)
{
    static int logged;
    u32 y_base;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_hud_layers g0=%u\n", (unsigned)arg0);
        fflush(stderr);
        logged = 1;
    }

    y_base = (i960_ld_u32(I960_WORKRAM, 0x2139d4, 0) == 0u) ? 3u : 1u;

    if (arg0 != 0u) {
        hud_layers_long(y_base);
        return;
    }

    g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
    g0 = 3;
    g1 = y_base;
    g3 = 24;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
    g0 = 3;
    g1 = y_base + 5u;
    g3 = 23;
    g4 = 0;
    y_base = y_base + 2u;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
    g0 = 5;
    g1 = y_base;
    g3 = 21;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
    g0 = 10;
    g1 = y_base;
    g3 = 22;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    i960_st_u32(I960_WORKRAM, 0x20aaf0, 0, (u32)-1);
    i960_st_u32(I960_WORKRAM, 0x20aaf4, 0, (u32)-1);
    i960_st_u32(I960_WORKRAM, 0x20aaf8, 0, (u32)-1);
    i960_st_u32(I960_WORKRAM, 0x20aafc, 0, (u32)-1);
    i960_st_u32(I960_WORKRAM, 0x20ab00, 0, (u32)-1);
    i960_st_u32(I960_WORKRAM, 0x20ab04, 0, 0);
}
