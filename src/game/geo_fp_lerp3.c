/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00003c18_50.asm */
// @rom 0x3c18 +0x50 geo_fp_lerp3

#include "i960_lift.h"
#include "i960_fp.h"

/* convention: kind=leaf_bx  args g0,g1,g2  link g14→g6 bx */
/* abi: u32 arg0=g0, void * arg1=g1, void * arg2=g2 → void */

/* pointers: g1=u32 *, g2=u32 *, g3=u32 *, g4=u32, g5=u32 */

void geo_fp_lerp3(u32 arg0, void * arg1, void * arg2)
{
    u32 * a1 = (u32 *)arg1;
    u32 * a2 = (u32 *)arg2;
    g0 = (uintptr_t)arg0;

    g6 = g14;
    g14 = 0;
    g4 = *(u32 *)a2;
    g5 = *(u32 *)a1;
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g5)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(arg0)) * (i960_u32_to_f64(g4)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    *(u32 *)g3 = (u32)g4;
    g4 = *(u32 *)((uintptr_t)a2 + 0x4);
    g5 = *(u32 *)((uintptr_t)a1 + 0x4);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g5)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(arg0)) * (i960_u32_to_f64(g4)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    *(u32 *)(g3 + 0x4) = (u32)g4;
    g4 = *(u32 *)((uintptr_t)a2 + 0x8);
    g5 = *(u32 *)((uintptr_t)a1 + 0x8);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g5)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(arg0)) * (i960_u32_to_f64(g4)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    *(u32 *)(g3 + 0x8) = (u32)g4;
    /* bx (g6) */
    return;
}
