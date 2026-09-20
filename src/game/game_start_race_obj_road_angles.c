/* Road-normal → pitch/yaw/roll @ 0x2F520 — called from road_attach 0x2C8D0.
 *
 * g0 = index_b sample (out+0x0c/10/14), g1 = normal @ node+0x9c,
 * g2 = angles @ node+0x38. Private frame (lda 0x70(sp),sp).
 *
 * Sample may be a host private-frame pointer (road_attach frame+0x40).
 *
 * source: disasm/maincpu/maincpu_02f520_380.asm */
// @rom 0x2f520 +0x378 game_start_race_obj_road_angles

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static u32 ld_samp(u32 va, u32 off, u8 *host)
{
    u32 v;

    if (host) {
        memcpy(&v, host + off, 4);
        return v;
    }
    return i960_ld_u32(I960_ABS, va + off, 0);
}

static u32 frame_ld(u8 *frame, u32 off)
{
    u32 v;

    memcpy(&v, frame + off, 4);
    return v;
}

static void frame_st(u8 *frame, u32 off, u32 v)
{
    memcpy(frame + off, &v, 4);
}

static u32 fmul(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) * i960_u32_to_f64(b));
}

static u32 fsub(u32 a, u32 b)
{
    return (u32)i960_f64_to_u32(i960_u32_to_f64(a) - i960_u32_to_f64(b));
}

/* MAME atanr src1,src2,dst → atan2(src2, src1). */
static u32 atan2_bits(u32 src1, u32 src2)
{
    return (u32)i960_f64_to_u32(
        atan2(i960_u32_to_f64(src2), i960_u32_to_f64(src1)));
}

void game_start_race_obj_road_angles(void *arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0xa0];
    u32 samp_va = (u32)(uintptr_t)arg0;
    u8 *samp_host = NULL;
    u32 nrm = arg1;
    u32 ang = arg2;
    u32 nx, ny, nz;
    u32 len;
    u32 ux, uy, uz;
    u32 sx, sy, sz;
    u32 c0, c1, c2;
    u32 b0, b1, b2;
    u32 yaw, pitch, roll;
    u32 rx, ry, rz;
    static int logged;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x70u;

    if (model2_ram_mut(samp_va) == NULL && arg0 != NULL)
        samp_host = (u8 *)arg0;

    if (nrm == 0u || ang == 0u)
        goto done;

    nx = i960_ld_u32(I960_ABS, nrm, 0);
    ny = i960_ld_u32(I960_ABS, nrm, 4);
    nz = i960_ld_u32(I960_ABS, nrm, 8);

    /* @0x2F52C–0x2F594: TGP 0x58; ble ret if length ≤ 0. */
    i960_mmio_write_u32(0x884000, 0x2c005858u);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, nx);
    i960_mmio_write_u32(0x884000, ny);
    i960_mmio_write_u32(0x884000, nz);
    len = i960_mmio_read_u32(0x884000);
    if (!(i960_u32_to_f64(len) > 0.0))
        goto done;

    /* @0x2F598–0x2F604: 0x20 push; 0x2f normalize → 0x40(fp). */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x17802f2fu);
    i960_mmio_write_u32(0x884000, nx);
    i960_mmio_write_u32(0x884000, ny);
    i960_mmio_write_u32(0x884000, nz);
    ux = i960_mmio_read_u32(0x884000);
    uy = i960_mmio_read_u32(0x884000);
    uz = i960_mmio_read_u32(0x884000);
    frame_st(frame, 0x40, ux);
    frame_st(frame, 0x44, uy);
    frame_st(frame, 0x48, uz);

    /* Sample @ g0+0x0c/10/14 (index_b unit XYZ). */
    sx = ld_samp(samp_va, 0x0cu, samp_host);
    sy = ld_samp(samp_va, 0x10u, samp_host);
    sz = ld_samp(samp_va, 0x14u, samp_host);

    /*
     * @0x2F62C–0x2F650 non-equal cross (sample EA ≠ 0x50(fp) on host).
     * i960 subr is src2−src1: with unit in r8/r9/r10 and sample in
     * fp0/g3/g13 this is S×U (not U×S). The prior lift used U×S and
     * flipped yaw by π — practice car faced −Z while travel is +Z.
     *   r4 = sy*uz - uy*sz
     *   r5 = ux*sz - sx*uz
     *   r6 = sx*uy - ux*sy
     */
    c0 = fsub(fmul(sy, uz), fmul(uy, sz));
    c1 = fsub(fmul(ux, sz), fmul(sx, uz));
    c2 = fsub(fmul(sx, uy), fmul(ux, sy));
    frame_st(frame, 0x50, c0);
    frame_st(frame, 0x54, c1);
    frame_st(frame, 0x58, c2);

    /*
     * @0x2F6A4–0x2F6C4 second cross (non-eq):
     *   r8 = c1*sz - sy*c2
     *   r9 = sx*c2 - c0*sz
     *   r10= c0*sy - sx*c1
     */
    b0 = fsub(fmul(c1, sz), fmul(sy, c2));
    b1 = fsub(fmul(sx, c2), fmul(c0, sz));
    b2 = fsub(fmul(c0, sy), fmul(sx, c1));

    /* @0x2F710–0x2F728: |b0|; atanr b2,|b0| → yaw @ ang+4; stq basis→0x40. */
    frame_st(frame, 0x40, b0);
    frame_st(frame, 0x44, b1);
    frame_st(frame, 0x48, b2);
    frame_st(frame, 0x60, b0);
    frame_st(frame, 0x64, b1);
    frame_st(frame, 0x68, b2);
    yaw = atan2_bits(b2, b0 & ~0x80000000u);
    i960_st_u32(I960_ABS, ang, 4, yaw);

    /* @0x2F72C–0x2F750: 0x25; 0x2a(-yaw); 0x2c(basis @ 0x40). */
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, yaw ^ 0x80000000u);
    i960_mmio_write_u32(0x884000, 0x16002c2cu);
    i960_mmio_write_u32(0x884000, frame_ld(frame, 0x40));
    i960_mmio_write_u32(0x884000, frame_ld(frame, 0x44));
    i960_mmio_write_u32(0x884000, frame_ld(frame, 0x48));
    rx = i960_mmio_read_u32(0x884000);
    ry = i960_mmio_read_u32(0x884000);
    rz = i960_mmio_read_u32(0x884000);
    /* Disasm stores g5 into 0x44(fp) then reloads for atan side path. */
    frame_st(frame, 0x44, ry);
    frame_st(frame, 0x74, ry);
    frame_st(frame, 0x78, rz);

    /* @0x2F7B0: atanr rz, ry → pitch @ ang. */
    pitch = atan2_bits(rz, ry);
    i960_st_u32(I960_ABS, ang, 0, pitch);

    /* @0x2F7C0–0x2F838: 0x25; 0x29(-pitch); 0x2a(-yaw); 0x2c(cross @ 0x50). */
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, pitch ^ 0x80000000u);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, yaw ^ 0x80000000u);
    i960_mmio_write_u32(0x884000, 0x16002c2cu);
    i960_mmio_write_u32(0x884000, frame_ld(frame, 0x50));
    i960_mmio_write_u32(0x884000, frame_ld(frame, 0x54));
    i960_mmio_write_u32(0x884000, frame_ld(frame, 0x58));
    rx = i960_mmio_read_u32(0x884000);
    ry = i960_mmio_read_u32(0x884000);
    rz = i960_mmio_read_u32(0x884000);
    frame_st(frame, 0x50, rx);
    frame_st(frame, 0x80, rx);
    frame_st(frame, 0x84, ry);
    frame_st(frame, 0x88, rz);

    /* @0x2F874: atanr rx, ry → roll @ ang+8; 0x21 pop. */
    roll = atan2_bits(rx, ry);
    i960_st_u32(I960_ABS, ang, 8, roll);
    i960_mmio_write_u32(0x884000, 0x10802121u);

    if (!logged) {
        fprintf(stderr,
                "lift: race_obj_road_angles ang=(%.3g,%.3g,%.3g)\n",
                i960_u32_to_f64(pitch),
                i960_u32_to_f64(yaw),
                i960_u32_to_f64(roll));
        fflush(stderr);
        logged = 1;
    }

done:
    fp = fp_save;
    sp = sp_save;
}
