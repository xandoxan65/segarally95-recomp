/* Car list-node frame @ 0x2BCF0 — staged handler 0x5CACF0 from road_attach.
 *
 * list_walk callx's this every race frame. seed_cars starts 0x2139c8 at 1, so
 * the first car publishes to 0x213b98 (slot 1). Slot 0 / 0x213b40 stays the
 * cam epilogue target that desert/chase/object_pen read via 0x213980[0].
 *
 * Entry (disasm):
 *   pose_publish @ 0x2BC08
 *   car_peer @ 0x2CC78
 *   then course friction + e8 state machine → road_span / motion helpers.
 *
 * After attach→pose_publish, n17 high bits come from pose+0x51 (0 on seed) so
 * e8==0. Practice therefore takes e8==0: yaw_step @ 0x2F3C0, motion
 * @ 0x304F0, then bounded integrate @ 0x2CCE0. e8==3 @ 0x2C440 is the
 * inline span path when the high nibble is armed.
 *
 * source: disasm/maincpu/maincpu_02bcf0_a80.asm */
// @rom 0x2bcf0 +0xa80 game_start_race_obj_car_frame

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"
#include "model2_hw.h"

#include "lift_syms.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void game_start_race_obj_car_peer(u32 arg0, u32 arg1, u32 arg2);
void game_start_race_obj_car_yaw_step(u32 arg0, u32 arg1, u32 arg2);
void game_start_race_obj_car_motion(u32 arg0, u32 arg1, u32 arg2);
void game_start_race_obj_car_integrate(u32 arg0, u32 arg1, u32 arg2);

static u32 frame_ld(u8 *frame, u32 off)
{
    u32 v;

    memcpy(&v, frame + off, 4);
    return v;
}

static void frame_st(u8 *frame, u32 off, u32 v)
{
    memcpy(frame + off, &v, 4);
}

static u32 f_add(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) + i960_u32_to_f64(b));
}

static u32 f_sub(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) - i960_u32_to_f64(b));
}

static u32 f_mul(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) * i960_u32_to_f64(b));
}

static u32 f_div(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) / i960_u32_to_f64(b));
}

/* e8==3 body @ 0x2C440–0x2C76C — span project + TGP 0x2f + xyz integrate. */
static void car_frame_e8_mode3(u32 node, u32 span_base, u32 wrap_n, u8 *frame)
{
    u32 g0, g1, g2, g3, g4, g5, g6, g7, g13;
    u32 r4;
    double t;
    double t2;

    /* @0x2C440–0x2C480: integrate vel@0x20/28 into xyz; clear vel. */
    g4 = i960_ld_u32(I960_ABS, node, 0xd8);
    i960_st_u32(I960_ABS, node, 0xac, g4);

    g1 = node + 0x14u;
    g7 = node + 0x20u;
    g4 = f_add(i960_ld_u32(I960_ABS, g1, 0), i960_ld_u32(I960_ABS, g7, 0));
    i960_st_u32(I960_ABS, g1, 0, g4);

    g0 = node + 0x1cu;
    g6 = node + 0x28u;
    g4 = f_add(i960_ld_u32(I960_ABS, g0, 0), i960_ld_u32(I960_ABS, g6, 0));
    i960_st_u32(I960_ABS, g0, 0, g4);

    i960_st_u32(I960_ABS, g7, 0, 0);
    i960_st_u32(I960_ABS, node, 0x24, 0);
    i960_st_u32(I960_ABS, g6, 0, 0);

    /* @0x2C484–0x2C494: snapshot 0x2c/30/34 → 0xc4/c8/cc. */
    g4 = i960_ld_u32(I960_ABS, node, 0x2c);
    g5 = i960_ld_u32(I960_ABS, node, 0x30);
    i960_st_u32(I960_ABS, node, 0xc4, g4);
    i960_st_u32(I960_ABS, node, 0xc8, g5);
    i960_st_u32(I960_ABS, node, 0xcc, i960_ld_u32(I960_ABS, node, 0x34));

    /* @0x2C498–0x2C50C: project car XZ onto span segment → t. */
    {
        u32 e4 = i960_ld_u32(I960_ABS, node, 0xe4);
        u32 row = span_base + (e4 << 4);
        u32 dx = f_sub(i960_ld_u32(I960_ABS, row, 0x10),
                       i960_ld_u32(I960_ABS, row, 0));
        u32 dz = f_sub(i960_ld_u32(I960_ABS, row, 0x18),
                       i960_ld_u32(I960_ABS, row, 8));
        u32 rx = f_sub(i960_ld_u32(I960_ABS, g1, 0),
                       i960_ld_u32(I960_ABS, row, 0));
        u32 rz = f_sub(i960_ld_u32(I960_ABS, g0, 0),
                       i960_ld_u32(I960_ABS, row, 8));
        u32 num = f_add(f_mul(dx, rx), f_mul(dz, rz));
        u32 den = f_add(f_mul(dx, dx), f_mul(dz, dz));

        frame_st(frame, 0x70, dx);
        frame_st(frame, 0x78, dz);
        frame_st(frame, 0x80, rx);
        frame_st(frame, 0x88, rz);
        g6 = f_div(num, den);
    }

    t = i960_u32_to_f64(g6);
    if (!(t >= 0.0))
        t = 0.0;
    t2 = t;

    /* @0x2C528–0x2C578: t>1 → advance 0x88 halfword + e4 with wrap. */
    if (t2 > 1.0) {
        i32 ti = (i32)t2;
        i32 idx88;
        i32 e4;

        g5 = node + 0x8au;
        idx88 = (i32)(int16_t)i960_ld_u16(I960_ABS, g5, 0);
        idx88 += ti;
        /* addo wrap,g4; remi wrap,g4 — keep non-negative remainder. */
        if (wrap_n != 0u) {
            idx88 = (i32)wrap_n + idx88;
            idx88 = idx88 % (i32)wrap_n;
            if (idx88 < 0)
                idx88 += (i32)wrap_n;
        }
        i960_st_u16(I960_ABS, node, 0x88, (u16)idx88);

        e4 = (i32)i960_ld_u32(I960_ABS, node, 0xe4);
        e4 += ti;
        if (e4 >= (i32)wrap_n)
            e4 -= (i32)wrap_n;
        i960_st_u32(I960_ABS, node, 0xe4, (u32)e4);

        t2 = t2 - (double)ti;
    }

    /* @0x2C57C–0x2C5C8: t' = t+0.5; if t'>1 use e4+2 and (t'-1), else e4+1. */
    {
        u32 e4 = i960_ld_u32(I960_ABS, node, 0xe4);
        double tp = t2 + 0.5;

        g7 = e4 + 1u;
        t2 = tp;
        if (tp > 1.0) {
            t2 = tp - 1.0;
            g7 = e4 + 2u;
        }
    }

    /* @0x2C5CC–0x2C60C: scale @ ac * const → node+0xb0; keep (1-t2). */
    {
        u32 ac = i960_ld_u32(I960_ABS, node, 0xac);
        double k = i960_rifl_read(0xd714cf45u, 0x3f72f684u);
        u32 sc = (u32)i960_f64_to_u32(i960_u32_to_f64(ac) * k);

        r4 = node + 0xb0u;
        i960_st_u32(I960_ABS, r4, 0, sc);
        g0 = (u32)i960_f64_to_u32(1.0 - t2);
    }

    /* @0x2C610–0x2C670: lerp span[g7]→span[g7] next * weights → target xyz @ fp. */
    {
        u32 row0 = span_base + (g7 << 4);
        u32 row1 = span_base + 0x10u + (g7 << 4);
        u32 w0 = g0;
        u32 w1 = (u32)i960_f64_to_u32(t2);
        u32 x0 = i960_ld_u32(I960_ABS, row0, 0);
        u32 y0 = i960_ld_u32(I960_ABS, row0, 4);
        u32 z0 = i960_ld_u32(I960_ABS, row0, 8);
        u32 x1 = i960_ld_u32(I960_ABS, row1, 0);
        u32 y1 = i960_ld_u32(I960_ABS, row1, 4);
        u32 z1 = i960_ld_u32(I960_ABS, row1, 8);
        u32 lx = f_mul(w0, x0);
        u32 ly = f_mul(w0, y0);
        u32 lz = f_mul(w0, z0);

        g7 = f_add(f_mul(w1, x1), lx);
        g5 = f_add(f_mul(w1, y1), ly);
        g6 = f_add(f_mul(w1, z1), lz);
        frame_st(frame, 0x40, g7);
        frame_st(frame, 0x44, g5);
        frame_st(frame, 0x48, g6);
    }

    /* @0x2C674–0x2C69C: delta = target − current xyz. */
    g2 = node + 0x14u;
    g3 = node + 0x1cu;
    g13 = node + 0x18u;
    g7 = f_sub(frame_ld(frame, 0x40), i960_ld_u32(I960_ABS, g2, 0));
    g6 = f_sub(frame_ld(frame, 0x48), i960_ld_u32(I960_ABS, g3, 0));
    g5 = f_sub(frame_ld(frame, 0x44), i960_ld_u32(I960_ABS, g13, 0));
    frame_st(frame, 0x40, g7);
    frame_st(frame, 0x48, g6);
    frame_st(frame, 0x44, g5);

    /* @0x2C6A0–0x2C6F8: TGP 0x2f normalize → node+0x9c/a0/a4. */
    i960_mmio_write_u32(0x884000, 0x17802f2fu);
    i960_mmio_write_u32(0x884000, g7);
    i960_mmio_write_u32(0x884000, g5);
    i960_mmio_write_u32(0x884000, g6);
    g4 = i960_mmio_read_u32(0x884000);
    frame_st(frame, 0x44, g5);
    g5 = node + 0x9cu;
    i960_st_u32(I960_ABS, g5, 0, g4);
    g4 = i960_mmio_read_u32(0x884000);
    g1 = node + 0xa0u;
    i960_st_u32(I960_ABS, g1, 0, g4);
    g4 = i960_mmio_read_u32(0x884000);
    g6 = node + 0xa4u;
    i960_st_u32(I960_ABS, g6, 0, g4);

    /* @0x2C6FC–0x2C734: unit * scale@b0 → vel 0x2c/30/34. */
    g4 = f_mul(i960_ld_u32(I960_ABS, g5, 0), i960_ld_u32(I960_ABS, r4, 0));
    g0 = node + 0x2cu;
    i960_st_u32(I960_ABS, g0, 0, g4);
    g4 = f_mul(i960_ld_u32(I960_ABS, g6, 0), i960_ld_u32(I960_ABS, r4, 0));
    g7 = node + 0x34u;
    i960_st_u32(I960_ABS, g7, 0, g4);
    g4 = f_mul(i960_ld_u32(I960_ABS, g1, 0), i960_ld_u32(I960_ABS, r4, 0));
    g6 = node + 0x30u;
    i960_st_u32(I960_ABS, g6, 0, g4);

    /* @0x2C738–0x2C768: xyz += vel; clear node+0x12. */
    g4 = f_add(i960_ld_u32(I960_ABS, g2, 0), i960_ld_u32(I960_ABS, g0, 0));
    i960_st_u32(I960_ABS, g2, 0, g4);
    g4 = f_add(i960_ld_u32(I960_ABS, g3, 0), i960_ld_u32(I960_ABS, g7, 0));
    i960_st_u32(I960_ABS, g3, 0, g4);
    g4 = f_add(i960_ld_u32(I960_ABS, g13, 0), i960_ld_u32(I960_ABS, g6, 0));
    i960_st_u32(I960_ABS, g13, 0, g4);
    i960_st_u16(I960_ABS, node, 0x12, 0);

    {
        static int logged_m3;

        if (!logged_m3) {
            lift_log(
                    "lift: car_frame e8=3 node=%#x xyz=(%.3g,%.3g,%.3g) "
                    "ang=(%.3g,%.3g,%.3g) e4=%u\n",
                    node,
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x14)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x18)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x1c)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x38)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x3c)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x40)),
                    i960_ld_u32(I960_ABS, node, 0xe4));
            fflush(stderr);
            logged_m3 = 1;
        }
    }
}

void game_start_race_obj_car_frame(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x90];
    u32 node = arg0;
    u32 course;
    u32 span_base;
    u32 wrap_n;
    u32 e4;
    u32 flag;
    u32 e8;
    u32 mode;
    u32 sel;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (node == 0u)
        return;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    /* @0x2BCF0: lda 0x50(sp),sp — private frame (host needs 0x88 temps). */
    sp = sp + 0x50u;

    if (!logged) {
        lift_log( "lift: race_obj_car_frame node=%#x e8=%u\n", node,
                i960_ld_u32(I960_ABS, node, 0xe8));
        fflush(stderr);
        logged = 1;
    }

    /* @0x2BCF8: bal pose_publish — refresh 0x213b40 after cam epilogue. */
    game_start_race_obj_pose_publish(node, 0, 0);

    /* @0x2BD00: bal car_peer. */
    g0 = node;
    game_start_race_obj_car_peer(node, 0, 0);

    /*
     * @0x2BD04–0x2BD78: node+0xb0 vs ~0.464 → friction pair @ 0x20cae4/e8;
     * course span base from 0x5dce7c[course]; wrap count @ 0x5dce70.
     */
    {
        u32 b0 = i960_ld_u32(I960_ABS, node, 0xb0);
        double thr_v = i960_u32_to_f64(b0);
        double thr = i960_rifl_read(0x901083dcu, 0x3fdda12fu);

        course = i960_host_race_course_index();
        span_base = model2_workram_mirror_u32(0x5dce7cu + (course << 4));
        /* @0x2BD40: ld 0x5dce70[course*16],r7 — wrap count word. */
        wrap_n = i960_ld_u32(I960_WORKRAM, 0x5dce70u + (course << 4), 0);

        if (thr_v < thr) {
            i960_st_u32(I960_WORKRAM, 0x20cae4, 0, 0x3f7eb852u);
            i960_st_u32(I960_WORKRAM, 0x20cae8, 0, 0x3f733333u);
        } else {
            i960_st_u32(I960_WORKRAM, 0x20cae4, 0, 0x3f75c28fu);
            i960_st_u32(I960_WORKRAM, 0x20cae8, 0, 0x3f75c28fu);
        }
    }

    /* @0x2BD80–0x2BDA4: span row byte → 0x5cabc0 → 0x20cae0; clear 0x20cac8. */
    e4 = i960_ld_u32(I960_ABS, node, 0xe4);
    {
        u8 row_b = i960_ld_u8(I960_ABS, span_base + (e4 << 4) + 0xcu, 0);
        u32 idx = ((u32)row_b << 2) & 28u;

        i960_st_u32(I960_WORKRAM, 0x20cac8, 0, 0);
        /* ROM table @ 0x5cabc0 (mirror); do not trust workram overlay. */
        i960_st_u32(I960_WORKRAM, 0x20cae0, 0,
                    model2_workram_mirror_u32(0x5cabc0u + idx));
    }

    /* @0x2BDAC–0x2BDB8: compare e8 vs (flags>>6); match → skip state machine. */
    flag = i960_ld_u8(I960_ABS, node + 0x11u, 0);
    e8 = i960_ld_u32(I960_ABS, node, 0xe8);
    mode = flag >> 6;
    if (e8 == mode)
        goto after_mode;

    /* @0x2BDBC–0x2BDD8: require scale sum > 0. */
    {
        u32 s0 = i960_ld_u32(I960_ABS, node, 0x64);
        u32 s1 = i960_ld_u32(I960_ABS, node, 0x68);
        double sum = i960_u32_to_f64(s0) + i960_u32_to_f64(s1);

        if (!(sum > 0.0))
            goto after_mode;
    }

    /* @0x2BDDC–0x2BDFC: mode select. */
    sel = mode;
    if ((i32)e8 > (i32)mode)
        sel = e8 - 1u;
    i960_st_u32(I960_WORKRAM, 0x20cac8, 0, 1u);

    if (sel == 0u) {
        /* @0x2BE0C–0x2BE24 — g5 stays 0 into st e8. */
        i960_st_u32(I960_ABS, node, 0xe0, 0);
        i960_st_u32(I960_ABS, node, 0x44, 0);
        i960_st_u32(I960_ABS, node, 0x48, 0);
        i960_st_u32(I960_ABS, node, 0x4c, 0);
        i960_st_u32(I960_ABS, node, 0xd0, 0);
        i960_st_u32(I960_ABS, node, 0xd4, 0);
        mode = 0;
    } else if (sel == 2u && i960_ld_u32(I960_ABS, node, 0xe8) == 3u) {
        /* @0x2BE28–0x2BE68 + xyz blend @ 0x2BE6C–0x2BF2C → g5=1. */
        u32 xyz = node + 0x14u;
        double k = i960_rifl_read(0x9999999au, 0x3fd99999u);

        g0 = i960_host_race_course_index();
        g1 = (u32)(i32)(int16_t)i960_ld_u16(I960_ABS, node, 0x88);
        g2 = xyz;
        g3 = 0;
        geo_view_table_index_a((void *)(uintptr_t)g0, (u32)g1,
                               (void *)(uintptr_t)g2);
        g0 = xyz;
        g2 = 0;
        geo_view_table_index_b((void *)(uintptr_t)g0, frame + 0x40u, 0);
        g0 = (u32)(uintptr_t)(frame + 0x40u);
        g1 = node + 0x2cu;
        g2 = node + 0x38u;
        game_start_race_obj_road_angles(frame + 0x40u, (u32)g1, (u32)g2);

        /* @0x2BE6C–0x2BF2C: blend fp samples into xyz (mulrl × 0.4). */
        {
            double dk = k;
            u32 px = (u32)i960_f64_to_u32(
                i960_u32_to_f64(i960_ld_u32(I960_ABS, xyz, 0))
                + i960_u32_to_f64(frame_ld(frame, 0x4c)) * dk);
            /* y += sample58 + 0.4*sample50 (disasm addrl order). */
            u32 py = (u32)i960_f64_to_u32(
                i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x18))
                + (i960_u32_to_f64(frame_ld(frame, 0x58))
                   + i960_u32_to_f64(frame_ld(frame, 0x50)) * dk));
            u32 pz = (u32)i960_f64_to_u32(
                i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x1c))
                + i960_u32_to_f64(frame_ld(frame, 0x54)) * dk);

            i960_st_u32(I960_ABS, xyz, 0, px);
            i960_st_u32(I960_ABS, node, 0x18, py);
            i960_st_u32(I960_ABS, node, 0x1c, pz);
        }
        mode = 1u;
    } else if (sel == 1u || sel == 2u) {
        mode = 1u;
    } else if (sel == 3u) {
        /* @0x2BF38–0x2BF58 — g5 stays 3 into st e8. */
        i960_st_u32(I960_ABS, node, 0x70, 0x3e4ccccdu);
        i960_st_u32(I960_ABS, node, 0x6c, 0x3e4ccccdu);
        i960_st_u32(I960_ABS, node, 0x68, 0x3e4ccccdu);
        i960_st_u32(I960_ABS, node, 0x64, 0x3e4ccccdu);
        mode = 3u;
    } else {
        /* @0x2BF5C — clear a8; st g5 (sel) to e8. */
        i960_st_u32(I960_ABS, node, 0xa8, 0);
        mode = sel;
    }
    i960_st_u32(I960_ABS, node, 0xe8, mode);

after_mode:
    /* @0x2BF64–0x2BFA0: bank word → node+0xd8. */
    {
        u32 pose = i960_ld_u32(I960_ABS, node, 0x8c);
        u32 pen = i960_ld_u32(I960_WORKRAM, 0x2020c0, 0) & 7u;
        u32 p51 = i960_ld_u8(I960_ABS, pose + 0x51u, 0);
        u32 bank = ((p51 << 3) & 0x1f8u);
        u32 dst = node + 0xd8u;
        u32 w = i960_ld_u32(I960_WORKRAM, 0x20cb00u + bank + (pen << 2), 0);

        i960_st_u32(I960_ABS, dst, 0, w);
        if ((i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0) <= 0)
            i960_st_u32(I960_ABS, dst, 0, 0);
    }

    /* @0x2BFA4–0x2C76C: e8 dispatch. Practice seed leaves e8==0.
     * Disasm: mov r5,g0; mov r6,g1 before every call. r6 is still
     * span_base from 0x5dce7c[course*16] (@0x2BD30). lda 0xd8(r5),g6 is
     * only the bank-word store at 0x2BF74 (g6, not r6). Motion indexes
     * (g1)[e4*16] span rows; integrate ldob 0xc(r12)[e4*16] is the same
     * table. Passing node+0xd8 read the car object as road verts. */
    e8 = i960_ld_u32(I960_ABS, node, 0xe8);
    if (e8 == 0u) {
        g0 = node;
        g1 = span_base;
        game_start_race_obj_road_span((u32)g0, (u32)g1, 0);
        g0 = node;
        game_start_race_obj_car_yaw_step(node, 0, 0);
        g0 = node;
        g1 = span_base;
        game_start_race_obj_car_motion(node, span_base, 0);
        g0 = node;
        g1 = span_base;
        game_start_race_obj_car_integrate(node, span_base, 0);
    } else if (e8 == 1u) {
        g0 = node;
        g1 = span_base;
        game_start_race_obj_road_span((u32)g0, (u32)g1, 0);
        g0 = node;
        game_start_race_obj_car_yaw_step(node, 0, 0);
        g0 = node;
        g1 = span_base;
        game_start_race_obj_car_motion(node, span_base, 0);
        g0 = node;
        g1 = span_base;
        i960_call_rom(0x2e650);
    } else if (e8 == 2u) {
        g0 = node;
        g1 = span_base;
        game_start_race_obj_road_span((u32)g0, (u32)g1, 0);
        /* @0x2C02C–0x2C438 still unlifted (TGP + call 0x2EEF0). */
    } else if (e8 == 3u) {
        car_frame_e8_mode3(node, span_base, wrap_n, frame);
    }

    fp = fp_save;
    sp = sp_save;
}
