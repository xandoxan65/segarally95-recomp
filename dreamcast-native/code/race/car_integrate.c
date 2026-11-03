/* Car integration (0x2CCE0) and its force core (0x2CE58-0x2E344).
 *
 * Per tick: apply half the queued wall impulse, step the velocity (car_velocity), predict the next pose, solve wall
 * contacts on it (car_contact), then sample the road under the four wheels: each gives a spring/damper force along
 * the road normal. Those, the cornering force from the rear wheels' slide and the lateral slip give a linear force
 * and a torque, integrated through the mass node+0x50 and the inertias node+0x54/58/5c. Angles are wrapped and
 * pitch/roll capped at pi/8.
 *
 * The frame layout is the original's: other code reads none of it, but it keeps the port checkable line by line
 * against the lifted routine until the whole tick is native. */
#include "race/car.h"
#include "race/course.h"
#include "race/fp.h"
#include "race/road.h"
#include "sr_tgp.h"
#include "m2_math.h"
#include <math.h>

static void tgp0(uint32_t cmd) { sr_tgp_cmd(cmd, 0, 0, 0, 0); }
static void tgp1(uint32_t cmd, uint32_t a) { sr_tgp_cmd(cmd, &a, 1, 0, 0); }
static void tgp_vec3(uint32_t cmd, const uint32_t in[3], uint32_t out[3]) { sr_tgp_cmd(cmd, in, 3, out, 3); }

static uint32_t tgp_length(const uint32_t v[3]) {
    const uint32_t in[6] = {0, 0, 0, v[0], v[1], v[2]};
    uint32_t r;
    sr_tgp_cmd(0x2c005858u, in, 6, &r, 1);
    return r;
}

static void tgp_pose(const uint32_t xyz[3], uint32_t pitch, uint32_t yaw, uint32_t roll) {
    tgp0(0x12802525u);
    sr_tgp_cmd(0x13802727u, xyz, 3, 0, 0);
    tgp1(0x15002a2au, yaw);
    tgp1(0x14802929u, pitch);
    tgp1(0x15802b2bu, roll);
}

/* 0x30380: keep a point within the road's lateral limit, in the span frame (TGP matrix slots 0 and 16). */
static void xz_clamp(uint8_t *xyz) {
    uint32_t cursor = sr_ld32(0x20a290u), scratch[3];
    tgp0(0x10002020u);
    sr_st32(cursor, 0);
    tgp0(0x30806161u);
    {
        const uint32_t v[3] = {frame_ld(xyz, 0), 0, frame_ld(xyz, 8)};
        tgp_vec3(0x16002c2cu, v, scratch);
    }
    sr_st32(0x20a290u, cursor + 4);
    {
        const uint32_t xp = scratch[0], thresh = sr_ld32(0x20cae0u);
        uint32_t ax = xp;
        if(D(xp) < 0.0) ax = fneg(xp);
        if(D(ax) > D(thresh)) {
            uint32_t out[3];
            scratch[0] = D(xp) < 0.0 ? fneg(thresh) : thresh;
            cursor = sr_ld32(0x20a290u);
            sr_st32(cursor, 16);
            tgp0(0x30806161u);
            tgp_vec3(0x16002c2cu, scratch, out);
            frame_st(xyz, 0, out[0]);
            frame_st(xyz, 8, out[2]);
            sr_st32(0x20a290u, cursor + 4);
        }
    }
    tgp0(0x10802121u);
}

/* 0x30320: the road cell under the predicted position pred (host words); off the road, clamp it laterally and
 * look again, then fall back to the road point under the car. The search runs on node+0x14 as the original does,
 * so the published position is put back afterwards. */
void sr_car_index_refresh(uint32_t node, uint32_t pred[3]) {
    uint8_t *xyz = sr_at(node + CAR_POS), *index = sr_at(node + CAR_SPAN_ACC);
    uint32_t save[3], save_index, i;
    for(i = 0; i < 3; i++) {
        save[i] = frame_ld(xyz, i * 4);
        frame_st(xyz, i * 4, pred[i]);
    }
    save_index = frame_ld(index, 0);
    sr_road_cell_pack(xyz, index);
    if((int16_t)sr_ld16(node + CAR_SPAN_ACC) < 0) {
        frame_st(index, 0, save_index);
        xz_clamp(xyz);
        sr_road_cell_pack(xyz, index);
        if((int16_t)sr_ld16(node + CAR_SPAN_ACC) < 0) {
            frame_st(xyz, 0, sr_ld32(node + CAR_ROAD_XZ));
            frame_st(xyz, 4, sr_ld32(node + CAR_ROAD_Y));
            frame_st(xyz, 8, sr_ld32(node + CAR_ROAD_XZ + 8));
            frame_st(index, 0, save_index);
        }
    }
    for(i = 0; i < 3; i++) {
        pred[i] = frame_ld(xyz, i * 4);
        frame_st(xyz, i * 4, save[i]);
    }
}

/* 0x30140: one wheel's road sample and spring. The original reuses one stack slot for all four calls, so a miss
 * (flag 15, plane words untouched) springs off the previous wheel's plane: kept here as a static. */
static uint8_t s_force_slot_fp[0xa0];

static uint32_t force_slot(const uint32_t point[3], uint32_t scalar_va, uint32_t meta_va, uint32_t scale,
                           uint32_t force_out[3], uint32_t *sample_y, uint16_t *counter, uint32_t slot) {
    uint8_t *sample = s_force_slot_fp;
    uint8_t pt[12];
    uint32_t plane_x, plane_y, plane_w, flags, depth;
    double response, capped, spring, damping, depth_scale, damp_scale;
    const double global_scale = D(sr_ld32(0x214124u));

    memcpy(pt, point, 12);
    sr_road_query_b(pt, sample + 0x40, 0);
    plane_x = frame_ld(sample, 0x50);
    plane_y = frame_ld(sample, 0x54);
    depth = frame_ld(sample, 0x58);
    plane_w = frame_ld(sample, 0x4c);
    flags = sample[0x5c];
    *sample_y = depth;
    sr_st32(meta_va, gadd(point[1], depth));
    *counter = (uint16_t)(*counter | (((uint16_t)(sr_ld32(0x2141dcu) & 15u)) << (slot * 4)));
    if((flags & 0x80u) || !(D(depth) >= 0.0)) {
        sr_st32(scalar_va, 0);
        force_out[0] = force_out[1] = force_out[2] = 0;
        return 0;
    }
    depth_scale = D(sr_ld32(slot <= 1 ? 0x2141d4u : 0x2141d8u));
    damp_scale = D(sr_ld32(slot <= 1 ? 0x2141dcu : 0x2141e0u));
    capped = D(depth);
    spring = capped * depth_scale * 100.0 * global_scale;
    if(capped > 0.2) {
        spring += (capped - 0.2) * D(scale);
        capped = 0.2;
    }
    damping = (D(sr_ld32(scalar_va)) - capped) * damp_scale * global_scale * 3600.0;
    response = spring - damping;
    if(response < 0.0) response = 0.0;
    sr_st32(scalar_va, F(capped));
    force_out[0] = gmul(F(response), plane_w);
    force_out[1] = gmul(F(response), plane_x);
    force_out[2] = gmul(F(response), plane_y);
    return 1;
}

static uint32_t angle_cap(uint32_t bits) {
    double angle = D(sr_angle_wrap(bits));
    const double limit = D(0x3ec90fdbu);
    if(fabs(angle) > limit) angle = signbit(angle) ? -limit : limit;
    return F(angle);
}

static uint32_t force_core(uint32_t node, uint32_t spans, uint8_t *frame) {
    static const uint32_t corner[4][3] = {
        {0xbf228f5cu, 0xbecccccdu, 0x3fa00000u},
        {0x3f228f5cu, 0xbecccccdu, 0x3fa00000u},
        {0xbf28f5c3u, 0xbecccccdu, 0xbfa00000u},
        {0x3f28f5c3u, 0xbecccccdu, 0xbfa00000u},
    };
    uint32_t predicted[4][3], current[4][3], force[4][3], xyz[3], velocity[3], unit[3];
    uint32_t contacts = 0, course, i, lane;
    int32_t road_index;
    uint16_t slot_counter = 0;
    double side_sum = 0.0, length_sum = 0.0;

    memset(force, 0, sizeof(force));

    /* Wheel points at the predicted pose; the rear ones also at the current pose. */
    tgp0(0x10002020u);
    xyz[0] = frame_ld(frame, 0x70);
    xyz[1] = frame_ld(frame, 0x74);
    xyz[2] = frame_ld(frame, 0x78);
    tgp_pose(xyz, frame_ld(frame, 0xe0), frame_ld(frame, 0xe4), frame_ld(frame, 0xe8));
    for(i = 0; i < 4; i++) {
        tgp_vec3(0x16002c2cu, corner[i], predicted[i]);
        for(lane = 0; lane < 3; lane++) frame_st(frame, 0x40 + i * 12 + lane * 4, predicted[i][lane]);
    }
    xyz[0] = sr_ld32(node + CAR_POS);
    xyz[1] = sr_ld32(node + CAR_POS + 4);
    xyz[2] = sr_ld32(node + CAR_POS + 8);
    tgp_pose(xyz, sr_ld32(node + CAR_ANG), sr_ld32(node + CAR_ANG + 4), sr_ld32(node + CAR_ANG + 8));
    tgp_vec3(0x16002c2cu, corner[2], current[2]);
    tgp_vec3(0x16002c2cu, corner[3], current[3]);
    for(i = 2; i < 4; i++)
        for(lane = 0; lane < 3; lane++)
            frame_st(frame, 0xb0 + (i - 2) * 12 + lane * 4, gsub(predicted[i][lane], current[i][lane]));
    tgp0(0x10802121u);

    {
        uint32_t pred[3] = {frame_ld(frame, 0x70), frame_ld(frame, 0x74), frame_ld(frame, 0x78)};
        sr_car_index_refresh(node, pred);
        frame_st(frame, 0x70, pred[0]);
        frame_st(frame, 0x74, pred[1]);
        frame_st(frame, 0x78, pred[2]);
    }

    /* Wheel springs. */
    course = sr_race_course_index();
    road_index = (int16_t)sr_ld16(node + CAR_SPAN_ACC);
    for(i = 0; i < 4; i++) {
        uint32_t sample_y, hit;
        uint8_t pt[12];
        const uint32_t scale = F(D(sr_ld32(node + 0x50)) * 0.25);
        memcpy(pt, predicted[i], 12);
        sr_road_query_a(course, (uint32_t)road_index, pt, 0);
        hit = force_slot(predicted[i], node + 0x64 + i * 4, node + 0xb4 + i * 4, scale, force[i], &sample_y,
                         &slot_counter, i);
        frame_st(frame, 0x80 + i * 12, force[i][0]);
        frame_st(frame, 0x84 + i * 12, force[i][1]);
        frame_st(frame, 0x88 + i * 12, force[i][2]);
        frame_st(frame, 0x110 + i * 4, sample_y);
        contacts = (contacts << 1) + hit;
    }
    frame_st(frame, 0x1a0, contacts);
    sr_st16(node + 0x12, slot_counter);

    /* Rear wheel slide against the heading. */
    tgp0(0x10002020u);
    tgp0(0x12802525u);
    tgp1(0x15002a2au, sr_ld32(node + CAR_ANG + 4));
    {
        static const uint32_t forward[3] = {0, 0, 0x3f800000u};
        tgp_vec3(0x16002c2cu, forward, unit);
    }
    frame_st(frame, 0x130, unit[0]);
    frame_st(frame, 0x134, unit[1]);
    frame_st(frame, 0x138, unit[2]);
    for(i = 0; i < 2; i++) {
        const uint32_t src[3] = {frame_ld(frame, 0xb0 + i * 12), frame_ld(frame, 0xb4 + i * 12),
                                 frame_ld(frame, 0xb8 + i * 12)};
        const uint32_t length = tgp_length(src);
        if(D(length) > sr_d(0xd9d7bdbbu, 0x3ddb7cdfu)) {
            uint32_t cu[3];
            tgp_vec3(0x17802f2fu, src, cu);
            length_sum += D(length);
            side_sum += D(unit[2]) * D(cu[0]) - D(unit[0]) * D(cu[2]);
        }
    }

    /* Drift angle between heading and velocity → node+0xd0, its change → node+0xd4. */
    velocity[0] = sr_ld32(node + CAR_VEL);
    velocity[1] = sr_ld32(node + CAR_VEL + 4);
    velocity[2] = sr_ld32(node + CAR_VEL + 8);
    {
        const uint32_t speed = tgp_length(velocity);
        if(D(speed) > sr_d(0x47ae147bu, 0x3f847ae1u)) {
            double cross, angle;
            uint32_t old_d0;
            tgp_vec3(0x17802f2fu, velocity, unit);
            cross = D(frame_ld(frame, 0x138)) * D(unit[0]) - D(frame_ld(frame, 0x130)) * D(unit[2]);
            angle = fabs(cross) > 1.0 ? 0.0 : m2_asin(cross);
            if(fabs(angle) > 1.5707963267948966) angle = signbit(angle) ? -1.5707963267948966 : 1.5707963267948966;
            old_d0 = sr_ld32(node + 0xd0);
            sr_st32(node + 0xd0, F(angle));
            sr_st32(node + 0xd4, gsub(F(angle), old_d0));
        } else {
            sr_st32(node + 0xd0, 0);
            sr_st32(node + 0xd4, 0);
        }
    }

    /* Cornering torque from the slide (grip by the road surface class), and the lateral slip's. */
    {
        const double scale = D(sr_ld32(0x214124u)), coeff = D(sr_ld32(0x2141c4u));
        const double side_pi = side_sum * sr_d(0x54442d18u, 0x400921fbu);
        const uint32_t idx = sr_ld32(node + CAR_SPAN);
        const uint32_t lane_b = ((uint32_t)sr_ld8(spans + 0xc + idx * 16) >> 2) & 28u;
        const uint32_t table = sr_rom_mirror32(0x5cabe0u + lane_b);
        const double gain = scale * 1000.0 * coeff * D(table);
        uint32_t sin_arg, sine, dx, dz, dist, lat[3], lat_n[3];
        double product, mass_half;
        if(fabs(side_pi) > sr_d(0x54442d18u, 0x3ff921fbu))
            sin_arg = 0x3fc90fdbu;
        else
            sin_arg = side_pi < 0.0 ? F(-side_pi) : F(side_pi);
        sr_tgp_cmd(0x0a801515u, &sin_arg, 1, &sine, 1);
        product = gain * D(sine);
        if(side_sum >= 0.0) product = -product;
        {
            const double mixed = length_sum * fabs(side_sum), floor = D(0x3ca3d70au);
            if(mixed < floor) product *= mixed / floor;
        }
        frame_st(frame, 0x104, F(D(frame_ld(frame, 0x104)) + product * 1.25));

        dx = gsub(sr_ld32(node + CAR_VEL), sr_ld32(node + 0xc4));
        dz = gsub(sr_ld32(node + CAR_VEL + 8), sr_ld32(node + 0xcc));
        {
            const uint32_t v[3] = {dx, 0, dz};
            dist = tgp_length(v);
            tgp_vec3(0x17802f2fu, v, lat_n);
        }
        mass_half = D(sr_ld32(node + 0x50)) * 0.5 * D(dist);
        lat[0] = F(mass_half * D(lat_n[0]));
        lat[1] = lat_n[1];
        lat[2] = F(mass_half * D(lat_n[2]));
        tgp0(0x12802525u);
        tgp1(0x15002a2au, fneg(sr_ld32(node + CAR_ANG + 4)));
        tgp_vec3(0x16002c2cu, lat, lat_n);
        frame_st(frame, 0x150, lat_n[0]);
        frame_st(frame, 0x154, lat_n[1]);
        frame_st(frame, 0x158, lat_n[2]);
        frame_st(frame, 0x104, F(D(frame_ld(frame, 0x104)) - D(lat_n[0]) * 1.25));
    }
    tgp0(0x10802121u);

    {
        const double k02 = sr_d(0x9999999au, 0x3fc99999u), k125 = 1.25;
        const double k0635 = sr_d(0x851eb852u, 0x3fe451ebu), k066 = sr_d(0x51eb851fu, 0x3fe51eb8u);
        uint32_t lat[3], lat_n[3], mass, tf[4][3], raw_x[4], raw_y[4], raw_z[4];
        double t100, t104, t108, f0, f1, f2, sample_scale;

        tgp0(0x10002020u);
        lat[0] = gsub(sr_ld32(node + CAR_VEL), sr_ld32(node + 0xc4));
        lat[1] = 0;
        lat[2] = gsub(sr_ld32(node + CAR_VEL + 8), sr_ld32(node + 0xcc));
        tgp0(0x12802525u);
        tgp1(0x15002a2au, fneg(sr_ld32(node + CAR_ANG + 4)));
        tgp_vec3(0x16002c2cu, lat, lat_n);
        frame_st(frame, 0x150, lat_n[0]);
        frame_st(frame, 0x154, lat_n[1]);
        frame_st(frame, 0x158, lat_n[2]);
        mass = sr_ld32(node + 0x50);
        t100 = D(frame_ld(frame, 0x100)) + D(lat_n[2]) * D(mass) * k02;
        t108 = D(frame_ld(frame, 0x108)) - D(lat_n[0]) * D(mass) * k02;
        frame_st(frame, 0x100, F(t100));
        frame_st(frame, 0x108, F(t108));

        /* One side of the car off the ground: roll it towards the wheels that still touch. */
        sample_scale = D(sr_ld32(0x2141d4u)) * 100.0 * D(sr_ld32(0x214124u)) * k125;
        if(D(sr_ld32(node + 0x64)) == 0.0 && D(sr_ld32(node + 0x68)) > 0.0) {
            t108 += D(frame_ld(frame, 0x110)) * sample_scale;
            frame_st(frame, 0x108, F(t108));
        }
        if(D(sr_ld32(node + 0x68)) == 0.0 && D(sr_ld32(node + 0x64)) > 0.0) {
            t108 = D(frame_ld(frame, 0x108)) - D(frame_ld(frame, 0x114)) * sample_scale;
            frame_st(frame, 0x108, F(t108));
        }

        /* Wheel forces into the car's frame: torque from their lever arms, linear force as summed. */
        tgp0(0x10802121u);
        tgp0(0x10002020u);
        tgp0(0x12802525u);
        tgp1(0x15802b2bu, fneg(sr_ld32(node + CAR_ANG + 8)));
        tgp1(0x14802929u, fneg(sr_ld32(node + CAR_ANG)));
        tgp1(0x15002a2au, fneg(sr_ld32(node + CAR_ANG + 4)));
        for(i = 0; i < 4; i++) {
            raw_x[i] = frame_ld(frame, 0x80 + i * 12);
            raw_y[i] = frame_ld(frame, 0x84 + i * 12);
            raw_z[i] = frame_ld(frame, 0x88 + i * 12);
            lat[0] = raw_x[i];
            lat[1] = raw_y[i];
            lat[2] = raw_z[i];
            tgp_vec3(0x16002c2cu, lat, tf[i]);
            frame_st(frame, 0x160 + i * 12, tf[i][0]);
            frame_st(frame, 0x164 + i * 12, tf[i][1]);
            frame_st(frame, 0x168 + i * 12, tf[i][2]);
        }
        t100 = D(frame_ld(frame, 0x100));
        t104 = D(frame_ld(frame, 0x104));
        t108 = D(frame_ld(frame, 0x108));
        f0 = D(frame_ld(frame, 0xf0));
        f1 = D(frame_ld(frame, 0xf4));
        f2 = D(frame_ld(frame, 0xf8));
        {
            const double c0y_c1y = D(tf[0][1]) + D(tf[1][1]), c2y_c3y = D(tf[2][1]) + D(tf[3][1]);
            const double c0x_c1x = D(tf[0][0]) + D(tf[1][0]), c2x_c3x = D(tf[2][0]) + D(tf[3][0]);
            const double dy01 = D(tf[0][1]) - D(tf[1][1]), dy23 = D(tf[2][1]) - D(tf[3][1]);
            const double dz01 = D(tf[0][2]) - D(tf[1][2]), dz23 = D(tf[2][2]) - D(tf[3][2]);
            t100 += (c0y_c1y - c2y_c3y) * k125 + (D(tf[0][2]) + D(tf[1][2]) + D(tf[2][2]) + D(tf[3][2])) * k02;
            t104 += (c2x_c3x - c0x_c1x) * k125 + dz01 * k0635 + dz23 * k066;
            t108 += dy01 * k0635 + dy23 * k066 - (D(tf[0][0]) + D(tf[1][0]) + D(tf[2][0]) + D(tf[3][0])) * k02;
        }
        f0 += D(raw_x[0]) + D(raw_x[1]) + D(raw_x[2]) + D(raw_x[3]);
        f1 += D(raw_y[0]) + D(raw_y[1]) + D(raw_y[2]) + D(raw_y[3]);
        f2 += D(raw_z[0]) + D(raw_z[1]) + D(raw_z[2]) + D(raw_z[3]);
        frame_st(frame, 0xf0, F(f0));
        frame_st(frame, 0xf4, F(f1));
        frame_st(frame, 0xf8, F(f2));
        frame_st(frame, 0x100, F(t100));
        frame_st(frame, 0x104, F(t104));
        frame_st(frame, 0x108, F(t108));
        tgp0(0x10802121u);
    }

    /* Torque / inertia into the angular velocity; on the ground, ease pitch and roll back when still turning. */
    for(i = 0; i < 3; i++) {
        const uint32_t ang = sr_ld32(node + 0x44 + i * 4);
        const double impulse = D(frame_ld(frame, 0x100 + i * 4)) / D(sr_ld32(node + 0x54 + i * 4));
        sr_st32(node + 0x44 + i * 4, F(D(ang) + impulse));
    }
    if(contacts != 15) {
        static const uint8_t angle_off[2] = {0x40, 0x38}, rate_off[2] = {0x4c, 0x44};
        for(i = 0; i < 2; i++) {
            double angle = D(sr_ld32(node + angle_off[i]));
            const double rate = D(sr_ld32(node + rate_off[i]));
            if((angle < 0.0) == (rate < 0.0)) {
                angle -= angle * angle * (angle < 0.0 ? -0.3 : 0.3);
                sr_st32(node + angle_off[i], F(angle));
            }
        }
    }
    return contacts;
}

void sr_car_integrate(uint32_t node, uint32_t spans) {
    uint8_t frame[0x1c0];
    uint32_t vx, vz, contact, contacts;
    if(node == 0) return;
    memset(frame, 0, sizeof(frame));

    /* Half the queued wall impulse into velocity and position. */
    vx = gmul(sr_ld32(node + 0x20), 0x3f000000u);
    vz = gmul(sr_ld32(node + 0x28), 0x3f000000u);
    sr_st32(node + 0x20, vx);
    sr_st32(node + 0x24, 0);
    sr_st32(node + 0x28, vz);
    sr_st32(node + CAR_VEL, gadd(sr_ld32(node + CAR_VEL), vx));
    sr_st32(node + CAR_VEL + 8, gadd(sr_ld32(node + CAR_VEL + 8), vz));
    sr_st32(node + CAR_POS, gadd(sr_ld32(node + CAR_POS), vx));
    sr_st32(node + CAR_POS + 4, gadd(sr_ld32(node + CAR_POS + 4), 0));
    sr_st32(node + CAR_POS + 8, gadd(sr_ld32(node + CAR_POS + 8), vz));
    sr_st32(node + 0x20, 0);
    sr_st32(node + 0x24, 0);
    sr_st32(node + 0x28, 0);

    sr_st32(node + 0xc4, sr_ld32(node + CAR_VEL));
    sr_st32(node + 0xc8, sr_ld32(node + CAR_VEL + 4));
    sr_st32(node + 0xcc, sr_ld32(node + CAR_VEL + 8));
    sr_car_velocity_step(node);

    /* Predicted pose: position + velocity, angles + angular velocity. */
    {
        uint32_t i;
        for(i = 0; i < 3; i++) {
            frame_st(frame, 0x70 + i * 4, gadd(sr_ld32(node + CAR_POS + i * 4), sr_ld32(node + CAR_VEL + i * 4)));
            frame_st(frame, 0xe0 + i * 4, gadd(sr_ld32(node + CAR_ANG + i * 4), sr_ld32(node + 0x44 + i * 4)));
        }
    }
    contact = sr_car_contact_solve(node, frame + 0x70, frame + 0xe0);
    frame_st(frame, 0x190, contact);
    contacts = force_core(node, spans, frame);

    {
        const uint32_t ay = sr_ld32(node + 0x48);
        const uint32_t corr = gadd(gmul(sr_ld32(0x2141ccu), sr_ld32(node + 0xd4)), gmul(sr_ld32(0x2141d0u), ay));
        sr_st32(node + 0x48, gsub(ay, corr));
    }
    sr_st32(node + CAR_ANG, gadd(sr_ld32(node + CAR_ANG), sr_ld32(node + 0x44)));
    sr_st32(node + CAR_ANG + 4, gadd(sr_ld32(node + CAR_ANG + 4), sr_ld32(node + 0x48)));
    sr_st32(node + CAR_ANG + 8, gadd(sr_ld32(node + CAR_ANG + 8), sr_ld32(node + 0x4c)));

    /* Force / mass into velocity (gravity 0x214124 on y), then move. */
    {
        const double mass = D(sr_ld32(node + 0x50));
        if(mass != 0.0) {
            sr_st32(node + CAR_VEL, gadd(sr_ld32(node + CAR_VEL), F(D(frame_ld(frame, 0xf0)) / mass)));
            sr_st32(node + CAR_VEL + 4,
                    gadd(sr_ld32(node + CAR_VEL + 4), F(D(frame_ld(frame, 0xf4)) / mass - D(sr_ld32(0x214124u)))));
            sr_st32(node + CAR_VEL + 8, gadd(sr_ld32(node + CAR_VEL + 8), F(D(frame_ld(frame, 0xf8)) / mass)));
        }
    }
    sr_st32(node + CAR_POS, gadd(sr_ld32(node + CAR_POS), sr_ld32(node + CAR_VEL)));
    sr_st32(node + CAR_POS + 4, gadd(sr_ld32(node + CAR_POS + 4), sr_ld32(node + CAR_VEL + 4)));
    sr_st32(node + CAR_POS + 8, gadd(sr_ld32(node + CAR_POS + 8), sr_ld32(node + CAR_VEL + 8)));

    /* Damping: by the wall penetration, else airborne (no wheel down) or the surface's rolling resistance. */
    if(D(contact) > 0.0) {
        double factor = sr_d(0x8b439581u, 0x3fefe76cu) - D(contact) * D(sr_ld32(0x2141f0u));
        if(factor < 0.0) factor = 0.0;
        sr_st32(node + CAR_VEL, F(D(sr_ld32(node + CAR_VEL)) * factor));
        sr_st32(node + CAR_VEL + 4, F(D(sr_ld32(node + CAR_VEL + 4)) * factor));
        sr_st32(node + CAR_VEL + 8, F(D(sr_ld32(node + CAR_VEL + 8)) * factor));
    } else {
        const double factor = contacts == 0 ? sr_d(0x074a771du, 0x3fefffebu) : D(sr_ld32(0x2141ecu));
        sr_st32(node + CAR_VEL, F(D(sr_ld32(node + CAR_VEL)) * factor));
        sr_st32(node + CAR_VEL + 8, F(D(sr_ld32(node + CAR_VEL + 8)) * factor));
    }

    sr_st32(node + CAR_ANG, angle_cap(sr_ld32(node + CAR_ANG)));
    sr_st32(node + CAR_ANG + 4, sr_angle_wrap(sr_ld32(node + CAR_ANG + 4)));
    sr_st32(node + CAR_ANG + 8, angle_cap(sr_ld32(node + CAR_ANG + 8)));
}
