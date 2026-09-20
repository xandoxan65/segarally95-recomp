/* Race HUD progress / stage bar @ 0x1EDB0 — hud_frame passes g0=1 when
 * 0x2139d4==0. g0==0 seeds a glyph and sets cache −1; g0!=0 maps course
 * progress into bar cells on batch 0x20ac90.
 *
 * source: disasm/maincpu/maincpu_01edb0_180.asm */
// @rom 0x1edb0 +0x164 game_start_race_hud_progress

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_hud_progress(u32 arg0, u32 arg1, u32 arg2)
{
    u32 course;
    u32 tab;
    u32 scale;
    double frac;
    i32 cells;
    u32 cached;
    u32 full;
    u32 rem;
    u32 i;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_hud_progress g0=%u\n", (unsigned)arg0);
        fflush(stderr);
        logged = 1;
    }

    if (arg0 == 0u) {
        g0 = 8;
        g1 = 0;
        g2 = i960_ld_u32(I960_WORKRAM, 0x20ac90, 0);
        g3 = 9;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        i960_st_u32(I960_WORKRAM, 0x20ab44, 0, (u32)-1);
        return;
    }

    course = i960_host_race_course_index();
    tab = model2_workram_mirror_u32(0x005dcac8u + (course * 3u) * 4u);
    scale = model2_workram_mirror_u32(tab);
    {
        u32 rows = i960_ld_u32(I960_WORKRAM, 0x2020c4, 0);
        u32 cursor = i960_ld_u32(I960_WORKRAM, 0x2140c4, 0);
        u32 num = rows * scale;
        double num_f = (double)(i32)num;
        double den_f = (double)(i32)cursor;

        /* Unsigned cvtir bias when num appears negative. */
        if ((i32)num < 0)
            num_f += i960_u32_to_f64(0x4f800000u);
        if (num_f == 0.0)
            frac = 0.0;
        else
            frac = den_f / num_f;
    }

    if (frac < 0.0)
        frac = 0.0;
    if (frac > 1.0)
        frac = 1.0;

    cells = (i32)(frac * i960_rifl_read(0, 0x40750000u)); /* ×336 */
    cached = i960_ld_u32(I960_WORKRAM, 0x20ab44, 0);
    if ((u32)cells == cached)
        return;
    i960_st_u32(I960_WORKRAM, 0x20ab44, 0, (u32)cells);

    full = (u32)(cells + 7) >> 3;
    if (full != 0u)
        full -= 1u;
    rem = (u32)cells - (full << 3);

    for (i = 0; i < full; i++) {
        g0 = i + 9u;
        g1 = 2;
        g2 = i960_ld_u32(I960_WORKRAM, 0x20ac90, 0);
        g3 = 8;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }

    g0 = i + 9u;
    g1 = 2;
    g2 = i960_ld_u32(I960_WORKRAM, 0x20ac90, 0);
    g3 = rem;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    /* Single pad glyph at x=full+10 when full <= 40 (addo 31,9). */
    if (i <= 40u) {
        g0 = i + 10u;
        g1 = 2;
        g2 = i960_ld_u32(I960_WORKRAM, 0x20ac90, 0);
        g3 = 0;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    }
}
