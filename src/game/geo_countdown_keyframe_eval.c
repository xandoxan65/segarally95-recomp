/* Countdown keyframe eval @ 0x3F550 — interpolate a 7-word pose into dest.
 *
 * Args (from 0x3F9A0): g0=dest (0x215c00), g1=keyframe base, g2=t,
 * g4=limit (120), g5=mode. Mode 3: single 3f4d0 sample into dest.
 * Else: sample floor(t)+{-1,0,+1,+2} into 0x215b80/ba0/bc0/be0 and cubic
 * B-spline blend (N_i,3 — same family as geo_attract_fp_series; 1/6
 * constant @ 0x3F730). Components 0..5 blended; word 6 (draw gate) copied
 * from the floor sample at 0x215bb8 unblended.
 *
 * source: disasm/maincpu/maincpu_03f550_450.asm */
// @rom 0x3f550 +0x3d0 geo_countdown_keyframe_eval

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <string.h>

static u32 fbits(double v)
{
    return (u32)i960_f64_to_u32(v);
}

static double f64(u32 bits)
{
    return i960_u32_to_f64(bits);
}

static void sample_at(double t, u32 lim_bits, u32 src, u32 dst)
{
    g0 = fbits(t);
    g1 = lim_bits;
    g2 = src;
    g3 = dst;
    geo_countdown_keyframe_copy((u32)g0, (u32)g1, (u32)g2);
}

void geo_countdown_keyframe_eval(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x60];
    u32 dest = arg0;
    u32 src = arg1;
    u32 lim_bits = (u32)g4;
    u32 mode = (u32)g5;
    double t;
    double floor_t;
    double u;
    double w0, w1, w2, w3;
    u32 i;

    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x20;

    t = f64((u32)g2);

    /* @0x3F568: mode == 3 → single sample into dest via 0x215ba0 scratch. */
    if (mode == 3u) {
        sample_at(t, lim_bits, src, 0x00215ba0u);
        for (i = 0; i < 7u; i++)
            i960_st_u32(I960_ABS, dest, i << 2,
                        i960_ld_u32(I960_ABS, 0x00215ba0u, i << 2));
        fp = fp_save;
        sp = sp_save;
        return;
    }

    floor_t = (double)(i32)t;
    u = t - floor_t;

    /* Four B-spline control samples at floor+{-1,0,+1,+2}. */
    sample_at(floor_t - 1.0, lim_bits, src, 0x00215b80u);
    sample_at(floor_t, lim_bits, src, 0x00215ba0u);
    sample_at(floor_t + 1.0, lim_bits, src, 0x00215bc0u);
    sample_at(floor_t + 2.0, lim_bits, src, 0x00215be0u);

    /*
     * Uniform cubic B-spline basis (disasm builds via mulrl with
     * 0x3fc55554:c62af21c ≈ 1/6 @ 0x3F730 / 0x3F768).
     */
    {
        double u2 = u * u;
        double u3 = u2 * u;
        double inv6 = i960_rifl_read(0xc62af21cu, 0x3fc55554u);

        w0 = (1.0 - u) * (1.0 - u) * (1.0 - u) * inv6;
        w1 = (3.0 * u3 - 6.0 * u2 + 4.0) * inv6;
        w2 = (-3.0 * u3 + 3.0 * u2 + 3.0 * u + 1.0) * inv6;
        w3 = u3 * inv6;
    }

    for (i = 0; i < 6u; i++) {
        double a = f64(i960_ld_u32(I960_ABS, 0x00215b80u, i << 2));
        double b = f64(i960_ld_u32(I960_ABS, 0x00215ba0u, i << 2));
        double c = f64(i960_ld_u32(I960_ABS, 0x00215bc0u, i << 2));
        double d = f64(i960_ld_u32(I960_ABS, 0x00215be0u, i << 2));

        i960_st_u32(I960_ABS, dest, i << 2, fbits(w0 * a + w1 * b + w2 * c + w3 * d));
    }
    /* @0x3F914: gate word from floor sample, not blended. */
    i960_st_u32(I960_ABS, dest, 0x18,
                i960_ld_u32(I960_ABS, 0x00215bb8u, 0));

    fp = fp_save;
    sp = sp_save;
}
