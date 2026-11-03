/* Callees of the player car's scene update (geo_view_scene_frame @ 0x34F40): corner probes, wheel spring
 * forces, the landing latch, steering smoothing, the gap scale, the pose copy, the RNG and the sound queue.
 * See include/race/player_scene.h for what each one reads and writes. */
#include "race/player_scene.h"
#include "race/sound_cmd.h"
#include "race/course.h"
#include "race/fp.h"
#include "race/road.h"
#include "sr_tgp.h"
#include <math.h>
#include <stdio.h>

/* ---- RNG (0x5CCE8) ---- */

uint32_t sr_rng_next(void) {
    const uint64_t prod = (uint64_t)sr_ld32(0x2197b0u) * 0x5d588b65ull;
    const uint32_t lo = (uint32_t)prod, hi = (uint32_t)(prod >> 32);
    const uint32_t out = (lo + (hi << 1) + (lo >> 31)) & 0x7fffffffu;
    sr_st32(0x2197b0u, out);
    return out;
}

/* ---- steering (0x395D8, 0x39A90) ---- */

uint32_t sr_steer_amount(void) {
    const int32_t raw = (int32_t)sr_ld8(0x202050u) - 0x80;
    uint32_t mag = raw < 0 ? (uint32_t)-raw : (uint32_t)raw;
    double v;
    if(mag > 0x50u) mag = 0x50u;
    v = (double)(int32_t)mag * sr_d(0xae147ae1u, 0x3feae147u) / 80.0; /* 0.84 / 80 in double */
    return F(raw < 0 ? -v : v);
}

uint32_t sr_player_steer_smooth(void) {
    const double cur = D(sr_steer_amount()), prev = D(sr_ld32(0x2142c0u));
    const uint32_t bits = F(prev + (cur - prev) * 0.9);
    sr_st32(0x2142c0u, bits);
    return bits ^ 0x80000000u;
}

/* ---- landing latch (0x34CE0) ---- */

void sr_player_mode_latch(uint32_t xbits) {
    const float x = sr_f(xbits);
    uint32_t count;
    if(x < 0.f || sr_ld32(0x214120u) == 2u) {
        sr_st32(0x213870u, 0);
        sr_st32(0x21396cu, 0);
        return;
    }
    if(x == 0.f) {
        count = sr_ld32(0x213870u) + 1u;
        sr_st32(0x213870u, count);
        if(sr_ld32(0x21396cu) != 0u) {
            sr_st32(0x21396cu, 0);
            sr_st32(0x2142c8u, 3u);
        }
        return;
    }
    sr_text_cursor(2, 46);
    count = sr_ld32(0x213870u);
    if((int32_t)count >= 30) {
        if(sr_ld32(0x21396cu) == 0u && sr_ld32(0x2142c8u) == 0u && sr_ld32(0x2142d4u) == 0u) {
            sr_st32(0x2142c8u, 2u);
            sr_st32(0x2142d0u, 5u);
            sr_st32(0x21396cu, 1u);
        }
        sr_st32(0x213870u, 0);
        return;
    }
    if(sr_ld32(0x2142c8u) == 0u) {
        sr_st32(0x2142c8u, 1u);
        sr_st32(0x2142ccu, sr_ld32(0x213964u));
        sr_st32(0x2142d0u, sr_ld32(0x213968u));
    }
    if(x > 0.03f) {
        sr_sound_cmd_fill(0x47u, 0x7fu);
        sr_sound_cmd_fill(0x0eu, 0x7fu);
    } else {
        sr_sound_cmd_fill(0x49u, 0x7fu);
    }
    sr_st32(0x213870u, 0);
}

/* ---- pose copy (0x34078) ---- */

void sr_player_pose_publish(uint32_t node) {
    static const uint8_t map[][2] = {
        {0x50, 0x10}, {0x54, 0x88}, {0x00, 0x14}, {0x04, 0x18}, {0x08, 0x1c}, {0x0c, 0x2c}, {0x10, 0x30},
        {0x14, 0x34}, {0x18, 0x38}, {0x1c, 0x3c}, {0x20, 0x40}, {0x28, 0x80}, {0x2c, 0x84}, {0x24, 0x74},
        {0x30, 0x64}, {0x34, 0x68}, {0x38, 0x6c}, {0x3c, 0x70},
    };
    const uint32_t pose = sr_ld32(node + 0x8c);
    unsigned i;
    for(i = 0; i < sizeof(map) / sizeof(map[0]); i++) sr_st32(pose + map[i][0], sr_ld32(node + map[i][1]));
    for(i = 0; i < 4; i++) sr_st32(pose + 0x40 + 4 * i, sr_ld32(PS_CORNER_HEIGHT + 4 * i));
}

/* ---- gap scale (0x33E90) ----
 * scene_frame calls this only while 0x20a560 > 0, and practice keeps it at -1, so the seeded drives never reach
 * it. A one-off probe that called it every tick (solo, and with 0x2139c0 forced to 2 against car 0x500100's pose)
 * matched the original on every call with a valid pose pointer. */

#define CAR_COUNT  0x2139c0u
#define CAR_POSES  0x214134u /* pose block of each other car */
#define GAP_SCALE  0x214124u

void sr_player_gap_scale(uint32_t pose, uint8_t *frame) {
    const uint32_t span = sr_ld16(pose + 0x56);
    double gap = 0.0, sum;
    uint32_t list = CAR_POSES, bits;
    int32_t k;
    frame_st(frame, 0x40, sr_ld32(pose));
    frame_st(frame, 0x44, sr_ld32(pose + 4));
    frame_st(frame, 0x48, sr_ld32(pose + 8));
    if((int32_t)sr_ld32(CAR_COUNT) > 1) {
        for(k = 1;; k++) {
            const uint32_t other = sr_ld32(list);
            const int32_t dspan = (int32_t)(span - sr_ld16(other + 0x56));
            const uint32_t ox = sr_ld32(other), oy = sr_ld32(other + 4), oz = sr_ld32(other + 8);
            const uint32_t in[6] = {frame_ld(frame, 0x40), ox, frame_ld(frame, 0x44), oy, frame_ld(frame, 0x48), oz};
            uint32_t dist, by_span, by_dist;
            double d;
            frame_st(frame, 0x58, oz);
            frame_st(frame, 0x50, ox);
            frame_st(frame, 0x54, oy);
            sr_tgp_cmd(0x2c005858u, in, 6, &dist, 1);
            by_span = F(10.0 * (double)dspan);
            d = D(dist);
            if(dspan < 0) d = -d;
            by_dist = F(d);
            if(!(D(by_span) <= D(by_dist))) by_span = by_dist;
            if(gap > D(by_span)) gap = D(by_span);
            list += 4;
            if(!(k + 1 < (int32_t)sr_ld32(CAR_COUNT))) break;
        }
    }
    bits = F(10.0 * D(F(gap)));
    sum = 3500.0 + D(bits);
    if(!(sum >= 2600.0)) {
        sr_st32(GAP_SCALE, 0x3b7745bau); /* 9.81 / 2600 */
        return;
    }
    sr_st32(GAP_SCALE, F(sr_d(0x51eb851fu, 0x40239eb8u) / sum)); /* 9.81 */
}

/* ---- wheel spring force (0x37200) ---- */

#define CORNER_STRIDE 0x24u
#define NODE_CORNER_FLAGS 0x12u /* u16: road flag nibble per corner */
#define NODE_COMPRESS     0x64u /* 4 words */
#define NODE_SPRING_K     0x50u

static float ldf(uint32_t va) { return sr_f(sr_ld32(va)); }
static float hf(const uint8_t *p, uint32_t off) { return sr_f(frame_ld(p, off)); }

uint32_t sr_player_wheel_force(uint32_t node, const uint8_t *pos, const uint8_t *vec, const uint8_t *dir,
                               uint32_t scale, uint8_t *out, uint32_t slot) {
    const uint32_t rec = PS_CORNER_ROAD + slot * CORNER_STRIDE;
    const uint32_t lane = slot << 2;
    /* The record: +0 .. +0xc plane (w12..w15), +0x10 .. +0x1c normal xyz, height, flags (w8..w11), +0x20 weight. */
    const uint32_t w8 = sr_ld32(rec + 0x10), w9 = sr_ld32(rec + 0x14), w11 = sr_ld32(rec + 0x1c);
    const uint32_t w15 = sr_ld32(rec + 0xc), w20 = sr_ld32(rec + 0x20);
    uint32_t height = sr_ld32(rec + 0x18);
    const uint32_t nibble = w11 & 15u;
    uint32_t packed;

    sr_st32(PS_CORNER_HEIGHT + lane, sr_b(hf(vec, 4) + sr_f(height)));
    packed = (sr_ld16(node + NODE_CORNER_FLAGS) & ~(15u << lane)) | (nibble << lane);
    sr_st16(node + NODE_CORNER_FLAGS, packed);
    sr_st32(0x214208u, sr_ld32(0x214208u) | (nibble << 1));

    if((w11 & 0x80u) || sr_f(height) < 0.f) {
        /* In the air or on a wall cell: no force, except the opposite corner's hold-down when it is compressed. */
        sr_st32(node + NODE_COMPRESS + lane, 0);
        sr_st32(PS_CORNER_FORCE + lane, 0);
        frame_st(out, 0, 0);
        frame_st(out, 4, 0);
        frame_st(out, 8, 0);
        if(ldf(node + NODE_COMPRESS + ((slot ^ 1u) << 2)) > 0.f) {
            float acc = (float)sr_d(0xbc6a7efau, 0x3f789374u) * sr_f(w20); /* 0.006 */
            acc *= sr_f(scale);
            acc *= ldf(node + NODE_SPRING_K);
            acc *= sr_f(w8);
            frame_st(out, 4, sr_b(acc));
        }
        sr_st32(PS_CORNER_SIN + lane, 0);
        sr_st32(PS_CORNER_DIST + lane, 0);
        return 0;
    }

    {
        /* Spring rate per axle: slots 2/3 (rear) 0x2141d8 / 0x2141e0, slots 0/1 0x2141d4 / 0x2141dc. */
        const uint32_t rate = slot > 1u ? sr_ld32(0x2141d8u) : sr_ld32(0x2141d4u);
        const uint32_t damp = slot > 1u ? sr_ld32(0x2141e0u) : sr_ld32(0x2141dcu);
        const float gscale = ldf(GAP_SCALE);
        float force = sr_f(rate) * sr_f(height);
        float rel;
        force = force * 100.f;
        force *= gscale;
        if(sr_f(height) > 0.2f) {
            float acc = (sr_f(height) - 0.2f) * sr_f(scale);
            acc *= ldf(node + NODE_SPRING_K);
            force = acc + force;
            height = 0x3e4ccccdu; /* 0.2f */
        }
        rel = ldf(node + NODE_COMPRESS + lane) - sr_f(height);
        rel = rel * sr_f(damp);
        rel = rel * gscale;
        force = force - rel * 3600.f;
        if(force < 0.f) force = 0.f;
        {
            const uint32_t fy = sr_b(force * sr_f(w8)), fz = sr_b(force * sr_f(w9));
            const uint32_t fx = sr_b(force * sr_f(w15));
            sr_st32(node + NODE_COMPRESS + lane, height);
            sr_st32(PS_CORNER_FORCE + lane, sr_b(force));
            frame_st(out, 4, fy);
            frame_st(out, 8, fz);
            frame_st(out, 0, fx);
        }
    }

    {
        const uint32_t in[6] = {0, 0, 0, frame_ld(pos, 0), frame_ld(pos, 4), frame_ld(pos, 8)}; /* |pos - 0| */
        uint32_t dist, flat[3], n[3];
        float cross, dot, s;
        uint32_t sign, c;
        sr_tgp_cmd(0x2c005858u, in, 6, &dist, 1);
        if(sr_f(dist) <= (float)sr_d(0xd9d7bdbbu, 0x3ddb7cdfu)) { /* 1e-10 */
            sr_st32(PS_CORNER_SIN + lane, 0);
            sr_st32(PS_CORNER_DIST + lane, 0);
            return 1;
        }
        flat[0] = frame_ld(pos, 0);
        flat[1] = 0;
        flat[2] = frame_ld(pos, 8);
        sr_tgp_cmd(0x17802f2fu, flat, 3, n, 3); /* normalize */
        cross = sr_f(n[0]) * hf(dir, 8) - sr_f(n[2]) * hf(dir, 0);
        sign = cross >= 0.f ? 0x3f800000u : 0xbf800000u;
        dot = sr_f(n[0]) * hf(dir, 0) + sr_f(n[2]) * hf(dir, 8);
        c = sr_b(dot);
        if(dot < 0.f) c ^= 0x80000000u;
        if(sr_f(c) > 1.f) c = 0x3f800000u;
        s = sr_f(c);
        s = s * s;
        s = 1.f - s;
        if(s < 0.f) s = 0.f;
        s = sqrtf(s);
        s = sr_f(sign) * s;
        sr_st32(PS_CORNER_DIST + lane, dist);
        sr_st32(PS_CORNER_SIN + lane, sr_b(s));
    }
    return 1;
}

/* ---- corner probes (0x34380) ---- */

#define CORNER_TABLE 0x5d2e38u /* 4 corner offsets (xyz), ROM-mirrored */

uint32_t sr_player_scene_prep(uint32_t node, uint8_t *pos, uint8_t *ang, uint8_t *frame) {
    const uint32_t course = sr_race_course_index();
    const uint32_t index = (uint32_t)(int32_t)(int16_t)sr_ld16(node + 0x88);
    uint32_t max_weight = 0, i;

    for(i = 0; i < 4; i++) {
        const uint32_t t = CORNER_TABLE + 12u * i;
        const uint32_t in[4] = {i, sr_ld32(t - 8), sr_ld32(t - 4), sr_ld32(t)};
        sr_tgp_cmd(0x21004242u, in, 4, 0, 0); /* load corner vector i */
    }
    sr_tgp_cmd(0x10002020u, 0, 0, 0, 0); /* push */
    sr_tgp_cmd(0x12802525u, 0, 0, 0, 0); /* identity */
    { const uint32_t a = frame_ld(ang, 4); sr_tgp_cmd(0x15002a2au, &a, 1, 0, 0); } /* rotate y */
    { const uint32_t a = frame_ld(ang, 0); sr_tgp_cmd(0x14802929u, &a, 1, 0, 0); } /* rotate x */
    { const uint32_t a = frame_ld(ang, 8); sr_tgp_cmd(0x15802b2bu, &a, 1, 0, 0); } /* rotate z */
    for(i = 0; i < 4; i++) {
        uint32_t v[3], k;
        sr_tgp_cmd(0x24004848u, &i, 1, 0, 0);  /* transform vector i in place */
        sr_tgp_cmd(0x21804343u, &i, 1, v, 3); /* read it */
        for(k = 0; k < 3; k++) {
            frame_st(frame, 0x70 + 12 * i + 4 * k, v[k]);
            frame_st(frame, 0x40 + 12 * i + 4 * k, gadd(frame_ld(pos, 4 * k), v[k]));
        }
    }
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0); /* pop */
    sr_st32(PS_HIT_MASK, 0);
    sr_road_query_a(course, index, frame + 0x40, 0);

    for(i = 0; i < 4; i++) {
        const uint32_t vec_off = 0x70 + 12 * i, obj_off = 0x40 + 12 * i, out_off = 0xa0 + 12 * i;
        const uint32_t out_va = PS_CORNER_ROAD + i * CORNER_STRIDE, record = out_va + 0xc;
        sr_road_query_b(frame + obj_off, sr_at(out_va), 0);
        if(i < 3) sr_road_query_a(course, index, frame + obj_off + 12, 0); /* start the next corner's query */

        if(sr_ld8(out_va + 0x1c) & 0x80u) {
            /* Wall cell: push the corner back along the wall plane's in-plane direction. */
            uint32_t wx = sr_ld32(record), wy = sr_ld32(record + 4), wz = sr_ld32(record + 8);
            const uint32_t vx = frame_ld(frame, vec_off), vy = frame_ld(frame, vec_off + 4);
            const uint32_t vz = frame_ld(frame, vec_off + 8);
            uint32_t nx, ny, nz;
            double len, proj, p0, p1, weight, factor;

            sr_st32(PS_HIT_MASK, sr_ld32(PS_HIT_MASK) + (1u << i));
            nx = fsub(fmul(wy, vz), fmul(vy, wz));
            ny = fsub(fmul(vx, wz), fmul(wx, vz));
            nz = fsub(fmul(wx, vy), fmul(vx, wy));
            if(i == 0) {
                frame_st(frame, 0xe0, nx);
                frame_st(frame, 0xe4, ny);
                frame_st(frame, 0xe8, nz);
            }
            frame_st(frame, 0xd0, nx);
            frame_st(frame, 0xd4, ny);
            frame_st(frame, 0xd8, nz);
            /* On the i960 the first record (0x2138dc) is the same memory as fp+0xd0: corner 0 writes it too. */
            if(i == 0) {
                sr_st32(record, nx);
                sr_st32(record + 4, ny);
                sr_st32(record + 8, nz);
                wx = nx;
                wy = ny;
                wz = nz;
            }
            {
                const uint32_t cx = nx, cy = ny, cz = nz;
                nx = fsub(fmul(cy, wz), fmul(wy, cz));
                ny = fsub(fmul(wx, cz), fmul(cx, wz));
                nz = fsub(fmul(cx, wy), fmul(wx, cy));
            }
            frame_st(frame, 0xd0, nx);
            frame_st(frame, 0xd4, ny);
            frame_st(frame, 0xd8, nz);
            if(i == 0) {
                sr_st32(record, nx);
                sr_st32(record + 4, ny);
                sr_st32(record + 8, nz);
            }
            len = sqrt(D(nx) * D(nx) + D(ny) * D(ny) + D(nz) * D(nz));
            if(len > sr_d(0xff9fdba8u, 0x380fffffu)) {
                nx = F(D(nx) / len);
                ny = F(D(ny) / len);
                nz = F(D(nz) / len);
            } else {
                nx = 0;
                ny = 0;
                nz = 0x3f800000u;
            }
            frame_st(frame, 0xd0, nx);
            frame_st(frame, 0xd4, ny);
            frame_st(frame, 0xd8, nz);
            if(i == 0) {
                sr_st32(record, nx);
                sr_st32(record + 4, ny);
                sr_st32(record + 8, nz);
                wx = nx;
                wy = ny;
                wz = nz;
            }
            proj = D(nx) * D(vx) + D(ny) * D(vy) + D(nz) * D(vz);
            /* sqrtr of a negative dot is an invalid op on the i960; the reference takes 0. */
            proj = !(proj == proj) || proj < 0.0 ? 0.0 : sqrt(proj);
            p0 = D(sr_ld32(node + 0x50));
            p1 = D(sr_ld32(node + 0x54));
            weight = D(sr_ld32(out_va + 0x18));
            factor = ((p0 * p1) * 0.5) / (p1 + p0 * proj * proj);
            factor *= weight;
            frame_st(frame, out_off, F(D(wx) * factor));
            frame_st(frame, out_off + 4, F(D(wy) * factor));
            frame_st(frame, out_off + 8, F(D(wz) * factor));
            {
                const uint32_t cand = sr_ld32(out_va + 0x18);
                if(D(cand) >= D(max_weight)) max_weight = cand;
            }
        } else {
            frame_st(frame, out_off, 0);
            frame_st(frame, out_off + 4, 0);
            frame_st(frame, out_off + 8, 0);
        }
    }

    if(D(max_weight) != 0.0) {
        double reduction = 0.0, step, sx = 0.0, sz = 0.0;
        for(i = 0; i < 4; i++) {
            const uint32_t off = 12 * i;
            uint32_t in[6], r[3], rv;
            frame_st(frame, 0x74 + off, 0);
            frame_st(frame, 0xa4 + off, 0);
            in[0] = frame_ld(frame, 0x74 + off);
            in[1] = frame_ld(frame, 0xa8 + off);
            in[2] = frame_ld(frame, 0x78 + off);
            in[3] = frame_ld(frame, 0xa4 + off);
            in[4] = frame_ld(frame, 0xa0 + off);
            in[5] = frame_ld(frame, 0x70 + off);
            sr_tgp_cmd(0x2d805b5bu, in, 6, r, 3); /* cross product */
            frame_st(frame, 0xd0, r[0]);
            frame_st(frame, 0xd4, r[1]);
            frame_st(frame, 0xd8, r[2]);
            {
                const uint32_t din[6] = {0, 0, 0, r[0], r[1], r[2]};
                sr_tgp_cmd(0x2c005858u, din, 6, &rv, 1); /* length */
            }
            reduction -= fabs(D(rv));
        }
        step = reduction / D(sr_ld32(node + 0x58));
        sr_st32(node + 0x48, gadd(sr_ld32(node + 0x48), F(step)));
        frame_st(ang, 4, gadd(sr_ld32(node + 0x3c), sr_ld32(node + 0x48)));
        for(i = 0; i < 4; i++) {
            sx += D(frame_ld(frame, 0xa0 + 12 * i));
            sz += D(frame_ld(frame, 0xa8 + 12 * i));
        }
        sr_st32(node + 0x2c, gadd(sr_ld32(node + 0x2c), F(sx / D(sr_ld32(node + 0x50)))));
        sr_st32(node + 0x34, gadd(sr_ld32(node + 0x34), F(sz / D(sr_ld32(node + 0x50)))));
        frame_st(pos, 0, gadd(sr_ld32(node + 0x14), sr_ld32(node + 0x2c)));
        frame_st(pos, 8, gadd(sr_ld32(node + 0x1c), sr_ld32(node + 0x34)));
        if(sr_ld32(0x2142c8u) == 0u) {
            int32_t q = (int32_t)(step * 1000.0);
            if(q <= 0) q = -q;
            sr_st32(0x213964u, (uint32_t)q);
            sr_st32(0x213968u, (uint32_t)(int32_t)(step * 8000.0));
        }
    }
    return max_weight;
}
