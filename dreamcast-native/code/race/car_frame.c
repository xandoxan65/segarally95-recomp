/* The car's per-tick update (0x2BCF0), run for each car node by the race list walk.
 *
 * Publishes the pose, finds the neighbouring cars, picks the grip pair and the lateral limit from the road
 * surface under the car, then runs the drive mode node+0xe8: 0 on the road (span, yaw, motion, integrate), 1 and
 * 2 the scripted modes, 3 locked to the span line. A mode change requested through flags node+0x11 resets state
 * first. `frame` is the original's 0xa0-byte frame, which road_span and road_span_basis also write. */
#include "race/car.h"
#include "race/course.h"
#include "race/fp.h"
#include "race/road.h"
#include "sr_tgp.h"
#include <stdio.h>
#include <stdlib.h>

/* Modes 1 and 2 and the span-snap angles are not ported yet: practice never selects them. */
static void not_ported(const char *what) {
    static int reported;
    if(!reported++) printf("sr: %s is not ported yet\n", what);
}

void sr_car_e8_mode1(uint32_t node) { (void)node; not_ported("car mode 1 (0x2E650)"); }
void sr_car_e8_mode2(uint32_t node, uint32_t spans) { (void)node; (void)spans; not_ported("car mode 2 (0x2EEF0)"); }
void sr_road_angles(uint8_t *sample, uint32_t vel_va, uint32_t ang_va) {
    (void)sample; (void)vel_va; (void)ang_va;
    not_ported("road angles (0x2B6E0)");
}

/* Mode 3 (0x2C440): follow the span line at the speed node+0xd8. */
static void car_mode3(uint32_t node, uint32_t spans, uint32_t wrap_n, uint8_t *frame) {
    uint32_t e4, row, g7, g0, r[3], unit[3];
    double t, t2;
    sr_st32(node + 0xac, sr_ld32(node + 0xd8));
    sr_st32(node + CAR_POS, fadd(sr_ld32(node + CAR_POS), sr_ld32(node + 0x20)));
    sr_st32(node + CAR_POS + 8, fadd(sr_ld32(node + CAR_POS + 8), sr_ld32(node + 0x28)));
    sr_st32(node + 0x20, 0);
    sr_st32(node + 0x24, 0);
    sr_st32(node + 0x28, 0);
    sr_st32(node + 0xc4, sr_ld32(node + CAR_VEL));
    sr_st32(node + 0xc8, sr_ld32(node + CAR_VEL + 4));
    sr_st32(node + 0xcc, sr_ld32(node + CAR_VEL + 8));

    e4 = sr_ld32(node + CAR_SPAN);
    row = spans + (e4 << 4);
    {
        const uint32_t dx = fsub(sr_ld32(row + 0x10), sr_ld32(row)), dz = fsub(sr_ld32(row + 0x18), sr_ld32(row + 8));
        const uint32_t rx = fsub(sr_ld32(node + CAR_POS), sr_ld32(row));
        const uint32_t rz = fsub(sr_ld32(node + CAR_POS + 8), sr_ld32(row + 8));
        frame_st(frame, 0x70, dx);
        frame_st(frame, 0x78, dz);
        frame_st(frame, 0x80, rx);
        frame_st(frame, 0x88, rz);
        t = D(fdiv(fadd(fmul(dx, rx), fmul(dz, rz)), fadd(fmul(dx, dx), fmul(dz, dz))));
    }
    if(!(t >= 0.0)) t = 0.0;
    t2 = t;
    if(t2 > 1.0) {
        const int32_t ti = (int32_t)t2;
        uint16_t acc;
        int32_t wrapped, idx;
        acc = (uint16_t)(sr_ld16(node + 0x8a) + (uint32_t)ti);
        sr_st16(node + 0x8a, acc);
        wrapped = (int16_t)sr_ld16(node + 0x8a);
        if(wrap_n) {
            wrapped += (int32_t)wrap_n;
            wrapped %= (int32_t)wrap_n;
        }
        sr_st16(node + CAR_SPAN_ACC, (uint32_t)wrapped);
        idx = (int32_t)sr_ld32(node + CAR_SPAN) + ti;
        if(idx >= (int32_t)wrap_n) idx -= (int32_t)wrap_n;
        sr_st32(node + CAR_SPAN, (uint32_t)idx);
        t2 = t2 - (double)ti;
    }
    {
        const double tp = t2 + 0.5;
        e4 = sr_ld32(node + CAR_SPAN);
        g7 = e4 + 1;
        t2 = tp;
        if(tp > 1.0) {
            t2 = tp - 1.0;
            g7 = e4 + 2;
        }
    }
    sr_st32(node + 0xb0, F(D(sr_ld32(node + 0xac)) * sr_d(0xd714cf45u, 0x3f72f684u)));
    g0 = F(1.0 - t2);
    {
        const uint32_t row0 = spans + (g7 << 4), row1 = row0 + 0x10, w1 = F(t2);
        const uint32_t tx = fadd(fmul(w1, sr_ld32(row1)), fmul(g0, sr_ld32(row0)));
        const uint32_t ty = fadd(fmul(w1, sr_ld32(row1 + 4)), fmul(g0, sr_ld32(row0 + 4)));
        const uint32_t tz = fadd(fmul(w1, sr_ld32(row1 + 8)), fmul(g0, sr_ld32(row0 + 8)));
        r[0] = fsub(tx, sr_ld32(node + CAR_POS));
        r[2] = fsub(tz, sr_ld32(node + CAR_POS + 8));
        r[1] = fsub(ty, sr_ld32(node + CAR_POS + 4));
        frame_st(frame, 0x40, r[0]);
        frame_st(frame, 0x48, r[2]);
        frame_st(frame, 0x44, r[1]);
    }
    sr_tgp_cmd(0x17802f2fu, r, 3, unit, 3);
    sr_st32(node + 0x9c, unit[0]);
    sr_st32(node + 0xa0, unit[1]);
    sr_st32(node + 0xa4, unit[2]);
    sr_st32(node + CAR_VEL, fmul(unit[0], sr_ld32(node + 0xb0)));
    sr_st32(node + CAR_VEL + 8, fmul(unit[2], sr_ld32(node + 0xb0)));
    sr_st32(node + CAR_VEL + 4, fmul(unit[1], sr_ld32(node + 0xb0)));
    sr_st32(node + CAR_POS, fadd(sr_ld32(node + CAR_POS), sr_ld32(node + CAR_VEL)));
    sr_st32(node + CAR_POS + 8, fadd(sr_ld32(node + CAR_POS + 8), sr_ld32(node + CAR_VEL + 8)));
    sr_st32(node + CAR_POS + 4, fadd(sr_ld32(node + CAR_POS + 4), sr_ld32(node + CAR_VEL + 4)));
    sr_st16(node + 0x12, 0);
}

void sr_car_frame(uint32_t node) {
    uint8_t frame[0xa0];
    uint32_t course, spans, wrap_n, flag, e8, mode, sel;
    if(node == 0) return;
    memset(frame, 0, sizeof(frame));

    sr_pose_publish(node);
    sr_car_peer(node);

    /* Grip pair by speed scale; this course's span table and span count. */
    course = sr_race_course_index();
    spans = sr_rom_mirror32(0x5dce7cu + (course << 4));
    wrap_n = sr_ld32(0x5dce70u + (course << 4));
    if(D(sr_ld32(node + 0xb0)) < sr_d(0x901083dcu, 0x3fdda12fu)) {
        sr_st32(0x20cae4u, 0x3f7eb852u);
        sr_st32(0x20cae8u, 0x3f733333u);
    } else {
        sr_st32(0x20cae4u, 0x3f75c28fu);
        sr_st32(0x20cae8u, 0x3f75c28fu);
    }
    /* Lateral limit by the surface class of the current span row. */
    {
        const uint32_t idx = ((uint32_t)sr_ld8(spans + (sr_ld32(node + CAR_SPAN) << 4) + 0xc) << 2) & 28u;
        sr_st32(0x20cac8u, 0);
        sr_st32(0x20cae0u, sr_rom_mirror32(0x5cabc0u + idx));
    }

    flag = sr_ld8(node + CAR_FLAGS11);
    e8 = sr_ld32(node + 0xe8);
    mode = flag >> 6;
    if(e8 != mode && D(sr_ld32(node + 0x64)) + D(sr_ld32(node + 0x68)) > 0.0) {
        sel = (int32_t)e8 > (int32_t)mode ? e8 - 1 : mode;
        sr_st32(0x20cac8u, 1);
        if(sel == 0) {
            sr_st32(node + 0xe0, 0);
            sr_st32(node + 0x44, 0);
            sr_st32(node + 0x48, 0);
            sr_st32(node + 0x4c, 0);
            sr_st32(node + 0xd0, 0);
            sr_st32(node + 0xd4, 0);
            mode = 0;
        } else if(sel == 2 && sr_ld32(node + 0xe8) == 3) {
            /* Leaving the span line: snap onto the road surface and take its angles. */
            const uint32_t xyz = node + CAR_POS;
            const double k = sr_d(0x9999999au, 0x3fd99999u);
            sr_road_query_a(sr_race_course_index(), (uint32_t)(int32_t)(int16_t)sr_ld16(node + CAR_SPAN_ACC), sr_at(xyz),
                            0);
            sr_road_query_b(sr_at(xyz), frame + 0x40, 0);
            sr_road_angles(frame + 0x40, node + CAR_VEL, node + CAR_ANG);
            {
                const uint32_t px = F(D(sr_ld32(xyz)) + D(frame_ld(frame, 0x4c)) * k);
                const uint32_t py = F(D(sr_ld32(node + 0x18)) + (D(frame_ld(frame, 0x58)) + D(frame_ld(frame, 0x50)) * k));
                const uint32_t pz = F(D(sr_ld32(node + 0x1c)) + D(frame_ld(frame, 0x54)) * k);
                sr_st32(xyz, px);
                sr_st32(node + 0x18, py);
                sr_st32(node + 0x1c, pz);
            }
            mode = 1;
        } else if(sel == 1 || sel == 2) {
            mode = 1;
        } else if(sel == 3) {
            sr_st32(node + 0x70, 0x3e4ccccdu);
            sr_st32(node + 0x6c, 0x3e4ccccdu);
            sr_st32(node + 0x68, 0x3e4ccccdu);
            sr_st32(node + 0x64, 0x3e4ccccdu);
            mode = 3;
        } else {
            sr_st32(node + CAR_LATERAL, 0);
            mode = sel;
        }
        sr_st32(node + 0xe8, mode);
    }

    /* Target speed from the gear/throttle bank, zero before GO. */
    {
        const uint32_t pose = sr_ld32(node + CAR_POSE);
        const uint32_t pen = sr_ld32(0x2020c0u) & 7u;
        const uint32_t bank = ((uint32_t)sr_ld8(pose + 0x51) << 3) & 0x1f8u;
        sr_st32(node + 0xd8, sr_ld32(0x20cb00u + bank + (pen << 2)));
        if((int32_t)sr_ld32(0x214120u) <= 0) sr_st32(node + 0xd8, 0);
    }

    e8 = sr_ld32(node + 0xe8);
    if(e8 == 0) {
        sr_road_span(node, spans, frame);
        sr_car_yaw_step(node);
        sr_car_motion(node, spans);
        sr_car_integrate(node, spans);
    } else if(e8 == 1) {
        sr_road_span(node, spans, frame);
        sr_car_yaw_step(node);
        sr_car_motion(node, spans);
        sr_car_e8_mode1(node);
    } else if(e8 == 2) {
        sr_car_e8_mode2(node, spans);
    } else if(e8 == 3) {
        car_mode3(node, spans, wrap_n, frame);
    }
}
