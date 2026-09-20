/* Angle wrap @ 0x27B28 — clamp float g0 into [-π, π] by ±2π once.
 * Called 3× from geo_view_scene_frame epilogue path.
 * source: disasm/maincpu/maincpu_027b28_9c.asm */
// @rom 0x27b28 +0x9c geo_view_float_clamp

#include "i960_lift.h"
#include "i960_fp.h"

u32 geo_view_float_clamp(u32 arg0, u32 arg1, u32 arg2)
{
    float x;
    double pi;
    double two_pi;

    (void)arg1;
    (void)arg2;

    x = (float)i960_u32_to_f64(arg0);
    /* movrl 0x400921FB_54442D18 → π; 0x401921FB_54442D18 → 2π. */
    pi = i960_rifl_read(0x54442d18u, 0x400921fbu);
    two_pi = i960_rifl_read(0x54442d18u, 0x401921fbu);

    if (x > 0.f) {
        if ((double)x > pi)
            x = (float)((double)x - two_pi);
    } else {
        double npi = i960_rifl_read(0x54442d18u, 0xc00921fbu);

        if ((double)x < npi)
            x = (float)((double)x + two_pi);
    }
    g0 = (u32)i960_f64_to_u32((double)x);
    return (u32)g0;
}
