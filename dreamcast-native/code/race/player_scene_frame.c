/* The player car's per-tick scene update (geo_view_scene_frame @ 0x34F40), the list handler of the scene node at
 * 0x500000 (the car driven by the inputs).
 *
 * Order of work, as in the original (ROM addresses at the labels):
 *   steering smoothing into node+0x74; gap scale (only while the timer 0x20a560 runs); road cell index;
 *   damping of node+0x20..0x28 when that vector is longer than ~0.002; corner probes (sr_player_scene_prep, whose
 *   result, the largest wall weight, is kept at fp+0x1a0); the car's box corners transformed on the TGP; the road
 *   re-queried under corners that hit a wall; the heading basis and the slip angle 0x214200/04; the four wheel
 *   spring forces (sr_player_wheel_force) and the ground mask 0x213974; tyre loads 0x21420c..0x214218; the
 *   force sum into fp+0x120 and the torque sum into fp+0x150; the speed word 0x20b0b4;
 *   sr_player_frame_update; angular velocity node+0x44..0x4c from the torque, clamped to +-0.1; self-levelling of
 *   roll and pitch on the ground; yaw damping; surface sounds; velocity from the force, gravity 0x214124,
 *   position and angles; the wall latch (sr_player_mode_latch) and wall scrub while touching a wall, else the
 *   drag scale 0x2141ec (adjusted by the car ahead while the timer runs); damping when 0x214120 == 2; the top
 *   speed clamp; angle wrap; the pose copy.
 *
 * This is a close transcription of the lifted routine: the i960 registers are locals here (g0..g13, r3..r15,
 * fp0..fp3; g14 is the constant 0 the original keeps in it), fp is the routine's own 0x220-byte frame, and
 * double chains the original keeps in register pairs (movrl) stay in double. Callees are the native ports.
 *
 * TGP: the original writes command words and arguments to the FIFO and reads results later, sometimes after the
 * next command has been written. tgp_w/tgp_r below collect one command and its inputs, run it with sr_tgp_cmd and
 * queue its outputs, so the TGP sees the same word sequence. */
#include "race/player_scene.h"
#include "race/course.h"
#include "race/fp.h"
#include "race/road.h"
#include "race/sound_cmd.h"
#include "sr_tgp.h"
#include "m2_math.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;

/* ---- TGP command assembly ---- */

typedef struct {
    u32 cmd, n_in, n_out;
} tgp_shape_t;

/* Every command this routine sends: inputs and outputs. */
static const tgp_shape_t k_tgp_shapes[] = {
    {0x10002020u, 0, 0}, /* push matrix */
    {0x10802121u, 0, 0}, /* pop matrix */
    {0x12802525u, 0, 0}, /* identity */
    {0x13802727u, 3, 0}, /* translate */
    {0x14802929u, 1, 0}, /* rotate x */
    {0x15002a2au, 1, 0}, /* rotate y */
    {0x15802b2bu, 1, 0}, /* rotate z */
    {0x16002c2cu, 3, 3}, /* transform vector */
    {0x17802f2fu, 3, 3}, /* normalize */
    {0x08801111u, 1, 1}, /* 0x11: table lookup (inverse) */
    {0x2c005858u, 6, 1}, /* distance */
    {0x2c805959u, 6, 1}, /* 0x59 */
    {0x2d805b5bu, 6, 3}, /* cross product */
};

typedef struct {
    const tgp_shape_t *shape;
    u32 in[6];
    unsigned n_in;
    u32 out[8];
    unsigned out_r, out_n;
} tgp_fifo_t;

static void tgp_run(tgp_fifo_t *q) {
    u32 res[3];
    unsigned i;
    sr_tgp_cmd(q->shape->cmd, q->in, q->shape->n_in, res, q->shape->n_out);
    for(i = 0; i < q->shape->n_out; i++) q->out[(q->out_r + q->out_n++) & 7] = res[i];
    q->shape = 0;
}

static void tgp_write(tgp_fifo_t *q, u32 w) {
    if(!q->shape) {
        unsigned i;
        for(i = 0; i < sizeof(k_tgp_shapes) / sizeof(k_tgp_shapes[0]); i++)
            if(k_tgp_shapes[i].cmd == w) break;
        if(i == sizeof(k_tgp_shapes) / sizeof(k_tgp_shapes[0])) {
            fprintf(stderr, "sr_player_scene_frame: unknown TGP command %08x\n", (unsigned)w);
            abort();
        }
        q->shape = &k_tgp_shapes[i];
        q->n_in = 0;
    } else {
        q->in[q->n_in++] = w;
    }
    if(q->n_in == q->shape->n_in) tgp_run(q);
}

static u32 tgp_read(tgp_fifo_t *q) {
    u32 v;
    if(!q->out_n) {
        fprintf(stderr, "sr_player_scene_frame: TGP read with no result\n");
        abort();
    }
    v = q->out[q->out_r];
    q->out_r = (q->out_r + 1) & 7;
    q->out_n--;
    return v;
}

#define tgp_w(w) tgp_write(&tgp, (u32)(w))
#define tgp_r() tgp_read(&tgp)

/* ---- the original's float helpers ---- */

/* movrl pair <-> double. */
static inline double RR(uintptr_t lo, uintptr_t hi) { return sr_d((u32)lo, (u32)hi); }
static inline void RW(uintptr_t *lo, uintptr_t *hi, double v) {
    u64 u;
    memcpy(&u, &v, 8);
    *lo = (u32)u;
    *hi = (u32)(u >> 32);
}

/* The i960 leaves the destination on an invalid operation: keep `dest` when the double result is not finite. */
static u32 fadd_leave(u32 dest, u32 addend) {
    const double s = D(dest) + D(addend);
    return m2_isfinite(s) ? F(s) : dest;
}

static u32 fdiv_leave(u32 dest, u32 num, u32 den) {
    const double s = D(num) / D(den);
    return m2_isfinite(s) ? F(s) : dest;
}

void sr_player_scene_frame(uint32_t node) {
    const u32 arg0_p = node;
    uint8_t frame[0x220];
    uintptr_t fp;
    uintptr_t g0 = 0, g1 = 0, g2 = 0, g3 = 0, g4 = 0, g5 = 0, g6 = 0, g7 = 0;
    uintptr_t g8 = 0, g9 = 0, g10 = 0, g11 = 0, g12 = 0, g13 = 0;
    const uintptr_t g14 = 0;
    uintptr_t r4 = 0, r5 = 0, r6 = 0, r7 = 0, r8 = 0, r9 = 0, r10 = 0, r11 = 0, r12 = 0, r14 = 0,
              r15 = 0;
    double fp0 = 0, fp1 = 0, fp2 = 0, fp3 = 0;
    tgp_fifo_t tgp;

    memset(&tgp, 0, sizeof(tgp));
    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    *(u32 *)(frame + 0x1f0) = arg0_p;
    g6 = arg0_p + 16;
    g5 = sr_ld8(0x2139d8);
    g4 = sr_ld8(g6);
    g9 = 0x7f;
    g5 = g5 << 7;
    g4 = g9 & g4;
    g4 = g4 | g5;
    sr_st8(g6, (u32)((u8)g4));
    g4 = sr_ld32(0x2020a8);
    r4 = arg0_p + 0x74;
    if ((unsigned char)g4 == 9)
        goto L_00034f94;
    {
        g0 = sr_player_steer_smooth();
    }
    goto L_00034f98;

    L_00034f94:
        g0 = 0;

    L_00034f98:
        sr_st32(r4, (u32)g0);
        g4 = sr_ld32(0x20a560);
        /* @0x34FA4: cmpibge 0,g4,0x34fb4 — Intel src1>=src2 → skip when timer <= 0. */
        if ((i32)g4 <= 0)
            goto L_00034fb4;
        g10 = *(u32 *)(frame + 0x1f0);
        g0 = sr_ld32(g10 + 0x8cu);
        {
            sr_player_gap_scale((u32)g0, frame); /* scene_prep overwrites its frame slots before any read */
        }

    L_00034fb4:
        g11 = *(u32 *)(frame + 0x1f0);
        r4 = g11 + 0x88;
        g0 = g11 + 20;
        g1 = r4;
        {
            g0 = (u32)sr_road_cell_pack(sr_at((u32)g0), sr_at((u32)g1));
        }
    g4 = sr_ld16(r4);
    if ((int)(short)(u16)g4 < 0)
        sr_st16(r4, (u32)((u16)g14));
    g4 = sr_ld16(r4);
    g12 = *(u32 *)(frame + 0x1f0);
    sr_st32(0x2140c8, (u32)g4);
    g4 = sr_ld16(g12 + 0x8au);
    sr_st32(0x2140c4, (u32)g4);
    g4 = sr_ld32(g12 + 0x24u);
    if (D((u32)g4) < 0.0) {
        g4 = g4 ^ (1u << 31);
    }
    fp0 = D((u32)g4);
    fp2 = 0.1;
    if (fp0 <= fp2)
        goto L_00035048;
    g8 = *(u32 *)(frame + 0x1f0);
    g5 = g8 + 0x24;
    g4 = 0x3dcccccdu;
    if (D((u32)sr_ld32(g5))
        < 0.0) {
        g4 = 0xbdcccccdu;
    }
    sr_st32(g5, (u32)g4);

    L_00035048:
        g9 = 0x2c005858u;
        tgp_w(g9);
        tgp_w(g14);
        g10 = *(u32 *)(fp + 0x1f0);
        tgp_w(g14);
        tgp_w(g14);
        g1 = g10 + 0x20;
        g4 = sr_ld32(g1);
        tgp_w(g4);
        g2 = g10 + 0x24;
        g4 = sr_ld32(g2);
        tgp_w(g4);
        g0 = g10 + 0x28;
        g4 = sr_ld32(g0);
        tgp_w(g4);
        g11 = tgp_r();
        fp0 = D((u32)g11);
        fp0 = D((u32)g11);
        fp2 = RR(0xd2f1a9fcu, 0x3f60624du);
        if (fp0 <= fp2)
            goto L_000351a8;
        g8 = 15;
        sr_st32(0x213974, (u32)g8);
        g4 = sr_ld32(g1);
        g7 = 0x3f19999au;
        g4 = fmul(g7, g4);
        g6 = g10 + 0x2c;
        g5 = sr_ld32(g6);
        g4 = fadd(g5, g4);
        sr_st32(g6, (u32)g4);
        g4 = sr_ld32(g0);
        g4 = fmul(g7, g4);
        g6 = g10 + 0x34;
        g5 = sr_ld32(g6);
        g4 = fadd(g5, g4);
        sr_st32(g6, (u32)g4);
        g9 = sr_ld32(g1);
        fp0 = D((u32)g9);
        fp0 = D((u32)g9);
        g10 = 0 | (1u << 31);
        g11 = 0x3fd99999u;
        fp1 = RR(g10, g11);
        RW(&r14, &r15, (fp1) * (fp0));
        fp0 = RR(r14, r15);
        g4 = F(fp0);
        sr_st32(g1, (u32)g4);
        r15 = sr_ld32(g0);
        fp0 = D((u32)r15);
        fp0 = D((u32)r15);
        RW(&g8, &g9, (fp1) * (fp0));
        fp0 = RR(g8, g9);
        g4 = F(fp0);
        sr_st32(g0, (u32)g4);
        g9 = *(u32 *)(fp + 0x1f0);
        g5 = sr_ld32(g1);
        g6 = g9 + 20;
        g4 = sr_ld32(g6);
        g4 = fadd(g5, g4);
        sr_st32(g6, (u32)g4);
        g6 = g9 + 24;
        g4 = sr_ld32(g6);
        g5 = sr_ld32(g2);
        g4 = fadd(g5, g4);
        sr_st32(g6, (u32)g4);
        g6 = g9 + 28;
        g4 = sr_ld32(g6);
        g5 = sr_ld32(g0);
        g4 = fadd(g5, g4);
        sr_st32(g6, (u32)g4);
        sr_st32(g1, (u32)g14);
        sr_st32(g2, (u32)g14);
        sr_st32(g0, (u32)g14);

    L_000351a8:
        g10 = *(u32 *)(fp + 0x1f0);
        g4 = sr_ld32(g10 + 0x14);
        g5 = sr_ld32(g10 + 0x2c);
        g4 = fadd_leave((u32)g4, (u32)g5);
        g1 = fp + 0x130;
        *(u32 *)(fp + 0x130) = (u32)g4;
        g4 = sr_ld32(g10 + 0x18);
        g5 = sr_ld32(g10 + 0x30);
        g4 = fadd_leave((u32)g4, (u32)g5);
        *(u32 *)(fp + 0x134) = (u32)g4;
        g4 = sr_ld32(g10 + 0x1c);
        g5 = sr_ld32(g10 + 0x34);
        g4 = fadd_leave((u32)g4, (u32)g5);
        *(u32 *)(fp + 0x138) = (u32)g4;
        g4 = sr_ld32(g10 + 0x38);
        g5 = sr_ld32(g10 + 0x44);
        g4 = fadd_leave((u32)g4, (u32)g5);
        g2 = fp + 0x140;
        *(u32 *)(fp + 0x140) = (u32)g4;
        g4 = sr_ld32(g10 + 0x3c);
        g5 = sr_ld32(g10 + 0x48);
        g4 = fadd_leave((u32)g4, (u32)g5);
        *(u32 *)(fp + 0x144) = (u32)g4;
        g5 = sr_ld32(g10 + 0x40);
        g4 = sr_ld32(g10 + 0x4c);
        g5 = fadd_leave((u32)g5, (u32)g4);
        g0 = *(u32 *)(fp + 0x1f0);
        *(u32 *)(fp + 0x148) = (u32)g5;
        {
            g0 = sr_player_scene_prep((u32)g0, (uint8_t *)g1, (uint8_t *)g2, frame);
        }
    g11 = 0x10002020u;
    tgp_w(g11);
    g12 = 0x12802525u;
    tgp_w(g12);
    r14 = 0x13802727u;
    tgp_w(r14);
    g4 = *(u32 *)(fp + 0x130);
    g5 = *(u32 *)(fp + 0x134);
    g6 = *(u32 *)(fp + 0x138);
    g7 = *(u32 *)(fp + 0x144);
    tgp_w(g4);
    tgp_w(g5);
    tgp_w(g6);
    r15 = 0x15002a2au;
    tgp_w(r15);
    tgp_w(g7);
    g8 = 0x14802929u;
    tgp_w(g8);
    g4 = *(u32 *)(fp + 0x140);
    g5 = *(u32 *)(fp + 0x148);
    tgp_w(g4);
    g9 = 0x15802b2bu;
    tgp_w(g9);
    tgp_w(g5);
    g10 = 0x16002c2cu;
    tgp_w(g10);
    g11 = 0xbf228f5cu;
    tgp_w(g11);
    g12 = 0xbecccccdu;
    tgp_w(g12);
    r14 = 0x3fa00000u;
    tgp_w(r14);
    g4 = tgp_r();
    g5 = tgp_r();
    g6 = tgp_r();
    r15 = g10;
    tgp_w(r15);
    g8 = 0x3f228f5cu;
    tgp_w(g8);
    g9 = 0xbecccccdu;
    tgp_w(g9);
    g10 = 0x3fa00000u;
    tgp_w(g10);
    *(u32 *)(fp + 0xf0) = (u32)g4;
    g4 = tgp_r();
    *(u32 *)(fp + 0xf4) = (u32)g5;
    *(u32 *)(fp + 0xf8) = (u32)g6;
    *(u32 *)(fp + 0xfc) = (u32)g4;
    g3 = tgp_r();
    g13 = tgp_r();
    g11 = r15;
    tgp_w(g11);
    g12 = 0xbf28f5c3u;
    tgp_w(g12);
    r14 = 0xbecccccdu;
    tgp_w(r14);
    r15 = 0xbfa00000u;
    tgp_w(r15);
    g7 = tgp_r();
    g1 = tgp_r();
    g2 = tgp_r();
    g8 = g11;
    tgp_w(g8);
    g9 = 0x3f28f5c3u;
    tgp_w(g9);
    g10 = 0xbecccccdu;
    tgp_w(g10);
    g11 = 0xbfa00000u;
    tgp_w(g11);
    g4 = tgp_r();
    g5 = tgp_r();
    g6 = tgp_r();
    g12 = 0x10802121u;
    tgp_w(g12);
    r14 = 0x10002020u;
    tgp_w(r14);
    r15 = 0x12802525u;
    tgp_w(r15);
    *(u32 *)(fp + 0x100) = (u32)g3;
    *(u32 *)(fp + 0x104) = (u32)g13;
    *(u32 *)(fp + 0x108) = (u32)g7;
    *(u32 *)(fp + 0x10c) = (u32)g1;
    *(u32 *)(fp + 0x110) = (u32)g2;
    *(u32 *)(fp + 0x114) = (u32)g4;
    *(u32 *)(fp + 0x118) = (u32)g5;
    *(u32 *)(fp + 0x11c) = (u32)g6;
    *(u32 *)(fp + 0x150) = (u32)g14;
    *(u32 *)(fp + 0x154) = (u32)g14;
    *(u32 *)(fp + 0x158) = (u32)g14;
    *(u32 *)(fp + 0x120) = (u32)g14;
    *(u32 *)(fp + 0x124) = (u32)g14;
    g8 = *(u32 *)(fp + 0x1f0);
    *(u32 *)(fp + 0x128) = (u32)g14;
    g4 = sr_ld32(g8 + 0x14);
    g5 = sr_ld32(g8 + 0x18);
    g6 = sr_ld32(g8 + 0x1c);
    g9 = 0x13802727u;
    tgp_w(g9);
    tgp_w(g4);
    tgp_w(g5);
    tgp_w(g6);
    g4 = sr_ld32(g8 + 0x3c);
    g10 = 0x15002a2au;
    tgp_w(g10);
    tgp_w(g4);
    g4 = sr_ld32(g8 + 0x38);
    g11 = 0x14802929u;
    tgp_w(g11);
    tgp_w(g4);
    g4 = sr_ld32(g8 + 0x40);
    g12 = 0x15802b2bu;
    tgp_w(g12);
    tgp_w(g4);
    r14 = 0x16002c2cu;
    tgp_w(r14);
    r15 = 0xbf228f5cu;
    tgp_w(r15);
    g8 = 0xbecccccdu;
    tgp_w(g8);
    g9 = 0x3fa00000u;
    tgp_w(g9);
    g7 = tgp_r();
    g1 = tgp_r();
    g2 = tgp_r();
    g10 = r14;
    tgp_w(g10);
    g11 = 0x3f228f5cu;
    tgp_w(g11);
    g12 = 0xbecccccdu;
    tgp_w(g12);
    r14 = 0x3fa00000u;
    tgp_w(r14);
    g4 = tgp_r();
    g5 = tgp_r();
    g6 = tgp_r();
    r15 = g10;
    tgp_w(r15);
    *(u32 *)(fp + 0x40) = (u32)g7;
    *(u32 *)(fp + 0x44) = (u32)g1;
    *(u32 *)(fp + 0x48) = (u32)g2;
    *(u32 *)(fp + 0x4c) = (u32)g4;
    *(u32 *)(fp + 0x50) = (u32)g5;
    *(u32 *)(fp + 0x54) = (u32)g6;
    g8 = 0xbf28f5c3u;
    tgp_w(g8);
    g9 = 0xbecccccdu;
    tgp_w(g9);
    g10 = 0xbfa00000u;
    tgp_w(g10);
    g7 = tgp_r();
    g1 = tgp_r();
    g2 = tgp_r();
    g11 = r15;
    tgp_w(g11);
    g12 = 0x3f28f5c3u;
    tgp_w(g12);
    r14 = 0xbecccccdu;
    tgp_w(r14);
    r15 = 0xbfa00000u;
    tgp_w(r15);
    g3 = sr_ld32(0x213960);
    g4 = tgp_r();
    g5 = tgp_r();
    g6 = tgp_r();
    g8 = 0x10802121u;
    tgp_w(g8);
    *(u32 *)(fp + 0x58) = (u32)g7;
    *(u32 *)(fp + 0x5c) = (u32)g1;
    *(u32 *)(fp + 0x60) = (u32)g2;
    *(u32 *)(fp + 0x64) = (u32)g4;
    *(u32 *)(fp + 0x68) = (u32)g5;
    *(u32 *)(fp + 0x6c) = (u32)g6;
    *(u32 *)(fp + 0x1a0) = (u32)g0;
    if (g3 == 0)
        goto L_00035738;

    r5 = 0x2138d0u;
    r6 = 0;
    g9 = 0x21393cu;

    L_000356d8:
        g4 = sr_ld8(0x213960);
        if (g4 & 1u) {
            uint8_t *xyz = frame + 0xf0 + (u32)r6;

            g1 = sr_ld32(0x2140c8);
            /*
             * Disasm @ 0x356EC: ld 0x214354. Under --practice that cell is
             * still car class; host_race_course_index remaps to 20a8c4 so
             * table_index matches the loaded course (desert).
             */
            g0 = sr_race_course_index();
            g3 = 0;
            /*
             * Disasm 0x356F4 forms a private-frame EA.  Keep that host
             * pointer full-width; r5 remains the guest out VA.
             */
            {
                sr_road_query_a((u32)g0, (u32)g1, xyz, 0);
            }
            {
                sr_road_query_b(xyz, sr_at((u32)r5), 0);
            }
        }

        g4 = sr_ld32(0x213960);
        r5 = r5 + 0x24;
        r6 = r6 + 12;
        g4 = (uintptr_t)((i32)g4 >> 1);
        sr_st32(0x213960, (u32)g4);
        if (r5 <= g9)
            goto L_000356d8;

    L_00035738:
        g7 = *(u32 *)(fp + 0xf0);
        g4 = *(u32 *)(fp + 0x40);
        g0 = *(u32 *)(fp + 0xf4);
        r7 = *(u32 *)(fp + 0x44);
        g1 = *(u32 *)(fp + 0xf8);
        r9 = *(u32 *)(fp + 0x48);
        g2 = *(u32 *)(fp + 0xfc);
        r10 = *(u32 *)(fp + 0x4c);
        g3 = *(u32 *)(fp + 0x100);
        r11 = *(u32 *)(fp + 0x50);
        g13 = *(u32 *)(fp + 0x104);
        r12 = *(u32 *)(fp + 0x54);
        r4 = *(u32 *)(fp + 0x108);
        r5 = *(u32 *)(fp + 0x10c);
        r6 = *(u32 *)(fp + 0x110);
        g6 = *(u32 *)(fp + 0x114);
        g5 = *(u32 *)(fp + 0x118);
        r8 = *(u32 *)(fp + 0x68);
        g7 = fsub(g7, g4);
        g4 = *(u32 *)(fp + 0x11c);
        g0 = fsub(g0, r7);
        r7 = *(u32 *)(fp + 0x6c);
        g10 = *(u32 *)(fp + 0x58);
        g12 = *(u32 *)(fp + 0x5c);
        r14 = *(u32 *)(fp + 0x60);
        g1 = fsub(g1, r9);
        g8 = *(u32 *)(fp + 0x64);
        g2 = fsub(g2, r10);
        *(u32 *)(fp + 0x40) = (u32)g7;
        g3 = fsub(g3, r11);
        *(u32 *)(fp + 0x44) = (u32)g0;
        g13 = fsub(g13, r12);
        *(u32 *)(fp + 0x48) = (u32)g1;
        g11 = g10;
        r4 = fsub(r4, g11);
        *(u32 *)(fp + 0x4c) = (u32)g2;
        fp3 = D((u32)g12);
        r5 = F((D((u32)r5)) - (fp3));
        *(u32 *)(fp + 0x50) = (u32)g3;
        r15 = r14;
        r6 = fsub(r6, r15);
        *(u32 *)(fp + 0x54) = (u32)g13;
        g9 = g8;
        g6 = fsub(g6, g9);
        *(u32 *)(fp + 0x58) = (u32)r4;
        *(u32 *)(fp + 0x5c) = (u32)r5;
        g5 = fsub(g5, r8);
        *(u32 *)(fp + 0x60) = (u32)r6;
        g4 = fsub(g4, r7);
        *(u32 *)(fp + 0x64) = (u32)g6;
        g11 = *(u32 *)(fp + 0x1f0);
        *(u32 *)(fp + 0x68) = (u32)g5;
        *(u32 *)(fp + 0x6c) = (u32)g4;
        g10 = 0x10002020u;
        tgp_w(g10);
        g12 = 0x17802f2fu;
        tgp_w(g12);
        g1 = g11 + 0x2c;
        g4 = sr_ld32(g1);
        tgp_w(g4);
        g0 = g11 + 0x30;
        g4 = sr_ld32(g0);
        tgp_w(g4);
        g7 = g11 + 0x34;
        g4 = sr_ld32(g7);
        tgp_w(g4);
        g4 = tgp_r();
        g13 = fp + 0x160;
        *(u32 *)(fp + 0x160) = (u32)g4;
        g4 = tgp_r();
        g3 = fp + 0x164;
        *(u32 *)(fp + 0x164) = (u32)g4;
        g4 = tgp_r();
        g2 = fp + 0x168;
        *(u32 *)(fp + 0x168) = (u32)g4;
        r14 = 0x12802525u;
        tgp_w(r14);
        g4 = sr_ld32(g11 + 0x3c);
        r15 = 0x15002a2au;
        tgp_w(r15);
        tgp_w(g4);
        g8 = 0x16002c2cu;
        tgp_w(g8);
        tgp_w(g14);
        tgp_w(g14);
        fp3 = 1.0;
        g9 = F(fp3);
        tgp_w(g9);
        g4 = tgp_r();
        g5 = tgp_r();
        g6 = tgp_r();
        r14 = g8 << 1;
        tgp_w(r14);
        tgp_w(g14);
        tgp_w(g14);
        *(u32 *)(fp + 0xac) = (u32)g4;
        *(u32 *)(fp + 0xb0) = (u32)g5;
        *(u32 *)(fp + 0xb4) = (u32)g6;
        tgp_w(g14);
        g4 = sr_ld32(g1);
        tgp_w(g4);
        g4 = sr_ld32(g0);
        tgp_w(g4);
        g4 = sr_ld32(g7);
        tgp_w(g4);
        r15 = tgp_r();
        fp0 = D((u32)r15);
        fp0 = D((u32)r15);
        fp3 = 0.01;
        if (fp0 <= fp3)
            goto L_00035ac4;
        g4 = *(u32 *)(fp + 0xb0);
        g5 = *(u32 *)(fp + 0x168);
        g6 = *(u32 *)(fp + 0xb4);
        g7 = *(u32 *)(fp + 0x164);
        g0 = *(u32 *)(fp + 0x160);
        r14 = 0x2d805b5bu;
        tgp_w(r14);
        g1 = *(u32 *)(fp + 0xac);
        tgp_w(g4);
        tgp_w(g5);
        tgp_w(g6);
        tgp_w(g7);
        tgp_w(g0);
        tgp_w(g1);
        g4 = tgp_r();
        *(u32 *)(fp + 0x160) = (u32)g4;
        g4 = tgp_r();
        *(u32 *)(fp + 0x164) = (u32)g4;
        g4 = tgp_r();
        *(u32 *)(fp + 0x168) = (u32)g4;
        r15 = *(u32 *)(fp + 0x164);
        /* Disasm @ 0x35A00–0x35A0C: cpysre fp0,+0.0,fp0 → |r15|; movr r15,fp1. */
        fp0 = fabs(D((u32)r15));
        fp1 = D((u32)r15);
        if (fp0 <= 1.0)
            goto L_00035a1c;
        g5 = 0;
        goto L_00035a54;

    L_00035a1c:
        g4 = F((fp1) * (fp1));
        g4 = F((1.0) - (D((u32)g4)));
        g4 = F(sqrt(D((u32)g4)));
        if (D((u32)g4) == 0.0)
            goto L_00035a3c;
        /* divr g4,fp1,g4 → fp1/g4; atanr +1.0,g4,g5 → m2_atan2(g4, 1.0). */
        g4 = F((fp1) / (D((u32)g4)));
        g5 = F(m2_atan(D((u32)g4)));
        goto L_00035a54;

    L_00035a3c:
        g5 = 0xbfc90fdbu;
        if (fp1 > 0.0) {
            g5 = 0x3fc90fdbu;
        }

    L_00035a54:
        fp0 = D((u32)g5);
        fp3 = 1.5707963267948966;
        if (fp0 <= fp3)
            goto L_00035a80;
        g5 = 0x3fc90fdbu;
        goto L_00035aa4;

    L_00035a80:
        fp2 = RR(0x54442d18u, 0xbff921fbu);
        if (fp0 < fp2) {
            g5 = 0xbfc90fdbu;
        }

    L_00035aa4:
        g4 = sr_ld32(0x214200);
        g4 = fsub(g5, g4);
        sr_st32(0x214200, (u32)g5);
        sr_st32(0x214204, (u32)g4);
        goto L_00035ad4;

    L_00035ac4:
        sr_st32(0x214204, (u32)g14);
        sr_st32(0x214200, (u32)g14);

    L_00035ad4:
        g8 = *(u32 *)(fp + 0x1f0);
        g4 = sr_ld32(g8 + 0x74);
        g9 = 0x15002a2au;
        tgp_w(g9);
        tgp_w(g4);
        g10 = 0x16002c2cu;
        tgp_w(g10);
        tgp_w(g14);
        tgp_w(g14);
        fp3 = 1.0;
        g11 = F(fp3);
        tgp_w(g11);
        g4 = tgp_r();
        r4 = fp + 0xa0;
        *(u32 *)(fp + 0xa0) = (u32)g4;
        sr_st32(0x214208, (u32)g14);
        g13 = tgp_r();
        g7 = tgp_r();
        r14 = 0x10802121u;
        tgp_w(r14);

        /* @0x35B5C: four geo_view_copro_vec_push with private-frame EAs. */
        g1 = fp + 0x40;
        g2 = fp + 0xf0;
        g3 = r4;
        g0 = *(u32 *)(fp + 0x1f0);
        g4 = 0x3e800000u;
        g5 = fp + 0x70;
        g6 = 0;
        *(u32 *)(fp + 0xa4) = (u32)g13;
        *(u32 *)(fp + 0xa8) = (u32)g7;
        {
            g0 = sr_player_wheel_force((u32)g0, (const uint8_t *)g1, (const uint8_t *)g2, (const uint8_t *)g3, (u32)g4, (uint8_t *)g5, (u32)g6);
        }
    g3 = r4;
    g4 = 0x3e800000u;
    sr_st32(0x213974, (u32)g0);
    g0 = *(u32 *)(fp + 0x1f0);
    g6 = 1;
    g1 = fp + 0x4c;
    g2 = fp + 0xfc;
    g5 = fp + 0x7c;
    {
        g0 = sr_player_wheel_force((u32)g0, (const uint8_t *)g1, (const uint8_t *)g2, (const uint8_t *)g3, (u32)g4, (uint8_t *)g5, (u32)g6);
    }
    g4 = sr_ld32(0x213974);
    g5 = g4 + (u32)(g0 << 1);
    g4 = 0x3e800000u;
    g6 = 2;
    r4 = fp + 0xac;
    g0 = *(u32 *)(fp + 0x1f0);
    g1 = fp + 0x58;
    g2 = fp + 0x108;
    sr_st32(0x213974, (u32)g5);
    g3 = r4;
    g5 = fp + 0x88;
    {
        g0 = sr_player_wheel_force((u32)g0, (const uint8_t *)g1, (const uint8_t *)g2, (const uint8_t *)g3, (u32)g4, (uint8_t *)g5, (u32)g6);
    }
    g4 = sr_ld32(0x213974);
    g5 = g4 + (u32)(g0 << 2);
    g4 = 0x3e800000u;
    g6 = 3;
    g0 = *(u32 *)(fp + 0x1f0);
    g3 = r4;
    g1 = fp + 0x64;
    sr_st32(0x213974, (u32)g5);
    g2 = fp + 0x114;
    g5 = fp + 0x94;
    {
        g0 = sr_player_wheel_force((u32)g0, (const uint8_t *)g1, (const uint8_t *)g2, (const uint8_t *)g3, (u32)g4, (uint8_t *)g5, (u32)g6);
    }
    g4 = sr_ld8(0x214208);
    g5 = sr_ld32(0x213974);
    r15 = 31 + 17;
    g4 = r15 & g4;
    g0 = g5 + (u32)(g0 << 3);
    sr_st32(0x213974, (u32)g0);
    if (g4 != 0)
        goto L_00035c68;
    sr_st32(0x2138c0, (u32)g14);
    goto L_00035c88;

    L_00035c60:
        g7 = 1;
        goto L_00035cac;

    L_00035c68:
        g4 = sr_ld32(0x2138c0);
        if ((unsigned char)g4 != 0)
            goto L_00035c88;
        g0 = 0x7b;
        {
            sr_sound_cmd((u32)g0);
        }
    g8 = 1;
    sr_st32(0x2138c0, (u32)g8);

    L_00035c88:
        g9 = *(u32 *)(fp + 0x1f0);
        g7 = 0;
        g5 = 0;
        g6 = sr_ld16(g9 + 0x12);

    L_00035c98:
        g4 = g6 & 15;
        if ((unsigned char)g4 == 6)
            goto L_00035c60;
        g6 = g6 >> 4;
        g5 = g5 + 0x1;
        if (3 >= (signed char)g5)
            goto L_00035c98;

    L_00035cac:
        g4 = sr_ld32(0x21396c);
        if ((unsigned char)g4 != 0)
            goto L_00035d00;
        if ((unsigned char)g7 == 0)
            goto L_00035ce4;
        g4 = sr_ld32(0x2142d4);
        if ((unsigned char)g4 != 0)
            goto L_00035ce4;
        g10 = 2;
        g11 = 5;
        sr_st32(0x2142c8, (u32)g10);
        sr_st32(0x2142d0, (u32)g11);
        goto L_00035d00;

    L_00035ce4:
        if ((unsigned char)g7 != 0)
            goto L_00035d00;
        g4 = sr_ld32(0x2142d4);
        if ((unsigned char)g4 != 0) {
            g12 = 3;
            sr_st32(0x2142c8, (u32)g12);
        }

    L_00035d00:
        g3 = sr_ld32(0x213880);
        g1 = sr_ld32(0x213884);
        g2 = sr_ld32(0x213890);
        g0 = sr_ld32(0x213894);
        g6 = sr_ld32(0x213888);
        g7 = sr_ld32(0x21388c);
        g5 = sr_ld32(0x213898);
        g3 = fadd(g1, g3);
        g4 = sr_ld32(0x21389c);
        g2 = fadd(g0, g2);
        g6 = fadd(g7, g6);
        g5 = fadd(g4, g5);
        g4 = sr_ld32(0x213974);
        sr_st32(0x21420c, (u32)g3);
        g4 = 3 & g4;
        sr_st32(0x214210, (u32)g2);
        sr_st32(0x214214, (u32)g6);
        sr_st32(0x214218, (u32)g5);
        if (g4 == 3) {
            fp0 = D((u32)g3);
            fp1 = D((u32)g2);
            r14 = 0;
            r15 = 0x3fe00000u;
            fp2 = RR(r14, r15);
            RW(&g8, &g9, (fp2) * (fp0));
            RW(&g10, &g11, (fp2) * (fp1));
            fp0 = RR(g8, g9);
            fp1 = RR(g10, g11);
            g4 = F(fp0);
            g5 = F(fp1);
            sr_st32(0x21420c, (u32)g4);
            sr_st32(0x214210, (u32)g5);
        }
    /* @0x35DC4: if (0x213974 & 12) == 12, half-scale 0x214214/218. */
    g4 = sr_ld32(0x213974);
    g4 = g4 & 12;
    if ((unsigned char)g4 == 12) {
        g11 = sr_ld32(0x214214);
        g12 = sr_ld32(0x214218);
        fp0 = D((u32)g11);
        fp1 = D((u32)g12);
        r14 = 0;
        r15 = 0x3fe00000u;
        fp2 = RR(r14, r15);
        RW(&g8, &g9, (fp2) * (fp0));
        RW(&g10, &g11, (fp2) * (fp1));
        fp0 = RR(g8, g9);
        fp1 = RR(g10, g11);
        g4 = F(fp0);
        g5 = F(fp1);
        sr_st32(0x214214, (u32)g4);
        sr_st32(0x214218, (u32)g5);
    }
    /* @0x35E2C: always — push negated node+0x40 / +0x38. */
    g11 = 0x10002020u;
    r14 = *(u32 *)(fp + 0x1f0);
    tgp_w(g11);
    g12 = 0x12802525u;
    tgp_w(g12);
    g4 = sr_ld32(r14 + 0x40);
    g4 = g4 ^ (1u << 31);
    r15 = 0x15802b2bu;
    tgp_w(r15);
    tgp_w(g4);
    g4 = sr_ld32(r14 + 0x38);
    g4 = g4 ^ (1u << 31);
    g8 = 0x14802929u;
    tgp_w(g8);
    r14 = r14 + 0x3c;
    *(u32 *)(fp + 0x1b0) = (u32)r14;
    tgp_w(g4);
    g4 = sr_ld32(r14);
    g4 = g4 ^ (1u << 31);
    g13 = *(u32 *)(fp + 0x70);
    r15 = 0x15002a2au;
    tgp_w(r15);
    g2 = *(u32 *)(fp + 0x74);
    tgp_w(g4);
    g8 = 0x16002c2cu;
    tgp_w(g8);
    g3 = *(u32 *)(fp + 0x78);
    tgp_w(g13);
    tgp_w(g2);
    tgp_w(g3);
    g4 = tgp_r();
    g9 = fp + 0x170;
        /* the original spills fp+0x170 to fp+0x1e0 and stores through it below */
    *(u32 *)(fp + 0x170) = (u32)g4;
    g4 = tgp_r();
    *(u32 *)(fp + 0x174) = (u32)g4;
    g4 = tgp_r();
    *(u32 *)(fp + 0x178) = (u32)g4;
    g10 = g8;
    tgp_w(g10);
    g11 = *(u32 *)(fp + 0x7c);
    g12 = *(u32 *)(fp + 0x80);
    r14 = *(u32 *)(fp + 0x84);
    tgp_w(g11);
    tgp_w(g12);
    tgp_w(r14);
    g4 = tgp_r();
    *(u32 *)(fp + 0x17c) = (u32)g4;
    g4 = tgp_r();
    *(u32 *)(fp + 0x180) = (u32)g4;
    r10 = *(u32 *)(fp + 0x174);
    r5 = tgp_r();
    r15 = g10;
    tgp_w(r15);
    g8 = *(u32 *)(fp + 0x88);
    g9 = *(u32 *)(fp + 0x8c);
    g10 = *(u32 *)(fp + 0x90);
    tgp_w(g8);
    tgp_w(g9);
    tgp_w(g10);
    g7 = tgp_r();
    g4 = tgp_r();
    r4 = tgp_r();
    g11 = r15;
    tgp_w(g11);
    g12 = *(u32 *)(fp + 0x94);
    r14 = *(u32 *)(fp + 0x98);
    r15 = *(u32 *)(fp + 0x9c);
    tgp_w(g12);
    tgp_w(r14);
    tgp_w(r15);
    g5 = tgp_r();
    *(u32 *)(fp + 0x18c) = (u32)g4;
    g4 = tgp_r();
    r8 = *(u32 *)(fp + 0x180);
    *(u32 *)(fp + 0x198) = (u32)g4;
    g0 = *(u32 *)(fp + 0x18c);
    g6 = *(u32 *)(fp + 0x198);
    g8 = fadd(r8, r10);
    g9 = fadd(g6, g0);
    fp1 = D((u32)g8);
    fp1 = D((u32)g8);
    fp0 = D((u32)g9);
    fp0 = D((u32)g9);
    RW(&g10, &g11, (fp1) - (fp0));
    fp2 = 1.35;
    fp1 = RR(g10, g11);
    RW(&r14, &r15, (fp2) * (fp1));
    r6 = *(u32 *)(fp + 0x150);
    fp3 = D((u32)r6);
    *(u32 *)(fp + 0x184) = (u32)r5;
    RW(&r6, &r7, fp3);
    fp1 = RR(r14, r15);
    *(u32 *)(fp + 0x188) = (u32)g7;
    RW(&r6, &r7, (fp1) + (RR(r6, r7)));
    *(u32 *)(fp + 0x190) = (u32)r4;
    g4 = tgp_r();
    *(u32 *)(fp + 0x194) = (u32)g5;
    fp2 = RR(r6, r7);
    r6 = F(fp2);
    *(u32 *)(fp + 0x19c) = (u32)g4;
    *(u32 *)(fp + 0x150) = (u32)r6;
    r9 = *(u32 *)(fp + 0x188);
    r12 = *(u32 *)(fp + 0x194);
    g8 = fadd(r12, r9);
    g5 = *(u32 *)(fp + 0x170);
    g4 = *(u32 *)(fp + 0x17c);
    g5 = fadd(g4, g5);
    r15 = g8;
    fp2 = D((u32)r15);
    r10 = fsub(r10, r8);
    *(u32 *)(fp + 0x1d0) = (u32)g8;
    RW(&r14, &r15, fp2);
    *(u64 *)(fp + 0x1d0) = ((u64)r15 << 32) | (u32)r14;
    { u64 _p = *(u64 *)(fp + 0x1d0); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
    fp0 = D((u32)g5);
    g0 = fsub(g0, g6);
    fp2 = RR(r14, r15);
    RW(&r14, &r15, (fp2) - (fp0));
    g9 = *(u32 *)(fp + 0x184);
    g7 = *(u32 *)(fp + 0x178);
    fp3 = D((u32)r10);
    g10 = g9;
    g8 = 0x51eb851fu;
    g9 = 0x3fe51eb8u;
    g4 = fadd(g10, g7);
    RW(&r10, &r11, fp3);
    fp3 = D((u32)g0);
    *(u64 *)(fp + 0x1d0) = ((u64)r15 << 32) | (u32)r14;
    g7 = fsub(g10, g7);
    g10 = 0x851eb852u;
    g11 = 0x3fe451ebu;
    fp1 = RR(g8, g9);
    { u64 _p = *(u64 *)(fp + 0x1d0); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
    g8 = 0x9999999au;
    g9 = 0x3ff59999u;
    g6 = *(u32 *)(fp + 0x190);
    RW(&g0, &g1, fp3);
    fp3 = RR(g10, g11);
    r8 = *(u32 *)(fp + 0x19c);
    fp2 = RR(g8, g9);
    RW(&r10, &r11, (fp3) * (RR(r10, r11)));
    fp3 = RR(r14, r15);
    g9 = g7;
    RW(&r14, &r15, (fp2) * (fp3));
    fp3 = D((u32)g9);
    r4 = fsub(r8, g6);
    g4 = fadd(g6, g4);
    *(u64 *)(fp + 0x1d0) = ((u64)r15 << 32) | (u32)r14;
    r14 = 0x851eb852u;
    r15 = 0x3fe451ebu;
    g5 = fadd(r9, g5);
    RW(&g8, &g9, fp3);
    fp3 = RR(r14, r15);
    r14 = *(u32 *)(fp + 0x158);
    *(u32 *)(fp + 0x200) = (u32)g7;
    fp2 = D((u32)r4);
    *(u64 *)(fp + 0x200) = ((u64)g9 << 32) | (u32)g8;
    g4 = fadd(r8, g4);
    g8 = *(u32 *)(fp + 0x154);
    { u64 _p = *(u64 *)(fp + 0x200); g10 = (u32)_p; g11 = (u32)(_p >> 32); }
    g5 = fadd(r12, g5);
    RW(&g0, &g1, (fp1) * (RR(g0, g1)));
    RW(&r4, &r5, fp2);
    RW(&r4, &r5, (fp1) * (RR(r4, r5)));
    fp1 = D((u32)r14);
    fp1 = D((u32)r14);
    fp0 = D((u32)g8);
    fp2 = RR(g10, g11);
    RW(&g10, &g11, (fp3) * (fp2));
    fp2 = D((u32)g4);
    { u64 _p = *(u64 *)(fp + 0x1d0); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
    fp0 = D((u32)g8);
    fp3 = D((u32)g5);
    RW(&r8, &r9, fp2);
    fp2 = RR(r14, r15);
    RW(&r14, &r15, (fp2) + (fp0));
    g8 = 0x9999999au;
    g9 = 0x3fc99999u;
    RW(&r10, &r11, (RR(g0, g1)) + (RR(r10, r11)));
    RW(&g0, &g1, fp3);
    fp3 = RR(g8, g9);
    g8 = *(u32 *)(fp + 0x80);
    fp0 = RR(r14, r15);
    r14 = *(u32 *)(fp + 0x7c);
    g9 = g8;
    *(u64 *)(fp + 0x200) = ((u64)g11 << 32) | (u32)g10;
    g2 = fadd(g9, g2);
    { u64 _p = *(u64 *)(fp + 0x200); g8 = (u32)_p; g9 = (u32)(_p >> 32); }
    RW(&r10, &r11, (RR(r10, r11)) + (fp1));
    r15 = r14;
    g13 = fadd(r15, g13);
    RW(&r8, &r9, (fp3) * (RR(r8, r9)));
    fp3 = RR(g8, g9);
    RW(&r4, &r5, (RR(r4, r5)) + (fp3));
    fp2 = 0.2;
    r15 = *(u32 *)(fp + 0x88);
    g10 = *(u32 *)(fp + 0x84);
    g8 = *(u32 *)(fp + 0x8c);
    RW(&g0, &g1, (fp2) * (RR(g0, g1)));
    fp1 = RR(r10, r11);
    g12 = F(fp1);
    fp2 = D((u32)r15);
    r14 = F(fp0);
    g11 = g10;
    g10 = *(u32 *)(fp + 0x90);
    g13 = F((fp2) + (D((u32)g13)));
    fp2 = D((u32)r6);
    *(u64 *)(fp + 0x200) = ((u64)r5 << 32) | (u32)r4;
    fp1 = D((u32)g12);
    fp0 = D((u32)r14);
    fp0 = D((u32)r14);
    RW(&r14, &r15, fp2);
    *(u64 *)(fp + 0x1d0) = ((u64)r15 << 32) | (u32)r14;
    RW(&r14, &r15, fp1);
    g9 = g8;
    g2 = fadd(g9, g2);
    fp3 = RR(r4, r5);
    RW(&g8, &g9, (fp3) + (fp0));
    *(u64 *)(fp + 0x200) = ((u64)r15 << 32) | (u32)r14;
    r15 = *(u32 *)(fp + 0x94);
    g7 = *(u32 *)(fp + 0x120);
    fp0 = RR(g8, g9);
    g8 = *(u32 *)(fp + 0x98);
    r14 = *(u32 *)(fp + 0x9c);
    g3 = fadd(g11, g3);
    g11 = g10;
    g3 = fadd(g11, g3);
    { u64 _p = *(u64 *)(fp + 0x1d0); g10 = (u32)_p; g11 = (u32)(_p >> 32); }
    g5 = *(u32 *)(fp + 0x124);
    fp2 = D((u32)r15);
    r15 = r14;
    g3 = fadd(r15, g3);
    r15 = *(u32 *)(fp + 0x1a0);
    g6 = *(u32 *)(fp + 0x128);
    g9 = g8;
    g13 = F((fp2) + (D((u32)g13)));
    fp2 = D((u32)r15);
    g2 = fadd(g9, g2);
    g13 = fadd(g7, g13);
    { u64 _p = *(u64 *)(fp + 0x200); g8 = (u32)_p; g9 = (u32)(_p >> 32); }
    RW(&r14, &r15, fp2);
    g2 = fadd(g5, g2);
    *(u64 *)(fp + 0x1c0) = ((u64)r15 << 32) | (u32)r14;
    fp3 = RR(g10, g11);
    g3 = fadd(g6, g3);
    RW(&r8, &r9, (RR(r8, r9)) + (fp3));
    fp3 = RR(g8, g9);
    { u64 _p = *(u64 *)(fp + 0x1c0); g8 = (u32)_p; g9 = (u32)(_p >> 32); }
    *(u32 *)(fp + 0x120) = (u32)g13;
    *(u32 *)(fp + 0x124) = (u32)g2;
    *(u32 *)(fp + 0x128) = (u32)g3;
    RW(&g0, &g1, (fp3) - (RR(g0, g1)));
    fp3 = RR(r8, r9);
    *(u64 *)(fp + 0x200) = ((u64)g1 << 32) | (u32)g0;
    g0 = F(fp3);
    { u64 _p = *(u64 *)(fp + 0x200); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
    fp3 = RR(g8, g9);
    *(u32 *)(fp + 0x158) = (u32)g12;
    *(u64 *)(fp + 0x1d0) = ((u64)r9 << 32) | (u32)r8;
    g4 = F(fp0);
    fp2 = RR(r14, r15);
    g1 = F(fp2);
    *(u32 *)(fp + 0x150) = (u32)g0;
    *(u32 *)(fp + 0x154) = (u32)g4;
    *(u32 *)(fp + 0x158) = (u32)g1;
    if (fp3 == 0.0)
        goto L_00036508;
    r14 = *(u32 *)(fp + 0x1f0);
    r15 = 0x16002c2cu;
    tgp_w(r15);
    g7 = r14 + 0x2c;
    g4 = sr_ld32(g7);
    tgp_w(g4);
    g6 = r14 + 0x30;
    g4 = sr_ld32(g6);
    tgp_w(g4);
    g5 = r14 + 0x34;
    g4 = sr_ld32(g5);
    tgp_w(g4);
    g4 = tgp_r();
    /* @0x363A4: st g4,(g8), g8 = the fp+0x170 spilled at 0x1e0. */
    *(u32 *)(fp + 0x170) = (u32)g4;
    g4 = tgp_r();
    *(u32 *)(fp + 0x174) = (u32)g4;
    g4 = tgp_r();
    *(u32 *)(fp + 0x178) = (u32)g4;
    g9 = r15;
    tgp_w(g9);
    g4 = sr_ld32(g7);
    tgp_w(g4);
    g4 = sr_ld32(g6);
    tgp_w(g4);
    g4 = sr_ld32(g5);
    tgp_w(g4);
    fp0 = 0.1;
    g4 = tgp_r();
    RW(&r14, &r15, (fp0) * (fp3));
    /* @0x36410: second st g4,(g8) into fp+0x170. */
    *(u32 *)(fp + 0x170) = (u32)g4;
    g4 = tgp_r();
    *(u32 *)(fp + 0x174) = (u32)g4;
    g4 = tgp_r();
    *(u32 *)(fp + 0x178) = (u32)g4;
    fp1 = RR(r14, r15);
    r15 = *(u32 *)(fp + 0x1f0);
    g5 = r15 + 0x50;
    g8 = sr_ld32(g5);
    fp0 = D((u32)g8);
    fp0 = D((u32)g8);
    RW(&g10, &g11, (fp0) * (fp1));
    fp1 = RR(g10, g11);
    g11 = *(u32 *)(fp + 0x178);
    fp0 = D((u32)g11);
    fp0 = D((u32)g11);
    RW(&r14, &r15, (fp0) * (fp1));
    fp3 = 0.2;
    fp1 = RR(r14, r15);
    RW(&g8, &g9, (fp3) * (fp1));
    fp0 = D((u32)g0);
    { u64 _p = *(u64 *)(fp + 0x1c0); g10 = (u32)_p; g11 = (u32)(_p >> 32); }
    fp1 = RR(g8, g9);
    g8 = 0;
    g9 = 0x3fe00000u;
    RW(&r14, &r15, (fp0) - (fp1));
    fp1 = RR(g8, g9);
    fp3 = RR(g10, g11);
    RW(&g10, &g11, (fp1) * (fp3));
    fp0 = RR(r14, r15);
    g4 = F(fp0);
    *(u32 *)(fp + 0x150) = (u32)g4;
    g5 = sr_ld32(g5);
    fp0 = D((u32)g5);
    fp0 = D((u32)g5);
    fp1 = RR(g10, g11);
    RW(&r14, &r15, (fp0) * (fp1));
    fp1 = RR(r14, r15);
    r15 = *(u32 *)(fp + 0x170);
    fp0 = D((u32)r15);
    fp0 = D((u32)r15);
    RW(&g8, &g9, (fp0) * (fp1));
    fp3 = 0.2;
    fp1 = RR(g8, g9);
    RW(&g10, &g11, (fp3) * (fp1));
    fp0 = D((u32)g1);
    fp1 = RR(g10, g11);
    RW(&r14, &r15, (fp1) + (fp0));
    fp0 = RR(r14, r15);
    g4 = F(fp0);
    *(u32 *)(fp + 0x158) = (u32)g4;

    L_00036508:
        g4 = sr_ld32(0x2020b4);
        r15 = 0x10802121u;
        tgp_w(r15);
        if ((unsigned char)g4 == 0)
            goto L_000365e8;
        g8 = 0x10002020u;
        g10 = *(u32 *)(fp + 0x1b0);
        tgp_w(g8);
        g9 = 0x12802525u;
        tgp_w(g9);
        g4 = sr_ld32(g10);
        g4 = g4 ^ (1u << 31);
        g11 = 0x15002a2au;
        tgp_w(g11);
        r14 = *(u32 *)(fp + 0x1f0);
        tgp_w(g4);
        g12 = 0x16002c2cu;
        tgp_w(g12);
        g4 = sr_ld32(r14 + 0x2c);
        tgp_w(g4);
        g4 = sr_ld32(r14 + 0x30);
        tgp_w(g4);
        g4 = sr_ld32(r14 + 0x34);
        tgp_w(g4);
        g5 = tgp_r();
        g6 = tgp_r();
        g4 = tgp_r();
        *(u32 *)(fp + 0x168) = (u32)g4;
        g4 = *(u32 *)(fp + 0x168);
        *(u32 *)(fp + 0x160) = (u32)g5;
        *(u32 *)(fp + 0x164) = (u32)g6;
        if (D((u32)g4) < 0.0) {
            g4 = g4 ^ (1u << 31);
        }
    sr_st32(0x20b0b4, (u32)g4);
    tgp_w(r15);
    goto L_0003670c;

    L_000365e8:
        g4 = sr_ld32(0x20a560);
        /*
         * @0x365F0: cmpibge 0,g4,0x366ac — Intel: branch when timer <= 0.
         * Countdown keeps bare |vel| in 0x20b0b4; after GO the TGP 0x11
         * arm (table inv @ firmware 0x1A7) scales by k/0x214124.
         */
        if ((i32)g4 <= 0)
            goto L_000366ac;
        g8 = sr_ld32(0x214124);
        fp0 = D((u32)g8);
        fp0 = D((u32)g8);
        fp1 = 0.002802857142857143;
        RW(&r14, &r15, (fp1) / (fp0));
        fp1 = RR(r14, r15);
        r15 = 0x2c005858u;
        tgp_w(r15);
        tgp_w(g14);
        g8 = *(u32 *)(fp + 0x1f0);
        tgp_w(g14);
        tgp_w(g14);
        g4 = sr_ld32(g8 + 0x2c);
        tgp_w(g4);
        g4 = sr_ld32(g8 + 0x30);
        tgp_w(g4);
        g4 = sr_ld32(g8 + 0x34);
        tgp_w(g4);
        g4 = F(fp1);
        g5 = tgp_r();
        g9 = 0x8801111u;
        tgp_w(g9);
        tgp_w(g4);
        g4 = tgp_r();
        g5 = fmul(g4, g5);
        sr_st32(0x20b0b4, (u32)g5);
        goto L_0003670c;

    L_000366ac:
        g10 = 0x2c005858u;
        tgp_w(g10);
        tgp_w(g14);
        g11 = *(u32 *)(fp + 0x1f0);
        tgp_w(g14);
        tgp_w(g14);
        g4 = sr_ld32(g11 + 0x2c);
        tgp_w(g4);
        g4 = sr_ld32(g11 + 0x30);
        tgp_w(g4);
        g4 = sr_ld32(g11 + 0x34);
        tgp_w(g4);
        g4 = tgp_r();
        sr_st32(0x20b0b4, (u32)g4);

    L_0003670c:
        g4 = sr_ld32(0x2138a4);
        g5 = sr_ld32(0x2138a0);
        g12 = *(u32 *)(fp + 0x1f0);
        g4 = fsub(g4, g5);
        g5 = sr_ld32(g12 + 0x50);
        g4 = fmul(g5, g4);
        fp0 = D((u32)g4);
        fp1 = 0.005;
        RW(&g8, &g9, (fp1) * (fp0));
        fp0 = RR(g8, g9);
        g0 = *(u32 *)(fp + 0x1f0);
        g3 = F(fp0);
        g1 = fp + 0x120;
        g2 = fp + 0x150;
        {
            sr_player_frame_update((u32)g0, (uint32_t *)g1, (uint32_t *)g2, (u32)g3);
        }
    /* Reload node — disasm keeps g12 via callee-save; host may still clobber. */
    g12 = *(u32 *)(fp + 0x1f0);
    g5 = sr_ld32(g12 + 0x54);
    g4 = *(u32 *)(fp + 0x150);
    g7 = g12 + 0x44;
    g4 = fdiv_leave(0, (u32)g4, (u32)g5);
    g5 = sr_ld32(g7);
    sr_st32(g7, (u32)(fadd_leave((u32)g5, (u32)g4)));
    g4 = *(u32 *)(fp + 0x154);
    g5 = sr_ld32(g12 + 0x58);
    g6 = g12 + 0x48;
    g4 = fdiv_leave(0, (u32)g4, (u32)g5);
    g5 = sr_ld32(g6);
    sr_st32(g6, (u32)(fadd_leave((u32)g5, (u32)g4)));
    g4 = *(u32 *)(fp + 0x158);
    g5 = sr_ld32(g12 + 0x5c);
    g6 = g12 + 0x4c;
    g4 = fdiv_leave(0, (u32)g4, (u32)g5);
    g5 = sr_ld32(g6);
    sr_st32(g6, (u32)(fadd_leave((u32)g5, (u32)g4)));
    g4 = sr_ld32(g7);
    if (D((u32)g4) < 0.0) {
        g4 = g4 ^ (1u << 31);
    }
    fp0 = D((u32)g4);
    fp3 = 0.1;
    if (fp0 <= fp3)
        goto L_0003680c;
    r14 = *(u32 *)(fp + 0x1f0);
    g5 = r14 + 0x44;
    /* Clamp to ±0.1 keeping the sign of the live value (disasm @ 0x367E4). */
    g4 = 0x3dcccccdu;
    if (D((u32)sr_ld32(g5)) < 0.0)
        g4 = 0xbdcccccdu;
    sr_st32(g5, (u32)g4);

    L_0003680c:
        r15 = *(u32 *)(fp + 0x1f0);
        g4 = sr_ld32(r15 + 0x48);
        if (D((u32)g4) < 0.0) {
            g4 = g4 ^ (1u << 31);
        }
    fp0 = D((u32)g4);
    fp3 = 0.1;
    if (fp0 <= fp3)
        goto L_00036868;
    r14 = *(u32 *)(fp + 0x1f0);
    g5 = r14 + 0x48;
    g4 = 0x3dcccccdu;
    if (D((u32)sr_ld32(g5)) < 0.0)
        g4 = 0xbdcccccdu;
    sr_st32(g5, (u32)g4);

    L_00036868:
        r15 = *(u32 *)(fp + 0x1f0);
        g4 = sr_ld32(r15 + 0x4c);
        if (D((u32)g4) < 0.0) {
            g4 = g4 ^ (1u << 31);
        }
    fp0 = D((u32)g4);
    fp3 = 0.1;
    if (fp0 <= fp3)
        goto L_000368c4;
    r14 = *(u32 *)(fp + 0x1f0);
    g5 = r14 + 0x4c;
    g4 = 0x3dcccccdu;
    if (D((u32)sr_ld32(g5)) < 0.0)
        g4 = 0xbdcccccdu;
    sr_st32(g5, (u32)g4);

    L_000368c4:
        g0 = 2;
        g1 = 25;
        sr_text_cursor((u32)g0, (u32)g1);
    g4 = sr_ld32(0x213974);
    if ((unsigned char)g4 == 15)
        goto L_000369b4;
    r15 = *(u32 *)(fp + 0x1f0);
    g4 = sr_ld32(r15 + 0x4c);
    fp3 = 1.0;
    RW(&g8, &g9, fp3);
    *(u64 *)(fp + 0x200) = ((u64)g9 << 32) | (u32)g8;
    if (D((u32)g4) < 0.0) {
        r14 = 0;
        r15 = 0xbff00000u;
        *(u64 *)(fp + 0x200) = ((u64)r15 << 32) | (u32)r14;
    }
    r15 = *(u32 *)(fp + 0x1f0);
    g5 = r15 + 0x40;
    g8 = sr_ld32(g5);
    fp1 = D((u32)g8);
    fp0 = 1.0;
    /* Disasm @ 0x36918: cmpr fp1,+0.0 — fp1 is the live angle, not float bits. */
    if (fp1 < 0.0) {
        g10 = 0;
        g11 = 0xbff00000u;
        fp0 = RR(g10, g11);
    }
    { u64 _p = *(u64 *)(fp + 0x200); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
    fp2 = RR(r14, r15);
    if (fp2 != fp0)
        goto L_000369b4;
    g4 = F((fp1) * (fp1));
    g8 = sr_ld32(0x5d3e3c);
    RW(&g10, &g11, fp1);
    fp0 = D((u32)g8);
    fp0 = D((u32)g8);
    *(u64 *)(fp + 0x200) = ((u64)g11 << 32) | (u32)g10;
    /*
     * Disasm @ 0x36954–0x36968: cmpr angle,+0.0 then movr ang²→fp1 then bge.
     * Condition is the angle sign (CC preserved), not ang² (always ≥0).
     */
    {
        double ang = RR(g10, g11);

        fp1 = D((u32)g4);
        if (ang >= 0.0)
            goto L_00036994;
    }
    RW(&r14, &r15, fp0);
    r15 = r15 ^ (1u << 31);
    fp0 = RR(r14, r15);
    RW(&g8, &g9, (fp1) * (fp0));
    { u64 _p = *(u64 *)(fp + 0x200); g10 = (u32)_p; g11 = (u32)(_p >> 32); }
    fp3 = RR(g10, g11);
    fp0 = RR(g8, g9);
    RW(&g10, &g11, (fp3) - (fp0));
    fp0 = RR(g10, g11);
    goto L_000369ac;

    L_00036994:
        RW(&r14, &r15, (fp1) * (fp0));
        { u64 _p = *(u64 *)(fp + 0x200); g8 = (u32)_p; g9 = (u32)(_p >> 32); }
        fp3 = RR(g8, g9);
        fp0 = RR(r14, r15);
        RW(&g8, &g9, (fp3) - (fp0));
        fp0 = RR(g8, g9);

    L_000369ac:
        g4 = F(fp0);
        sr_st32(g5, (u32)g4);

    L_000369b4:
        g0 = 2;
        g1 = 26;
        sr_text_cursor((u32)g0, (u32)g1);
    g4 = sr_ld32(0x213974);
    if ((unsigned char)g4 == 15)
        goto L_00036a5c;
    r14 = *(u32 *)(fp + 0x1f0);
    g5 = r14 + 0x38;
    r15 = sr_ld32(g5);
    fp0 = D((u32)r15);
    g4 = F((fp0) * (fp0));
    g8 = sr_ld32(0x5d3e3c);
    fp1 = D((u32)g8);
    fp1 = D((u32)g8);
    RW(&r14, &r15, fp0);
    fp2 = RR(0x66666666u, 0x3ff66666u);
    RW(&g6, &g7, (fp2) * (fp1));
    *(u64 *)(fp + 0x1d0) = ((u64)r15 << 32) | (u32)r14;
    /*
     * Disasm @ 0x369E8 cmpr pitch,+0.0 … 0x36A18 bge after movr pitch²→fp1.
     * Branch on pitch sign (CC), not on pitch² / u32_to_f64(double).
     */
    {
        double pitch = RR(r14, r15);

        fp1 = D((u32)g4);
        if (pitch >= 0.0)
            goto L_00036a3c;
    }
    g7 = g7 ^ (1u << 31);
    fp0 = RR(g6, g7);
    RW(&g8, &g9, (fp1) * (fp0));
    fp3 = RR(r14, r15);
    fp0 = RR(g8, g9);
    RW(&g10, &g11, (fp3) - (fp0));
    fp0 = RR(g10, g11);
    goto L_00036a54;

    L_00036a3c:
        RW(&g6, &g7, (fp1) * (RR(g6, g7)));
        { u64 _p = *(u64 *)(fp + 0x1d0); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
        fp2 = RR(r14, r15);
        fp0 = RR(g6, g7);
        RW(&r14, &r15, (fp2) - (fp0));
        fp0 = RR(r14, r15);

    L_00036a54:
        g4 = F(fp0);
        sr_st32(g5, (u32)g4);

    L_00036a5c:
        g6 = sr_ld32(0x2141cc);
        g8 = *(u32 *)(fp + 0x1f0);
        g7 = sr_ld32(0x214204);
        g4 = sr_ld32(0x2141d0);
        g0 = g8 + 0x48;
        g5 = sr_ld32(g0);
        g6 = fmul(g7, g6);
        g4 = fmul(g5, g4);
        g6 = fadd(g4, g6);
        g5 = fsub(g5, g6);
        sr_st32(g0, (u32)g5);
        g4 = sr_ld32(0x202008);
        g9 = 0xff;
        g4 = g4 + 4;
        g4 = g4 & 7;
        g4 = g4 & g9;
        if ((unsigned char)g4 != 0)
            goto L_00036b98;
        g4 = sr_ld32(0x213974);
        if ((unsigned char)g4 != 0)
            goto L_00036ac4;
        r4 = 0;
        goto L_00036afc;

    L_00036ac4:
        g10 = sr_ld32(0x20b0b4);
        fp0 = D((u32)g10);
        fp0 = D((u32)g10);
        r14 = 0;
        r15 = 0x405f4000;
        fp1 = RR(r14, r15);
        RW(&g8, &g9, (fp1) * (fp0));
        fp0 = RR(g8, g9);
        r4 = (uintptr_t)(i32)(fp0);
        g9 = 0x7f;
        if ((signed char)r4 > (signed char)g9) {
            r4 = 0x7f;
        }

    L_00036afc:
        g4 = sr_ld8(0x214208);
        /* Disasm: lda 0xcc,g10 — mask immediate, not a ROM host pointer. */
        g10 = 0xcc;
        g4 = g10 & g4;
        if ((unsigned char)g4 == 0)
            goto L_00036b54;
        g4 = sr_ld32(0x2138c4);
        if ((unsigned char)g4 != 0)
            goto L_00036b34;
        g0 = 0xb7;
        g1 = 0;
        {
            sr_sound_cmd_fill((u32)g0, (u32)g1);
        }
    g11 = 1;
    sr_st32(0x2138c4, (u32)g11);

    L_00036b34:
        /* Disasm: lda 0xb8,g0 — tile descriptor index, not ROM+0xb8. */
        g0 = 0xb8;
        g1 = r4;
        {
            sr_sound_cmd_fill((u32)g0, (u32)g1);
        }
    g4 = r4 << 3;
    r4 = (uintptr_t)((i32)g4 / (i32)10);
    /* Disasm: lda 0x5d3e40,r5 — guest table VA (ROM-mirrored workram). */
    r5 = 0x5d3e40u;
    goto L_00036b78;

    L_00036b54:
        g4 = sr_ld32(0x2138c4);
        if ((unsigned char)g4 == 0)
            goto L_00036b6c;
        g0 = 0xb8;
        g1 = 0;
        {
            sr_sound_cmd_fill((u32)g0, (u32)g1);
        }

    L_00036b6c:
        r4 = (u32)r4 >> 1;
        r5 = 0x5d3ec0u;

    L_00036b78:
        if ((unsigned char)r4 == 0)
            goto L_00036b98;
        g0 = sr_rng_next();
    g12 = 0xff;
    g4 = g12 & g0;
    if ((signed char)g4 >= (signed char)r4)
        goto L_00036b98;
    g0 = 31 & g0;
    g0 = sr_ld32((u32)r5 + (u32)((u32)(g0 << 2)));
    {
        sr_sound_cmd((u32)g0);
    }

    L_00036b98:
        r14 = *(u32 *)(fp + 0x1a0);
        fp0 = D((u32)r14);
        if (fp0 <= 0.0)
            goto L_00036cc4;
        r15 = sr_ld32(0x20b0b4);
        fp0 = D((u32)r15);
        fp0 = D((u32)r15);
        fp3 = RR(0xed097b42u, 0x3fa7b425u);
        if (fp0 >= fp3)
            goto L_00036cc4;
        g4 = *(u32 *)(fp + 0xb0);
        g5 = sr_ld32(0x214368);
        g6 = *(u32 *)(fp + 0xb4);
        g7 = sr_ld32(0x214364);
        g0 = sr_ld32(0x214360);
        r14 = 0x2d805b5bu;
        tgp_w(r14);
        g1 = *(u32 *)(fp + 0xac);
        tgp_w(g4);
        tgp_w(g5);
        tgp_w(g6);
        tgp_w(g7);
        tgp_w(g0);
        tgp_w(g1);
        g5 = tgp_r();
        g4 = tgp_r();
        *(u32 *)(fp + 0x164) = (u32)g4;
        r15 = *(u32 *)(fp + 0x164);
        fp0 = 3.141592653589793;
        fp1 = D((u32)r15);
        fp1 = D((u32)r15);
        r14 = 0xeb851eb8u;
        r15 = 0x3f8eb851u;
        RW(&g10, &g11, (fp0) * (fp1));
        fp0 = RR(r14, r15);
        fp1 = RR(g10, g11);
        RW(&g8, &g9, (fp0) * (fp1));
        g4 = tgp_r();
        fp1 = RR(g8, g9);
        g9 = *(u32 *)(fp + 0x1f0);
        *(u32 *)(fp + 0x160) = (u32)g5;
        *(u32 *)(fp + 0x168) = (u32)g4;
        g5 = g9 + 0x48;
        g10 = sr_ld32(g5);
        fp0 = D((u32)g10);
        fp0 = D((u32)g10);
        RW(&r14, &r15, (fp0) - (fp1));
        fp0 = RR(r14, r15);
        g4 = F(fp0);
        sr_st32(g5, (u32)g4);

    L_00036cc4:
        r15 = *(u32 *)(fp + 0x1f0);
        g7 = *(u32 *)(fp + 0x120);
        g6 = r15 + 0x50;
        g4 = sr_ld32(g6);
        g7 = fdiv_leave(0, (u32)g7, (u32)g4);
        *(u32 *)(fp + 0xe0) = (u32)g7;
        g4 = *(u32 *)(fp + 0x124);
        g5 = sr_ld32(g6);
        g4 = fdiv_leave(0, (u32)g4, (u32)g5);
        *(u32 *)(fp + 0xe4) = (u32)g4;
        g4 = *(u32 *)(fp + 0x128);
        g5 = sr_ld32(g6);
        g4 = fdiv_leave(0, (u32)g4, (u32)g5);
        *(u32 *)(fp + 0xe8) = (u32)g4;
        g0 = r15 + 0x2c;
        g4 = sr_ld32(g0);
        sr_st32(g0, (u32)(fadd_leave((u32)g4, (u32)g7)));
        g7 = r15 + 0x30;
        g4 = sr_ld32(g7);
        g5 = *(u32 *)(fp + 0xe4);
        sr_st32(g7, (u32)(fadd_leave((u32)g4, (u32)g5)));
        g1 = r15 + 0x34;
        g4 = sr_ld32(g1);
        g5 = *(u32 *)(fp + 0xe8);
        sr_st32(g1, (u32)(fadd_leave((u32)g4, (u32)g5)));
        g4 = sr_ld32(g7);
        g5 = sr_ld32(0x214124);
        {
            double vy = D((u32)g4) - D((u32)g5);

            sr_st32(g7, m2_isfinite(vy) ? F(vy) : (u32)g4);
        }
        g6 = r15 + 0x38;
        g4 = sr_ld32(g6);
        g5 = sr_ld32(r15 + 0x44);
        sr_st32(g6, (u32)(fadd_leave((u32)g4, (u32)g5)));
        g6 = r15 + 0x3c;
        g4 = sr_ld32(g6);
        g5 = sr_ld32(r15 + 0x48);
        sr_st32(g6, (u32)(fadd_leave((u32)g4, (u32)g5)));
        g6 = r15 + 0x40;
        g4 = sr_ld32(g6);
        g5 = sr_ld32(r15 + 0x4c);
        sr_st32(g6, (u32)(fadd_leave((u32)g4, (u32)g5)));
        g4 = sr_ld32(0x214120);
        if ((unsigned char)g4 == 0) {
            sr_st32(g1, (u32)g14);
            sr_st32(g0, (u32)g14);
        }
    g8 = *(u32 *)(fp + 0x1f0);
    g5 = sr_ld32(g0);
    g6 = g8 + 20;
    g4 = sr_ld32(g6);
    g4 = fadd_leave((u32)g4, (u32)g5);
    sr_st32(g6, (u32)g4);
    g6 = g8 + 24;
    g4 = sr_ld32(g6);
    g5 = sr_ld32(g7);
    g4 = fadd_leave((u32)g4, (u32)g5);
    sr_st32(g6, (u32)g4);
    g6 = g8 + 28;
    g4 = sr_ld32(g6);
    g5 = sr_ld32(g1);
    g4 = fadd_leave((u32)g4, (u32)g5);
    g0 = *(u32 *)(fp + 0x1a0);
    sr_st32(g6, (u32)g4);
    {
        sr_player_mode_latch((u32)g0);
    }
    g9 = *(u32 *)(fp + 0x1a0);
    fp1 = D((u32)g9);
    if (fp1 <= 0.0)
        goto L_00036eb4;
    /*
     * Disasm @ 0x36DF0: ld 0x214354; cmpibne 3. This is the car-class cell
     * (Delta MT → 3 after confirm @ 0x152A4), not the course remap — do not
     * substitute host_race_course_index (desert 20a8c4 would skip this damp).
     */
    g4 = sr_ld32(0x214354);
    if ((unsigned char)g4 != 3)
        goto L_00036e38;
    fp0 = RR(0xcccccccdu, 0x3ffcccccu);
    RW(&r14, &r15, (fp0) * (fp1));
    fp0 = RR(0x7ae147aeu, 0x3fefae14u);
    fp1 = RR(r14, r15);
    RW(&g10, &g11, (fp0) - (fp1));
    fp0 = RR(g10, g11);
    goto L_00036e7c;

    L_00036e38:
        g11 = *(u32 *)(fp + 0x1a0);
        g4 = sr_ld32(0x2141f0);
        fp3 = D((u32)g11);
        g4 = F((D((u32)g4)) * (fp3));
        fp0 = D((u32)g4);
        fp0 = D((u32)g4);
        fp1 = RR(0x8b439581u, 0x3fefe76cu);
        RW(&g8, &g9, (fp1) - (fp0));
        fp0 = RR(g8, g9);
        if (fp0 < 0.0) {
            fp0 = 0.0;
        }

    L_00036e7c:
        g6 = F(fp0);
        g9 = *(u32 *)(fp + 0x1f0);
        g5 = g9 + 0x2c;
        g4 = sr_ld32(g5);
        g4 = fmul(g4, g6);
        sr_st32(g5, (u32)g4);
        g5 = g9 + 0x30;
        g4 = sr_ld32(g5);
        g4 = fmul(g4, g6);
        sr_st32(g5, (u32)g4);
        g5 = g9 + 0x34;
        g4 = sr_ld32(g5);
        g4 = fmul(g4, g6);
        goto L_00037044;

    L_00036eb4:
        g4 = sr_ld32(0x213974);
        /* @0x36EBC: cmpibe 0,g4,0x37048 */
        if (g4 == 0)
            goto L_00037048;
        g4 = sr_ld32(0x20a560);
        g13 = sr_ld32(0x2141ec);
        /* @0x36ED0: cmpibge 0,g4,0x37024 — skip follow/car vel scale when timer <= 0. */
        if ((i32)g4 <= 0)
            goto L_00037024;
        g7 = sr_ld32(0x2139f8);
        /* @0x36EDC: cmpibe 0,g7,0x37024 */
        if (g7 == 0)
            goto L_00037024;
        g6 = sr_ld32(0x213980);
        g4 = sr_ld32(g7);
        g5 = sr_ld32(g6);
        g4 = fsub(g4, g5);
        *(u32 *)(fp + 0x160) = (u32)g4;
        g4 = sr_ld32(g7 + 0x4);
        g5 = sr_ld32(g6 + 0x4);
        g4 = fsub(g4, g5);
        *(u32 *)(fp + 0x164) = (u32)g4;
        g4 = sr_ld32(g7 + 0x8);
        g5 = sr_ld32(g6 + 0x8);
        g4 = fsub(g4, g5);
        g10 = 0x2c805959u;
        tgp_w(g10);
        *(u32 *)(fp + 0x168) = (u32)g4;
        g4 = sr_ld32(g7 + 0xc);
        g0 = *(u32 *)(fp + 0x160);
        tgp_w(g4);
        tgp_w(g0);
        g4 = sr_ld32(g7 + 0x10);
        g6 = *(u32 *)(fp + 0x164);
        tgp_w(g4);
        tgp_w(g6);
        g4 = sr_ld32(g7 + 0x14);
        g5 = *(u32 *)(fp + 0x168);
        tgp_w(g4);
        tgp_w(g5);
        g11 = tgp_r();
        fp0 = D((u32)g11);
        fp1 = D((u32)g11);
        if (fp0 <= 0.0)
            goto L_00037024;
        g12 = 0x2c005858u;
        tgp_w(g12);
        tgp_w(g14);
        tgp_w(g14);
        tgp_w(g14);
        tgp_w(g0);
        tgp_w(g6);
        tgp_w(g5);
        g4 = tgp_r();
        g4 = fmul(g4, g4);
        g4 = F((fp1) / (D((u32)g4)));
        fp0 = D((u32)g4);
        r14 = 0x47ae147bu;
        r15 = 0x3f747ae1u;
        fp1 = D((u32)g4);
        fp1 = RR(r14, r15);
        RW(&g8, &g9, (fp1) * (fp0));
        fp1 = D((u32)g13);
        fp0 = RR(g8, g9);
        RW(&g10, &g11, (fp0) + (fp1));
        fp1 = RR(g10, g11);
        if (fp1 <= 1.0)
            goto L_0003701c;
        g4 = 0x3f800000u;
        goto L_00037020;

    L_0003701c:
        g4 = F(fp1);

    L_00037020:
        g13 = g4;

    L_00037024:
        g11 = *(u32 *)(fp + 0x1f0);
        g5 = g11 + 0x2c;
        g4 = sr_ld32(g5);
        g4 = fmul(g4, g13);
        sr_st32(g5, (u32)g4);
        g5 = g11 + 0x34;
        g4 = sr_ld32(g5);
        g4 = fmul(g4, g13);

    L_00037044:
        sr_st32(g5, (u32)g4);

    L_00037048:
        g4 = sr_ld32(0x214120);
        if ((unsigned char)g4 != 2)
            goto L_000370ac;
        g12 = *(u32 *)(fp + 0x1f0);
        g5 = g12 + 0x2c;
        r14 = sr_ld32(g5);
        fp0 = D((u32)r14);
        fp0 = D((u32)r14);
        fp1 = RR(0x624dd2fu, 0x3fef9581u);
        RW(&g10, &g11, (fp1) * (fp0));
        fp0 = RR(g10, g11);
        g4 = F(fp0);
        sr_st32(g5, (u32)g4);
        g5 = g12 + 0x34;
        g11 = sr_ld32(g5);
        fp0 = D((u32)g11);
        fp0 = D((u32)g11);
        RW(&r14, &r15, (fp1) * (fp0));
        fp0 = RR(r14, r15);
        g4 = F(fp0);
        sr_st32(g5, (u32)g4);

    L_000370ac:
        r15 = 0x2c005858u;
        tgp_w(r15);
        tgp_w(g14);
        g8 = *(u32 *)(fp + 0x1f0);
        tgp_w(g14);
        tgp_w(g14);
        g5 = g8 + 0x2c;
        g4 = sr_ld32(g5);
        tgp_w(g4);
        g6 = g8 + 0x30;
        g4 = sr_ld32(g6);
        tgp_w(g4);
        g7 = g8 + 0x34;
        g4 = sr_ld32(g7);
        tgp_w(g4);
        g9 = tgp_r();
        fp0 = D((u32)g9);
        fp0 = D((u32)g9);
        fp3 = RR(0xa12f684cu, 0x3ff284bdu);
        if (fp0 <= fp3)
            goto L_000371a4;
        fp1 = RR(0xa12f684cu, 0x3ff284bdu);
        RW(&g8, &g9, (fp1) / (fp0));
        fp1 = RR(g8, g9);
        g9 = sr_ld32(g5);
        fp0 = D((u32)g9);
        fp0 = D((u32)g9);
        RW(&g10, &g11, (fp1) * (fp0));
        fp0 = RR(g10, g11);
        g4 = F(fp0);
        sr_st32(g5, (u32)g4);
        g11 = sr_ld32(g6);
        fp0 = D((u32)g11);
        fp0 = D((u32)g11);
        RW(&r14, &r15, (fp1) * (fp0));
        fp0 = RR(r14, r15);
        g4 = F(fp0);
        sr_st32(g6, (u32)g4);
        r15 = sr_ld32(g7);
        fp0 = D((u32)r15);
        fp0 = D((u32)r15);
        RW(&g8, &g9, (fp1) * (fp0));
        fp0 = RR(g8, g9);
        g4 = F(fp0);
        sr_st32(g7, (u32)g4);

    L_000371a4:
        g9 = *(u32 *)(fp + 0x1f0);
        r4 = g9 + 0x38;
        g0 = sr_ld32(r4);
        g0 = sr_angle_wrap((u32)g0);
    r5 = g9 + 0x3c;
    sr_st32(r4, (u32)g0);
    g0 = sr_ld32(r5);
    g0 = sr_angle_wrap((u32)g0);
    r4 = g9 + 0x40;
    sr_st32(r5, (u32)g0);
    g0 = sr_ld32(r4);
    g0 = sr_angle_wrap((u32)g0);
    sr_st32(r4, (u32)g0);
    g0 = *(u32 *)(fp + 0x1f0);
    sr_player_pose_publish((u32)g0);}
