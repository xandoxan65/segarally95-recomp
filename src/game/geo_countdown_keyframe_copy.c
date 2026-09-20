/* Countdown keyframe copy @ 0x3F4D0 — sample one 7-word pose from a
 * stride-7 table.
 *
 * g0 = time (float), g1 = limit (float, exclusive upper for floor index),
 * g2 = source base, g3 = dest (7 words).
 * Clamps t into [0, limit−1], floor → index, copies words [0..6].
 *
 * source: disasm/maincpu/maincpu_03f4d0_80.asm */
// @rom 0x3f4d0 +0x78 geo_countdown_keyframe_copy

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

void geo_countdown_keyframe_copy(u32 arg0, u32 arg1, u32 arg2)
{
    double t;
    double lim;
    i32 idx;
    u32 src;
    u32 dst;
    u32 i;

    (void)arg2;

    t = i960_u32_to_f64(arg0);
    lim = i960_u32_to_f64(arg1);
    src = (u32)g2;
    dst = (u32)g3;

    /* @0x3F4D0–0x3F4F8: t < 0 → 0; t >= lim → lim − 1. */
    if (t < 0.0)
        t = 0.0;
    else if (!(t < lim))
        t = lim - 1.0;

    /* @0x3F500–0x3F50C: cvtzril → idx; EA = src + idx*7*4. */
    idx = (i32)t;
    if (idx < 0)
        idx = 0;
    src = src + (u32)idx * 28u;

    for (i = 0; i < 7u; i++)
        i960_st_u32(I960_ABS, dst, i << 2, i960_ld_u32(I960_ABS, src, i << 2));
}
