/* Sqrt mix B @ 0x389B8: same as mix A with scale @ 0x5D6684.
 * source: disasm/maincpu/maincpu_0389b8_54.asm */
// @rom 0x389b8 +0x54 geo_view_sqrt_mix_b

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include <math.h>

u32 geo_view_sqrt_mix_b(u32 arg0, u32 arg1, u32 arg2)
{
    float x;
    float scale;
    float out;

    (void)arg1;
    (void)arg2;

    x = (float)i960_u32_to_f64(arg0);
    scale = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d6684, 0));
    out = scale * sqrtf(x) + (1.f - scale) * (x * x);
    g0 = (u32)i960_f64_to_u32((double)out);
    return (u32)g0;
}
