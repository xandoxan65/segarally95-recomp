/* Rival e8==1 body @ 0x2E650.
 *
 * car_frame @ 0x2C018 calls this after road_span, yaw_step and motion.
 * It is the only store of node+0x3c on the e8==1 path: road_angles into
 * the caller frame, then either a snap of that triple or the blend at
 * 0x2ECB4 (pitch/roll by 0.9, yaw snap or a fixed step).
 *
 * call 0x2acc0 leaves g14 = 0x2E8A8. The bit7-clear early-out stores
 * that link into node+0x68/6c/70.
 *
 * source: disasm/maincpu/maincpu_02e650_8a0.asm
 */
// @rom 0x2e650 +0x894 game_start_race_obj_car_e8_mode1

#include "i960_lift.h"
#include "m2_math.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_hw.h"

#include <string.h>

/* Return address of `call 0x2acc0` at 0x2E8A4. */
#define MODE1_INDEX_B_LINK 0x2e8a8u

void game_start_race_obj_car_velocity_step(u32 arg0, u32 arg1, u32 arg2);
void game_start_race_obj_car_index_refresh(u32 node, u32 pred[3]);
void geo_view_table_index_a(void *arg0, u32 arg1, void *arg2);
void geo_view_table_index_b(void *arg0, void *arg1, u32 arg2);
void game_start_race_obj_road_angles_host(void *samp, u32 nrm, void *ang);
u32 geo_view_float_clamp(u32 arg0, u32 arg1, u32 arg2);
void tile_attract_angle_fixup(u32 arg0, u32 arg1);

static u32 fld(u8 *frame, u32 off)
{
    u32 v;

    memcpy(&v, frame + off, 4);
    return v;
}

static void fst(u8 *frame, u32 off, u32 v)
{
    memcpy(frame + off, &v, 4);
}

static u32 nd(u32 node, u32 off)
{
    return i960_ld_u32(I960_ABS, node, off);
}

static void ns(u32 node, u32 off, u32 v)
{
    i960_st_u32(I960_ABS, node, off, v);
}

static double f64(u32 bits)
{
    return i960_u32_to_f64(bits);
}

static u32 f32(double v)
{
    return (u32)i960_f64_to_u32(v);
}

static u32 f_add(u32 a, u32 b)
{
    return f32(f64(a) + f64(b));
}

static u32 f_sub(u32 a, u32 b)
{
    return f32(f64(a) - f64(b));
}

static u32 f_mul(u32 a, u32 b)
{
    return f32(f64(a) * f64(b));
}

static double dconst(u32 lo, u32 hi)
{
    return i960_rifl_read(lo, hi);
}

static void fifo(u32 w)
{
    i960_mmio_write_u32(0x884000u, w);
}

static u32 fifo_ld(void)
{
    return i960_mmio_read_u32(0x884000u);
}

static u32 wr(u32 addr)
{
    return i960_ld_u32(I960_WORKRAM, addr, 0);
}

/* bal 0x27BD8 then sample + 0.9 * delta. Pitch @ 0x2ECE0, roll @ 0x2ED28. */
static u32 blend_ang(u32 sample, u32 current)
{
    double mixed;

    tile_attract_angle_fixup(sample, current);
    mixed = f64(sample)
        + f64((u32)g0) * dconst(0xcccccccdu, 0x3fecccccu);
    return f32(mixed);
}

/* @0x2EE00: m2_sin(pitch) scale, integrate xyz, scale velocity by r13. */
static void mode1_epilogue(u32 node, u8 *frame, u32 r13)
{
    u32 pitch = nd(node, 0x38u);
    u32 vx = nd(node, 0x2cu);
    u32 vy = nd(node, 0x30u);
    u32 vz = nd(node, 0x34u);
    u32 x = nd(node, 0x14u);
    u32 y = nd(node, 0x18u);
    u32 z = nd(node, 0x1cu);
    u32 scaled;
    u32 sum;
    u32 diff;

    /* TGP 0x15 of node+0x38, then ×1.25 (0x3ff40000). */
    fifo(0x0a801515u);
    fifo(pitch);
    scaled = f32(f64(fifo_ld()) * dconst(0u, 0x3ff40000u));

    ns(node, 0x14u, f_add(x, vx));
    ns(node, 0x18u, f_add(y, vy));
    ns(node, 0x1cu, f_add(z, vz));
    ns(node, 0x2cu, f_mul(r13, vx));
    ns(node, 0x30u, f_mul(r13, vy));
    ns(node, 0x34u, f_mul(r13, vz));

    /* @0x2EEA4: (fp+0x58 + fp+0x84) ± the scaled sine. */
    sum = f_add(fld(frame, 0x58u), fld(frame, 0x84u));
    diff = f_sub(sum, scaled);
    sum = f_add(sum, scaled);
    ns(node, 0xb8u, sum);
    ns(node, 0xb4u, sum);
    ns(node, 0xc0u, diff);
    ns(node, 0xbcu, diff);
}

/* @0x2ECB4: clamp road angles, blend pitch/roll, snap or step yaw. */
static void mode1_blend_angles(u32 node, u8 *frame)
{
    u32 pitch = geo_view_float_clamp(fld(frame, 0x90u), 0, 0);
    u32 yaw = geo_view_float_clamp(fld(frame, 0x94u), 0, 0);
    u32 roll = geo_view_float_clamp(fld(frame, 0x98u), 0, 0);
    u32 delta;
    double abs_d;
    double thresh;

    fst(frame, 0x90u, pitch);
    fst(frame, 0x94u, yaw);
    fst(frame, 0x98u, roll);

    ns(node, 0x38u, blend_ang(pitch, nd(node, 0x38u)));
    ns(node, 0x40u, blend_ang(roll, nd(node, 0x40u)));

    /* @0x2ED64: g0 = clamped yaw, g1 = node+0x3c. */
    tile_attract_angle_fixup(yaw, nd(node, 0x3cu));
    delta = (u32)g0;
    abs_d = f64(delta & 0x7fffffffu);
    thresh = dconst(0xa68c6f10u, 0x3f91e2efu);
    /* cmprl |delta|, thresh; bge → step. Smaller delta snaps the sample. */
    if (abs_d >= thresh) {
        double step = f64(delta) < 0.0
            ? dconst(0xa68c6f10u, 0xbf91e2efu)
            : thresh;

        ns(node, 0x3cu, f32(f64(nd(node, 0x3cu)) - step));
    } else {
        ns(node, 0x3cu, yaw);
    }
}

/*
 * @0x2EAA0 bit7 clear. r10 is fp+0x58. Negative sample zeros the
 * scale slots with the index_b link; otherwise the 0x2141d0/e0 blend
 * writes node+0x30 and the scale triple. Both sides call road_angles.
 * Returns the r13 scale the epilogue multiplies into velocity.
 */
static u32 mode1_blend(u32 node, u8 *frame)
{
    u32 w50 = fld(frame, 0x50u);
    u32 s58 = fld(frame, 0x58u);
    u32 q_d4 = wr(0x2141d4u);
    u32 q_d8 = wr(0x2141d8u);
    u32 q_dc = wr(0x2141dcu);
    u32 q_e0 = wr(0x2141e0u);
    u32 r13 = wr(0x2141ecu);
    u32 n64 = nd(node, 0x64u);
    u32 t124 = wr(0x214124u);
    u32 g5;

    if (f64(s58) < 0.0) {
        ns(node, 0x70u, MODE1_INDEX_B_LINK);
        ns(node, 0x6cu, MODE1_INDEX_B_LINK);
        ns(node, 0x68u, MODE1_INDEX_B_LINK);
        g5 = 0u;
    } else {
        double sum = f64(f_add(q_d4, q_d8));
        double acc_d = sum * dconst(0u, 0x3fe00000u) * dconst(0u, 0x40590000u);
        double mid = acc_d * f64(t124) * dconst(0u, 0x40100000u);
        u32 acc = f32(f64(s58) * mid);
        u32 n50 = nd(node, 0x50u);
        double half_base;
        double scale;
        u32 g4;

        /* @0x2EB6C: s58 > 0.2 folds node+0x50 in and replaces r10. */
        if (f64(s58) > dconst(0x9999999au, 0x3fc99999u)) {
            acc = f32(f64(acc) + (f64(s58) - dconst(0x9999999au, 0x3fc99999u))
                * f64(n50));
            s58 = 0x3e4ccccdu;
        }
        g5 = f_sub(n64, s58);
        half_base = f64(f_add(q_dc, q_e0)) * dconst(0u, 0x3fe00000u);
        scale = half_base * f64(t124) * dconst(0u, 0x40ac2000u)
            * dconst(0u, 0x40100000u);
        g4 = f32(f64(acc) - f64(g5) * scale);
        if (f64(g4) < 0.0)
            g4 = 0u;
        g4 = f32(f64(g4) * f64(w50) / f64(n50));
        ns(node, 0x70u, s58);
        ns(node, 0x6cu, s58);
        ns(node, 0x68u, s58);
        g4 = f_add(g4, nd(node, 0x30u));
        g5 = s58;
        ns(node, 0x30u, g4);
    }

    /* @0x2EC80: both sides. r13 stays the 0x2141ec word.
     * fp+0x58 stays the table_index_b height. The epilogue at 0x2EE58
     * adds it to fp+0x84; nothing in 0x2EAA0–0x2EC9C stores this slot. */
    ns(node, 0x64u, g5);
    g5 = f_sub(nd(node, 0x30u), t124);
    game_start_race_obj_road_angles_host(frame + 0x40u, node + 0x2cu,
                                         frame + 0x90u);
    ns(node, 0x30u, g5);
    mode1_blend_angles(node, frame);
    return r13;
}

/* @0x2E8CC bit7 set: project the sample, snap road_angles onto the node. */
static u32 mode1_snap(u32 node, u8 *frame)
{
    u32 w4c = fld(frame, 0x4cu);
    u32 w50 = fld(frame, 0x50u);
    u32 w54 = fld(frame, 0x54u);
    u32 w58 = fld(frame, 0x58u);
    double half = f64(w58) * dconst(0u, 0x3fe00000u);
    u32 cross0;
    u32 cross1;
    u32 cross2;
    double diff;
    u32 r10;
    u32 r13;

    ns(node, 0x20u, f32(f64(nd(node, 0x20u)) + half * f64(w4c)));
    ns(node, 0x28u, f32(f64(nd(node, 0x28u)) + half * f64(w54)));

    /* @0x2E954: (const − fp+0x58 * 0x2141f0), clamped at 0. */
    diff = dconst(0x8b439581u, 0x3fefe76cu) - f64(f_mul(w58, wr(0x2141f0u)));
    if (diff < 0.0)
        diff = 0.0;

    fifo(0x2d805b5bu);
    fifo(w50);
    fifo(wr(0x20cad8u));
    fifo(w54);
    fifo(wr(0x20cad4u));
    fifo(wr(0x20cad0u));
    fifo(w4c);
    cross0 = fifo_ld();
    cross1 = fifo_ld();
    cross2 = fifo_ld();

    r13 = f32(diff * f64(wr(0x2141ecu)));
    r10 = cross1;
    if (f64(r10) < 0.0)
        r10 ^= 0x80000000u;

    fst(frame, 0x4cu, cross0);
    fst(frame, 0x50u, r10);
    fst(frame, 0x54u, cross2);
    ns(node, 0x70u, 0x3dcccccdu);
    ns(node, 0x6cu, 0x3dcccccdu);
    ns(node, 0x68u, 0x3dcccccdu);
    ns(node, 0x64u, 0x3dcccccdu);

    game_start_race_obj_road_angles_host(frame + 0x40u, node + 0x2cu,
                                         frame + 0x90u);
    /* ldq of the angle triple: pitch+yaw at +0x38, roll at +0x40. */
    ns(node, 0x38u, fld(frame, 0x90u));
    ns(node, 0x3cu, fld(frame, 0x94u));
    ns(node, 0x40u, fld(frame, 0x98u));
    return r13;
}

void game_start_race_obj_car_e8_mode1(u32 node)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    uintptr_t g8_save = g8;
    uintptr_t g9_save = g9;
    uintptr_t g10_save = g10;
    uintptr_t g11_save = g11;
    uintptr_t g12_save = g12;
    u8 frame[0x100];
    u32 hx;
    u32 hz;
    u32 pred[3];
    u32 orient[3];
    i32 index;
    u32 flag;
    u32 scaled;
    u32 r13;

    if (node == 0u)
        return;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    /*
     * @0x2E664: half of node+0x20/0x28 added into vx/x and vz/z,
     * then those two slots and node+0x24 stored 0.
     */
    hx = f32(f64(nd(node, 0x20u)) * dconst(0u, 0x3fe00000u));
    hz = f32(f64(nd(node, 0x28u)) * dconst(0u, 0x3fe00000u));
    ns(node, 0x20u, 0u);
    ns(node, 0x24u, 0u);
    ns(node, 0x28u, 0u);
    ns(node, 0x2cu, f_add(nd(node, 0x2cu), hx));
    ns(node, 0x14u, f_add(nd(node, 0x14u), hx));
    ns(node, 0x1cu, f_add(nd(node, 0x1cu), hz));
    ns(node, 0x34u, f_add(nd(node, 0x34u), hz));
    ns(node, 0xc4u, nd(node, 0x2cu));
    ns(node, 0xc8u, nd(node, 0x30u));
    ns(node, 0xccu, nd(node, 0x34u));

    g0 = node;
    game_start_race_obj_car_velocity_step(node, 0, 0);

    /* @0x2E730: predicted = (x+vx, y+vy, z+vz) at fp+0x70. */
    pred[0] = f_add(nd(node, 0x14u), nd(node, 0x2cu));
    pred[1] = f_add(nd(node, 0x18u), nd(node, 0x30u));
    pred[2] = f_add(nd(node, 0x1cu), nd(node, 0x34u));
    fst(frame, 0x70u, pred[0]);
    fst(frame, 0x74u, pred[1]);
    fst(frame, 0x78u, pred[2]);
    g0 = node;
    g1 = (uintptr_t)(frame + 0x70u);
    g2 = node + 0x88u;
    game_start_race_obj_car_index_refresh(node, pred);
    fst(frame, 0x70u, pred[0]);
    fst(frame, 0x74u, pred[1]);
    fst(frame, 0x78u, pred[2]);

    /*
     * @0x2E778: push, identity, translate predicted, yaw/pitch/roll,
     * then 0x2c of (0, −0.4, 0). ldq after the refresh reloads Z
     * from fp+0x78.
     */
    fifo(0x10002020u);
    fifo(0x12802525u);
    fifo(0x13802727u);
    fifo(pred[0]);
    fifo(pred[1]);
    fifo(pred[2]);
    fifo(0x15002a2au);
    fifo(nd(node, 0x3cu));
    fifo(0x14802929u);
    fifo(nd(node, 0x38u));
    fifo(0x15802b2bu);
    fifo(nd(node, 0x40u));
    fifo(0x16002c2cu);
    fifo(0u);
    fifo(0xbecccccdu);
    fifo(0u);
    orient[0] = fifo_ld();
    orient[1] = fifo_ld();
    orient[2] = fifo_ld();
    fifo(0x10802121u);
    fst(frame, 0x80u, orient[0]);
    fst(frame, 0x84u, orient[1]);
    fst(frame, 0x88u, orient[2]);

    /* @0x2E878: ldis node+0x88, ld 0x214354, table_index_a then _b. */
    index = (i32)(int16_t)i960_ld_u16(I960_ABS, node, 0x88u);
    g0 = wr(0x214354u);
    g1 = (u32)index;
    g2 = (uintptr_t)(frame + 0x80u);
    g3 = 0;
    geo_view_table_index_a((void *)(uintptr_t)g0, (u32)g1, frame + 0x80u);
    g0 = (uintptr_t)(frame + 0x80u);
    g1 = (uintptr_t)(frame + 0x40u);
    g2 = 0;
    geo_view_table_index_b(frame + 0x80u, frame + 0x40u, 0);
    g14 = MODE1_INDEX_B_LINK;

    /* @0x2E8A8: stos (flag_half * 0x1111) to node+0x12. */
    {
        i32 half = (i32)(int16_t)(fld(frame, 0x5cu) & 0xffffu);

        scaled = (u32)half;
        scaled = (scaled << 12) + (scaled << 8) + (scaled << 4) + scaled;
        i960_st_u16(I960_ABS, node, 0x12u, (u16)scaled);
    }
    flag = fld(frame, 0x5cu) & 0xffu;
    if ((flag & 0x80u) != 0u)
        r13 = mode1_snap(node, frame);
    else
        r13 = mode1_blend(node, frame);
    mode1_epilogue(node, frame, r13);

    g8 = g8_save;
    g9 = g9_save;
    g10 = g10_save;
    g11 = g11_save;
    g12 = g12_save;
    fp = fp_save;
    sp = sp_save;
}
