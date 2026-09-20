/* Car road-motion / steering leaf @ 0x304F0.
 *
 * Projects the car and road-reference points through TGP vector operators,
 * samples two adjacent span tangents, derives a signed steering correction,
 * updates the e0/dc transition state, and publishes a8/ac motion terms.
 *
 * The nominal symbol ends at 0x30D78.  The ble at 0x30D34 targets the
 * disassembly-backed shared cold tail at 0x30D78 (also included below).
 *
 * source: disasm/maincpu/maincpu_0304f0_900.asm */
// @rom 0x304f0 +0x888 game_start_race_obj_car_motion

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_hw.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static u32 motion_f32(double value)
{
    return (u32)i960_f64_to_u32(value);
}

static double motion_f64(u32 bits)
{
    return i960_u32_to_f64(bits);
}

static u32 motion_add(u32 a, u32 b)
{
    return motion_f32(motion_f64(a) + motion_f64(b));
}

static u32 motion_sub(u32 a, u32 b)
{
    return motion_f32(motion_f64(a) - motion_f64(b));
}

static u32 motion_mul(u32 a, u32 b)
{
    return motion_f32(motion_f64(a) * motion_f64(b));
}

static u32 motion_div(u32 a, u32 b)
{
    return motion_f32(motion_f64(a) / motion_f64(b));
}

static u32 motion_neg(u32 bits)
{
    return bits ^ 0x80000000u;
}

static void motion_fifo_cmd(u32 marker)
{
    i960_mmio_write_u32(0x884000u, marker);
}

static void motion_fifo_vec3(u32 marker, const u32 v[3], u32 out[3])
{
    motion_fifo_cmd(marker);
    i960_mmio_write_u32(0x884000u, v[0]);
    i960_mmio_write_u32(0x884000u, v[1]);
    i960_mmio_write_u32(0x884000u, v[2]);
    out[0] = i960_mmio_read_u32(0x884000u);
    out[1] = i960_mmio_read_u32(0x884000u);
    out[2] = i960_mmio_read_u32(0x884000u);
}

static u32 motion_fifo_pair(u32 marker, const u32 a[3], const u32 b[3])
{
    motion_fifo_cmd(marker);
    i960_mmio_write_u32(0x884000u, a[0]);
    i960_mmio_write_u32(0x884000u, a[1]);
    i960_mmio_write_u32(0x884000u, a[2]);
    i960_mmio_write_u32(0x884000u, b[0]);
    i960_mmio_write_u32(0x884000u, b[1]);
    i960_mmio_write_u32(0x884000u, b[2]);
    return i960_mmio_read_u32(0x884000u);
}

void game_start_race_obj_car_motion(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    uintptr_t g8_save = g8;
    uintptr_t g9_save = g9;
    uintptr_t g10_save = g10;
    u8 frame[0xc0];
    u32 node = arg0;
    u32 spans = arg1;
    u32 first_xform[3];
    u32 ref_xform[3];
    u32 car_unit[3];
    u32 tangent[3];
    u32 cross[3] = { 0, 0, 0 };
    u32 delta[3];
    u32 ref_delta[3];
    u32 r8_term;
    /* g1 = span table base (car_frame r6). @0x307E8: r6 = ld 0xd8(g0). */
    u32 ac_value = 0;
    u32 weighted = 0;
    u32 weight = 0;
    double road_mix;
    static int logged;

    (void)arg2;
    if (node == 0u)
        return;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    /* @0x304F4: lda 0x90(sp),sp; fp temps extend through 0xb8. */
    sp = sp + 0x90u;

    if (!logged) {
        fprintf(stderr, "lift: car_motion node=%#x\n", node);
        fflush(stderr);
        logged = 1;
    }

    /*
     * @0x30508–0x30538: push, allocate one zero command word through the
     * 0x20a290 cursor, then TGP 0x61 (load matrix stored by 0x60 at selector 0).
     */
    motion_fifo_cmd(0x10002020u);
    {
        u32 cursor = i960_ld_u32(I960_WORKRAM, 0x20a290u, 0);

        i960_st_u32(I960_WORKRAM, 0x20a290u, 0, cursor + 4u);
        i960_st_u32(I960_ABS, cursor, 0, 0);
    }
    motion_fifo_cmd(0x30806161u);

    /* @0x30540–0x30590: transform car XZ with TGP 0x2c. */
    {
        u32 v[3] = {
            i960_ld_u32(I960_ABS, node, 0x14),
            0,
            i960_ld_u32(I960_ABS, node, 0x1c),
        };

        motion_fifo_vec3(0x16002c2cu, v, first_xform);
    }

    /* @0x30590–0x305F8: transform reference XZ, then pop. */
    {
        u32 ref = i960_ld_u32(I960_WORKRAM, 0x20cac0u, 0);
        u32 v[3] = {
            i960_ld_u32(I960_ABS, ref, 0),
            0,
            i960_ld_u32(I960_ABS, ref, 8),
        };

        motion_fifo_vec3(0x16002c2cu, v, ref_xform);
    }
    motion_fifo_cmd(0x10802121u);

    /* @0x305FC–0x30694: normalize node velocity through TGP 0x2f. */
    {
        u32 v[3] = {
            i960_ld_u32(I960_ABS, node, 0x2c),
            0,
            i960_ld_u32(I960_ABS, node, 0x34),
        };

        motion_fifo_vec3(0x17802f2fu, v, car_unit);
    }

    /*
     * @0x30698–0x307C0: normalize two span tangents. TGP 0x5b push order is
     * (Ay,Bz,Az,By,Bx,Ax) with A=car_unit, B=tangent — same scramble as
     * scene_frame @ 0x35994. 0x5cabb0 weights cross lane +4 (Y).
     */
    for (u32 i = 0; i < 2u; ++i) {
        u32 e4 = i960_ld_u32(I960_ABS, node, 0xe4);
        u32 row = spans + ((e4 + i * 2u) << 4);
        u32 v[3] = {
            motion_sub(i960_ld_u32(I960_ABS, row, 0x10),
                       i960_ld_u32(I960_ABS, row, 0)),
            0,
            motion_sub(i960_ld_u32(I960_ABS, row, 0x18),
                       i960_ld_u32(I960_ABS, row, 8)),
        };
        u32 table_weight;

        motion_fifo_vec3(0x17802f2fu, v, tangent);
        motion_fifo_cmd(0x2d805b5bu);
        i960_mmio_write_u32(0x884000u, car_unit[1]);
        i960_mmio_write_u32(0x884000u, tangent[2]);
        i960_mmio_write_u32(0x884000u, car_unit[2]);
        i960_mmio_write_u32(0x884000u, tangent[1]);
        i960_mmio_write_u32(0x884000u, tangent[0]);
        i960_mmio_write_u32(0x884000u, car_unit[0]);
        for (u32 lane = 0; lane < 3u; ++lane)
            cross[lane] = i960_mmio_read_u32(0x884000u);

        table_weight =
            i960_ld_u32(I960_WORKRAM, 0x5cabb0u + i * 4u, 0);
        weighted = motion_add(weighted, motion_mul(table_weight, cross[1]));
        weight = motion_add(weight, table_weight);
    }
    road_mix = fabs(motion_f64(motion_div(weighted, weight)));

    /*
     * @0x307D4–0x30880: TGP 0x58 measures the car/reference XZ delta.
     * r8 initially holds node+0xa8 and is replaced only by the steering
     * clamp paths below.
     */
    {
        u32 ref = i960_ld_u32(I960_WORKRAM, 0x20cac4u, 0);
        const u32 zero[3] = { 0, 0, 0 };

        delta[0] = motion_sub(i960_ld_u32(I960_ABS, node, 0x14),
                              i960_ld_u32(I960_ABS, ref, 0));
        delta[1] = 0;
        delta[2] = motion_sub(i960_ld_u32(I960_ABS, node, 0x1c),
                              i960_ld_u32(I960_ABS, ref, 8));
        r8_term = i960_ld_u32(I960_ABS, node, 0xa8);

        if (motion_f64(motion_fifo_pair(0x2c005858u, zero, delta))
            < i960_rifl_read(0u, 0x403e0000u)) {
            u32 v[3];
            u32 old_delta[3];

            /* @0x30884–0x30934: normalize future and current deltas. */
            ref_delta[0] =
                motion_sub(i960_ld_u32(I960_ABS, ref, 0x0c),
                           i960_ld_u32(I960_ABS, node, 0x2c));
            ref_delta[1] = 0;
            ref_delta[2] =
                motion_sub(i960_ld_u32(I960_ABS, ref, 0x14),
                           i960_ld_u32(I960_ABS, node, 0x34));
            motion_fifo_vec3(0x17802f2fu, ref_delta, v);
            motion_fifo_vec3(0x17802f2fu, delta, old_delta);

            /*
             * @0x30938–0x30A30: sqrt(dot) gate, then the non-alias branch
             * (fp+0x90 != fp+0xa0) computes old_delta × v.
             */
            {
                u32 dot_bits = motion_add(
                    motion_add(motion_mul(old_delta[0], v[0]),
                               motion_mul(old_delta[1], v[1])),
                    motion_mul(old_delta[2], v[2]));
                double dot = motion_f64(dot_bits);

                if (dot > 0.0) {
                    double root = sqrt(dot);

                    if (root > 0.0) {
                        cross[0] =
                            motion_sub(motion_mul(v[1], old_delta[2]),
                                       motion_mul(old_delta[1], v[2]));
                        cross[1] =
                            motion_sub(motion_mul(old_delta[0], v[2]),
                                       motion_mul(v[0], old_delta[2]));
                        cross[2] =
                            motion_sub(motion_mul(v[0], old_delta[1]),
                                       motion_mul(old_delta[0], v[1]));

                        if (fabs(motion_f64(cross[1]))
                            > i960_rifl_read(0xcac08312u, 0x3fc645a1u)) {
                            u32 limit =
                                i960_ld_u32(I960_WORKRAM, 0x20cae0u, 0);

                            r8_term = motion_f64(cross[1]) < 0.0
                                    ? limit : motion_neg(limit);
                        }
                    }
                }
            }
        } else {
            /* @0x30A94–0x30AC0: far path uses transformed X sign. */
            u32 limit = i960_ld_u32(I960_WORKRAM, 0x20cae0u, 0);

            r8_term = motion_f64(ref_xform[0]) < 0.0
                    ? limit : motion_neg(limit);
        }
    }

    /*
     * @0x30AC8–0x30BE0: span e4+20 with wrap. TGP 0x50(car,row) → dist;
     * if dist < 200 re-issue the same order into fp1, else fp1 = 200.
     */
    {
        u32 course = i960_host_race_course_index();
        u32 wrap =
            i960_ld_u32(I960_WORKRAM, 0x5dce70u + (course << 4), 0);
        u32 idx = i960_ld_u32(I960_ABS, node, 0xe4) + 20u;
        u32 row;
        u32 car_pos[3] = {
            i960_ld_u32(I960_ABS, node, 0x14),
            i960_ld_u32(I960_ABS, node, 0x18),
            i960_ld_u32(I960_ABS, node, 0x1c),
        };
        u32 road_pos[3];
        u32 d;
        double ahead;

        if (idx >= wrap)
            idx -= wrap;
        row = spans + (idx << 4);
        road_pos[0] = i960_ld_u32(I960_ABS, row, 0);
        road_pos[1] = i960_ld_u32(I960_ABS, row, 4);
        road_pos[2] = i960_ld_u32(I960_ABS, row, 8);

        d = motion_fifo_pair(0x28005050u, car_pos, road_pos);
        ahead = motion_f64(d);
        if (ahead < i960_rifl_read(0u, 0x40690000u))
            ahead = motion_f64(
                motion_fifo_pair(0x28005050u, car_pos, road_pos));
        else
            ahead = motion_f64(0x43480000u);

        /*
         * @0x30BE0–0x30D18: e0 transition.  r6 starts as ld 0xd8(g0)
         * (@0x307E8) and is replaced by the computed mode-1 floor.
         */
        {
            u32 state = i960_ld_u32(I960_ABS, node, 0xe0);

            ac_value = i960_ld_u32(I960_ABS, node, 0xd8);
            if (state == 0u) {
                if (road_mix > i960_rifl_read(0x9999999au, 0x3fb99999u)
                    || ahead
                           < i960_rifl_read(0u, 0x4067c000u)) {
                    u32 d8 = i960_ld_u32(I960_ABS, node, 0xd8);

                    if (motion_f64(d8) > 0.0) {
                        i960_st_u32(I960_ABS, node, 0xdc, d8);
                        i960_st_u32(I960_ABS, node, 0xe0, 1u);
                    }
                }
            } else if (state == 1u) {
                double candidate =
                    (i960_rifl_read(0u, 0x4062c000u)
                     - road_mix * i960_rifl_read(0u, 0x4072c000u))
                    + ahead * i960_rifl_read(0u, 0x3fd00000u);
                u32 dc;

                if (candidate < i960_rifl_read(0u, 0x40490000u))
                    candidate = motion_f64(0x42480000u);
                ac_value = motion_f32(candidate);
                dc = i960_ld_u32(I960_ABS, node, 0xdc);
                if (motion_f64(ac_value) < motion_f64(dc))
                    i960_st_u32(I960_ABS, node, 0xdc, ac_value);
                else
                    ac_value = dc;

                if (road_mix
                    < i960_rifl_read(0x9999999au, 0x3fb99999u))
                    i960_st_u32(I960_ABS, node, 0xe0, 0);
            }
        }
    }

    /* @0x30D18–0x30D5C: publish steering and acceleration while scale > 0. */
    if (motion_f64(motion_add(
            i960_ld_u32(I960_ABS, node, 0x64),
            i960_ld_u32(I960_ABS, node, 0x68))) > 0.0) {
        if ((i32)i960_ld_u32(I960_WORKRAM, 0x214120u, 0) > 0) {
            u32 old = i960_ld_u32(I960_ABS, node, 0xa8);
            u32 k = 0x3f7f7ceeu;
            u32 next;
            static unsigned drive_log;

            /* @0x30D44–0x30D58: a8 += (1-k)*(r8-a8); no clamp. */
            next = motion_sub(
                r8_term, motion_mul(motion_sub(r8_term, old), k));
            i960_st_u32(I960_ABS, node, 0xa8, next);
            drive_log++;
            if ((drive_log % 60u) == 1u
                || fabs(motion_f64(i960_ld_u32(I960_ABS, node, 0xa8))) > 1.0e4
                || fabs(motion_f64(old)) > 16.0) {
                fprintf(stderr,
                        "lift: car_motion drive node=%#x a8=%.3g ac=%.3g "
                        "lim=%.3g xyz=(%.4g,%.4g,%.4g) "
                        "an=0x%02x/%02x/%02x gear=%u\n",
                        node,
                        motion_f64(next),
                        motion_f64(ac_value),
                        motion_f64(i960_ld_u32(I960_WORKRAM, 0x20cae0u, 0)),
                        motion_f64(i960_ld_u32(I960_ABS, node, 0x14)),
                        motion_f64(i960_ld_u32(I960_ABS, node, 0x18)),
                        motion_f64(i960_ld_u32(I960_ABS, node, 0x1c)),
                        (unsigned)i960_ld_u8(I960_WORKRAM, 0x202050u, 0),
                        (unsigned)i960_ld_u8(I960_WORKRAM, 0x202051u, 0),
                        (unsigned)i960_ld_u8(I960_WORKRAM, 0x202052u, 0),
                        (unsigned)i960_ld_u32(I960_WORKRAM, 0x202044u, 0));
                fflush(stderr);
            }
        }
        i960_st_u32(I960_ABS, node, 0xac, ac_value);
    } else {
        /*
         * @0x30D78: |fp+0x40| vs *0x20cae0; if greater, a8 = ±limit
         * (sign of x); else a8 = x. Then clear bit 4 of node+0x10.
         */
        u32 x = first_xform[0];
        u32 limit = i960_ld_u32(I960_WORKRAM, 0x20cae0u, 0);
        u32 ax = x;
        u32 out;
        u8 flags;

        if (motion_f64(ax) < 0.0)
            ax = motion_neg(ax);
        if (motion_f64(ax) >= motion_f64(limit))
            out = motion_f64(x) < 0.0 ? motion_neg(limit) : limit;
        else
            out = x;
        i960_st_u32(I960_ABS, node, 0xa8, out);
        flags = (u8)i960_ld_u8(I960_ABS, node, 0x10);
        i960_st_u8(I960_ABS, node, 0x10, (u8)(flags & ~(1u << 4)));
    }

    /* @0x30D60 epilogue: restore g8 and saved g9/g10 plus host fp/sp. */
    g8 = g8_save;
    g9 = g9_save;
    g10 = g10_save;
    fp = fp_save;
    sp = sp_save;
}
