/* RNG @ 0x5CCE8 — 31-bit LCG on seed @ 0x2197B0 (mult 0x5D588B65).
 * bal from geo_view_scene_frame / geo_view_matrix_mode_gate.
 * source: disasm/maincpu/maincpu_05cce8_40.asm */
// @rom 0x5cce8 +0x40 geo_rng_u8

#include "i960_lift.h"
#include "i960_mem.h"

u32 geo_rng_u8(u32 arg0, u32 arg1, u32 arg2)
{
    u32 seed;
    u32 lo;
    u32 hi;
    u32 out;
    u64 prod;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    seed = i960_ld_u32(I960_WORKRAM, 0x2197b0, 0);
    /* emul 0x5d588b65, seed → lo/hi; chkbit31(lo); addc hi,hi; addo lo */
    prod = (u64)seed * 0x5d588b65ull;
    lo = (u32)prod;
    hi = (u32)(prod >> 32);
    out = lo + (hi << 1) + ((lo >> 31) & 1u);
    out &= 0x7fffffffu; /* clrbit 31 */
    i960_st_u32(I960_WORKRAM, 0x2197b0, 0, out);
    g0 = out;
    return (u32)g0;
}
