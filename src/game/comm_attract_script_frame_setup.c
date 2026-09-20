/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_0104a0_264.asm */
// @rom 0x104a0 +0x264 comm_attract_script_frame_setup

#include "i960_lift.h"
#include "i960_mem.h"
#include "comm_attract_script_frame.h"
#include "lift_syms.h"
#include "model2_rom.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → u32 g0 */

static u32 fp_variant(void)
{
    return comm_attract_fp_u32(0x5c) & 0xffu;
}

static void script_case_emit(u32 mode)
{
    u32 rec_idx;
    u32 span_a;
    u32 span_b;
    u32 scale;
    u32 glyph_off;

    rec_idx = comm_attract_fp_u32(0x60);
    scale = i960_ld_u32(I960_WORKRAM, 0x20a804, 0);
    glyph_off = (u32)i960_ld_u64(I960_WORKRAM, rec_idx * 8u, 0x5ad090);
    span_a = comm_attract_fp_u32(0x50);
    span_b = comm_attract_fp_u32(0x4c);
    scale = (u32)g7 * scale;
    span_a = span_a - span_b;
    if (scale != 0u)
        scale = span_a / scale;
    g0 = mode;
    g1 = scale + glyph_off;
    comm_attract_script_glyph_dispatch((u32)g0, (u32)g1, 0);
}

u32 comm_attract_script_frame_setup(u32 arg0, u32 arg1, u32 arg2)
{
    u32 script_idx;
    u32 variant;
    uintptr_t sp_save = sp;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* Disasm allocates 0x30; restore so attract loops do not walk sp forever. */
    sp = sp + 0x30;
    comm_attract_script_frame_load();

    script_idx = i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0);
    script_idx -= 1u;
    if (script_idx > 6u)
        goto L_00010704;

    switch (script_idx) {
    case 0:
        goto L_000106f8;
    case 1:
        goto L_00010510;
    case 2:
        goto L_0001054c;
    case 3:
        goto L_0001055c;
    case 4:
        goto L_000105cc;
    case 5:
        goto L_00010610;
    case 6:
        goto L_00010680;
    default:
        goto L_00010704;
    }

    L_00010510:
        variant = fp_variant();
        if (variant == 2u)
            goto L_0001051c;
        if (variant != 4u)
            goto L_000106f8;
        goto L_0001051c;

    L_0001051c:
        script_case_emit(2);
        sp = sp_save;
        return (u32)g0;

    L_0001054c:
        variant = fp_variant();
        if (variant == 2u || variant == 4u)
            goto L_000106c0;
        goto L_000106f8;

    L_0001055c:
        variant = fp_variant();
        if (variant == 4u)
            goto L_00010594;
        if (variant != 2u)
            goto L_000106f8;
        goto L_00010594;

    L_00010594:
        script_case_emit(2);
        sp = sp_save;
        return (u32)g0;

    L_000105cc:
        variant = fp_variant();
        if (variant == 4u)
            goto L_00010604;
        if (variant != 2u)
            goto L_000106f8;
        goto L_00010604;

    L_00010604:
        script_case_emit(5);
        sp = sp_save;
        return (u32)g0;

    L_00010610:
        variant = fp_variant();
        if (variant == 2u)
            goto L_000106c0;
        if (variant == 4u)
            goto L_00010648;
        goto L_000106f8;

    L_00010648:
        script_case_emit(6);
        sp = sp_save;
        return (u32)g0;

    L_00010680:
        variant = fp_variant();
        if (variant == 4u)
            goto L_000106b8;
        if (variant != 2u)
            goto L_000106f8;
        goto L_000106b8;

    L_000106b8:
        script_case_emit(7);
        sp = sp_save;
        return (u32)g0;

    L_000106c0:
        script_case_emit(3);
        sp = sp_save;
        return (u32)g0;

    L_000106f8:
        g0 = 1;
        g1 = 0;
        comm_attract_script_glyph_dispatch((u32)g0, (u32)g1, 0);
        sp = sp_save;
        return (u32)g0;

    L_00010704:
        sp = sp_save;
        return 0;
}
