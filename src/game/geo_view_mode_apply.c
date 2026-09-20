/* View mode apply @ 0x39C40 — writes 0x202049 from pitch/FOV state.
 * Modes in 0x2142C8: 0=full (copro 0x0A801515), 1=countdown, 2/3=latch.
 * source: disasm/maincpu/maincpu_039af0_430.asm */
// @rom 0x39c40 +0x2d0 geo_view_mode_apply

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void mode_apply_write_byte(u32 value)
{
    i960_st_u8(I960_WORKRAM, 0x202049, 0, (u8)value);
}

void geo_view_mode_apply(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode;
    u32 prev_c4;
    float g0f;
    float g1f;
    float g2f;
    float g6f;
    float abs_g2;
    float t;
    float ang;
    float k;
    float fifo_out;
    i32 code;

    g0 = arg0;
    g1 = arg1;
    g2 = arg2;

    sp = sp + 16u;

    mode = i960_ld_u32(I960_WORKRAM, 0x2142c8, 0);
    prev_c4 = i960_ld_u32(I960_WORKRAM, 0x2142c4, 0);
    i960_st_u32(I960_WORKRAM, 0x2142c4, 0, arg0);
    g0f = (float)i960_u32_to_f64(arg0);
    g6f = g0f - (float)i960_u32_to_f64(prev_c4);

    if (mode == 1u)
        goto mode_countdown;
    if ((i32)mode < 1) {
        if (mode == 0u)
            goto mode_full;
        goto mode_done_clear;
    }
    if (mode == 2u)
        goto mode_latch2;
    if (mode == 3u)
        goto mode_latch3;
    goto mode_done_clear;

mode_full:
    g2f = (float)i960_u32_to_f64(arg2);
    abs_g2 = (g2f < 0.f) ? -g2f : g2f;
    /* 0.1 * abs + 0.9 * (0.1 * abs) * abs  — matches mulrl chain */
    t = 0.9f * sqrtf(abs_g2) + 0.1f * (abs_g2 * abs_g2);
    ang = t * (float)M_PI;

    k = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d8534, 0));
    i960_mmio_write_u32(0x884000, 0x0a801515u);
    i960_mmio_write_u32(0x884000, (u32)i960_f64_to_u32((double)ang));
    g1f = (float)i960_u32_to_f64(arg1) * k;
    fifo_out = (float)i960_u32_to_f64(i960_mmio_read_u32(0x884000));
    g1f = g1f * fifo_out;

    /* if |g0| > 0.05 (0x3FA99999 9999999A), subtract 0x5d8530 * g6 */
    {
        float mag0 = (g0f < 0.f) ? -g0f : g0f;

        if (mag0 > 0.05f) {
            float s = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d8530, 0));

            g1f = g1f - s * g6f;
        }
    }

    {
        float c = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x214278, 0));
        float scaled = c * g0f + g1f;

        g0 = geo_view_trig_scale((u32)i960_f64_to_u32((double)scaled), 0, 0);
    }

    code = (i32)(float)i960_u32_to_f64((u32)g0); /* cvtzri */
    g1f = (float)i960_u32_to_f64((u32)g0);
    if (code < 0)
        code = -code;
    if (code > 31)
        code = 31;

    if (code == 0) {
        u8 flag = (u8)i960_ld_u8(I960_WORKRAM, 0x20201a, 0);

        if (flag == 1u) {
            mode_apply_write_byte(0);
            return;
        }
        {
            u32 w = i960_ld_u32(I960_WORKRAM, 0x202008, 0) & 3u;

            if (w == 0u) {
                code = -1;
                g1f = -1.f;
            } else {
                code = 1;
                g1f = 1.f;
            }
        }
    }

    if (g1f < 0.f)
        code = code + 0x7f;
    else
        code = code + 0xbf;
    mode_apply_write_byte((u32)code);
    return;

mode_countdown:
    {
        i32 n = (i32)i960_ld_u32(I960_WORKRAM, 0x2142d0, 0);
        float fov = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x214270, 0));
        float v = (float)n * (fov + 0.1f);

        code = (i32)v; /* cvtzri */
        if (code < -31)
            code = -31;
        if (code > 31)
            code = 31;
        if (code < 0)
            code = (-64) - code; /* 0xffffffc0 - code */
        else
            code = code - 0x80;
        mode_apply_write_byte((u32)code);

        {
            u32 cc = i960_ld_u32(I960_WORKRAM, 0x2142cc, 0);

            cc = cc - 1u;
            i960_st_u32(I960_WORKRAM, 0x2142cc, 0, cc);
            if ((i32)cc == -1)
                i960_st_u32(I960_WORKRAM, 0x2142c8, 0, 0);
        }
    }
    return;

mode_latch2:
    {
        u32 b = i960_ld_u8(I960_WORKRAM, 0x2142d0, 0);

        i960_st_u32(I960_WORKRAM, 0x2142d4, 0, 1u);
        i960_st_u32(I960_WORKRAM, 0x2142c8, 0, 0);
        mode_apply_write_byte(b + 16u);
    }
    return;

mode_latch3:
    i960_st_u32(I960_WORKRAM, 0x2142c8, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2142d4, 0, 0);
    mode_apply_write_byte(16u);
    return;

mode_done_clear:
    /* fallthrough target of unknown mode — clear latch like 0x39EB8 */
    i960_st_u32(I960_WORKRAM, 0x2142c8, 0, 0);
}
