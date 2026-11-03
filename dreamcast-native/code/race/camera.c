/* Race camera: handler slot (0x20310), chase view (0x1FDC0) and its light direction (0x29120).
 *
 * The chase camera copies the followed object's pose (0x213980[0], the cam follow block at 0x213b40) to the eye
 * at 0x20220c and its angles to 0x202200, turns the TGP's view by the negated angles, sets the eye 0.75 above the
 * object, latches that as the view, and eases 0x20ab5c towards the yaw. */
#include "race/camera.h"
#include "race/fp.h"
#include "race/road.h"
#include "race/render_hooks.h"
#include "sr_tgp.h"
#include "model2_tgp.h"
#include <stdio.h>

static void not_ported(const char *what) {
    static int reported;
    if(!reported++) printf("sr: %s is not ported yet\n", what);
}

void sr_cam_light_dir(uint32_t x, uint32_t y, uint32_t z, uint32_t g14) {
    uint32_t m[12], in[3] = {x, y, z}, out[3];
    sr_tgp_cmd(0x13002626u, 0, 0, m, 12);
    m[9] = m[10] = m[11] = 0;
    sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
    sr_tgp_cmd(0x11002222u, m, 12, 0, 0);
    sr_tgp_cmd(0x16002c2cu, in, 3, out, 3);
    sr_hook_geo_light_dir(g14, out[0], out[1], out[2]);
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
}

void sr_cam_rebind(void) {
    sr_cam_bind(sr_ld32(0x2140c0u) ? SR_CAM_CHASE_ALT : SR_CAM_CHASE, 0);
}

void sr_cam_chase(void) {
    const uint32_t obj = sr_ld32(0x213980u);
    uint32_t px, py, pz, pitch, yaw, roll, blend;
    if(obj == 0 || !sr_at(obj)) return;
    px = sr_ld32(obj);
    py = sr_ld32(obj + 4);
    pz = sr_ld32(obj + 8);
    sr_st32(0x20220cu, px);
    sr_st32(0x202210u, py);
    sr_st32(0x202214u, pz);
    pitch = sr_ld32(obj + 0x18);
    yaw = sr_ld32(obj + 0x1c);
    roll = sr_ld32(obj + 0x20);
    sr_st32(0x202200u, pitch);
    sr_st32(0x202204u, yaw);
    sr_st32(0x202208u, roll);

    {
        uint32_t a = fneg(roll);
        sr_tgp_cmd(0x15802b2bu, &a, 1, 0, 0);
        a = fneg(pitch);
        sr_tgp_cmd(0x14802929u, &a, 1, 0, 0);
        a = fneg(yaw);
        sr_tgp_cmd(0x15002a2au, &a, 1, 0, 0);
    }
    /* The original passes the g14 its caller left in the register; in the race that is 0. */
    sr_cam_light_dir(0xbf34fdf4u, 0xbf34fdf4u, 0x3f34fdf4u, 0);
    /* The reference skips the eye translate when the followed pose is not finite. */
    if(finite_bits(px) && finite_bits(py) && finite_bits(pz)) {
        const uint32_t t[3] = {fneg(px), F(D(fneg(py)) - 0.75), fneg(pz)};
        sr_tgp_cmd(0x13802727u, t, 3, 0, 0);
    }
    sr_st32(0x2139d8u, 0);
    model2_tgp_latch_view_matrix();
    sr_cam_rebind();
    blend = sr_angle_wrap(fadd(sr_ld32(0x202204u), sr_ld32(0x20ab5cu)));
    blend = F(D(sr_ld32(0x20ab5cu)) - D(blend) * sr_d(0x9999999au, 0x3fa99999u));
    sr_st32(0x20ab5cu, blend);
    sr_st32(0x20ab5cu, sr_angle_wrap(blend));
}

void sr_spline_point(uint32_t table_va, uint32_t tbits, uint32_t out[3]) {
    /* Basis weights in double as the original keeps them; each product and sum rounds to float. */
    const double t = D(tbits), omt = 1.0 - t;
    const uint32_t t2f = F(t * t), t3f = F(t * D(t2f));
    const double n0 = omt * (omt * omt) / 6.0;
    const double n1 = 0.66666666666666663 + (D(t3f) / 2.0 - D(t2f));
    const double n2 = 0.16666666666666666 + (t / 2.0 + (D(t2f) / 2.0 - D(t3f) / 2.0));
    const double n3 = D(t3f) / 6.0;
    const uint32_t w0 = F(n0), w1 = F(n1), w2 = F(n2), w3 = F(n3);
    uint32_t i;
    for(i = 0; i < 3; i++) {
        uint32_t v = fadd(fmul(sr_ld32(table_va + 0xc + i * 4), w1), fmul(sr_ld32(table_va + i * 4), w0));
        v = fadd(fmul(sr_ld32(table_va + 0x18 + i * 4), w2), v);
        out[i] = fadd(fmul(sr_ld32(table_va + 0x24 + i * 4), w3), v);
    }
}

void sr_matrix_row_swap(uint8_t *m) {
    static const uint8_t swap[3][2] = {{4, 12}, {8, 24}, {20, 28}};
    uint32_t t[3], i, r[12];
    for(i = 0; i < 3; i++) {
        const uint32_t a = frame_ld(m, swap[i][0]), b = frame_ld(m, swap[i][1]);
        frame_st(m, swap[i][0], b);
        frame_st(m, swap[i][1], a);
    }
    for(i = 0; i < 3; i++) {
        t[i] = fneg(frame_ld(m, 0x24 + i * 4));
        frame_st(m, 0x24 + i * 4, 0);
    }
    for(i = 0; i < 12; i++) r[i] = frame_ld(m, i * 4);
    sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
    sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
    sr_tgp_cmd(0x11002222u, r, 12, 0, 0);
    sr_tgp_cmd(0x13802727u, t, 3, 0, 0);
    sr_tgp_cmd(0x13002626u, 0, 0, r, 12);
    for(i = 0; i < 12; i++) frame_st(m, i * 4, r[i]);
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
}

/* The opening fly-in: the eye moves along a spline through four points set around the followed car (0x5be9f0..),
 * t advancing by about 0.006 a tick; at t >= 1 the chase camera takes over. Fades are queued at 0x213840. */
void sr_cam_desert(void) {
    const uint32_t obj = sr_ld32(0x213980u);
    uint32_t ctrl[4][3], pt[3], pos[3], ang[3], i, eye[3], spl[3];
    double t, t_bump;
    if(obj == 0 || !sr_at(obj)) return;
    for(i = 0; i < 3; i++) {
        pos[i] = sr_ld32(obj + i * 4);
        ang[i] = sr_ld32(obj + 0x18 + i * 4);
    }
    /* Each control point: the point 0x5c7890 in the car's frame, offset by the k-th xyz of the 12 words at
     * 0x5be9f0 (the original's frame keeps them contiguous). */
    pt[0] = sr_ld32(0x5c7890u);
    pt[1] = sr_ld32(0x5c7894u);
    pt[2] = sr_ld32(0x5c7898u);
    {
        uint32_t table[12];
        for(i = 0; i < 4; i++) {
            table[i] = sr_ld32(0x5be9f0u + i * 4);
            table[4 + i] = sr_ld32(0x5bea00u + i * 4);
            table[8 + i] = sr_ld32(0x5bea10u + i * 4);
        }
        for(i = 0; i < 4; i++) {
            uint32_t tr[3], in[3];
            const uint32_t k = i * 3;
            sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
            sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
            sr_tgp_cmd(0x15002a2au, &ang[1], 1, 0, 0);
            sr_tgp_cmd(0x14802929u, &ang[0], 1, 0, 0);
            sr_tgp_cmd(0x15802b2bu, &ang[2], 1, 0, 0);
            tr[0] = table[k];
            tr[1] = table[k + 1];
            tr[2] = table[k + 2];
            sr_tgp_cmd(0x13802727u, tr, 3, 0, 0);
            in[0] = pt[0];
            in[1] = pt[1];
            in[2] = pt[2];
            sr_tgp_cmd(0x16002c2cu, in, 3, ctrl[i], 3);
            sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
        }
    }
    for(i = 0; i < 12; i++) sr_st32(0x217000u + i * 4, ctrl[i / 3][i % 3]);
    sr_spline_point(0x217000u, sr_ld32(0x20ab58u), spl);
    t = D(sr_ld32(0x20ab58u));
    t_bump = t + sr_d(0xbc6a7efau, 0x3f789374u);
    sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
    {
        uint32_t a = fneg(ang[2]);
        sr_tgp_cmd(0x15802b2bu, &a, 1, 0, 0);
        a = fneg(ang[0]);
        sr_tgp_cmd(0x14802929u, &a, 1, 0, 0);
        a = fneg(ang[1]);
        sr_tgp_cmd(0x15002a2au, &a, 1, 0, 0);
    }
    for(i = 0; i < 3; i++) eye[i] = fadd(pos[i], spl[i]);
    sr_st32(0x202214u, eye[2]);
    sr_st32(0x2139d8u, 1);
    sr_st32(0x20220cu, eye[0]);
    sr_st32(0x202210u, eye[1]);
    {
        const uint32_t tr[3] = {fneg(eye[0]), fneg(eye[1]), fneg(eye[2])};
        sr_tgp_cmd(0x13802727u, tr, 3, 0, 0);
    }
    model2_tgp_latch_view_matrix();
    if(t_bump >= sr_d(0x6872b021u, 0x3feced91u)) {
        sr_st32(0x213840u, 4);
        sr_st32(0x213844u, 0);
        sr_st32(0x213848u, 1u << 8);
        sr_st32(0x21384cu, 1u << 6);
    }
    sr_st32(0x20ab58u, F(t_bump));
    if(t_bump >= 1.0) {
        sr_st32(0x2139d8u, 0);
        sr_cam_bind(SR_CAM_CHASE, 0);
        sr_st32(0x20ab58u, 0);
        sr_st32(0x213840u, 4);
        sr_st32(0x213844u, 1u << 8);
        sr_st32(0x213848u, 0);
        sr_st32(0x21384cu, (uint32_t)-0x40);
    }
    sr_st32(0x20ab5cu, fneg(ang[1]));
}

/* The other view (toggled with 0x2140c0): from the car, looking ahead, 1.3 up and 0x20ac74 + 5 forward. */
void sr_cam_chase_alt(void) {
    const uint32_t obj = sr_ld32(0x213980u);
    uint8_t m[0x30];
    uint32_t pos[3], ang[3], blend, i, focus[3];
    if(obj == 0 || !sr_at(obj)) return;
    for(i = 0; i < 3; i++) {
        pos[i] = sr_ld32(obj + i * 4);
        ang[i] = sr_ld32(obj + 0x18 + i * 4);
    }
    blend = sr_angle_wrap(fadd(sr_ld32(0x20ab5cu), ang[1]));
    blend = F(D(sr_ld32(0x20ab5cu)) - D(blend) * sr_d(0x9999999au, 0x3fa99999u));
    sr_st32(0x20ab5cu, blend);
    blend = sr_angle_wrap(blend);
    {
        const uint32_t half_yaw = F(D(ang[0]) * 0.5);
        const uint32_t tr[3] = {0, 0xbfa66666u, F(D(sr_ld32(0x20ac74u)) + 5.0)};
        uint32_t a = F(sr_d(0xa2525ff7u, 0x3fb1df46u) - D(half_yaw));
        sr_tgp_cmd(0x13802727u, tr, 3, 0, 0);
        sr_tgp_cmd(0x14802929u, &a, 1, 0, 0);
        sr_tgp_cmd(0x15002a2au, &blend, 1, 0, 0);
    }
    sr_st32(0x202270u, 0x43960000u);
    sr_st32(0x202274u, 0x43960000u);
    sr_st32(0x20ab5cu, blend);
    sr_hook_geo_view_range(0, 0x43960000u, 0x43960000u);
    sr_cam_light_dir(0xbf34fdf4u, 0xbf34fdf4u, 0x3f34fdf4u, 0);
    {
        const uint32_t tr[3] = {fneg(pos[0]), fneg(pos[1]), fneg(pos[2])};
        uint32_t r[12];
        sr_tgp_cmd(0x13802727u, tr, 3, 0, 0);
        sr_tgp_cmd(0x13002626u, 0, 0, r, 12);
        for(i = 0; i < 12; i++) frame_st(m, i * 4, r[i]);
    }
    sr_matrix_row_swap(m);
    {
        const uint32_t c[3] = {sr_ld32(0x5c7890u), sr_ld32(0x5c7894u), sr_ld32(0x5c7898u)};
        uint32_t r[12];
        for(i = 0; i < 12; i++) r[i] = frame_ld(m, i * 4);
        sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
        sr_tgp_cmd(0x11002222u, r, 12, 0, 0);
        sr_tgp_cmd(0x16002c2cu, c, 3, focus, 3);
    }
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
    sr_st32(0x2139d8u, 1);
    sr_st32(0x20220cu, focus[0]);
    sr_st32(0x202210u, focus[1]);
    sr_st32(0x202214u, focus[2]);
    sr_st32(0x202200u, ang[0]);
    sr_st32(0x202204u, ang[1]);
    sr_st32(0x202208u, ang[2]);
    model2_tgp_latch_view_matrix();
    sr_cam_rebind();
}

void sr_cam_bind(uint32_t handler, uint32_t g14) {
    if(handler) {
        sr_st32(SR_CAM_SLOT, handler);
        if(handler == SR_CAM_SENTINEL) not_ported("camera seed (0x1F1D0)");
        return;
    }
    if(sr_ld8(0x202064u) & 0x80u) {
        const uint32_t phase = (sr_ld32(0x2140c0u) + 1) & 1;
        sr_st32(0x2140c0u, phase);
        if(phase) sr_st32(0x2139d8u, g14);
    }
    switch(sr_ld32(SR_CAM_SLOT)) {
    case 0: break;
    case SR_CAM_CHASE: sr_cam_chase(); break;
    case SR_CAM_DESERT: sr_cam_desert(); break;
    case SR_CAM_CHASE_ALT: sr_cam_chase_alt(); break;
    default: not_ported("camera handler"); break;
    }
}
