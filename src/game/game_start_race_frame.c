/* Post-START race frame @ 0x1BBA0 — champ/practice scene table continuation.
 * After attract_hud_setup advances 0x2020ac, this runs the in-game 3D frame
 * (TGP/copro submit, record index, object pen). Many callees still host
 * call_rom; lifted ones are called directly.
 * source: disasm/maincpu/maincpu_01bba0_2a0.asm */
// @rom 0x1bba0 +0x298 game_start_race_frame

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_geo.h"
#include "model2_hw.h"
#include "model2_memory.h"
#include "model2_rom.h"
#include "i960_host.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_frame(u32 arg0, u32 arg1, u32 arg2)
{
    u32 timer;
    u32 course;
    u32 scale;
    u32 acc;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        fprintf(stderr, "lift: race_frame (post-START game path)\n");
        logged = 1;
    }

    /*
     * Disasm @ 0x1BBA0: no mesh clear. Same 3D path as attract:
     * cam_bind → … → course_view_bind (TGP 0x05 / catalog_span) →
     * copro_submit → object_pen. Host OpenGL draws the published latch.
     */

    /*
     * @0x1BBA0–0x1BBFC: cmpibge 0,timer → 0x1bbfc (keep 25.0f) when timer <= 0.
     * Fallthrough when timer > 0: if count < 2, blend (2−count)*2.5 off 11.25.
     */
    timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    scale = 0x41c80000u; /* 25.0f default store */
    if ((i32)timer > 0) {
        u32 count = i960_ld_u32(I960_WORKRAM, 0x213978, 0);

        if ((i32)count < 2) {
            double t = (double)(i32)(2u - count);
            double v = 11.25 - t * 2.5;

            scale = (u32)i960_f64_to_u32(v);
        }
    }
    i960_st_u32(I960_WORKRAM, 0x213970, 0, scale);

    geo_fifo_bootstrap(0, 0, 0);
    i960_mmio_write_u32(0x884000, 0x11802323u);
    i960_mmio_write_u32(0x884000, 0x12802525u);

    g0 = 1;
    attract_logo_dispatch(1, 0, 0);

    /* --practice intro: analog 0x80 until GO (fov_scale rest). */
    i960_host_skip_practice_idle_wheel();
    g0 = 0;
    game_start_race_cam_bind(0, 0, 0);
    game_start_race_sub_dispatch(0, 0, 0);

    g0 = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    game_start_race_obj_bind((u32)g0, 0, 0);

    g0 = i960_ld_u32(I960_WORKRAM, 0x20aac0, 0);
    /* @0x1BC5x: list-flag prep + PRG budget → 0x214344 (was silent call_rom). */
    game_start_race_list_flag_prep((u32)g0, 0, 0);
    game_start_race_prg_budget(0, 0, 0);

    {
        static int depth_list_logged;
        u32 head = i960_ld_u32(I960_WORKRAM, 0x2139f8, 0);

        if (!depth_list_logged && head != 0u) {
            fprintf(stderr,
                    "lift: race depth-list head=%#x flag=%u mode=%u "
                    "depth=%.3g count=%u d8=%u\n",
                    head,
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x2139f0, 0),
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x2139f4, 0),
                    i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2139fc, 0)),
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x213978, 0),
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x2139d8, 0));
            fflush(stderr);
            depth_list_logged = 1;
        }
    }

    timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    if ((i32)timer > 0)
        i960_call_rom(0x24980);

    /*
     * Cam latched the view in cam_bind. pen_walk → pen_desc → far/glyph
     * compose object R on the *current* TGP parent (0x20 push / 0x05).
     * Re-assert the latch before the walk so mode≠1 cars (world yaw) are
     * not parented under a dirty matrix from sub_dispatch / 0x24980 —
     * same host contract as the post-copro restore below.
     */
    model2_hw_restore_latched_view_matrix();

    g0 = 1;
    game_start_race_object_pen_walk(1, 0, 0);

    course = i960_host_race_course_index();
    /* cmpibl 3,course → skip when course > 3. Desert/forest/mountain/lakeside run. */
    if ((i32)course <= 3) {
        u32 alt = i960_ld_u32(I960_WORKRAM, 0x2020a4, 0);

        if (alt == 0u) {
            g1 = model2_workram_mirror_u32(0x5db310u + (course << 4));
            g0 = i960_ld_u32(I960_WORKRAM, 0x2140c8, 0);
            g2 = model2_workram_mirror_u32(0x5db314u + (course << 4));
            geo_attract_record_index((u32)g0, (u32)g1, (u32)g2);
        } else {
            g1 = model2_workram_mirror_u32(0x5db318u + (course << 4));
            g0 = i960_ld_u32(I960_WORKRAM, 0x2140c8, 0);
            g2 = model2_workram_mirror_u32(0x5db31cu + (course << 4));
            geo_attract_record_index((u32)g0, (u32)g1, (u32)g2);
        }
    }

    {
        u32 flag = i960_ld_u32(I960_WORKRAM, 0x2139d8, 0);
        u32 a = i960_ld_u32(I960_WORKRAM, 0x214344, 0);
        u32 b = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);

        acc = a + b;
        if (flag != 0u) {
            if (acc > 0xbb8u)
                acc = 0xbb8u;
        }
        i960_st_u32(I960_WORKRAM, 0x214344, 0, acc);
        /*
         * Cam latched the view earlier; object_pen_walk / unlifted leaves may
         * have dirtied TGP since (same class as attract object_extra missing
         * pop). course_view_bind's 0x05 must emit the cam view — restore it.
         */
        model2_hw_restore_latched_view_matrix();
        g0 = course;
        g1 = i960_ld_u32(I960_WORKRAM, 0x2140c8, 0);
        g2 = acc;
        geo_attract_course_view_bind((u32)g0, (u32)g1, (u32)g2);
        geo_attract_copro_submit(0, 0, 0);
        /*
         * copro_submit may leave a non-view matrix (0x55 emit). object_pen
         * composes object R/T on the *current* TGP parent — same class as
         * attract object_extra missing pop. Restore latched cam view so the
         * car and course share one parent (scenery vs car alignment).
         */
        model2_hw_restore_latched_view_matrix();
    }

    if (i960_ld_u32(I960_WORKRAM, 0x2139d8, 0) != 0u) {
        /*
         * Disasm @ 0x1BD48: ld 0x213980 → object_pen only. Solo publish
         * puts cam follow at slot0 (0x213b40); driveable car is slot1
         * (0x213b98). Desert zoom films follow (view R(−follow) cancels
         * glyph angles → rear-to-cam hero). World-yaw cars are filmed by
         * pen_walk @ 0x24C50 / secondary pen_desc below — not by rewriting
         * follow angles or retargeting pen to 0x213984.
         */
        u32 obj_va = i960_ld_u32(I960_WORKRAM, 0x213980, 0);
        u8 *obj_host = model2_ram_mut(obj_va);

        i960_mmio_write_u32(0x800030u, 0u);
        i960_mmio_write_u32(GEO_PRG_FIFO, 0x80u);
        i960_mmio_write_u32(GEO_PRG_FIFO, 0x01f00200u);
        i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
        i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
        i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
        i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
        g1 = 0x3f800000u;
        g2 = 0x270fu;
        /*
         * 0x213980 holds a guest object VA. object_pen/glyph_emit take a host
         * pointer (attract fp shadows). Casting the VA directly SIGBUS'd on
         * ARM — resolve through workram first.
         */
        if (obj_host)
            geo_attract_object_pen(obj_host, (u32)g1, (u32)g2);

        if (i960_ld_u32(I960_WORKRAM, 0x2139f8, 0) != 0u) {
            u8 b = i960_ld_u8(I960_WORKRAM, 0x2139f0, 0);

            if ((b & 2u) != 0u) {
                u32 obj = i960_ld_u32(I960_WORKRAM, 0x2139f8, 0);
                u8 flags = i960_ld_u8(I960_ABS, obj + 0x50u, 0);

                if ((flags & 0x80u) != 0u) {
                    static unsigned sec_logs;
                    u32 follow = i960_ld_u32(I960_WORKRAM, 0x213980, 0);

                    if (sec_logs < 8u || (sec_logs % 60u) == 0u) {
                        fprintf(stderr,
                                "lift: race_frame secondary pen_desc "
                                "obj=%#x yaw=%.3g follow=%#x fyaw=%.3g "
                                "mode=%u d8=%u\n",
                                obj,
                                i960_u32_to_f64(
                                    i960_ld_u32(I960_ABS, obj, 0x1c)),
                                follow,
                                follow ? i960_u32_to_f64(
                                    i960_ld_u32(I960_ABS, follow, 0x1c))
                                       : 0.0,
                                (unsigned)i960_ld_u32(
                                    I960_WORKRAM, 0x2139f4, 0),
                                (unsigned)i960_ld_u32(
                                    I960_WORKRAM, 0x2139d8, 0));
                        fflush(stderr);
                    }
                    sec_logs++;
                    g0 = 0x002139f0u;
                    game_start_race_object_pen_desc(0x002139f0u, 0, 0);
                }
            }
        }
    }

    if (i960_ld_u32(I960_WORKRAM, 0x213a04, 0) == 1u)
        i960_st_u32(I960_WORKRAM, 0x213a04, 0, 2u);

    {
        u32 lo = i960_ld_u32(I960_ABS, 0x00f00008u, 0);
        u32 hi = i960_ld_u32(I960_ABS, 0x00f0000cu, 0);

        (void)hi;
        if (lo == 0xfffffu)
            lo = 0u;
        i960_st_u32(I960_WORKRAM, 0x20aac0, 0, lo + 0x15f90u);
    }
    /*
     * @0x1BDxx: walk pool list — desert node frame does 0x20 then catalogs
     * assuming current TGP is the cam view. copro_submit / object_pen may
     * have left a car-local matrix (same class as attract object_extra pop).
     * Restore the latched view before the walk so scenery 0x05 binds parent
     * correctly — otherwise catalogs draw off-frustum (large black areas).
     */
    model2_hw_restore_latched_view_matrix();
    game_start_race_list_walk(0, 0, 0);
    g0 = 0;
    game_start_race_geo_prg_slot(0, 0, 0);

    if (i960_ld_u32(I960_WORKRAM, 0x202000, 0) != 1u) {
        u32 p = i960_ld_u32(I960_WORKRAM, 0x20aac0, 0);

        i960_st_u32(I960_WORKRAM, 0x20aac0, 0, p + 0xfff9e580u);
    }
}
