/* Road span tracking (0x311E0) and the span basis it builds on the TGP (0x31320).
 *
 * A course's span table is rows of 16 bytes (x, y, z, attribute) along the road. Each tick the car's XZ is
 * projected on its current span segment; past the end it moves on (wrapping at the course's span count), the
 * road height under the car is interpolated into node+0x94, and the basis routine places the lateral offset
 * node+0xa8 across a quadratic B-spline through three rows, writing the world XZ to node+0x90/0x98.
 * Both write temporaries into the caller's frame (car_frame's), which later code there reads. */
#include "race/car.h"
#include "race/course.h"
#include "race/fp.h"
#include "sr_tgp.h"

static void tgp0(uint32_t cmd) { sr_tgp_cmd(cmd, 0, 0, 0, 0); }

static void tgp3(uint32_t cmd, uint32_t a, uint32_t b, uint32_t c) {
    const uint32_t in[3] = {a, b, c};
    sr_tgp_cmd(cmd, in, 3, 0, 0);
}

void sr_road_span_basis(uint32_t row, uint32_t tbits, uint32_t ox, uint32_t out, uint8_t *frame) {
    const double t = D(tbits), t2 = t * t, half = 0.5;
    /* Quadratic B-spline weights; the i960's subrl is src2 - src1. */
    const uint32_t ht2 = F(t2 * half);
    const uint32_t w_g5 = F(t - 1.0);
    const uint32_t w_r5 = F(1.0 - (t + t));
    const uint32_t w_r4 = F(-t2 + t + half);
    const uint32_t w_g4 = F(t2 * half - t + half);
    const double f00 = D(sr_ld32(row)), f08 = D(sr_ld32(row + 8)), f10 = D(sr_ld32(row + 0x10));
    const double f18 = D(sr_ld32(row + 0x18)), f20 = D(sr_ld32(row + 0x20)), f28 = D(sr_ld32(row + 0x28));
    const double wr5 = D(w_r5), wg5 = D(w_g5), wr4 = D(w_r4), wg4 = D(w_g4), fht2 = D(ht2);
    uint32_t bx, by, bz, cx, cursor, o[3];

    {
        double s_r8 = wr5 * f10, s_r6 = wg5 * f00, s_g7 = wr4 * f10, s_r9 = wg4 * f00;
        double s_g5 = wg5 * f08, s_r5 = wr5 * f18, s_r4 = wr4 * f18, s_g4 = wg4 * f08;
        double s_g0 = t * f20, s_g6 = fht2 * f20, s_g1 = t * f28, s_g7b = fht2 * f28;
        s_g5 += s_r5;
        s_r9 += s_g7;
        s_r6 += s_r8;
        s_g4 += s_r4;
        s_g5 += s_g1;
        bx = F(s_r9 + s_g6);
        by = F(s_g5);
        bz = F(s_r6 + s_g0);
        cx = F(s_g4 + s_g7b);
    }
    frame_st(frame, 0x50, bz);
    frame_st(frame, 0x54, 0);
    frame_st(frame, 0x40, bx);
    frame_st(frame, 0x44, 0);
    frame_st(frame, 0x58, by);
    frame_st(frame, 0x48, cx);

    tgp0(0x10002020u);
    tgp0(0x12802525u);
    tgp3(0x2a005454u, bz, 0, by);
    tgp3(0x13802727u, fneg(bx), 0, fneg(cx));

    /* Matrix slot 0: the span origin; slot 16: oriented along the span. 0x20a290 is the TGP list cursor. */
    cursor = sr_ld32(0x20a290u);
    if(cursor) sr_st32(cursor, 0);
    tgp0(0x30006060u);
    tgp0(0x12802525u);
    tgp3(0x13802727u, bx, 0, cx);
    if(cursor) {
        cursor += 4;
        sr_st32(0x20a290u, cursor);
    }
    tgp3(0x28805151u, bz, 0, by);
    if(cursor) sr_st32(cursor, 16);
    tgp0(0x30006060u);
    {
        const uint32_t in[3] = {ox, 0, 0};
        sr_tgp_cmd(0x16002c2cu, in, 3, o, 3);
    }
    if(out) {
        sr_st32(out, o[0]);
        sr_st32(out + 8, o[2]);
    }
    frame_st(frame, 0x44, o[1]);
    tgp0(0x10802121u);
    if(cursor) {
        cursor += 4;
        sr_st32(0x20a290u, cursor);
    }
    sr_st32(0x20cad8u, ox);
    sr_st32(0x20cad0u, bz);
    sr_st32(0x20cad4u, 0);
}

void sr_road_span(uint32_t node, uint32_t span_base, uint8_t *frame) {
    uint32_t index, row, dx, dz, rx, rz, t, max_n, basis_ix, basis_t;
    double tp;
    if(node == 0 || span_base == 0) return;

    index = sr_ld32(node + CAR_SPAN);
    row = span_base + (index << 4);
    dx = fsub(sr_ld32(row + 0x10), sr_ld32(row));
    dz = fsub(sr_ld32(row + 0x18), sr_ld32(row + 8));
    rx = fsub(sr_ld32(node + CAR_POS), sr_ld32(row));
    rz = fsub(sr_ld32(node + CAR_POS + 8), sr_ld32(row + 8));
    {
        const uint32_t num = F(D(dx) * D(rx) + D(dz) * D(rz));
        const uint32_t den = F(D(dx) * D(dx) + D(dz) * D(dz));
        t = fdiv(num, den);
    }
    max_n = sr_ld32(0x5dce70u + (sr_race_course_index() << 4));
    frame_st(frame, 0x58, rz);
    frame_st(frame, 0x50, rx);
    frame_st(frame, 0x48, dz);
    frame_st(frame, 0x40, dx);

    if(!(D(t) >= 0.0)) t = 0;
    if(D(t) > 1.0) {
        const int32_t ti = (int32_t)D(t);
        index = (uint32_t)((int32_t)index + ti);
        t = fsub(t, F((double)ti));
        if((int32_t)index >= (int32_t)max_n) index = (uint32_t)((int32_t)index - (int32_t)max_n);
    }
    sr_st32(node + CAR_SPAN, index);
    row = span_base + (index << 4);
    {
        const double y0 = D(sr_ld32(row + 4)), y1 = D(sr_ld32(row + 0x14));
        sr_st32(node + CAR_ROAD_Y, F((y1 - y0) * D(t) + y0));
    }

    tp = D(t) + 0.5;
    basis_ix = sr_ld32(node + CAR_SPAN);
    basis_t = F(tp);
    if(tp > 1.0) {
        basis_t = F(tp - 1.0);
        basis_ix++;
        if((int32_t)basis_ix >= (int32_t)max_n) basis_ix = 0;
    }
    sr_road_span_basis(span_base + (basis_ix << 4), basis_t, sr_ld32(node + CAR_LATERAL), node + CAR_ROAD_XZ, frame);
}
