/* Angle fraction @ 0x3AAE8 — table lerp from g0 degrees-like input.
 * source: disasm/maincpu/maincpu_03aae8_a4.asm */
// @rom 0x3aae8 +0xa4 geo_view_angle_frac

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

u32 geo_view_angle_frac(u32 arg0, u32 arg1, u32 arg2)
{
    float x;
    float q;
    float frac;
    i32 trunc_x;
    i32 rem;
    i32 idx;
    u32 base;
    float lo;
    float hi;
    float scale;
    float out;

    (void)arg2;

    x = (float)i960_u32_to_f64(arg0);
    q = x / 100.f; /* movrl 0x40590000 → 100.0 */
    trunc_x = (i32)x; /* cvtzri */
    rem = trunc_x % 100; /* remi 0x64 */
    idx = (i32)(q / 1.1f);
    frac = (float)rem / 100.f;

    /*
     * Disasm @ 0x3AB40–0x3AB58:
     *   ld 0x2142f8,g4 ; lda (g4)[g5*4],g4 ; ld (g4),g0 ; ld 0x4(g4),g5
     * 0x2142f8 holds a ROM-mirror float table base (0x5d8f20 / 0x5d90c0).
     * lda only forms EA = base+idx*4 — lo/hi are adjacent floats, not a
     * pointer chase. Extra ld through table[idx] treated float bits as VA.
     */
    base = i960_ld_u32(I960_WORKRAM, 0x2142f8, 0) + (u32)(idx << 2);
    lo = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, base, 0));
    hi = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, base + 4u, 0));
    scale = (float)i960_u32_to_f64(arg1) *
            (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d930c, 0));
    scale *= (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x214124, 0));
    out = (lo + frac * (hi - lo)) * scale;
    /* bal ABI: g1 *= 0x5d930c * 0x214124 (disasm @ 0x3AB5C/0x3AB78). */
    g1 = (u32)i960_f64_to_u32((double)scale);
    g0 = (u32)i960_f64_to_u32((double)out);
    return (u32)g0;
}
