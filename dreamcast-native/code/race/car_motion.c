/* Car road motion and steering target (0x304F0).
 *
 * Steers the lateral offset node+0xa8 towards a target: around another car close ahead/behind (0x20cac0/c4,
 * from car_peer) it is pushed to the side the other car is not on, by the course's limit 0x20cae0; otherwise it
 * eases back. The road's curvature over the next spans and the distance to the span 20 rows ahead set the
 * speed cap node+0xac through the corner state node+0xe0/0xdc. */
#include "race/car.h"
#include "race/course.h"
#include "race/fp.h"
#include "sr_tgp.h"
#include <math.h>

static void tgp0(uint32_t cmd) { sr_tgp_cmd(cmd, 0, 0, 0, 0); }

static void tgp_vec3(uint32_t cmd, const uint32_t v[3], uint32_t out[3]) { sr_tgp_cmd(cmd, v, 3, out, 3); }

static uint32_t tgp_pair(uint32_t cmd, const uint32_t a[3], const uint32_t b[3]) {
    const uint32_t in[6] = {a[0], a[1], a[2], b[0], b[1], b[2]};
    uint32_t r;
    sr_tgp_cmd(cmd, in, 6, &r, 1);
    return r;
}

void sr_car_motion(uint32_t node, uint32_t spans) {
    uint32_t first_xform[3], ref_xform[3], car_unit[3], tangent[3], cross[3] = {0, 0, 0}, delta[3];
    uint32_t r8_term, ac_value, weighted = 0, weight = 0, i;
    double road_mix;
    if(node == 0) return;

    /* The span frame stored by road_span_basis in matrix slot 0. */
    tgp0(0x10002020u);
    {
        const uint32_t cursor = sr_ld32(0x20a290u);
        sr_st32(0x20a290u, cursor + 4);
        sr_st32(cursor, 0);
        sr_tgp_arm_ext_selector(0);
    }
    tgp0(0x30806161u);
    {
        const uint32_t v[3] = {sr_ld32(node + CAR_POS), 0, sr_ld32(node + CAR_POS + 8)};
        tgp_vec3(0x16002c2cu, v, first_xform);
    }
    {
        const uint32_t ref = sr_ld32(0x20cac0u);
        const uint32_t v[3] = {sr_ld32(ref), 0, sr_ld32(ref + 8)};
        tgp_vec3(0x16002c2cu, v, ref_xform);
    }
    tgp0(0x10802121u);
    {
        const uint32_t v[3] = {sr_ld32(node + CAR_VEL), 0, sr_ld32(node + CAR_VEL + 8)};
        tgp_vec3(0x17802f2fu, v, car_unit);
    }

    /* Road curvature: the car's heading crossed with the next two span directions, weighted from 0x5cabb0. */
    for(i = 0; i < 2; i++) {
        const uint32_t row = spans + ((sr_ld32(node + CAR_SPAN) + i * 2) << 4);
        const uint32_t v[3] = {fsub(sr_ld32(row + 0x10), sr_ld32(row)), 0, fsub(sr_ld32(row + 0x18), sr_ld32(row + 8))};
        uint32_t w;
        tgp_vec3(0x17802f2fu, v, tangent);
        {
            /* 0x5b takes (Ay, Bz, Az, By, Bx, Ax): A = heading, B = span direction. */
            const uint32_t in[6] = {car_unit[1], tangent[2], car_unit[2], tangent[1], tangent[0], car_unit[0]};
            sr_tgp_cmd(0x2d805b5bu, in, 6, cross, 3);
        }
        w = sr_rom_mirror32(0x5cabb0u + i * 4);
        weighted = fadd(weighted, fmul(w, cross[1]));
        weight = fadd(weight, w);
    }
    road_mix = fabs(D(fdiv(weighted, weight)));

    {
        const uint32_t ref = sr_ld32(0x20cac4u);
        static const uint32_t zero[3] = {0, 0, 0};
        delta[0] = fsub(sr_ld32(node + CAR_POS), sr_ld32(ref));
        delta[1] = 0;
        delta[2] = fsub(sr_ld32(node + CAR_POS + 8), sr_ld32(ref + 8));
        r8_term = sr_ld32(node + CAR_LATERAL);

        if(D(tgp_pair(0x2c005858u, zero, delta)) < 30.0) {
            uint32_t v[3], old_delta[3];
            const uint32_t ref_delta[3] = {fsub(sr_ld32(ref + 0x0c), sr_ld32(node + CAR_VEL)), 0,
                                           fsub(sr_ld32(ref + 0x14), sr_ld32(node + CAR_VEL + 8))};
            tgp_vec3(0x17802f2fu, ref_delta, v);
            tgp_vec3(0x17802f2fu, delta, old_delta);
            {
                const uint32_t dot = fadd(fadd(fmul(old_delta[0], v[0]), fmul(old_delta[1], v[1])),
                                          fmul(old_delta[2], v[2]));
                if(D(dot) > 0.0) { /* the original also checks sqrt(dot) > 0, which follows */
                    cross[0] = fsub(fmul(v[1], old_delta[2]), fmul(old_delta[1], v[2]));
                    cross[1] = fsub(fmul(old_delta[0], v[2]), fmul(v[0], old_delta[2]));
                    cross[2] = fsub(fmul(v[0], old_delta[1]), fmul(old_delta[0], v[1]));
                    if(fabs(D(cross[1])) > sr_d(0xcac08312u, 0x3fc645a1u)) {
                        const uint32_t limit = sr_ld32(0x20cae0u);
                        r8_term = D(cross[1]) < 0.0 ? limit : fneg(limit);
                    }
                }
            }
        } else {
            const uint32_t limit = sr_ld32(0x20cae0u);
            r8_term = D(ref_xform[0]) < 0.0 ? limit : fneg(limit);
        }
    }

    {
        const uint32_t wrap = sr_ld32(0x5dce70u + (sr_race_course_index() << 4));
        uint32_t idx = sr_ld32(node + CAR_SPAN) + 20, row, state;
        const uint32_t car_pos[3] = {sr_ld32(node + CAR_POS), sr_ld32(node + CAR_POS + 4), sr_ld32(node + CAR_POS + 8)};
        uint32_t road_pos[3];
        double ahead;
        if(idx >= wrap) idx -= wrap;
        row = spans + (idx << 4);
        road_pos[0] = sr_ld32(row);
        road_pos[1] = sr_ld32(row + 4);
        road_pos[2] = sr_ld32(row + 8);
        ahead = D(tgp_pair(0x28005050u, car_pos, road_pos));
        if(ahead < 200.0)
            ahead = D(tgp_pair(0x28005050u, car_pos, road_pos));
        else
            ahead = 200.0;

        state = sr_ld32(node + 0xe0);
        ac_value = sr_ld32(node + 0xd8);
        if(state == 0) {
            if(road_mix > sr_d(0x9999999au, 0x3fb99999u) || ahead < 190.0) {
                const uint32_t d8 = sr_ld32(node + 0xd8);
                if(D(d8) > 0.0) {
                    sr_st32(node + 0xdc, d8);
                    sr_st32(node + 0xe0, 1);
                }
            }
        } else if(state == 1) {
            double candidate = (150.0 - road_mix * 300.0) + ahead * 0.25;
            uint32_t dc;
            if(candidate < 50.0) candidate = 50.0;
            ac_value = F(candidate);
            dc = sr_ld32(node + 0xdc);
            if(D(ac_value) < D(dc))
                sr_st32(node + 0xdc, ac_value);
            else
                ac_value = dc;
            if(road_mix < sr_d(0x9999999au, 0x3fb99999u)) sr_st32(node + 0xe0, 0);
        }
    }

    if(D(fadd(sr_ld32(node + 0x64), sr_ld32(node + 0x68))) > 0.0) {
        if((int32_t)sr_ld32(0x214120u) > 0) {
            const uint32_t old = sr_ld32(node + CAR_LATERAL);
            sr_st32(node + CAR_LATERAL, fsub(r8_term, fmul(fsub(r8_term, old), 0x3f7f7ceeu)));
        }
        sr_st32(node + 0xac, ac_value);
    } else {
        const uint32_t x = first_xform[0], limit = sr_ld32(0x20cae0u);
        uint32_t ax = x, out;
        if(D(ax) < 0.0) ax = fneg(ax);
        if(D(ax) >= D(limit))
            out = D(x) < 0.0 ? fneg(limit) : limit;
        else
            out = x;
        sr_st32(node + CAR_LATERAL, out);
        sr_st8(node + CAR_FLAGS10, sr_ld8(node + CAR_FLAGS10) & ~0x10u);
    }
}
