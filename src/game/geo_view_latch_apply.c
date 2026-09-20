/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_038680_24c.asm */
// @rom 0x38680 +0x24c geo_view_latch_apply

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  caller r10  callee r4 */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* call site: caller 0x382f4, g1=r10 */

void geo_view_latch_apply(u32 arg0, u32 arg1, u32 arg2)
{
    u32 r4;
    float fp0v;
    u32 base;

    (void)arg2;

    fp0v = (float)i960_u32_to_f64(arg0);
    r4 = arg0;
    if (fp0v >= 0.f)
        goto L_000386a8;
    i960_st_u32(I960_WORKRAM, 0x214230, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x21424c, 0, 15u);
    return;

    L_000386a8:
        g4 = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
        g4 = (g4 + 8u) & 7u;
        if ((u8)g4 != 0)
            goto L_000388c8;
        g4 = i960_ld_u32(I960_WORKRAM, 0x21424c, 0);
        if ((u8)arg1 == (u8)g4)
            goto L_00038730;
        i960_st_u32(I960_WORKRAM, 0x214230, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x214234, 0, 0);
        base = 0x5dcd10u + (arg1 << 4);
        g5 = i960_ld_u32(I960_WORKRAM, base, 0);
        g6 = i960_ld_u32(I960_WORKRAM, base + 4u, 0);
        g7 = i960_ld_u32(I960_WORKRAM, base + 8u, 0);
        i960_st_u32(I960_WORKRAM, 0x214238, 0, 0);
        g4 = i960_ld_u32(I960_WORKRAM, base + 0xcu, 0);
        i960_st_u32(I960_WORKRAM, 0x214244, 0, (u32)g5);
        i960_st_u32(I960_WORKRAM, 0x21424c, 0, arg1);
        i960_st_u32(I960_WORKRAM, 0x214248, 0, (u32)g6);
        i960_st_u32(I960_WORKRAM, 0x214240, 0, (u32)g7);
        i960_st_u32(I960_WORKRAM, 0x21423c, 0, (u32)g4);
        tile_texture_descriptor_apply(0xb5u, 0, 0);

    L_00038730:
        fp0v = (float)i960_u32_to_f64(arg0);
        if (fp0v != 0.f)
            goto L_0003874c;
        i960_st_u32(I960_WORKRAM, 0x214230, 0, 0);
        r4 = 0;
        goto L_000388a8;

    L_0003874c:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214230, 0);
        if ((u8)g4 != 0)
            goto L_000387fc;
        /* r4 = min(0x7f, (i32)(arg0 * 100.0 * 0.4) + 0x214240) — 0x40590000=100.0, 0.4=0x3fd99999… */
        fp0v = (float)i960_u32_to_f64(r4) * 100.f * 0.4f;
        r4 = (u32)((i32)fp0v + (i32)i960_ld_u32(I960_WORKRAM, 0x214240, 0));
        if ((i32)r4 > 0x7f)
            r4 = 0x7f;
        tile_texture_descriptor_apply(i960_ld_u32(I960_WORKRAM, 0x214244, 0), 0, 0);
        g4 = i960_ld_u32(I960_WORKRAM, 0x214244, 0);
        if ((u8)g4 == 0xb3u)
            tile_texture_descriptor_apply(0x95u, 0x7fu, 0);

        i960_st_u32(I960_WORKRAM, 0x214234, 0, (u32)-1);
        i960_st_u32(I960_WORKRAM, 0x214238, 0, r4);
        i960_st_u32(I960_WORKRAM, 0x214230, 0, 1u);
        goto L_000388a8;

    L_000387fc:
        fp0v = (float)i960_u32_to_f64(r4) * 100.f * 0.4f;
        r4 = (u32)((i32)fp0v + (i32)i960_ld_u32(I960_WORKRAM, 0x21423c, 0));
        if ((i32)r4 > 0x7f)
            r4 = 0x7f;
        g5 = i960_ld_u32(I960_WORKRAM, 0x214238, 0);
        g4 = (r4 >= g5) ? (r4 - g5) : (g5 - r4);
        if ((i32)g4 <= 3)
            goto L_000388a8;
        i960_st_u32(I960_WORKRAM, 0x214238, 0, r4);
        tile_texture_descriptor_apply(0xb5u, 0, 0);
        tile_texture_descriptor_apply(i960_ld_u32(I960_WORKRAM, 0x214248, 0), 0, 0);
        g4 = i960_ld_u32(I960_WORKRAM, 0x214248, 0);
        if ((u8)g4 == 0xb3u)
            tile_texture_descriptor_apply(0x95u, 0x7fu, 0);

    L_000388a8:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214234, 0);
        if ((u8)g4 == (u8)r4)
            goto L_000388c8;
        tile_texture_descriptor_apply(0xb5u, r4, 0);
        i960_st_u32(I960_WORKRAM, 0x214234, 0, r4);

    L_000388c8:
        return;
}
