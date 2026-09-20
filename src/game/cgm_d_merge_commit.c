/* Disasm-backed lone-D edge helper: bus merge @ 0x2A4E0 only.
 * 0x2A4E0 has no scratch/colorbase side effect (see maincpu_02a4e0_c0.asm).
 * Pen stripe write is a separate leaf @ 0x2A490 (trampoline 0x5C94D8). */
// helper (no single ROM entry)

#include "i960_lift.h"
#include "i960_mem.h"
#include "cgm_merge.h"
#include "cgm_format.h"

/* @0x2A62C bus coords: g1=merge_max, g0 so (g0+merge_lo)&3 == 3. */
static u32 merge_g0_for_d(u32 merge_lo)
{
    return (3u - (merge_lo & 3u)) & 3u;
}

/*
 * Merge raw u16 into packed bus @ 0x01080000 (same as palram_bus_merge_host).
 *
 * Does not write scratch @ 0x01800000 or GEO colorbase @ 0x01802000 — those are
 * 0x2A490 and boot @ 0x26758 / palram_geo_colorbase_refresh @ 0x333C8 respectively.
 * 0x26918 only enqueues a 32-byte src→scratch copy for later 0x268B0 drain.
 */
u32 cgm_d_merge_commit(u16 raw_u16)
{
    u32 g13;
    u32 merge_lo;
    u32 merge_max;
    u32 color15;
    model2_cgm_merge_ctx ctx;

    g13 = i960_ld_u32(I960_WORKRAM, CGM_G13_MASK, 0);
    if (g13 == 0u) {
        u32 slot = i960_ld_u32(I960_WORKRAM, CGM_SLOT_CURSOR, 0) & 0x3ffu;

        g13 = slot << 7;
        i960_st_u32(I960_WORKRAM, CGM_G13_MASK, 0, g13);
    }

    /* ``0x2A2E0`` spill defaults when indices out of range. */
    if (i960_ld_u32(I960_WORKRAM, 0x20c95c, 0) == 0
        && i960_ld_u32(I960_WORKRAM, 0x20c968, 0) == 0) {
        i960_st_u32(I960_WORKRAM, 0x20c95c, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x20c960, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x20c964, 0, 0x1efu);
        i960_st_u32(I960_WORKRAM, 0x20c968, 0, 0x17fu);
    }

    merge_lo = i960_ld_u32(I960_WORKRAM, 0x20c95c, 0);
    merge_max = i960_ld_u32(I960_WORKRAM, 0x20c968, 0);
    ctx.words = 0;
    ctx.word_cap = 0x8000u;
    ctx.merge_lo = merge_lo;
    ctx.merge_mid = i960_ld_u32(I960_WORKRAM, 0x20c960, 0);
    ctx.merge_max2 = i960_ld_u32(I960_WORKRAM, 0x20c964, 0);
    ctx.merge_max = merge_max;
    ctx.g13_mask = g13;

    color15 = palram_bus_merge(
        &ctx, merge_g0_for_d(merge_lo), merge_max, (u32)raw_u16);
    if (color15 == 0xffffffffu)
        color15 = (u32)raw_u16 & 0x7fffu;
    return color15;
}
