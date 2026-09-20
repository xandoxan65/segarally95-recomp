/* Desert practice race camera @ 0x1FA20 — staged callx target 0x5BEA20.
 *
 * Per-frame after race_frame TGP 0x23/0x25. Builds the view matrix:
 *   1. Pose four ROM control points (0x5BE9F0/A00/A10) through object
 *      yaw/pitch/roll @ 0x213980 into a B-spline table.
 *   2. Sample via geo_attract_fp_series (t @ 0x20ab58).
 *   3. TGP identity + negated object R + 0x27(−(obj+offset)) → view.
 *   4. Set 0x2139d8 so race_frame object_pen runs; if t≥1 switch to chase.
 *
 * source: disasm/maincpu/maincpu_01fa20_39c.asm */
// @rom 0x1fa20 +0x39c game_start_race_cam_desert

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_hw.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

#define CAM_SPLINE_SCRATCH 0x00217000u

static u32 f_neg(u32 bits)
{
    return bits ^ 0x80000000u;
}

static u32 frame_u32(u8 *frame, u32 off)
{
    u32 v;

    memcpy(&v, frame + off, 4);
    return v;
}

static void frame_st32(u8 *frame, u32 off, u32 v)
{
    memcpy(frame + off, &v, 4);
}

static void frame_st64(u8 *frame, u32 off, u32 lo, u32 hi)
{
    frame_st32(frame, off, lo);
    frame_st32(frame, off + 4u, hi);
}

void game_start_race_cam_desert(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u32 save_g8 = (u32)g8;
    u32 save_g9 = (u32)g9;
    u32 save_g10 = (u32)g10;
    u32 save_g11 = (u32)g11;
    u32 save_g12 = (u32)g12;
    u8 frame[0xe8];
    u32 obj;
    u32 slot;
    u32 t_bits;
    u32 eye_x, eye_y, eye_z;
    u32 px, py, pz;
    double t_val;
    double t_bump;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_cam_desert\n");
        fflush(stderr);
        logged = 1;
    }

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0xd0;

    obj = i960_ld_u32(I960_WORKRAM, 0x213980, 0);
    /*
     * Disasm loads (g4) with no null check. Race obj_list @ 0x22e20 publishes
     * m2comm/object VAs @ 0x1a121e8+…; script_finish may publish workram.
     * Refuse only unmapped cells — leave race_frame TGP 0x25 identity.
     */
    if (obj == 0u
        || (model2_ram_mut(obj) == NULL && model2_rom_at(obj) == NULL)) {
        g8 = save_g8;
        g9 = save_g9;
        g10 = save_g10;
        g11 = save_g11;
        g12 = save_g12;
        fp = fp_save;
        sp = sp_save;
        return;
    }

    /* ROM-mirrored control-point quads + transform point. */
    frame_st64(frame, 0xa0,
               i960_ld_u32(I960_WORKRAM, 0x5be9f0, 0),
               i960_ld_u32(I960_WORKRAM, 0x5be9f0, 4));
    frame_st64(frame, 0xa8,
               i960_ld_u32(I960_WORKRAM, 0x5be9f0, 8),
               i960_ld_u32(I960_WORKRAM, 0x5be9f0, 12));
    frame_st64(frame, 0xb0,
               i960_ld_u32(I960_WORKRAM, 0x5bea00, 0),
               i960_ld_u32(I960_WORKRAM, 0x5bea00, 4));
    frame_st64(frame, 0xb8,
               i960_ld_u32(I960_WORKRAM, 0x5bea00, 8),
               i960_ld_u32(I960_WORKRAM, 0x5bea00, 12));
    frame_st64(frame, 0xc0,
               i960_ld_u32(I960_WORKRAM, 0x5bea10, 0),
               i960_ld_u32(I960_WORKRAM, 0x5bea10, 4));
    frame_st64(frame, 0xc8,
               i960_ld_u32(I960_WORKRAM, 0x5bea10, 8),
               i960_ld_u32(I960_WORKRAM, 0x5bea10, 12));
    frame_st64(frame, 0xe0,
               i960_ld_u32(I960_WORKRAM, 0x5c7890, 0),
               i960_ld_u32(I960_WORKRAM, 0x5c7890, 4));
    frame_st32(frame, 0x48, i960_ld_u32(I960_WORKRAM, 0x5c7898, 0));

    /* Object pose: pos @ +0/+8, angles @ +0x18/+0x20. */
    frame_st64(frame, 0x80,
               i960_ld_u32(I960_ABS, obj, 0),
               i960_ld_u32(I960_ABS, obj, 4));
    frame_st32(frame, 0x88, i960_ld_u32(I960_ABS, obj, 8));
    frame_st64(frame, 0x90,
               i960_ld_u32(I960_ABS, obj, 0x18),
               i960_ld_u32(I960_ABS, obj, 0x1c));
    frame_st32(frame, 0x98, i960_ld_u32(I960_ABS, obj, 0x20));

    /*
     * Four iterations (g0 = 0,12,24,36): push → I → yaw/pitch/roll →
     * translate by table row → 0x2c point → store XYZ into spline slots
     * at fp+0x50 + g0 → pop.
     */
    for (slot = 0; slot <= 36u; slot += 12u) {
        u32 *row = (u32 *)(frame + 0x40u + slot);

        frame_st64(frame, 0x40,
                   frame_u32(frame, 0xe0),
                   frame_u32(frame, 0xe4));

        i960_mmio_write_u32(0x884000, 0x10002020u);
        i960_mmio_write_u32(0x884000, 0x12802525u);
        i960_mmio_write_u32(0x884000, 0x15002a2au);
        i960_mmio_write_u32(0x884000, frame_u32(frame, 0x94)); /* yaw */
        i960_mmio_write_u32(0x884000, 0x14802929u);
        i960_mmio_write_u32(0x884000, frame_u32(frame, 0x90)); /* pitch */
        i960_mmio_write_u32(0x884000, 0x15802b2bu);
        i960_mmio_write_u32(0x884000, frame_u32(frame, 0x98)); /* roll */

        /* Translation = control-point row at (fp+0x40+slot)+0x60. */
        i960_mmio_write_u32(0x884000, 0x13802727u);
        i960_mmio_write_u32(0x884000, row[0x60 / 4]);
        i960_mmio_write_u32(0x884000, row[0x64 / 4]);
        i960_mmio_write_u32(0x884000, row[0x68 / 4]);

        i960_mmio_write_u32(0x884000, 0x16002c2cu);
        i960_mmio_write_u32(0x884000, frame_u32(frame, 0x40));
        i960_mmio_write_u32(0x884000, frame_u32(frame, 0x44));
        i960_mmio_write_u32(0x884000, frame_u32(frame, 0x48));
        row[0x10 / 4] = i960_ld_u32(I960_MMIO, 0x884000, 0);
        row[0x14 / 4] = i960_ld_u32(I960_MMIO, 0x884000, 0);
        row[0x18 / 4] = i960_ld_u32(I960_MMIO, 0x884000, 0);

        i960_mmio_write_u32(0x884000, 0x10802121u);
    }

    /* Copy spline table to CRX scratch so fp_series can i960_ld it. */
    {
        u32 i;

        for (i = 0; i < 12u; i++)
            i960_st_u32(I960_ABS, CAM_SPLINE_SCRATCH, i << 2,
                        frame_u32(frame, 0x50u + (i << 2)));
    }
    t_bits = i960_ld_u32(I960_WORKRAM, 0x20ab58, 0);
    g0 = CAM_SPLINE_SCRATCH;
    g1 = t_bits;
    geo_attract_fp_series(CAM_SPLINE_SCRATCH, t_bits, 0);
    /*
     * Disasm keeps spline XYZ in g0/g1/g2 across the view-R setup until
     * stl/addr at 0x1FC88. Host FP helpers (rifl_read / f64_to_u32) share
     * the global register file and would clobber them — snapshot now.
     */
    {
        u32 sx = (u32)g0;
        u32 sy = (u32)g1;
        u32 sz = (u32)g2;
        static int eye_logged;
        static unsigned eye_ticks;

        t_bits = i960_ld_u32(I960_WORKRAM, 0x20ab58, 0);
        t_val = i960_u32_to_f64(t_bits);
        t_bump = t_val + i960_rifl_read(0xbc6a7efau, 0x3f789374u);

        /* View: I + negated object R + translate −(obj + spline). */
        i960_mmio_write_u32(0x884000, 0x12802525u);
        i960_mmio_write_u32(0x884000, 0x15802b2bu);
        i960_mmio_write_u32(0x884000, f_neg(frame_u32(frame, 0x98)));
        i960_mmio_write_u32(0x884000, 0x14802929u);
        i960_mmio_write_u32(0x884000, f_neg(frame_u32(frame, 0x90)));
        i960_mmio_write_u32(0x884000, 0x15002a2au);
        i960_mmio_write_u32(0x884000, f_neg(frame_u32(frame, 0x94)));
        i960_mmio_write_u32(0x884000, 0x13802727u);

        /*
         * Disasm @ 0x1FC80: ldq 0x80(fp) → r4=X, r5=Y, r6=Z.
         * eye = obj + spline; 0x20220c/210/214 = eye_x/y/z;
         * 0x27 payloads −eye_x, −eye_y, −eye_z.
         */
        px = frame_u32(frame, 0x80); /* r4 */
        py = frame_u32(frame, 0x84); /* r5 */
        pz = frame_u32(frame, 0x88); /* r6 */

        i960_st_u32(I960_WORKRAM, 0x20220c, 0, sx);
        i960_st_u32(I960_WORKRAM, 0x202210, 0, sy);
        eye_x = (u32)i960_f64_to_u32(i960_u32_to_f64(px) + i960_u32_to_f64(sx));
        eye_y = (u32)i960_f64_to_u32(i960_u32_to_f64(py) + i960_u32_to_f64(sy));
        eye_z = (u32)i960_f64_to_u32(i960_u32_to_f64(pz) + i960_u32_to_f64(sz));
        i960_st_u32(I960_WORKRAM, 0x202214, 0, eye_z);
        i960_st_u32(I960_WORKRAM, 0x2139d8, 0, 1u);
        i960_st_u32(I960_WORKRAM, 0x20220c, 0, eye_x);
        i960_st_u32(I960_WORKRAM, 0x202210, 0, eye_y);

        i960_mmio_write_u32(0x884000, f_neg(eye_x));
        i960_mmio_write_u32(0x884000, f_neg(eye_y));
        i960_mmio_write_u32(0x884000, f_neg(eye_z));

        eye_ticks++;
        if (!eye_logged || (eye_ticks % 30u) == 1u) {
            lift_log(
                    "lift: race_cam_desert obj=%#x pos=(%.3g,%.3g,%.3g) "
                    "ang=(%.3g,%.3g,%.3g) spline=(%.3g,%.3g,%.3g) "
                    "eye=(%.3g,%.3g,%.3g) t=%.3g an=0x%02x 214120=%d\n",
                    obj,
                    i960_u32_to_f64(px),
                    i960_u32_to_f64(py),
                    i960_u32_to_f64(pz),
                    i960_u32_to_f64(frame_u32(frame, 0x90)),
                    i960_u32_to_f64(frame_u32(frame, 0x94)),
                    i960_u32_to_f64(frame_u32(frame, 0x98)),
                    i960_u32_to_f64(sx),
                    i960_u32_to_f64(sy),
                    i960_u32_to_f64(sz),
                    i960_u32_to_f64(eye_x),
                    i960_u32_to_f64(eye_y),
                    i960_u32_to_f64(eye_z),
                    t_val,
                    (unsigned)i960_ld_u8(I960_WORKRAM, 0x202050, 0),
                    (int)(i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0));
            fflush(stderr);
            eye_logged = 1;
        }
    }
    /* Attract script_finish latches after compose; race desert must too. */
    model2_hw_latch_view_matrix();

    if (t_bump >= i960_rifl_read(0x6872b021u, 0x3feced91u)) {
        /*
         * @0x1FD30–0x1FD40: stq r12 → 0x213840.
         *   r12=4, r13=0, r14=setbit8 (=0x100), r15=setbit6 (=0x40)
         * After mode0 promote: mode=4, scale=0, target=0x100, step=+0x40
         * (fade toward wash). Swapping scale/step left scale=0x100 with
         * step=0 — colorxlat stuck fully washed (no 3D colour).
         */
        i960_st_u32(I960_WORKRAM, 0x213840, 0, 4u);
        i960_st_u32(I960_WORKRAM, 0x213844, 0, 0u);
        i960_st_u32(I960_WORKRAM, 0x213848, 0, 1u << 8);
        i960_st_u32(I960_WORKRAM, 0x21384c, 0, 1u << 6);
    }

    t_bits = (u32)i960_f64_to_u32(t_bump);
    i960_st_u32(I960_WORKRAM, 0x20ab58, 0, t_bits);
    if (t_bump >= 1.0) {
        /* Hand off to chase handler @ 0x5BEDC0. */
        static int handoff_logged;

        g0 = 0x005bedc0u;
        g14 = 0;
        i960_st_u32(I960_WORKRAM, 0x2139d8, 0, (u32)g14);
        game_start_race_cam_bind((u32)g0, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x20ab58, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x213840, 0, 4u);
        i960_st_u32(I960_WORKRAM, 0x213844, 0, 1u << 8);
        i960_st_u32(I960_WORKRAM, 0x213848, 0, 0u);
        i960_st_u32(I960_WORKRAM, 0x21384c, 0, (u32)(0u - 0x40u));
        if (!handoff_logged) {
            lift_log( "lift: race_cam_desert → chase (t>=1)\n");
            fflush(stderr);
            handoff_logged = 1;
        }
    }

    i960_st_u32(I960_WORKRAM, 0x20ab5c, 0, f_neg(frame_u32(frame, 0x94)));

    g8 = save_g8;
    g9 = save_g9;
    g10 = save_g10;
    g11 = save_g11;
    g12 = save_g12;
    fp = fp_save;
    sp = sp_save;
}
