/* The desert helicopter (race list node 0x500200 in desert practice): node_init (0x43790), node_frame (0x427A0)
 * and its callees. See include/race/desert_node.h for the node and table layout.
 *
 * Each tick the helicopter moves along its path table: the path time node+0x5c advances by the table's rate,
 * slowed by a TGP 0x58 distance between the followed pose and the helicopter; pose_step samples a cubic through the path rows
 * for height, yaw and tilt; a TGP-rotated drift is added to x/z. The road under it (road_query_a/b) gives the
 * height above ground: below 15 it draws its shadow, and over sand (surface 2, 3, 6, 7) it starts dust puffs from
 * two RNG draws. Then the body, main rotor and tail rotor are drawn, the dust is drawn and aged, the rotor sound
 * follows the camera distance, and the 0x20a730 flag block gets its position while the path time is inside the
 * table's window.
 *
 * Nothing the cars read: its RAM is the node, 0x216a54, the dust ring 0x216a70.., the flag block 0x20a730..,
 * the sound ring (0x97), the RNG seed 0x2197b0 (two draws per tick over sand), and the GEO list cursor
 * 0x20a290 / catalog count 0x20b940 shared with the renderer. Its TGP use is balanced (push/pop) but it
 * restores the latched camera view on entry and before drawing, as the reference does
 * (model2_hw_restore_latched_view_matrix, a host addition), so the TGP matrix it leaves is the camera view.
 *
 * The original passes the i960 link register g14 as data in several TGP words and GEO bank pokes; the values it
 * holds at each point are reproduced (`link` below). */
#include "race/desert_node.h"
#include "race/desert_hooks.h"
#include "race/course.h"
#include "race/fp.h"
#include "race/player_scene.h"
#include "race/road.h"
#include "race/sound_cmd.h"
#include "sr_tgp.h"
#include "model2_tgp.h"
#include "m2_math.h"
#include <math.h>
#include <string.h>

#define TGP_PUSH   0x10002020u
#define TGP_POP    0x10802121u
#define TGP_EMIT   0x02800505u /* current matrix to the GEO (object placement) */

#define NODE_FLAGS     0x02u
#define NODE_POINT     0x14u
#define NODE_POS       0x18u
#define NODE_ANG       0x24u
#define NODE_DRIFT     0x30u
#define NODE_KEY       0x54u
#define NODE_NEAR      0x58u
#define NODE_TIME      0x5cu
#define NODE_ROTOR     0x60u
#define NODE_TAIL      0x64u
#define NODE_TILT      0x68u
#define NODE_DIVISOR   0x70u
#define NODE_DRIFT_K   0x74u

#define CENTRE_LINE    0x215c40u /* course centre line points, 3 words each */
#define CENTRE_COUNT   0x216a50u
#define DUST_SLOTS     64u
#define DUST_STRIDE    0x1cu

static void tgp0(uint32_t cmd) { sr_tgp_cmd(cmd, 0, 0, 0, 0); }
static void tgp1(uint32_t cmd, uint32_t a) { sr_tgp_cmd(cmd, &a, 1, 0, 0); }
static void tgp3(uint32_t cmd, uint32_t a, uint32_t b, uint32_t c) {
    const uint32_t in[3] = {a, b, c};
    sr_tgp_cmd(cmd, in, 3, 0, 0);
}
static void tgp4(uint32_t cmd, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    const uint32_t in[4] = {a, b, c, d};
    sr_tgp_cmd(cmd, in, 4, 0, 0);
}
/* TGP 0x50: distance between two points. */
static uint32_t tgp_distance(const uint32_t a[3], const uint32_t b[3]) {
    const uint32_t in[6] = {a[0], a[1], a[2], b[0], b[1], b[2]};
    uint32_t d;
    sr_tgp_cmd(0x28005050u, in, 6, &d, 1);
    return d;
}

/* ---- path table (0x41B78) ---- */

uint32_t sr_desert_path_table(void) {
    const uint32_t course = sr_race_course_index() << 2;
    if(sr_ld32(0x202230u) == 0) return sr_ld32(0x5e0b10u + course);
    if((int32_t)sr_ld32(0x20a560u) >= 0) return sr_ld32(0x5e0b50u + course);
    return sr_ld32(0x5e0b30u + course);
}

/* ---- node init (0x43790) ---- */

void sr_desert_node_init(uint32_t node) {
    /* sqrt, sine and divide of constants in double: the drift amount, the same every race. */
    double s = m2_sin(sqrt(D(0x3d798ae7u)));
    double d = sr_d(0x684bda13u, 0x3feda12fu);
    if(s != 0.0) d = d / s;
    sr_st32(node + NODE_DIVISOR, 0x41200000u); /* 10.0 */
    sr_st16(node + NODE_KEY, 0);
    sr_st32(node + NODE_ROTOR, 0);
    sr_st32(node + NODE_TAIL, 0);
    sr_st32(node + NODE_TILT, 0);
    sr_st32(node + NODE_TILT + 4, 0);
    sr_st32(node + NODE_POINT, 0);
    sr_st32(node + NODE_DRIFT + 8, 0);
    sr_st32(node + NODE_DRIFT + 4, 0);
    sr_st32(node + NODE_TIME, 0x447a0000u); /* 1000.0: off the end of the path until the first key */
    sr_st32(node + NODE_DRIFT, 0);
    sr_st32(node + 4, 0x005e17a0u);         /* handler: node_frame */
    sr_st32(node + NODE_DRIFT_K, F(d));
}

/* ---- nearest centre line point (0x41F80) ---- */

uint32_t sr_desert_near_pick(uint32_t query, uint32_t index) {
    const uint32_t count = sr_ld32(CENTRE_COUNT);
    const uint32_t q[3] = {sr_ld32(query), 0, sr_ld32(query + 8)};
    int32_t prev = (int32_t)index - 1;
    uint32_t next = index + 1, best = index, best_d, d;
    if(prev < 0) prev += (int32_t)count;
    if(next >= count) next -= count;
#define CENTRE_XZ(i) {sr_ld32(CENTRE_LINE + (i) * 12u), 0, sr_ld32(CENTRE_LINE + (i) * 12u + 8)}
    {
        const uint32_t p[3] = CENTRE_XZ(index);
        best_d = tgp_distance(q, p);
    }
    {
        const uint32_t p[3] = CENTRE_XZ(next);
        d = tgp_distance(q, p);
        if(sr_f(best_d) > sr_f(d)) {
            best = next;
            best_d = d;
        }
    }
    {
        const uint32_t p[3] = CENTRE_XZ((uint32_t)prev);
        d = tgp_distance(q, p);
        if(sr_f(best_d) > sr_f(d)) best = (uint32_t)prev;
    }
#undef CENTRE_XZ
    return best;
}

/* ---- pose along the path (0x422B0) ---- */

/* 0x42270: path row `index` (clamped to the table): xyz into xyz[], returns the row's angle word. */
static uint32_t path_row(int32_t index, uint32_t xyz[3]) {
    const uint32_t table = sr_desert_path_table();
    const int32_t count = (int16_t)sr_ld16(table + 2);
    uint32_t row;
    if(index >= count) index = count - 1;
    if(index < 0) index = 0;
    row = sr_ld32(table + 0x10) + (uint32_t)index * 24u;
    xyz[0] = sr_ld32(row);
    xyz[1] = sr_ld32(row + 4);
    xyz[2] = sr_ld32(row + 8);
    return sr_ld32(row + 0x10);
}

void sr_desert_pose_step(uint32_t node) {
    const double half_pi = sr_d(0x54442d18u, 0x3ff921fbu);
    const double inv6 = sr_d(0xc62af21cu, 0x3fc55554u); /* 1/6, as the ROM rounds it */
    const double t = D(sr_ld32(node + NODE_TIME));
    int32_t first = (int32_t)(t - 1.0);
    const int32_t i0 = (int32_t)t;
    uint32_t p[4][3], ang[4], w[4], u, um1, um2, up1, by, yaw, out[3];
    if((double)first < 0.0) first = 0;

    /* Four rows around t; the angles are stored a quarter turn back. */
    ang[0] = F(D(path_row(first, p[0])) - half_pi);
    ang[1] = F(D(path_row(i0, p[1])) - half_pi);
    ang[2] = F(D(path_row(i0 + 1, p[2])) - half_pi);
    ang[3] = path_row(i0 + 2, p[3]);

    /* Cubic weights at u = frac(t): the ROM's product chain, each product rounded. */
    u = F(t - (double)i0);
    um1 = fsub(u, 0x3f800000u);
    um2 = fsub(u, 0x40000000u);
    up1 = fadd(u, 0x3f800000u);
    w[0] = F(D(fmul(fmul(fneg(u), um1), um2)) * inv6);
    w[1] = fmul(fmul(fmul(up1, um1), um2), 0x3f000000u);
    w[2] = fmul(fmul(fmul(fneg(up1), u), um2), 0x3f000000u);
    w[3] = F(D(fmul(fmul(up1, u), um1)) * inv6);

    /* The TGP rotates the weighted xz into the current heading (yaw negated) and takes the node's xz off. */
    tgp0(TGP_PUSH);
    tgp0(0x12802525u);
    tgp1(0x15002a2au, fneg(sr_ld32(node + NODE_ANG + 4)));
    tgp4(0x21004242u, 0, fmul(w[0], p[0][0]), 0, fmul(w[0], p[0][2]));
    tgp4(0x22004444u, 0, fmul(w[1], p[1][0]), 0, fmul(w[1], p[1][2]));
    tgp4(0x22004444u, 0, fmul(w[2], p[2][0]), 0, fmul(w[2], p[2][2]));
    tgp4(0x22004444u, 0, fmul(w[3], p[3][0]), 0, fmul(w[3], p[3][2]));

    by = fadd(fadd(fmul(w[0], p[0][1]), fmul(w[1], p[1][1])), fadd(fmul(w[2], p[2][1]), fmul(w[3], p[3][1])));
    ang[3] = F(D(ang[3]) - half_pi);
    yaw = fadd(fadd(fmul(w[0], ang[0]), fmul(w[1], ang[1])), fadd(fmul(w[2], ang[2]), fmul(w[3], ang[3])));

    tgp4(0x22804545u, 0, sr_ld32(node + NODE_POS), 0, sr_ld32(node + NODE_POS + 8));
    tgp1(0x24004848u, 0);
    tgp0(TGP_POP);

    sr_st32(node + NODE_ANG + 4, yaw);
    sr_st32(node + NODE_POS + 4, fadd(by, 0x40200000u)); /* + 2.5 */

    /* Lateral and forward offset of the path from the node, over the divisor: the tilt. */
    {
        const uint32_t zero = 0;
        sr_tgp_cmd(0x21804343u, &zero, 1, out, 3);
    }
    {
        const double div = D(sr_ld32(node + NODE_DIVISOR));
        sr_st32(node + NODE_TILT + 4, F((D(out[2]) / div) * sr_d(0x4ae74487u, 0xbfc65718u)));
        sr_st32(node + NODE_TILT, F((D(out[0]) / div) * sr_d(0x4ae74487u, 0x3fc65718u)));
    }
}

/* ---- dust puffs (0x41BE0, 0x41CA0) ---- */

void sr_desert_dust_spawn(uint32_t xyz, uint32_t scale) {
    const double s = D(scale);
    uint32_t life = SR_DESERT_DUST, i;
    if(s >= 15.0) return;
    for(i = 0; !(sr_f(sr_ld32(life)) <= -1.5f); i++) {
        if(i + 1 > 0x3f) return;
        life += DUST_STRIDE;
    }
    sr_st32(life - 0x18, sr_ld32(xyz));
    sr_st32(life - 0x14, sr_ld32(xyz + 4));
    sr_st32(life - 0x10, sr_ld32(xyz + 8));
    sr_st32(life, F((15.0 - s) * sr_d(0x851eb852u, 0x3fb851ebu)));
}

/* GEO list slot for the next object: the cursor 0x20a290 advances by a word and the slot gets the GEO write
 * pointer. */
static uint32_t geo_list_slot(void) {
    const uint32_t slot = sr_ld32(0x20a290u);
    const uint32_t wr = sr_hook_geo_list_ptr();
    sr_st32(0x20a290u, slot + 4);
    if(slot != 0) sr_st32(slot, wr);
    return wr;
}

static void catalog(uint32_t entry) {
    sr_hook_geo_catalog(entry);
    sr_st32(SR_DESERT_CATALOGS, sr_ld32(SR_DESERT_CATALOGS) + sr_ld32(entry + 12));
}

/* The current TGP matrix as an object, drawn with model catalog `entry` (bank word: the original's g14). */
static void draw_object(uint32_t entry, uint32_t bank_word) {
    const uint32_t wr = geo_list_slot();
    tgp0(TGP_EMIT);
    sr_hook_geo_object_start(wr);
    sr_hook_geo_bank_word(bank_word);
    catalog(entry);
}

void sr_desert_dust_draw(void) {
    uint32_t life = SR_DESERT_DUST, y_off = 0;
    /* The original reads each puff's y at 0x216a74 + 4 * (0x1c * i), not the puff's own: only the first puff
     * has its own height. */
    while(life <= SR_DESERT_DUST_END && !(sr_f(sr_ld32(life)) <= -1.5f)) {
        const uint32_t x = sr_ld32(life - 0x18), z = sr_ld32(life - 0x10);
        const uint32_t y = sr_ld32(0x216a74u + (y_off << 2));
        const uint32_t slot = sr_ld32(0x20a290u);
        const uint32_t wr = sr_hook_geo_list_ptr();
        const uint32_t size = fsub(0x40000000u, fabs_bits(sr_ld32(life))); /* 2 - |life| */
        if(slot != 0) sr_st32(slot, wr);
        {
            /* 0x55: billboard at xyz facing the eye (0x20220c / 0x202214), sized 1.4 x size x 1. */
            const uint32_t in[10] = {x, y, z, sr_ld32(0x20220cu), x, sr_ld32(0x202214u), z,
                                     0x3fb33333u, size, 0x3f800000u};
            sr_tgp_cmd(0x2a805555u, in, 10, 0, 0);
        }
        sr_st32(0x20a290u, slot + 4);
        sr_hook_geo_object_start(wr);
        {
            const uint32_t aged = F(D(sr_ld32(life)) - sr_d(0xeb851eb8u, 0x3faeb851u)); /* - 0.06 */
            sr_hook_geo_bank_word(0);
            sr_hook_geo_bank_word(0);
            catalog(0x028655c0u);
            sr_st32(life, aged);
        }
        life += DUST_STRIDE;
        y_off += DUST_STRIDE;
    }
}

/* ---- rotor sound (0x42190) ---- */

void sr_desert_sound_near(uint32_t node) {
    const uint32_t cam = sr_ld32(0x213980u);
    const uint32_t a[3] = {sr_ld32(node + NODE_POS), sr_ld32(node + NODE_POS + 4), sr_ld32(node + NODE_POS + 8)};
    const uint32_t b[3] = {sr_ld32(cam), sr_ld32(cam + 4), sr_ld32(cam + 8)};
    const uint32_t dist = tgp_distance(a, b);
    const uint32_t near = sr_ld8(node + NODE_NEAR);
    if(sr_f(dist) < 35.0f) {
        if((near & 1u) == 0) sr_sound_cmd_fill(0x97u, 0x7fu);
        sr_st8(node + NODE_NEAR, sr_ld8(node + NODE_NEAR) | 1u);
    } else {
        if(near & 1u) sr_sound_cmd_fill(0x97u, 0);
        sr_st8(node + NODE_NEAR, sr_ld8(node + NODE_NEAR) & ~1u);
    }
}

/* ---- flag block (0x46470) ---- */

uint32_t sr_desert_slot_flag(uint32_t pos, uint32_t angles, uint32_t lane, int32_t rank) {
    int32_t capped = rank;
    uint32_t flags, i;
    if(rank < 0) {
        flags = sr_ld8(SR_DESERT_FLAG);
        if(((flags >> 1) & 15u) == lane) {
            sr_st8(SR_DESERT_FLAG, flags & ~1u);
            return 0;
        }
    }
    if(rank >= 3) capped = 3;
    flags = sr_ld8(SR_DESERT_FLAG);
    /* A higher rank already holding the flag keeps it. */
    if(((flags >> 5) & 3u) > (uint32_t)capped && (flags & 1u)) return 1;
    flags |= 1u;
    flags = (flags & ~0x1eu) | ((lane & 15u) << 1);
    flags = (flags & ~0x60u) | (((uint32_t)capped & 3u) << 5);
    sr_st8(SR_DESERT_FLAG, flags);
    for(i = 0; i < 3; i++) {
        sr_st32(0x20a738u + i * 4, sr_ld32(pos + i * 4));
        sr_st32(0x20a744u + i * 4, sr_ld32(angles + i * 4));
    }
    return 1;
}

/* ---- the tick (0x427A0) ---- */

/* Shadow on the ground under the helicopter (close-up, height < 15 over a surface with a code <= 14): tilted by
 * the ground normal (TGP 0x3e angles), scaled by height, with a rotor shadow. */
static void draw_shadow(uint32_t node, const uint8_t *road, uint32_t link) {
    const uint32_t nx = frame_ld(road, 0x10), ny = frame_ld(road, 0x0c), nz = frame_ld(road, 0x14);
    const uint32_t height = frame_ld(road, 0x18);
    uint32_t in[2], tilt_x, tilt_z, scale;
    tgp0(TGP_PUSH);
    in[0] = nx;
    in[1] = ny;
    sr_tgp_cmd(0x1f003e3eu, in, 2, &tilt_x, 1);
    in[1] = nz;
    sr_tgp_cmd(0x1f003e3eu, in, 2, &tilt_z, 1);
    tgp3(0x13802727u, sr_ld32(node + NODE_POS),
         F((D(sr_ld32(node + NODE_POS + 4)) + D(height)) + sr_d(0xeb851eb8u, 0x3f9eb851u)), /* + 0.03 */
         sr_ld32(node + NODE_POS + 8));
    scale = F(sr_d(0x9999999au, 0x3fe99999u) - D(height) * sr_d(0x47ae147bu, 0x3f847ae1u)); /* 0.8 - h/100 */
    tgp1(0x15002a2au, sr_ld32(node + NODE_ANG + 4));
    tgp1(0x1a003434u, tilt_x & 0xffffu);
    tgp1(0x19003232u, tilt_z & 0xffffu);
    tgp3(0x14002828u, scale, 0x3f800000u, scale);
    draw_object(0x028657c0u, link);
    tgp1(0x15002a2au, sr_ld32(node + NODE_TAIL));
    draw_object(0x028657a0u, link);
    tgp0(TGP_POP);
}

void sr_desert_node_frame(uint32_t node) {
    uint8_t road[0x20];
    uint32_t link, view, table, key, turn;
    int32_t at_flags;

    /* Host reference: re-assert the latched camera view (list nodes before may have left the TGP elsewhere). */
    model2_tgp_restore_latched_view_matrix();

    /* Followed pose: the cam follow block, or in a practice run before the clock (timer < 0) the leading car. */
    if(sr_ld32(0x202230u) == 0 || (int32_t)sr_ld32(0x20a560u) >= 0)
        sr_st32(SR_DESERT_VIEW_BASE, 0x213b40u);
    else
        sr_st32(SR_DESERT_VIEW_BASE, sr_ld32(0x213b00u));

    /* TGP 0x58 (|B - A|) with A = (pose x, helicopter x, link), B = (link, pose y, helicopter z), in the original's
     * word order; the link register (0x4285c after the bal) fills two words. It slows the path time below. */
    link = 0x4285cu;
    view = sr_ld32(SR_DESERT_VIEW_BASE);
    {
        const uint32_t in[6] = {sr_ld32(view), sr_ld32(node + NODE_POS), link, link, sr_ld32(view + 4),
                                sr_ld32(node + NODE_POS + 8)};
        sr_tgp_cmd(0x2c005858u, in, 6, &turn, 1);
    }

    /* Path keys: when the followed pose's span (view+0x56) is below the next key's limit, restart the path from
     * row 0 at that span; table_pick has left the link at 0, which becomes the path time. */
    table = sr_desert_path_table();
    link = 0;
    key = sr_ld16(node + NODE_KEY);
    {
        const int32_t cur = (int16_t)key;
        if((int16_t)sr_ld16(view + 0x56) < (int16_t)sr_ld16(table + (uint32_t)(cur * 5) * 4u)) {
            const uint32_t row = sr_ld32(table + 0x10);
            sr_st32(node + NODE_POINT, (uint32_t)(int16_t)sr_ld16(view + 0x54));
            sr_st32(node + NODE_POS, sr_ld32(row));
            sr_st32(node + NODE_POS + 4, sr_ld32(row + 4));
            sr_st32(node + NODE_TIME, link);
            sr_st32(node + NODE_POS + 8, sr_ld32(row + 8));
            sr_st16(node + NODE_KEY, key + 1);
        }
    }

    if(D(sr_ld32(node + NODE_TIME)) >= (double)(int16_t)sr_ld16(table + 2)) {
        /* Past the path's end: idle, flag released. */
        sr_st8(node + NODE_FLAGS, sr_ld8(node + NODE_FLAGS) & ~2u);
        sr_desert_slot_flag(node + NODE_POS, node + NODE_ANG, 0, -1);
        return;
    }

    sr_st8(node + NODE_FLAGS, sr_ld8(node + NODE_FLAGS) | 2u);
    sr_st32(node + NODE_POINT, sr_desert_near_pick(node + NODE_POS, sr_ld32(node + NODE_POINT)));
    sr_desert_pose_step(node);

    /* Drift: (0, 0, drift amount) turned by yaw, then by tilt, read back into node+0x30..0x38. */
    tgp0(TGP_PUSH);
    tgp0(0x12802525u);
    tgp1(0x15002a2au, sr_ld32(node + NODE_ANG + 4));
    tgp0(TGP_PUSH);
    tgp1(0x15802b2bu, sr_ld32(node + NODE_TILT));
    tgp1(0x14802929u, sr_ld32(node + NODE_TILT + 4));
    tgp4(0x21004242u, link, link, sr_ld32(node + NODE_DRIFT_K), link);
    tgp1(0x24004848u, link);
    tgp0(TGP_POP);
    tgp0(TGP_POP);
    {
        uint32_t drift[3];
        sr_tgp_cmd(0x21804343u, &link, 1, drift, 3);
        sr_st32(node + NODE_DRIFT, drift[0]);
        sr_st32(node + NODE_DRIFT + 4, drift[1]);
        sr_st32(node + NODE_DRIFT + 8, drift[2]);
    }

    /* The road under it, then the drift, the rotors and the path time. */
    sr_road_query_a(sr_race_course_index(), sr_ld32(node + NODE_POINT), sr_at(node + NODE_POS), 0);
    sr_st32(node + NODE_POS, fadd(sr_ld32(node + NODE_POS), sr_ld32(node + NODE_DRIFT)));
    sr_st32(node + NODE_POS + 8, fadd(sr_ld32(node + NODE_POS + 8), sr_ld32(node + NODE_DRIFT + 8)));
    sr_st32(node + NODE_ANG, sr_ld32(node + NODE_TILT + 4));
    sr_st32(node + NODE_ANG + 8, sr_ld32(node + NODE_TILT));
    sr_st32(node + NODE_ROTOR, fadd(sr_ld32(node + NODE_ROTOR), 0x3f400000u)); /* + 0.75 */
    {
        /* Tail rotor + 2.3167; time += rate - rate * turn * 0.00071 (one rounding, as the original). */
        const double tail = D(sr_ld32(node + NODE_TAIL)) + sr_d(0x88888888u, 0x40028888u);
        const double rate = D(sr_ld32(table + 4));
        const double slow = D(turn) * sr_d(0xe434a9b1u, 0x3f4767dcu);
        sr_st32(node + NODE_TAIL, F(tail));
        sr_st32(node + NODE_TIME, F(D(sr_ld32(node + NODE_TIME)) + (rate - rate * slow)));
    }
    memset(road, 0, sizeof road);
    sr_road_query_b(sr_at(node + NODE_POS), road, 0);

    model2_tgp_restore_latched_view_matrix();
    /* Link after road_query_b (call 0x2acc0 returns to 0x42b38). */
    link = 0x42b38u;

    /* Surface code road+0x1c, height above it road+0x18. */
    at_flags = (int32_t)frame_ld(road, 0x1c);
    if(at_flags <= 14 && sr_f(frame_ld(road, 0x18)) > -15.0f) draw_shadow(node, road, link);

    /* Body at the node's pose, its catalog drawn 2.5 below the pose. */
    tgp0(TGP_PUSH);
    tgp3(0x13802727u, sr_ld32(node + NODE_POS), sr_ld32(node + NODE_POS + 4), sr_ld32(node + NODE_POS + 8));
    tgp1(0x15002a2au, sr_ld32(node + NODE_ANG + 4));
    tgp1(0x15802b2bu, sr_ld32(node + NODE_ANG + 8));
    tgp1(0x14802929u, sr_ld32(node + NODE_ANG));
    tgp0(TGP_PUSH);
    tgp3(0x13802727u, link, 0xc0200000u, link);
    draw_object(0x02865790u, link);
    sr_hook_pen_push(node + NODE_POS, node + NODE_ANG, 0x20220cu, 10);
    link = 0x42fdcu; /* call 0x3ef10 returns to 0x42fdc */
    tgp0(TGP_POP);
    /* Main rotor. */
    tgp0(TGP_PUSH);
    tgp1(0x15002a2au, sr_ld32(node + NODE_ROTOR));
    draw_object(0x028657a0u, link);
    tgp0(TGP_POP);
    /* Tail rotor, 1.15 down and 6 back, spun about x. */
    tgp3(0x13802727u, link, 0xbf933333u, 0xc0c00000u);
    tgp1(0x14802929u, sr_ld32(node + NODE_TAIL));
    draw_object(0x028657b0u, link);
    tgp0(TGP_POP);

    /* Over sand and dirt: a dust puff near the ground under it, jittered by two RNG draws. */
    if(at_flags == 2 || at_flags == 3 || at_flags == 6 || at_flags == 7) {
        const double span = sr_d(0, 0x40dfffc0u), k = sr_d(0x39ffd60fu, 0x3f1797ccu); /* 32767, 1/11000 */
        const uint32_t height = frame_ld(road, 0x18);
        uint32_t x, y, z;
        x = F(D(sr_ld32(node + NODE_POS)) + ((double)(sr_rng_next() & 0xffffu) - span) * k);
        y = F(D(sr_ld32(node + NODE_POS + 4)) + D(height) + sr_d(0x47ae147bu, 0x3f847ae1u));
        z = F(D(sr_ld32(node + NODE_POS + 8)) + ((double)(sr_rng_next() & 0xffffu) - span) * k);
        sr_st32(0x216a70u, x);
        sr_st32(0x216a74u, y);
        sr_st32(0x216a78u, z);
        sr_desert_dust_spawn(0x216a70u, fneg(height));
    }
    sr_desert_dust_draw();
    sr_desert_sound_near(node);

    /* Inside the table's time window the helicopter holds the flag block with rank 3. */
    {
        const float t = sr_f(sr_ld32(node + NODE_TIME));
        const int in_window = (sr_ld8(node + NODE_FLAGS) & 2u) && t >= sr_f(sr_ld32(table + 8)) &&
                              t < sr_f(sr_ld32(table + 0xc));
        sr_desert_slot_flag(node + NODE_POS, node + NODE_ANG, 0, in_window ? 3 : -1);
    }
}
