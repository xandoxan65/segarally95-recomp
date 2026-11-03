/* Car wall contact (0x2F8A0).
 *
 * The four corners of the car's box (table 0x5d2e30), turned by the predicted angles and placed at the predicted
 * position, are tested against walls (query flag bit 7). Each corner inside a wall gets an impulse along the wall
 * normal, scaled by the penetration and by how the corner's lever arm turns the car (0.5 m I / (m (u.l)^2 + I)).
 * The summed horizontal impulse and yaw torque are queued in node+0x20/0x28 and added to node+0x48, and the
 * predicted XZ is reset to position + velocity. Returns the deepest penetration (float bits), 0 for none. */
#include "race/car.h"
#include "race/course.h"
#include "race/fp.h"
#include "race/road.h"
#include "sr_tgp.h"

static void cross_5b(uint32_t ay, uint32_t bz, uint32_t az, uint32_t by, uint32_t bx, uint32_t ax, uint32_t out[3]) {
    const uint32_t in[6] = {ay, bz, az, by, bx, ax};
    sr_tgp_cmd(0x2d805b5bu, in, 6, out, 3);
}

uint32_t sr_car_contact_solve(uint32_t node, uint8_t *target, const uint8_t *angles) {
    uint8_t frame[0x1d0];
    uint32_t course, max_pen = 0, sum_x = 0, sum_z = 0, yaw_acc = 0, i;
    int32_t index;
    if(node == 0 || !target || !angles) return 0;
    memset(frame, 0, sizeof(frame));

    for(i = 0; i < 4; i++) {
        const uint32_t in[4] = {i, sr_ld32(0x5d2e30u + i * 12), sr_ld32(0x5d2e34u + i * 12), sr_ld32(0x5d2e38u + i * 12)};
        sr_tgp_cmd(0x21004242u, in, 4, 0, 0);
    }

    sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
    sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
    {
        uint32_t a = frame_ld(angles, 4);
        sr_tgp_cmd(0x15002a2au, &a, 1, 0, 0);
        a = frame_ld(angles, 0);
        sr_tgp_cmd(0x14802929u, &a, 1, 0, 0);
        a = frame_ld(angles, 8);
        sr_tgp_cmd(0x15802b2bu, &a, 1, 0, 0);
    }
    {
        const uint32_t px = frame_ld(target, 0), py = frame_ld(target, 4), pz = frame_ld(target, 8);
        for(i = 0; i < 4; i++) {
            const uint32_t off = i * 12;
            uint32_t l[3];
            sr_tgp_cmd(0x24004848u, &i, 1, 0, 0);
            sr_tgp_cmd(0x21804343u, &i, 1, l, 3);
            frame_st(frame, 0xa0 + off, l[0]);
            frame_st(frame, 0xa4 + off, l[1]);
            frame_st(frame, 0xa8 + off, l[2]);
            frame_st(frame, 0x40 + off, fadd(l[0], px));
            frame_st(frame, 0x44 + off, fadd(l[1], py));
            frame_st(frame, 0x48 + off, fadd(l[2], pz));
        }
    }
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);

    course = sr_race_course_index();
    index = (int16_t)sr_ld16(node + CAR_SPAN_ACC);
    for(i = 0; i < 4; i++) {
        uint8_t sample[0xe0];
        const uint32_t off = i * 12, force_off = 0x70 + i * 12;
        uint32_t flag, pen, nx, ny, nz, c1[3], c2[3], unit[3], dot, scale;
        const uint32_t lx = frame_ld(frame, 0xa0 + off), ly = frame_ld(frame, 0xa4 + off),
                       lz = frame_ld(frame, 0xa8 + off);
        memset(sample, 0, sizeof(sample));
        sr_road_query_a(course, (uint32_t)index, frame + 0x40 + off, 0);
        sr_road_query_b(frame + 0x40 + off, sample, 0);
        flag = sample[0x1c];
        pen = frame_ld(sample, 0x18);
        if(!(flag & 0x80u) || !(D(pen) > 0.0)) {
            frame_st(frame, force_off, 0);
            frame_st(frame, force_off + 4, 0);
            frame_st(frame, force_off + 8, 0);
            continue;
        }
        nx = frame_ld(sample, 0x0c);
        ny = frame_ld(sample, 0x10);
        nz = frame_ld(sample, 0x14);
        cross_5b(ny, lz, nz, ly, lx, nx, c1);          /* N x l */
        cross_5b(c1[1], nz, c1[2], ny, nx, c1[0], c2);  /* (N x l) x N */
        sr_tgp_cmd(0x17802f2fu, c2, 3, unit, 3);
        {
            const uint32_t in[6] = {unit[0], lx, unit[1], ly, unit[2], lz};
            sr_tgp_cmd(0x2c805959u, in, 6, &dot, 1);
        }
        {
            const uint32_t mass = sr_ld32(node + 0x50), iy = sr_ld32(node + 0x58);
            const uint32_t denom = fadd(fmul(fmul(mass, dot), dot), iy);
            const uint32_t half_m_iy = F(D(mass) * D(iy) * 0.5);
            scale = D(denom) == 0.0 ? 0 : fdiv(half_m_iy, denom);
        }
        scale = fmul(scale, pen);
        frame_st(frame, force_off, fmul(scale, nx));
        frame_st(frame, force_off + 4, fmul(scale, ny));
        frame_st(frame, force_off + 8, fmul(scale, nz));
        if(D(pen) > D(max_pen)) max_pen = pen;
    }
    if(!(D(max_pen) > 0.0)) return 0;

    for(i = 0; i < 4; i++) {
        const uint32_t off = i * 12;
        const uint32_t fx = frame_ld(frame, 0x70 + off), fz = frame_ld(frame, 0x78 + off);
        const uint32_t lx = frame_ld(frame, 0xa0 + off), lz = frame_ld(frame, 0xa8 + off);
        uint32_t cross[3], len;
        cross_5b(0, fz, lz, 0, fx, lx, cross);
        {
            const uint32_t in[6] = {0, 0, 0, cross[0], cross[1], cross[2]};
            sr_tgp_cmd(0x2c005858u, in, 6, &len, 1);
        }
        if(D(cross[1]) < 0.0) len = fneg(len);
        yaw_acc = fsub(yaw_acc, len);
        sum_x = fadd(sum_x, fx);
        sum_z = fadd(sum_z, fz);
    }
    {
        const double mass = D(sr_ld32(node + 0x50)), inertia = D(sr_ld32(node + 0x58));
        if(mass != 0.0) {
            const uint32_t dx = F(D(sum_x) / mass), dz = F(D(sum_z) / mass);
            sr_st32(node + 0x20, fadd(sr_ld32(node + 0x20), dx));
            sr_st32(node + 0x28, fadd(sr_ld32(node + 0x28), dz));
        }
        if(inertia != 0.0) sr_st32(node + 0x48, fadd(sr_ld32(node + 0x48), F(D(yaw_acc) / inertia)));
        frame_st(target, 0, fadd(sr_ld32(node + CAR_POS), sr_ld32(node + CAR_VEL)));
        frame_st(target, 8, fadd(sr_ld32(node + CAR_POS + 8), sr_ld32(node + CAR_VEL + 8)));
    }
    return max_pen;
}
