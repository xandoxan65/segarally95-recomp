/* Road surface queries on the TGP and the road cell search (0x2ABC0, 0x2ACC0, 0x2AED0, 0x2B0B0, 0x27B28).
 *
 * A course's road is a ring of quad cells. sr_road_cell_pack finds the cell under a point near its previous one;
 * sr_road_query_a hands the point and the cell's neighbourhood to the TGP (0x52 walks the course's collision data
 * in the copro data ROM), and sr_road_query_b reads the result back (0x53): a support plane, or for a wall
 * (flag bit 7) a wall normal and distance. Points and outputs are host pointers: frame temporaries, or RAM through
 * sr_at(). */
#include "race/road.h"
#include "race/course.h"
#include "race/fp.h"
#include "sr_tgp.h"

void sr_road_query_a(uint32_t course, uint32_t index, const uint8_t *xyz, uint32_t flags) {
    const uint32_t row = 0x5dcac0u + ((course * 3u) << 2);
    const uint32_t word0 = sr_ld32(row), word1 = sr_ld32(row + 4);
    uint32_t slot = sr_ld32(0x20a290u), count, cursor, blk[9];
    int32_t prev, next;

    /* 0x07: the course's cell count, looked up from the row word put through the cursor. */
    sr_st32(slot, word0);
    sr_tgp_arm_course_handle(word0);
    sr_tgp_cmd(0x03800707u, 0, 0, &count, 1);
    sr_st32(0x20a290u, slot + 4);
    if((int32_t)index < 0 || (int32_t)index >= (int32_t)count) return;

    prev = (int32_t)index - 1;
    if(prev < 0) prev += (int32_t)count;
    next = (int32_t)index + 1;
    if(next >= (int32_t)count) next -= (int32_t)count;

    cursor = sr_ld32(0x20a290u);
    blk[0] = frame_ld(xyz, 0);
    blk[1] = frame_ld(xyz, 4);
    blk[2] = frame_ld(xyz, 8);
    blk[3] = word1;
    blk[4] = count << 2;
    blk[5] = (uint32_t)prev << 2;
    blk[6] = index << 2;
    blk[7] = (uint32_t)next << 2;
    blk[8] = flags << 1;
    {
        uint32_t i;
        for(i = 0; i < 9; i++) sr_st32(cursor + i * 4, blk[i]);
    }
    sr_tgp_arm_52_block(blk);
    sr_tgp_cmd(0x29005252u, 0, 0, 0, 0);
    sr_st32(0x20a290u, cursor + 0x24);
}

void sr_road_query_b(const uint8_t *xyz, uint8_t *out, uint32_t slot) {
    uint32_t r[17], flag;
    const uint32_t arg = slot << 1;
    sr_tgp_cmd(0x29805353u, &arg, 1, r, 17);
    /* r[0..9] → fp+0x60..0x84, r[10] 0x88, r[11] 0x8c, r[12..15] → fp+0x50..0x5c (the plane), r[16] the flag. */
    flag = r[16];
    if(flag == 0) {
        /* No surface: only the flag; the plane words keep the last valid plane. */
        frame_st(out, 0x1c, 15);
        return;
    }
    {
        const uint32_t masked = (flag >> 16) & 0x8fu;
        frame_st(out, 0x1c, masked);
        if(!(masked & 0x80u)) {
            /* Support plane Ax + By + Cz + D = 0: height of the point above it, and its unit normal. */
            const uint32_t a = r[12], b = r[13], c = r[14], d = r[15];
            uint32_t h = fsub(fmul(fneg(a), frame_ld(xyz, 0)), fmul(c, frame_ld(xyz, 8)));
            uint32_t n[3];
            const uint32_t in[3] = {a, b, c};
            h = fdiv(fsub(h, d), b);
            h = fsub(h, frame_ld(xyz, 4));
            frame_st(out, 0x18, h);
            sr_tgp_cmd(0x17802f2fu, in, 3, n, 3);
            frame_st(out, 0x0c, n[0]);
            frame_st(out, 0x10, n[1]);
            frame_st(out, 0x14, n[2]);
            return;
        }
    }
    /* Wall: normal as given, distance from the wall's edge (fp+0x60/68 to 0x6c/74) over its length r[15]. */
    {
        const uint32_t ox = frame_ld(xyz, 0), oz = frame_ld(xyz, 8);
        uint32_t g4, g6;
        frame_st(out, 0x0c, r[12]);
        frame_st(out, 0x10, r[13]);
        frame_st(out, 0x14, r[14]);
        g4 = fmul(fsub(r[0], ox), fsub(r[5], oz));
        g6 = fmul(fsub(r[2], oz), fsub(r[3], ox));
        g4 = fabs_bits(fsub(g4, g6));
        frame_st(out, 0x18, fdiv(g4, r[15]));
    }
}

/* Cell side test: the point is inside a quad when its four edge crosses (0x5c) all have one sign. */
uint32_t sr_road_cell_probe(uint32_t pts, uint32_t rec, const uint8_t *xyz) {
    uint32_t x[4], z[4], e[4], i;
    const uint32_t ox = frame_ld(xyz, 0), oz = frame_ld(xyz, 8);
    for(i = 0; i < 4; i++) {
        const uint32_t ea = pts + (sr_ld16(rec + 4 + i * 2) * 3u << 2);
        x[i] = sr_ld32(ea);
        z[i] = sr_ld32(ea + 8);
    }
    for(i = 0; i < 4; i++) {
        const uint32_t j = (i + 1) & 3;
        const uint32_t in[6] = {oz, z[i], x[j], ox, x[i], z[j]};
        sr_tgp_cmd(0x2e005c5cu, in, 6, &e[i], 1);
    }
    if((int32_t)(e[0] & e[1] & e[2] & e[3]) < 0) return 1;
    if((int32_t)(e[0] | e[1] | e[2] | e[3]) < 0) return 0;
    return 1;
}

/* Search the cells around the previous one (alternating behind/ahead) for the one under xyz. `index` points at
 * two halfwords: the cell found and a running counter. Returns the cell, or -1 with both set to 0xffff. */
int32_t sr_road_cell_pack(const uint8_t *xyz, uint8_t *index) {
    const uint32_t tab = sr_race_course_index() * 3u;
    const uint32_t base = sr_ld32(0x5dcac8u + (tab << 2));
    const uint32_t n = sr_ld32(base), pts = sr_ld32(base + 4), recs = sr_ld32(base + 8);
    uint16_t half;
    int32_t r7, r5;
    memcpy(&half, index + 2, 2);
    r7 = (int16_t)half;
    if(r7 < 0) do r7 += (int32_t)n; while(r7 < 0);
    if((int32_t)n != 0) r7 %= (int32_t)n;
    if((int32_t)n > 0) {
        for(r5 = 0; r5 < (int32_t)n; r5++) {
            const int32_t r8 = (r5 & 1) ? (r5 >> 1) + 1 : (-r5) >> 1;
            const int32_t cell = (int32_t)((uint32_t)(r7 + r8 + (int32_t)n)) % (int32_t)n;
            if(sr_road_cell_probe(pts, recs + (((uint32_t)cell * 3u) << 2), xyz) & 0xffu) {
                const uint16_t c = (uint16_t)cell;
                uint16_t h;
                memcpy(&h, index + 2, 2);
                h = (uint16_t)(h + (uint32_t)r8);
                memcpy(index, &c, 2);
                memcpy(index + 2, &h, 2);
                return cell;
            }
        }
    }
    memset(index, 0xff, 4);
    return -1;
}

/* Wrap an angle into [-pi, pi] by one turn. */
uint32_t sr_angle_wrap(uint32_t bits) {
    const double pi = sr_d(0x54442d18u, 0x400921fbu), two_pi = sr_d(0x54442d18u, 0x401921fbu);
    float x = sr_f(bits);
    if(x > 0.f) {
        if((double)x > pi) x = (float)((double)x - two_pi);
    } else if((double)x < -pi) {
        x = (float)((double)x + two_pi);
    }
    return sr_b(x);
}
