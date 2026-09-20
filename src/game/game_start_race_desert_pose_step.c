/* Desert race pose step @ 0x422B0 — spline-sample node floats before TGP rotate.
 *
 * Private host frame (ROM lda 0x90(sp),sp). Inlines table-row fetch @ 0x42270
 * (bal table_pick + clamp + copy XYZ into host frame[] — never i960_st of a
 * truncated host fp+off). Weight products match the ROM mul chain (inv6 @
 * 0x3fc55554c62af21c, half @ 0x3fe00000); this interpolates the interior
 * knots (u=0 → P1), not uniform B-spline N_i,3.
 *
 * Angle samples: table word at ptr+0x10, subtract half_pi
 * (0x3ff921fb54442d18) before blend. Y blend then +2.5
 * (rifl high 0x40040000) → node+0x1c; angle blend → node+0x28.
 *
 * source: /tmp/dasm_422b0c/maincpu_0422b0_500.asm */
// @rom 0x422b0 +0x4ec game_start_race_desert_pose_step

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "lift_syms.h"

#include <string.h>

static u32 frame_u32(u8 *frame, u32 off)
{
    u32 v;

    memcpy(&v, frame + off, 4);
    return v;
}

static void frame_st32(u8 *frame, u32 off, u32 v)
{
    memcpy(frame + off, &v, 4);
}

static u32 f_neg(u32 bits)
{
    return bits ^ 0x80000000u;
}

static u32 f_mul(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) * i960_u32_to_f64(b));
}

static u32 f_add(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) + i960_u32_to_f64(b));
}

/* @0x42270 — dest is a host-frame offset, not a guest VA. */
static u32 table_row_fetch(u32 index, u8 *frame, u32 dest_off)
{
    u32 table;
    i32 count;
    u32 base;
    u32 scaled;
    u32 ptr;
    i32 idx = (i32)index;

    table = game_start_race_desert_table_pick(0, 0, 0);
    count = (i32)(signed short)(u16)i960_ld_u16(I960_ABS, table, 2);
    base = i960_ld_u32(I960_ABS, table, 0x10);
    if (idx >= count)
        idx = count - 1;
    if (idx < 0)
        idx = 0;
    scaled = (u32)idx + ((u32)idx << 1);
    ptr = base + (scaled << 3);
    frame_st32(frame, dest_off + 0u, i960_ld_u32(I960_ABS, ptr, 0));
    frame_st32(frame, dest_off + 4u, i960_ld_u32(I960_ABS, ptr, 4));
    frame_st32(frame, dest_off + 8u, i960_ld_u32(I960_ABS, ptr, 8));
    return i960_ld_u32(I960_ABS, ptr, 0x10);
}

void game_start_race_desert_pose_step(u32 node, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0xb0];
    double half_pi = i960_rifl_read(0x54442d18u, 0x3ff921fbu);
    double inv6 = i960_rifl_read(0xc62af21cu, 0x3fc55554u);
    double half = i960_rifl_read(0, 0x3fe00000u);
    double y_bias = i960_rifl_read(0, 0x40040000u); /* 2.5 */
    double t;
    double u;
    i32 i_clamped;
    i32 i0, i1, i2;
    u32 a0, a1, a2, a3;
    u32 w0, w1, w2, w3;
    u32 um1, up1, um2;
    u32 bx0, bz0, bx1, bz1, bx2, bz2, bx3, bz3;
    u32 by;
    u32 ang;
    u32 node28;
    u32 denom;
    u32 rx, rz;
    u32 yaw_seed;

    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x90;
    g14 = 0;

    t = i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x5c));
    /* subrl +1.0,fp0 → t - 1.0; cvtzri + clamp for first knot index. */
    i_clamped = (i32)(t - 1.0);
    if ((double)i_clamped < 0.0)
        i_clamped = 0;
    i0 = (i32)t;
    i1 = i0 + 1;
    i2 = i1 + 1;

    /* Four control points → 0x50/5c/68/74(fp); angles − half_pi (a3 deferred). */
    a0 = table_row_fetch((u32)i_clamped, frame, 0x50u);
    a0 = (u32)i960_f64_to_u32(i960_u32_to_f64(a0) - half_pi);
    a1 = table_row_fetch((u32)i0, frame, 0x5cu);
    a1 = (u32)i960_f64_to_u32(i960_u32_to_f64(a1) - half_pi);
    a2 = table_row_fetch((u32)i1, frame, 0x68u);
    a2 = (u32)i960_f64_to_u32(i960_u32_to_f64(a2) - half_pi);
    frame_st32(frame, 0xa0, a2);
    a3 = table_row_fetch((u32)i2, frame, 0x74u);
    /* a3 half_pi subtract is at 0x42640, after the Y3 mul uses the raw return. */

    u = t - (double)i0;
    /* Mul-chain weights @ 0x423C8–0x4257C (closed form of the same products). */
    um1 = (u32)i960_f64_to_u32(u - 1.0);
    um2 = (u32)i960_f64_to_u32(u - 2.0);
    up1 = (u32)i960_f64_to_u32(u + 1.0);
    {
        u32 u_bits = (u32)i960_f64_to_u32(u);
        u32 neg_u = f_neg(u_bits);
        u32 neg_up1 = f_neg(up1);
        u32 a = f_mul(f_mul(neg_u, um1), um2);           /* (-u)(u-1)(u-2) */
        u32 b = f_mul(f_mul(up1, um1), um2);             /* (u+1)(u-1)(u-2) */
        u32 c = f_mul(f_mul(neg_up1, u_bits), um2);      /* -(u+1)u(u-2) */
        u32 d = f_mul(f_mul(up1, u_bits), um1);          /* (u+1)u(u-1) */

        w0 = (u32)i960_f64_to_u32(i960_u32_to_f64(a) * inv6);
        w1 = (u32)i960_f64_to_u32(i960_u32_to_f64(b) * half);
        w2 = (u32)i960_f64_to_u32(i960_u32_to_f64(c) * half);
        w3 = (u32)i960_f64_to_u32(i960_u32_to_f64(d) * inv6);
    }

    node28 = node + 0x28u;
    frame_st32(frame, 0x90, node28);
    yaw_seed = f_neg(i960_ld_u32(I960_ABS, node28, 0));

    /* TGP push / clear / yaw seed (0x42420–0x424A8). */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, yaw_seed);

    /* Weighted XZ axis payloads (Y lane cleared with g14). */
    bx0 = f_mul(w0, frame_u32(frame, 0x50));
    bz0 = f_mul(w0, frame_u32(frame, 0x58));
    bx1 = f_mul(w1, frame_u32(frame, 0x5c));
    bz1 = f_mul(w1, frame_u32(frame, 0x64));
    bx2 = f_mul(w2, frame_u32(frame, 0x68));
    bz2 = f_mul(w2, frame_u32(frame, 0x70));
    bx3 = f_mul(w3, frame_u32(frame, 0x74));
    bz3 = f_mul(w3, frame_u32(frame, 0x7c));

    /* 0x42 slot0 ← w0·(X0,0,Z0); 0x44×3 accumulate w1..w3; 0x45 subtract
     * node XZ; 0x48/0x43 (0x424E0–0x42718). HLE: see model2_hw_fifo.c. */
    i960_mmio_write_u32(0x884000, 0x21004242u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, bx0);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, bz0);

    i960_mmio_write_u32(0x884000, 0x22004444u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, bx1);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, bz1);

    i960_mmio_write_u32(0x884000, 0x22004444u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, bx2);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, bz2);

    i960_mmio_write_u32(0x884000, 0x22004444u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, bx3);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, bz3);

    /* Y blend (g5 @ 0x42644); a3 − half_pi; angle blend (g6 @ 0x426C8). */
    by = f_add(f_add(f_mul(w0, frame_u32(frame, 0x54)),
                     f_mul(w1, frame_u32(frame, 0x60))),
               f_add(f_mul(w2, frame_u32(frame, 0x6c)),
                     f_mul(w3, frame_u32(frame, 0x78))));
    a3 = (u32)i960_f64_to_u32(i960_u32_to_f64(a3) - half_pi);
    ang = f_add(f_add(f_mul(w0, a0), f_mul(w1, a1)),
                f_add(f_mul(w2, frame_u32(frame, 0xa0)), f_mul(w3, a3)));

    /* 0x45 node XZ −= from slot; 0x48; 0x21 (0x425EC–0x426CC). */
    i960_mmio_write_u32(0x884000, 0x22804545u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x18));
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x20));
    i960_mmio_write_u32(0x884000, 0x24004848u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, 0x10802121u);

    /* st g6,(node+0x28); st (Y + 2.5),0x1c(node) — disasm 0x426E0/0x426E4. */
    i960_st_u32(I960_ABS, node28, 0, ang);
    i960_st_u32(I960_ABS, node, 0x1c,
                (u32)i960_f64_to_u32(i960_u32_to_f64(by) + y_bias));

    /* 0x43 readback → node+0x6c / +0x68 (0x426E8–0x42788). */
    i960_mmio_write_u32(0x884000, 0x21804343u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    rx = i960_mmio_read_u32(0x884000);
    (void)i960_mmio_read_u32(0x884000);
    rz = i960_mmio_read_u32(0x884000);

    denom = i960_ld_u32(I960_ABS, node, 0x70);
    {
        double inv_k = i960_rifl_read(0x4ae74487u, 0xbfc65718u);
        double pos_k = i960_rifl_read(0x4ae74487u, 0x3fc65718u);

        i960_st_u32(I960_ABS, node, 0x6c,
                    (u32)i960_f64_to_u32(
                        (i960_u32_to_f64(rz) / i960_u32_to_f64(denom)) * inv_k));
        i960_st_u32(I960_ABS, node, 0x68,
                    (u32)i960_f64_to_u32(
                        (i960_u32_to_f64(rx) / i960_u32_to_f64(denom)) * pos_k));
    }

    fp = fp_save;
    sp = sp_save;
}
