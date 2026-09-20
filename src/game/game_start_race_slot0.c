/* Race sub-slot 0 @ 0x1CA70 — first post-START race setup frame.
 * Seeds HUD row params, copies a word list, draws START-related scene
 * rows via draw_scene_dispatch, then advances 0x2020a8 by 2 so the next
 * race_frame hits table[1].
 * source: disasm/maincpu/maincpu_01ca70_2b0.asm */
// @rom 0x1ca70 +0x1a8 game_start_race_slot0

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_slot0(u32 arg0, u32 arg1, u32 arg2)
{
    u32 timer;
    u32 n;
    u32 row;
    u32 base_y;
    u32 y;
    u32 src;
    u32 dst;
    u32 w0;
    u32 w1;
    u32 choice;
    u32 course;
    u32 ptr;
    u32 bit;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_slot0 (post-START setup)\n");
        fflush(stderr);
        logged = 1;
    }

    /* @0x1CA70–0x1CAA0: row count pair from 0x2139d4. */
    if (i960_ld_u32(I960_WORKRAM, 0x2139d4, 0) == 0u) {
        i960_st_u32(I960_WORKRAM, 0x20aad0, 0, 3u);
        i960_st_u32(I960_WORKRAM, 0x20aad4, 0, 10u);
    } else {
        i960_st_u32(I960_WORKRAM, 0x20aad0, 0, 3u);
        i960_st_u32(I960_WORKRAM, 0x20aad4, 0, 8u);
    }

    i960_st_u32(I960_WORKRAM, 0x214120, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2020b0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a760, 0, 0);

    /*
     * Host keyboard: ←/→ writes analog[0] and latches on keyup so
     * mode_select table 2 / course / car lookups can hold a choice.
     * Cabinet pots sit at 0x80 unless the player is turning. After this
     * post-START setup, fov_scale @ 0x395D8 / scene_apply would keep
     * driving cam pitch from a leftover stop — recock to idle.
     */
    model2_io_analog_set(MODEL2_IO_AN_STEER, 0x80u);
    model2_io_analog_set(MODEL2_IO_AN_ACCEL, 0x00u);
    model2_io_analog_set(MODEL2_IO_AN_BRAKE, 0x00u);

    /* @0x1CAC0: lda 0x20aca0 — pass EA, not a load. */
    g0 = 0x0020aca0u;
    game_start_race_rank_list_seed((u32)g0, 0, 0);

    timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    if ((i32)timer < 0) {
        /*
         * @0x1CAD8–0x1CB18: copy (ptr, next) pairs from *0x20aca4 into
         * 0x20ab70 until next < 0, then store −1 sentinel.
         */
        n = 0u;
        if ((i32)i960_ld_u32(I960_WORKRAM, 0x20aca4, 0) > 0) {
            src = 0x0020aca4u;
            dst = 0x0020ab70u;
            for (;;) {
                w0 = i960_ld_u32(I960_WORKRAM, src, 0);
                src += 8u;
                w1 = i960_ld_u32(I960_WORKRAM, src, 0);
                i960_st_u32(I960_WORKRAM, dst, 0, w0);
                dst += 4u;
                n += 1u;
                /* 0x47330 seeds this list; without it, refuse to run away. */
                if ((i32)w1 < 0 || n >= 0x100u)
                    break;
            }
        }
        i960_st_u32(I960_WORKRAM, 0x20ab70, n << 2, (u32)-1);
    }

    /*
     * @0x1CB20–0x1CC6C: for each of 0x2020c4 rows, emit 8 draw_scene
     * dispatches with y = 20aad4 + row*3 and x from 20aad0 ± offsets.
     * Cap rows as a safety net (practice desert is 3 via 0x47330).
     */
    n = i960_ld_u32(I960_WORKRAM, 0x2020c4, 0);
    if ((i32)n > 0x40)
        n = 0x40u;
    if ((i32)n > 0) {
        if (logged == 1) {
            lift_log(
                    "lift: race_slot0 draw rows=%u batch=%u limit=%u\n",
                    (unsigned)n,
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x20ac88, 0),
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x20c954, 0));
            fflush(stderr);
            logged = 2;
        }
        for (row = 0u; (i32)row < (i32)n; row++) {
            base_y = i960_ld_u32(I960_WORKRAM, 0x20aad4, 0);
            y = base_y + row * 3u;

            g0 = i960_ld_u32(I960_WORKRAM, 0x20aad0, 0) - 2u;
            g1 = y;
            g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
            g3 = row + 11u;
            g4 = 0;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

            g0 = i960_ld_u32(I960_WORKRAM, 0x20aad0, 0);
            g1 = y;
            g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
            g3 = 20;
            g4 = 0;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

            g0 = i960_ld_u32(I960_WORKRAM, 0x20aad0, 0) + 2u;
            g1 = y;
            g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
            g3 = 21;
            g4 = 0;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

            g0 = i960_ld_u32(I960_WORKRAM, 0x20aad0, 0) + 3u;
            g1 = y;
            g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
            g3 = 20;
            g4 = 0;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

            g0 = i960_ld_u32(I960_WORKRAM, 0x20aad0, 0) + 5u;
            g1 = y;
            g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
            g3 = 20;
            g4 = 0;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

            g0 = i960_ld_u32(I960_WORKRAM, 0x20aad0, 0) + 7u;
            g1 = y;
            g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
            g3 = 22;
            g4 = 0;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

            g0 = i960_ld_u32(I960_WORKRAM, 0x20aad0, 0) + 8u;
            g1 = y;
            g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
            g3 = 20;
            g4 = 0;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

            g0 = i960_ld_u32(I960_WORKRAM, 0x20aad0, 0) + 10u;
            g1 = y;
            g2 = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);
            g3 = 20;
            g4 = 0;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        }
    }

    g0 = i960_ld_u32(I960_WORKRAM, 0x20aca4, 0);
    i960_st_u32(I960_WORKRAM, 0x2020c0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20ac98, 0, 1u);

    /* @0x1C830: clear lap/HUD banks; seed 0x20b0b0 from g0 for slot1 blend. */
    game_start_race_lap_bank_clear((u32)g0, 0, 0);
    geo_workram_pose_ring_init(0, 0, 0);
    i960_call_rom(0x5adb0);
    game_start_race_gate_bind(0, 0, 0);

    g0 = 0x9au;
    comm_palette_index_call(0x9au);
    g0 = 0;
    game_start_race_obj_scan(0, 0, 0);

    /* @0x1CCAC: 0x22940 return lands in g0 → 0x2020c8. */
    i960_st_u32(I960_WORKRAM, 0x2020c8, 0, (u32)g0);

    choice = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
    course = i960_host_race_course_index();
    if (choice == 0u)
        g0 = model2_workram_mirror_u32(0x5bb720u + (course << 3));
    else
        g0 = model2_workram_mirror_u32(0x5bb760u + (course << 3));
    comm_palette_index_call((u32)g0);

    /* @0x1CCEC–0x1CD10: flip bit31 of *(0x2140cc+0x3c); advance sub-slot. */
    ptr = i960_ld_u32(I960_WORKRAM, 0x2140cc, 0);
    bit = 0u;
    if (ptr != 0u) {
        bit = i960_ld_u32(I960_WORKRAM, ptr + 0x3cu, 0);
        bit ^= (1u << 31);
    }
    i960_st_u32(I960_WORKRAM, 0x20ab5c, 0, bit);
    {
        u32 sub = i960_ld_u32(I960_WORKRAM, 0x2020a8, 0);

        i960_st_u32(I960_WORKRAM, 0x2020a8, 0, sub + 2u);
        if (logged == 2) {
            lift_log( "lift: race_slot0 done sub→%u\n", (unsigned)(sub + 2u));
            fflush(stderr);
            logged = 3;
        }
    }
}
