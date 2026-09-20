/* Semantic C from MAME disasm @ 0x4350 — edit by hand; not tier-3 byte-matched. */
/* source: decomp/disasm/maincpu/maincpu_004350_234.asm */
// @rom 0x4350 +0x234 geo_lumaram_init

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"

#include <string.h>

extern void geo_fp_lerp3(u32 arg0, void *arg1, void *arg2);

/*
 * Chromatic colorxlat for odd banks (r13 = 1,3,…19). Even banks keep
 * geo_palette_lut_upload grayscale.
 *
 * Private host frame: temps at 0x40(fp)…0x78(fp). Stream @ 0x5A2EB4 must be
 * fully ROM-seeded (model2_workram_seed_palette_gamma).
 */

static u32 clamp_u8_trunc(double x)
{
    i64 v = (i64)x;
    u32 u;

    if (v < 0)
        return 0;
    u = (u32)v;
    if ((u & 0xffffu) > 0xffu)
        return 0xffu;
    return u & 0xffu;
}

void geo_lumaram_init(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x80];
    u32 *stream;
    i32 gate;
    u32 bank;
    u32 *start_rgb; /* 0x40: R,G  0x48: B */
    u32 *knot_rgb;  /* 0x50: R,G,B */
    u32 *lerp_out;  /* 0x60 */
    u32 thresh;     /* r6 */
    u32 thresh_lo;  /* r10 — stream word0, then prior thresh on knot */
    u32 luma_i;
    u16 *dst_b;
    u16 *dst_g;
    u16 *dst_r;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = (uintptr_t)frame + sizeof(frame);

    /* @0x4364–0x437C: count float @ 0x5A2EB0; skip if < 1 */
    gate = (i32)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5a2eb0, 0));
    if (gate < 1) {
        fp = fp_save;
        sp = sp_save;
        return;
    }

    stream = (u32 *)(model2_workram + 0xa2eb4);
    bank = 1u;
    start_rgb = (u32 *)(fp + 0x40);
    knot_rgb = (u32 *)(fp + 0x50);
    lerp_out = (u32 *)(fp + 0x60);

    for (;;) {
        u32 bank_base;

        /* Header: r10, g8, g9, g6, r6, g12, g13, g5 */
        thresh_lo = stream[0];
        start_rgb[0] = stream[1]; /* R0 */
        start_rgb[1] = stream[2]; /* G0 */
        start_rgb[2] = stream[3]; /* B0 @ 0x48 */
        thresh = stream[4];
        knot_rgb[0] = stream[5];
        knot_rgb[1] = stream[6];
        knot_rgb[2] = stream[7];
        stream += 8;

        bank_base = 0x01800000u + (bank << 9);
        dst_b = (u16 *)i960_vaddr_ptr(bank_base + 0x18000u);
        dst_g = (u16 *)i960_vaddr_ptr(bank_base + 0x14000u);
        dst_r = (u16 *)i960_vaddr_ptr(bank_base + 0x10000u);

        for (luma_i = 0; luma_i <= 0x3fu; luma_i++) {
            u32 t_bits = (u32)i960_f64_to_u32((double)(i32)luma_i / 63.0);
            double t = i960_u32_to_f64(t_bits);
            u32 u_bits;
            u32 r_i, g_i, b_i;

            if (t > i960_u32_to_f64(thresh)) {
                /* @0x4448–0x4478: start ← knot; load next thresh + knot RGB */
                start_rgb[0] = knot_rgb[0];
                start_rgb[1] = knot_rgb[1];
                start_rgb[2] = knot_rgb[2];
                thresh_lo = thresh;
                thresh = stream[0];
                knot_rgb[0] = stream[1];
                knot_rgb[1] = stream[2];
                knot_rgb[2] = stream[3];
                stream += 4;
            }

            u_bits = (u32)i960_f64_to_u32(
                (t - i960_u32_to_f64(thresh_lo))
                / (i960_u32_to_f64(thresh) - i960_u32_to_f64(thresh_lo)));

            g3 = (uintptr_t)lerp_out;
            geo_fp_lerp3(u_bits, start_rgb, knot_rgb);

            /* @0x4498 ldq (r11): R,G,B then ×255 */
            r_i = clamp_u8_trunc(i960_u32_to_f64(lerp_out[0]) * 255.0);
            g_i = clamp_u8_trunc(i960_u32_to_f64(lerp_out[1]) * 255.0);
            b_i = clamp_u8_trunc(i960_u32_to_f64(lerp_out[2]) * 255.0);

            if (dst_b)
                *dst_b++ = (u16)b_i;
            if (dst_g)
                *dst_g++ = (u16)g_i;
            if (dst_r)
                *dst_r++ = (u16)r_i;
        }

        /* @0x455C–0x456C: continue flag; r13 += 2 */
        {
            i32 cont = (i32)i960_u32_to_f64(stream[0]);

            stream += 1;
            bank += 2u;
            if (cont <= 1)
                break;
        }
    }

    fp = fp_save;
    sp = sp_save;
}
