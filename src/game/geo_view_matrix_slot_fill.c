/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03ac50_1a8.asm */
// @rom 0x3ac50 +0x1a8 geo_view_matrix_slot_fill

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  caller r5  callee r4,r5 */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

void geo_view_matrix_slot_fill(u32 arg0, u32 arg1, u32 arg2)
{
    u32 a1 = arg1;
    g0 = arg0;
    g2 = arg2;

    g4 = i960_ld_u32(I960_WORKRAM, 0x214208, 0);
    g6 = i960_ld_u32(I960_WORKRAM, 0x214314, 0);
    r7 = 3 << 8;
    g2 = r7 & g4;
    g5 = i960_f64_to_u32((double)(i32)(u32)(g6));
    if (0 > (signed char)g6) {
        g4 = 0x4f800000u;
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
    }
    g5 = i960_f64_to_u32((i960_u32_to_f64(arg0)) * (i960_u32_to_f64(g5)));
    fp0 = i960_u32_to_f64(g5);
    fp0 = i960_u32_to_f64(g5);
    r6 = 0;
    r7 = 0x40c13000u;
    fp1 = i960_rifl_read(r6, r7);
    i960_rifl_write(&r6, &r7, (fp0) / (fp1));
    fp0 = i960_rifl_read(r6, r7);
    { i64 _cvt = (i64)(fp0); g4 = (uintptr_t)(u32)_cvt; g5 = (uintptr_t)(u32)((u64)_cvt >> 32); };
    r5 = g4;
    if (g4 <= g6)
        goto L_0003acb8;
    r5 = g6;
    goto L_0003acc0;

    L_0003acb8:
        if ((unsigned char)g4 == 0) {
            r5 = 1;
        }

    L_0003acc0:
        g0 = i960_ld_u32(I960_WORKRAM, 0x214318, 0);
        g5 = i960_f64_to_u32((double)(i32)(u32)(g0));
        if (0 > (signed char)g0) {
            g4 = 0x4f800000u;
            g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
        }
    g4 = i960_f64_to_u32((i960_u32_to_f64(a1)) * (i960_u32_to_f64(g5)));
    fp2 = i960_u32_to_f64(g4);
    { i64 _cvt = (i64)(fp2); g6 = (uintptr_t)(u32)_cvt; g7 = (uintptr_t)(u32)((u64)_cvt >> 32); };
    g5 = i960_ld_u32(I960_WORKRAM, 0x214308, 0);
    r4 = g6;
    if (g6 >= g5)
        goto L_0003ad18;
    g4 = g5 - g6;
    g4 = g4 >> 2;
    if ((unsigned char)g4 == 0)
        goto L_0003ad10;
    g1 = g5 - g4;
    goto L_0003ad14;

    L_0003ad10:
        g1 = g5 - 1;

    L_0003ad14:
        r4 = g1;

    L_0003ad18:
        g4 = i960_ld_u32(I960_WORKRAM, 0x2142fc, 0);
        if ((unsigned char)g4 == 0)
            goto L_0003ad2c;
        r4 = g0;
        goto L_0003ad3c;

    L_0003ad2c:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214300, 0);
        if ((unsigned char)g4 != 0) {
            r4 = 0;
        }

    L_0003ad3c:
        g4 = i960_ld_u32(I960_WORKRAM, 0x21431c, 0);
        if ((unsigned char)g4 == 0)
            goto L_0003ad68;
        if ((unsigned char)g2 != 0)
            goto L_0003ad68;
        g0 = i960_ld_u32(I960_WORKRAM, 0x214324, 0);
        g1 = 0;
        tile_texture_descriptor_apply((u32)g0, (u32)g1, 0);
    i960_st_u32(I960_WORKRAM, 0x21431c, 0, (u32)g14);
    goto L_0003ad90;

    L_0003ad68:
        if ((unsigned char)g2 == 0)
            goto L_0003ad90;
        if ((unsigned char)g4 != 0)
            goto L_0003ad90;
        g0 = i960_ld_u32(I960_WORKRAM, 0x214320, 0);
        comm_palette_index_call((u32)g0);
    r6 = 1;
    i960_st_u32(I960_WORKRAM, 0x214304, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x21431c, 0, (u32)r6);

    L_0003ad90:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214304, 0);
        if ((unsigned char)g4 != r5)
            goto L_0003ada8;
        g4 = i960_ld_u32(I960_WORKRAM, 0x214308, 0);
        if ((unsigned char)g4 == r4)
            goto L_0003adf4;

    L_0003ada8:
        g4 = i960_ld_u32(I960_WORKRAM, 0x21431c, 0);
        if ((unsigned char)g4 == 0)
            goto L_0003adc4;
        g0 = i960_ld_u32(I960_WORKRAM, 0x214324, 0);
        g1 = r5;
        tile_texture_descriptor_apply((u32)g0, (u32)g1, 0);

    L_0003adc4:
        g0 = i960_ld_u32(I960_WORKRAM, 0x214310, 0);
        g1 = r4;
        tile_texture_descriptor_apply((u32)g0, (u32)g1, 0);
    g0 = i960_ld_u32(I960_WORKRAM, 0x21430c, 0);
    g1 = r5;
    tile_texture_descriptor_apply((u32)g0, (u32)g1, 0);
    i960_st_u32(I960_WORKRAM, 0x214308, 0, (u32)r4);
    i960_st_u32(I960_WORKRAM, 0x214304, 0, (u32)r5);

    L_0003adf4:
        return;
}
