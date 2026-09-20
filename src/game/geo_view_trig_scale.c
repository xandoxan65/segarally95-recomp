/* Angle → FOV-scaled trig table lerp @ 0x39B80.
 * Tables @ 0x5D8330 / 0x5D8430 (workram ROM mirror); scale by 0x214270.
 * source: disasm/maincpu/maincpu_039af0_430.asm */
// @rom 0x39b80 +0xb8 geo_view_trig_scale

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

u32 geo_view_trig_scale(u32 arg0, u32 arg1, u32 arg2)
{
    float angle = (float)i960_u32_to_f64(arg0);
    float mag = (angle < 0.f) ? -angle : angle;
    float clamped;
    i32 idx;
    float frac;
    u32 off;
    float a0, b0;
    float delta;
    float scale;
    float out;

    (void)arg1;
    (void)arg2;

    /* g7 = (mag >= 63.0) ? 63.0f : mag */
    clamped = (mag >= 63.f) ? 63.f : mag;
    idx = (i32)clamped; /* cvtzri */
    frac = clamped - (float)idx;
    off = (u32)idx << 2;

    a0 = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d8330, off));
    b0 = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d8430, off));

    if (idx != 63) {
        float a1 = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d8334, off));
        float b1 = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d8434, off));

        a0 = a0 + (a1 - a0) * frac;
        b0 = b0 + (b1 - b0) * frac;
    }

    delta = b0 - a0;
    scale = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x214270, 0));
    out = a0 + delta * scale;
    if (angle < 0.f)
        out = -out;

    g0 = i960_f64_to_u32((double)out);
    return (u32)g0;
}
