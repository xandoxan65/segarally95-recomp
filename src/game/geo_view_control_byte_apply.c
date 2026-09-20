/* Control byte apply @ 0x3B128 — updates frame+0x10 bit4 from 0x202052.
 * Returns scaled float bits in g0.
 * source: disasm/maincpu/maincpu_03b128_c4.asm */
// @rom 0x3b128 +0xc4 geo_view_control_byte_apply

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

u32 geo_view_control_byte_apply(u32 arg0, u32 arg1, u32 arg2)
{
    i32 raw;
    float scale;
    u8 flags;
    u32 out_bits;

    (void)arg1;
    (void)arg2;

    raw = (i32)((u8)i960_ld_u8(I960_WORKRAM, 0x202052, 0)) - 0x30;
    raw >>= 1;
    scale = (float)raw / 64.f; /* movrl 0x40500000 → 64.0 */

    flags = (u8)i960_ld_u8(I960_WORKRAM, arg0 + 0x10u, 0);
    if (scale > 0.6f)
        flags = (u8)(flags | 0x10u);
    else
        flags = (u8)(flags & (u8)~0x10u);
    i960_st_u8(I960_WORKRAM, arg0 + 0x10u, 0, flags);

    /*
     * g6 = scale; cmpibge 1,214120 → keep scale when 214120 <= 1;
     * else fall through and force g6 = 0.5f (Intel: src1 >= src2).
     */
    out_bits = (u32)i960_f64_to_u32((double)scale);
    if ((i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0) > 1)
        out_bits = 0x3f000000u; /* 0.5f */

    {
        float a = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x5d9310, 0));
        float b = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x214124, 0));
        float c = (float)i960_u32_to_f64(out_bits);
        float out = a * b * c;
        if (out < 0.f)
            out = 0.f;
        g0 = (u32)i960_f64_to_u32((double)out);
    }
    return (u32)g0;
}
