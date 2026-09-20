/* Pitch clamp / FOV latch @ 0x39AF0 (geo_view_params callee).
 * source: disasm/maincpu/maincpu_039af0_430.asm */
// @rom 0x39af0 +0x80 geo_view_pitch_clamp

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

void geo_view_pitch_clamp(u32 arg0, u32 arg1, u32 arg2)
{
    double prev;
    double cur;
    double blended;
    u32 bits;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* bal geo_view_fov_scale; st g0,0x2142c0 */
    g0 = geo_view_fov_scale(0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2142c0, 0, (u32)g0);

    /* bal geo_view_fov_scale again → g0 */
    g0 = geo_view_fov_scale(0, 0, 0);

    prev = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2142c0, 0));
    cur = i960_u32_to_f64((u32)g0);
    /* mulrl by 0.9 (0x3FECCCCC CCCCCCCD), not 0.1 */
    blended = prev + (cur - prev) * 0.9;
    bits = (u32)i960_f64_to_u32(blended);

    i960_st_u32(I960_WORKRAM, 0x2142d4, 0, 1u);
    i960_st_u32(I960_WORKRAM, 0x2142c8, 0, 0); /* g14 */
    i960_st_u32(I960_WORKRAM, 0x2142c0, 0, bits);
    /* notbit 31 — toggle IEEE sign into 0x2142c4 */
    i960_st_u32(I960_WORKRAM, 0x2142c4, 0, bits ^ 0x80000000u);
}
