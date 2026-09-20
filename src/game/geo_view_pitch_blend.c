/* Pitch blend @ 0x3AA38 — writes blended pitch to 0x2142E4.
 * source: disasm/maincpu/maincpu_03aa38_a8.asm */
// @rom 0x3aa38 +0xa8 geo_view_pitch_blend

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

void geo_view_pitch_blend(u32 arg0, u32 arg1, u32 arg2)
{
    float k;
    float a0;
    float a1;
    float w_ec;
    float t;
    float num;
    float den;
    float out;

    (void)arg2;

    /* k = load(0x5d9314) * 0.3  (0x3FD3333333333333) */
    k = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d9314, 0)) * 0.3f;
    a0 = (float)i960_u32_to_f64(arg0) *
         (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2142f4, 0));
    w_ec = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2142ec, 0));
    t = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d9304, 0)) / w_ec;
    a1 = (float)i960_u32_to_f64(arg1) - a0;
    num = k * a1;
    den = t + (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d9300, 0));
    /* divrl chain: (k * (g1 - g0*f4)) / w_ec / den  — disasm divides by fp0(=w_ec) then by den */
    out = (num / w_ec) / den;
    i960_st_u32(I960_WORKRAM, 0x2142e4, 0, (u32)i960_f64_to_u32((double)out));
}
