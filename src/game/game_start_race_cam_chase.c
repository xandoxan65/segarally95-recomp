/* Chase / non-desert race camera @ 0x1FDC0 — staged callx 0x5BEDC0.
 *
 * Installed by cam_boot when timer>=0 or non-practice/non-desert. Desert cam
 * @ 0x1FA20 also rebinds here when spline t >= 1.
 *
 * Per-frame: copy object pose from 0x213980 into 0x20220c/214/200/208, issue
 * TGP yaw/pitch/roll (0x2b/0x29/0x2a) with notbit-31 on angles, call helper
 * geo_view_matrix_dir_emit @ 0x29120, clear 0x2139d8, TGP 0x27 with adjusted
 * eye, then cam_rebind @ 0x202e0 + blend into 0x20ab5c via float_clamp @ 0x27b28.
 *
 * source: disasm/maincpu/maincpu_01fdc0_200.asm */
// @rom 0x1fdc0 +0x198 game_start_race_cam_chase

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_hw.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <math.h>
#include <stdio.h>

static u32 f_neg(u32 bits)
{
    return bits ^ 0x80000000u;
}

void game_start_race_cam_chase(u32 arg0, u32 arg1, u32 arg2)
{
    u32 obj;
    u32 px, py, pz;
    u32 yaw, pitch, roll;
    u32 mid;
    u32 blend;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    obj = i960_ld_u32(I960_WORKRAM, 0x213980, 0);
    if (obj == 0u
        || (model2_ram_mut(obj) == NULL && model2_rom_at(obj) == NULL))
        return;

    /* @0x1FDC0–0x1FDEC: pose → 0x20220c / 0x202200. */
    px = i960_ld_u32(I960_ABS, obj, 0);
    py = i960_ld_u32(I960_ABS, obj, 4);
    pz = i960_ld_u32(I960_ABS, obj, 8);
    i960_st_u32(I960_WORKRAM, 0x20220c, 0, px);
    i960_st_u32(I960_WORKRAM, 0x202210, 0, py);
    i960_st_u32(I960_WORKRAM, 0x202214, 0, pz);

    /*
     * Pose layout (pose_publish / desert cam): +0x18 pitch, +0x1c yaw, +0x20
     * roll. Disasm @ 0x1FDE0–0x1FE68: 0x2b←roll, 0x29←(+0x18), 0x2a←(+0x1c).
     */
    pitch = i960_ld_u32(I960_ABS, obj, 0x18);
    yaw = i960_ld_u32(I960_ABS, obj, 0x1c);
    roll = i960_ld_u32(I960_ABS, obj, 0x20);
    i960_st_u32(I960_WORKRAM, 0x202200, 0, pitch);
    i960_st_u32(I960_WORKRAM, 0x202204, 0, yaw);
    i960_st_u32(I960_WORKRAM, 0x202208, 0, roll);

    {
        static int y_nan_logged;
        double y = i960_u32_to_f64(py);
        double x = i960_u32_to_f64(px);
        double z = i960_u32_to_f64(pz);

        /*
         * One-shot when follow pose is non-finite or |Y|≫road scale — spring
         * / height / matrix-stack path (desert planes give |h|≈0.3 at seed).
         */
        if (!y_nan_logged
            && (!(x == x && y == y && z == z)
                || y > 100.0 || y < -100.0)) {
            u32 car = 0x00213b98u;

            lift_log(
                    "lift: race_cam_chase Y bad follow=%#x "
                    "pos=(%.3g,%.3g,%.3g) car213b98=(%.3g,%.3g,%.3g) "
                    "a0=%.3g a4=%.3g 214124=%.3g slot18=%.3g\n",
                    obj, x, y, z,
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, car, 0)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, car, 4)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, car, 8)),
                    i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2138a0, 0)),
                    i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2138a4, 0)),
                    i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x214124, 0)),
                    i960_u32_to_f64(
                        i960_ld_u32(I960_WORKRAM, 0x2138e8, 0)));
            fflush(stderr);
            y_nan_logged = 1;
        }
    }

    if (!logged) {
        /* Slot0 = cam follow (213b40); slot1 = first car (213b98). */
        u32 cam_pose = 0x00213b40u;
        u32 car_pose = 0x00213b98u;

        lift_log(
                "lift: race_cam_chase obj=%#x (follow) pos=(%.3g,%.3g,%.3g) "
                "ang=(%.3g,%.3g,%.3g) car213b98 ang=(%.3g,%.3g,%.3g) "
                "pos=(%.3g,%.3g,%.3g) 214200=%.3g 214204=%.3g "
                "an=0x%02x/%02x/%02x 214120=%d\n",
                obj,
                i960_u32_to_f64(px),
                i960_u32_to_f64(py),
                i960_u32_to_f64(pz),
                i960_u32_to_f64(pitch),
                i960_u32_to_f64(yaw),
                i960_u32_to_f64(roll),
                i960_u32_to_f64(i960_ld_u32(I960_ABS, car_pose, 0x18)),
                i960_u32_to_f64(i960_ld_u32(I960_ABS, car_pose, 0x1c)),
                i960_u32_to_f64(i960_ld_u32(I960_ABS, car_pose, 0x20)),
                i960_u32_to_f64(i960_ld_u32(I960_ABS, car_pose, 0)),
                i960_u32_to_f64(i960_ld_u32(I960_ABS, car_pose, 4)),
                i960_u32_to_f64(i960_ld_u32(I960_ABS, car_pose, 8)),
                i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x214200, 0)),
                i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x214204, 0)),
                (unsigned)i960_ld_u8(I960_WORKRAM, 0x202050, 0),
                (unsigned)i960_ld_u8(I960_WORKRAM, 0x202051, 0),
                (unsigned)i960_ld_u8(I960_WORKRAM, 0x202052, 0),
                (int)(i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0));
        (void)cam_pose;
        fflush(stderr);
        logged = 1;
    } else {
        static unsigned ang_log;

        ang_log++;
        if ((ang_log % 60u) == 0u) {
            u32 car_pose = 0x00213b98u;

            lift_log(
                    "lift: race_cam_chase ang=(%.3g,%.3g,%.3g) "
                    "follow=(%.3g,%.3g,%.3g) car=(%.3g,%.3g,%.3g) "
                    "an=0x%02x/%02x/%02x 214120=%d\n",
                    i960_u32_to_f64(pitch),
                    i960_u32_to_f64(yaw),
                    i960_u32_to_f64(roll),
                    i960_u32_to_f64(px),
                    i960_u32_to_f64(py),
                    i960_u32_to_f64(pz),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, car_pose, 0)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, car_pose, 4)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, car_pose, 8)),
                    (unsigned)i960_ld_u8(I960_WORKRAM, 0x202050, 0),
                    (unsigned)i960_ld_u8(I960_WORKRAM, 0x202051, 0),
                    (unsigned)i960_ld_u8(I960_WORKRAM, 0x202052, 0),
                    (int)(i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0));
            fflush(stderr);
        }
    }

    /*
     * Disasm @ 0x1FE08+: 0x2b/0x29/0x2a with notbit 31 — no leading 0x25.
     * race_frame already issued 0x25 before cam_bind.
     */
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    i960_mmio_write_u32(0x884000, f_neg(roll));
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, f_neg(pitch));
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, f_neg(yaw));

    geo_view_matrix_dir_emit(0xbf34fdf4u, 0xbf34fdf4u, 0x3f34fdf4u);

    /* @0x1FE74–0x1FEE0: 0x27(−x, (−y)−0.75, −z); clear object_pen gate.
     * Non-finite follow XYZ is leftover 0x2c / invalid-op; TGP 0x27 of NaN
     * latches a dead view. Skip the translate — race_frame already 0x25'd. */
    if (isfinite(i960_u32_to_f64(px))
        && isfinite(i960_u32_to_f64(py))
        && isfinite(i960_u32_to_f64(pz))) {
        mid = (u32)i960_f64_to_u32(
            i960_u32_to_f64(f_neg(py)) - i960_rifl_read(0, 0x3fe80000u));
        i960_mmio_write_u32(0x884000, 0x13802727u);
        i960_mmio_write_u32(0x884000, f_neg(px));
        i960_mmio_write_u32(0x884000, mid);
        i960_mmio_write_u32(0x884000, f_neg(pz));
    }
    /* Disasm `st g14` — host callx may leave g14 dirty; force the zero. */
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x2139d8, 0, (u32)g14);
    /* Same latch contract as attract script_finish / desert cam. */
    model2_hw_latch_view_matrix();

    game_start_race_cam_rebind(0, 0, 0);

    /* @0x1FEEC–0x1FF4C: pitch blend into 0x20ab5c via bal 0x27b28. */
    blend = geo_view_float_clamp(
        (u32)i960_f64_to_u32(
            i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x202204, 0))
            + i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20ab5c, 0))),
        0, 0);
    blend = (u32)i960_f64_to_u32(
        i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20ab5c, 0))
        - i960_u32_to_f64(blend)
              * i960_rifl_read(0x9999999au, 0x3fa99999u));
    i960_st_u32(I960_WORKRAM, 0x20ab5c, 0, blend);
    blend = geo_view_float_clamp(blend, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20ab5c, 0, blend);
}
