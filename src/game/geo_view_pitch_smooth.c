/* Race-path pitch smooth @ 0x39A90 (caller 0x34F8C).
 * source: disasm/maincpu/maincpu_039a90_60.asm */
// @rom 0x39a90 +0x58 geo_view_pitch_smooth

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

u32 geo_view_pitch_smooth(u32 arg0, u32 arg1, u32 arg2)
{
    double prev;
    double cur;
    double blended;
    u32 bits;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* bal geo_view_fov_scale → g0 (does not seed 0x2142c0 first) */
    g0 = geo_view_fov_scale(0, 0, 0);
    cur = i960_u32_to_f64((u32)g0);
    prev = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2142c0, 0));
    blended = prev + (cur - prev) * 0.9;
    bits = (u32)i960_f64_to_u32(blended);
    i960_st_u32(I960_WORKRAM, 0x2142c0, 0, bits);
    /* notbit 31,g4,g0 — return sign-toggled float */
    g0 = bits ^ 0x80000000u;
    return (u32)g0;
}
