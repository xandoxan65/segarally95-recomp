/* Angle delta wrap @ 0x27BD8 — shortest signed delta in (-π, π], then ±2π.
 * Called from tile_attract_string_draw angle lerp (bal @ 0x28B4C/0x28B6C/0x28B8C).
 * source: disasm/maincpu/maincpu_027bd8_90.asm */
// @rom 0x27bd8 +0x84 tile_attract_angle_fixup

#include "i960_lift.h"
#include "i960_fp.h"

/*
 * Disasm: lda 0x54442d18 / 0x400921fb then movrl — IEEE double immediates (π),
 * not memory addresses. Same for 2π with high word 0x401921fb.
 */

void tile_attract_angle_fixup(u32 arg0, u32 arg1)
{
    double delta;
    double adelta;
    double pi;
    double twopi;

    g0 = arg0;
    g1 = arg1;

    /* @0x27BE0: subr g0,g1,g0 — delta = to − from */
    delta = i960_u32_to_f64(g1) - i960_u32_to_f64(g0);
    g0 = (u32)i960_f64_to_u32(delta);

    /* @0x27BE4–0x27C08: if |delta| ≤ π, return delta */
    adelta = i960_u32_to_f64(g0 & 0x7fffffffu);
    pi = i960_rifl_read(0x54442d18u, 0x400921fbu);
    if (adelta <= pi)
        return;

    twopi = i960_rifl_read(0x54442d18u, 0x401921fbu);
    delta = i960_u32_to_f64(g0);
    /* @0x27C0C–0x27C4C: delta > 0 → delta−2π; else delta+2π */
    if (delta > 0.0)
        delta -= twopi;
    else
        delta += twopi;
    g0 = (u32)i960_f64_to_u32(delta);
}
