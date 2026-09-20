/* Cam list-node per-frame prep @ 0x34380 — guest node + host frame temps.
 *
 * Callx from geo_view_scene_frame (list walk handler 0x5d3f40 → ROM 0x34f40):
 *   g0 = pool node guest VA (e.g. 0x500000)
 *   g1 = caller's fp+0x130 (host)
 *   g2 = caller's fp+0x140 (host)
 *
 * Auto-lift treated all three as host pointers (SIGSEGV at guest 0x500088).
 * Keep the caller's fp (lda 0x100(sp) only) so 0x48 temps at 0x70(fp) reach
 * scene_frame's 0x2c at 0x35EC0.
 *
 * source: disasm/maincpu/maincpu_034380_958.asm */
// @rom 0x34380 +0x958 geo_view_scene_prep

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"
#include "i960_host.h"

#include "lift_syms.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static u32 frame_ld(const u8 *frame, u32 off)
{
    u32 v;

    memcpy(&v, frame + off, sizeof(v));
    return v;
}

static void frame_st(u8 *frame, u32 off, u32 v)
{
    memcpy(frame + off, &v, sizeof(v));
}

static u32 fadd(u32 a, u32 b)
{
    double s = i960_u32_to_f64(a) + i960_u32_to_f64(b);

    if (!isfinite(s))
        return a;
    return (u32)i960_f64_to_u32(s);
}

static u32 fsub(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) - i960_u32_to_f64(b));
}

static u32 fmul(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) * i960_u32_to_f64(b));
}

void * geo_view_scene_prep(void *arg0, void *arg1, void *arg2)
{
    u32 node_va = (u32)(uintptr_t)arg0;
    u8 *node = model2_ram_mut(node_va);
    u32 *a1 = (u32 *)arg1;
    u32 *a2 = (u32 *)arg2;
    /*
     * Disasm @ 0x34380: lda 0x100(sp),sp only — fp stays the caller's
     * (scene_frame's 0x220 host frame). 0x48 writes fp+0x70/4c/58/64; the
     * caller 0x2c's those at 0x35EC0. A private fp left the caller's 0x70
     * as memset-0, so vel-reproject / pitch used origin lever arms.
     */
    u8 *frame;
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u32 tab;
    u32 i;
    u32 course;
    u32 table_index;
    u32 max_word = 0;
    double reduction = 0.0;

    if (!node || !a1 || !a2 || fp == 0) {
        g0 = (uintptr_t)arg0;
        return arg0;
    }

    frame = (u8 *)fp;
    sp += 0x100u;

    r13 = node_va;
    course = i960_host_race_course_index();
    /* Disasm @ 0x343C4: ldis 0x88(g0),g11 — signed road index. */
    table_index = (u32)(i32)(int16_t)i960_ld_u16(I960_ABS, node_va, 0x88);
    g10 = course;
    g11 = table_index;

    /* @0x343B8–0x343E8: four 0x42 bursts from ROM-mirrored table @ 0x5d2e38. */
    tab = 0x5d2e38u;
    for (i = 0; i < 4u; i++) {
        i960_mmio_write_u32(0x884000, 0x21004242u);
        i960_mmio_write_u32(0x884000, i);
        i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, tab - 8u, 0));
        i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, tab - 4u, 0));
        i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, tab, 0));
        tab += 12u;
    }

    /* @0x343EC–0x34480: push view, yaw/pitch/roll from a2 (then 0x48/0x43). */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, a2[1]);
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, a2[0]);
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    i960_mmio_write_u32(0x884000, a2[2]);
    /*
     * @0x34488–0x346A0: 0x48(i) then 0x43(i) for slots 0..3, then add a1.
     * ROM pipelines 0x48(i+1) under the add of slot i (st g12=0x48 at
     * 0x344D0 with g13=1, then 0x43(1) at 0x34524). Host 0x48/0x43 are
     * synchronous so 0x48/0x43/read/add per i is the same dataflow.
     * A prior unrolling issued 0x43(1) where ROM issues 0x48(1), so only
     * slot 0 was rotated. Once 0x48 became in-place, a0 vs a4 (front/rear
     * ride heights) diverged after any nonzero cam R and pitch wound into
     * the air — 213974==0xf skips the damper.
     * First index is ROM `st g14` with g14 held at 0 (scene_frame clears it
     * and fov_scale also writes 0). Host callees can leave a link value in
     * g14 — 0x48 would then transform the wrong vslot (stale rotated AABB)
     * so a0 vs a4 diverge and MAIN extra launches follow Y.
     */
    g14 = 0;
    for (i = 0; i < 4u; i++) {
        u32 read_off = 0x70u + i * 12u;
        u32 sum_off = 0x40u + i * 12u;
        u32 sx;
        u32 sy;
        u32 sz;

        i960_mmio_write_u32(0x884000, 0x24004848u);
        i960_mmio_write_u32(0x884000, i);
        i960_mmio_write_u32(0x884000, 0x21804343u);
        i960_mmio_write_u32(0x884000, i);
        sx = i960_mmio_read_u32(0x884000);
        sy = i960_mmio_read_u32(0x884000);
        sz = i960_mmio_read_u32(0x884000);
        frame_st(frame, read_off + 0u, sx);
        frame_st(frame, read_off + 4u, sy);
        frame_st(frame, read_off + 8u, sz);
        frame_st(frame, sum_off + 0u, fadd(a1[0], sx));
        frame_st(frame, sum_off + 4u, fadd(a1[1], sy));
        frame_st(frame, sum_off + 8u, fadd(a1[2], sz));
    }

    g0 = course;
    g1 = table_index;
    g2 = (uintptr_t)(frame + 0x40);
    g3 = 0;
    i960_mmio_write_u32(0x884000, 0x10802121u);
    g14 = 0;
    i960_st_u32(I960_ABS, 0x213960, 0, (u32)g14);
    geo_view_table_index_a((void *)(uintptr_t)course, table_index, frame + 0x40);

    /* @0x346FC–0x34A50: four 0x24-byte work-table records @ 0x2138d0. */
    for (i = 0; i < 4u; i++) {
        u32 vec_off = 0x70u + i * 12u;
        u32 obj_off = 0x40u + i * 12u;
        u32 out_off = 0xa0u + i * 12u;
        /* Disasm: r9 = 0x2138d0, stride 36 (addo 31,5). */
        u32 out_va = 0x2138d0u + i * 36u;
        u32 record = out_va + 0xcu; /* 0x2138dc + i*36 */
        u32 flag;

        g0 = (uintptr_t)(frame + obj_off);
        g1 = (uintptr_t)out_va;
        g2 = 0;
        /* Guest out — vec_push / scene_frame read 0x2138d0 slots, not fp+0xd8. */
        geo_view_table_index_b(frame + obj_off, (void *)(uintptr_t)out_va, 0);

        /*
         * Disasm @ 0x34710: cmpibl 2,r5 → 0x34728 (skip extra 0x52).
         * cmpibl src1,src2 is src1<src2 (same as vec_push cmpibl 1,g6), so
         * the skip is r5>2. Extra 0x52 of corner i+1 runs for i=0,1,2 —
         * i<2 left corner 3 on corner 2's plane (false left/right height
         * split → follow yaw → desert spline X).
         */
        if (i < 3u) {
            g0 = course;
            g1 = table_index;
            g2 = (uintptr_t)(frame + obj_off + 12u);
            g3 = 0;
            geo_view_table_index_a((void *)(uintptr_t)course, table_index,
                                   frame + obj_off + 12u);
        }

        /* @0x34728: ldob (r3) after table_index_b (+ optional a). */
        flag = i960_ld_u8(I960_ABS, out_va + 0x1cu, 0);

        if ((flag & 0x80u) != 0u) {
            u32 wx = i960_ld_u32(I960_ABS, record, 0);
            u32 wy = i960_ld_u32(I960_ABS, record, 4);
            u32 wz = i960_ld_u32(I960_ABS, record, 8);
            u32 vx = frame_ld(frame, vec_off + 0u);
            u32 vy = frame_ld(frame, vec_off + 4u);
            u32 vz = frame_ld(frame, vec_off + 8u);
            u32 nx;
            u32 ny;
            u32 nz;
            double length;
            double projected;
            double factor;

            /* @0x34730–0x3474C: 213960 |= (1 << i) when bit7 hit. */
            i960_st_u32(I960_ABS, 0x213960, 0,
                        i960_ld_u32(I960_ABS, 0x213960, 0) + (1u << i));

            /* The i960's first record aliases fp+0xd0.  Keep that guest
             * stack alias explicit; host frame addresses cannot compare equal. */
            nx = fsub(fmul(wy, vz), fmul(vy, wz));
            ny = fsub(fmul(vx, wz), fmul(wx, vz));
            nz = fsub(fmul(wx, vy), fmul(vx, wy));
            if (i == 0u) {
                frame_st(frame, 0xe0, nx);
                frame_st(frame, 0xe4, ny);
                frame_st(frame, 0xe8, nz);
            }
            frame_st(frame, 0xd0, nx);
            frame_st(frame, 0xd4, ny);
            frame_st(frame, 0xd8, nz);
            if (i == 0u) {
                /*
                 * Guest fp is 0x21380c here, so fp+0xd0 aliases the first
                 * work record at 0x2138dc.  Mirror that alias explicitly
                 * while retaining a private host frame.
                 */
                i960_st_u32(I960_ABS, record, 0, nx);
                i960_st_u32(I960_ABS, record, 4, ny);
                i960_st_u32(I960_ABS, record, 8, nz);
                wx = nx;
                wy = ny;
                wz = nz;
            }

            /* The second cross product consumes all three original lanes;
             * keep them in temporaries so C evaluation order cannot feed an
             * updated component into a later lane. */
            {
                u32 cx = frame_ld(frame, 0xd0);
                u32 cy = frame_ld(frame, 0xd4);
                u32 cz = frame_ld(frame, 0xd8);

                nx = fsub(fmul(cy, wz), fmul(wy, cz));
                ny = fsub(fmul(wx, cz), fmul(cx, wz));
                nz = fsub(fmul(cx, wy), fmul(wx, cy));
            }
            frame_st(frame, 0xd0, nx);
            frame_st(frame, 0xd4, ny);
            frame_st(frame, 0xd8, nz);
            if (i == 0u) {
                i960_st_u32(I960_ABS, record, 0, nx);
                i960_st_u32(I960_ABS, record, 4, ny);
                i960_st_u32(I960_ABS, record, 8, nz);
            }

            length = sqrt(i960_u32_to_f64(nx) * i960_u32_to_f64(nx)
                          + i960_u32_to_f64(ny) * i960_u32_to_f64(ny)
                          + i960_u32_to_f64(nz) * i960_u32_to_f64(nz));
            if (length > i960_rifl_read(0xff9fdba8u, 0x380fffffu)) {
                nx = (u32)i960_f64_to_u32(i960_u32_to_f64(nx) / length);
                ny = (u32)i960_f64_to_u32(i960_u32_to_f64(ny) / length);
                nz = (u32)i960_f64_to_u32(i960_u32_to_f64(nz) / length);
            } else {
                nx = 0u;
                ny = 0u;
                nz = 0x3f800000u;
            }
            frame_st(frame, 0xd0, nx);
            frame_st(frame, 0xd4, ny);
            frame_st(frame, 0xd8, nz);
            if (i == 0u) {
                i960_st_u32(I960_ABS, record, 0, nx);
                i960_st_u32(I960_ABS, record, 4, ny);
                i960_st_u32(I960_ABS, record, 8, nz);
                wx = nx;
                wy = ny;
                wz = nz;
            }

            projected = i960_u32_to_f64(nx) * i960_u32_to_f64(vx)
                      + i960_u32_to_f64(ny) * i960_u32_to_f64(vy)
                      + i960_u32_to_f64(nz) * i960_u32_to_f64(vz);
            /*
             * ROM sqrtr of the dot @ 0x34994. Negative (bit7 plane vs AABB
             * hemisphere) is an i960 invalid op; host sqrt() yields NaN and
             * then a0/step poison follow. Same fail as the tiny-length
             * branch: projected = 0, no extra yaw.
             */
            if (!(projected == projected) || projected < 0.0)
                projected = 0.0;
            else
                projected = sqrt(projected);
            {
                double p0 = i960_u32_to_f64(i960_ld_u32(I960_ABS, record, 0x50));
                double p1 = i960_u32_to_f64(i960_ld_u32(I960_ABS, record, 0x54));
                double weight = i960_u32_to_f64(
                    i960_ld_u32(I960_ABS, out_va + 0x18u, 0));

                /* Remaining RE gap: the mulrl/divrl chain is algebraically
                 * preserved here; host double does not model i960 rounding. */
                factor = ((p0 * p1) * i960_rifl_read(0u, 0x3fe00000u))
                    / (p1 + p0 * projected * projected);
                factor *= weight;
            }
            frame_st(frame, out_off + 0u,
                     (u32)i960_f64_to_u32(i960_u32_to_f64(wx) * factor));
            frame_st(frame, out_off + 4u,
                     (u32)i960_f64_to_u32(i960_u32_to_f64(wy) * factor));
            frame_st(frame, out_off + 8u,
                     (u32)i960_f64_to_u32(i960_u32_to_f64(wz) * factor));

            {
                u32 candidate = i960_ld_u32(I960_ABS, out_va + 0x18u, 0);

                if (i960_u32_to_f64(candidate) >= i960_u32_to_f64(max_word))
                    max_word = candidate;
            }
        } else {
            frame_st(frame, out_off + 0u, 0);
            frame_st(frame, out_off + 4u, 0);
            frame_st(frame, out_off + 8u, 0);
        }
    }

    {
        static int prep_log;

        /*
         * Keep sampling the follow-cam node: boot/attract consume the first
         * two logs, so START never showed query Y vs leftover flag 15.
         */
        if (node_va == 0x500000u && prep_log < 10) {
            fprintf(stderr,
                    "lift: scene_prep cam idx=%d p0=(%.4g,%.4g,%.4g) "
                    "aabb0=(%.4g,%.4g,%.4g) h=%.4g/%.4g/%.4g/%.4g "
                    "f1c=%#x/%#x/%#x/%#x\n",
                    (int)(i32)table_index,
                    i960_u32_to_f64(frame_ld(frame, 0x40)),
                    i960_u32_to_f64(frame_ld(frame, 0x44)),
                    i960_u32_to_f64(frame_ld(frame, 0x48)),
                    i960_u32_to_f64(frame_ld(frame, 0x70)),
                    i960_u32_to_f64(frame_ld(frame, 0x74)),
                    i960_u32_to_f64(frame_ld(frame, 0x78)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, 0x2138d0u + 0x18u, 0)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, 0x2138d0u + 36u + 0x18u, 0)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, 0x2138d0u + 72u + 0x18u, 0)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, 0x2138d0u + 108u + 0x18u, 0)),
                    (unsigned)i960_ld_u32(I960_ABS, 0x2138ecu, 0),
                    (unsigned)i960_ld_u32(I960_ABS, 0x2138ecu + 36u, 0),
                    (unsigned)i960_ld_u32(I960_ABS, 0x2138ecu + 72u, 0),
                    (unsigned)i960_ld_u32(I960_ABS, 0x2138ecu + 108u, 0));
            fflush(stderr);
            prep_log++;
        }
    }

    if (i960_u32_to_f64(max_word) != 0.0) {
        /* @0x34A70–0x34B8C: form four 0x5b/0x58 reductions. */
        for (i = 0; i < 4u; i++) {
            u32 off = i * 12u;
            u32 rv;

            frame_st(frame, 0x74u + off, 0);
            frame_st(frame, 0xa4u + off, 0);
            i960_mmio_write_u32(0x884000, 0x2d805b5bu);
            i960_mmio_write_u32(0x884000, frame_ld(frame, 0x74u + off));
            i960_mmio_write_u32(0x884000, frame_ld(frame, 0xa8u + off));
            i960_mmio_write_u32(0x884000, frame_ld(frame, 0x78u + off));
            i960_mmio_write_u32(0x884000, frame_ld(frame, 0xa4u + off));
            i960_mmio_write_u32(0x884000, frame_ld(frame, 0xa0u + off));
            i960_mmio_write_u32(0x884000, frame_ld(frame, 0x70u + off));
            frame_st(frame, 0xd0, i960_mmio_read_u32(0x884000));
            frame_st(frame, 0xd4, i960_mmio_read_u32(0x884000));
            frame_st(frame, 0xd8, i960_mmio_read_u32(0x884000));

            i960_mmio_write_u32(0x884000, 0x2c005858u);
            i960_mmio_write_u32(0x884000, 0);
            i960_mmio_write_u32(0x884000, 0);
            i960_mmio_write_u32(0x884000, 0);
            i960_mmio_write_u32(0x884000, frame_ld(frame, 0xd0));
            i960_mmio_write_u32(0x884000, frame_ld(frame, 0xd4));
            i960_mmio_write_u32(0x884000, frame_ld(frame, 0xd8));
            rv = i960_mmio_read_u32(0x884000);
            reduction -= fabs(i960_u32_to_f64(rv));
        }

        {
            double step = reduction
                / i960_u32_to_f64(i960_ld_u32(I960_ABS, node_va, 0x58));
            double sx = 0.0;
            double sz = 0.0;

            i960_st_u32(I960_ABS, node_va, 0x48,
                        fadd(i960_ld_u32(I960_ABS, node_va, 0x48),
                             (u32)i960_f64_to_u32(step)));
            a2[1] = fadd(i960_ld_u32(I960_ABS, node_va, 0x3c),
                         i960_ld_u32(I960_ABS, node_va, 0x48));
            for (i = 0; i < 4u; i++) {
                sx += i960_u32_to_f64(frame_ld(frame, 0xa0u + i * 12u));
                sz += i960_u32_to_f64(frame_ld(frame, 0xa8u + i * 12u));
            }
            i960_st_u32(I960_ABS, node_va, 0x2c,
                        fadd(i960_ld_u32(I960_ABS, node_va, 0x2c),
                             (u32)i960_f64_to_u32(
                                 sx / i960_u32_to_f64(
                                     i960_ld_u32(I960_ABS, node_va, 0x50)))));
            i960_st_u32(I960_ABS, node_va, 0x34,
                        fadd(i960_ld_u32(I960_ABS, node_va, 0x34),
                             (u32)i960_f64_to_u32(
                                 sz / i960_u32_to_f64(
                                     i960_ld_u32(I960_ABS, node_va, 0x50)))));

            a1[0] = fadd(i960_ld_u32(I960_ABS, node_va, 0x14),
                         i960_ld_u32(I960_ABS, node_va, 0x2c));
            a1[2] = fadd(i960_ld_u32(I960_ABS, node_va, 0x1c),
                         i960_ld_u32(I960_ABS, node_va, 0x34));

            if (i960_ld_u32(I960_ABS, 0x2142c8, 0) == 0u) {
                i32 q = (i32)(step * i960_rifl_read(0, 0x408f4000u));

                if (q <= 0)
                    q = -q;
                i960_st_u32(I960_ABS, 0x213964, 0, (u32)q);
                i960_st_u32(I960_ABS, 0x213968, 0,
                            (u32)(i32)(step
                                      * i960_rifl_read(0, 0x40bf4000u)));
            }
        }
    }

    g0 = max_word;
    sp = sp_save;
    fp = fp_save;
    return (void *)(uintptr_t)max_word;
}
