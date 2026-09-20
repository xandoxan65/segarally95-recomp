/* Semantic lift: palram packed bus merge @ ROM 0x2A4E0 (D opcode upload path). */
// @rom 0x2a4e0 +0xb0 palram_bus_merge

#include "i960_lift.h"
#include "i960_mem.h"
#include "cgm_merge.h"

#define PALRAM_PACKED_BUS  0x01080000u
#define MERGE_LO           0x0020C95Cu
#define MERGE_MID          0x0020C960u
#define MERGE_MAX2         0x0020C964u
#define MERGE_MAX          0x0020C968u
#define G13_MASK           0x0020C958u

/* ``words == NULL`` → live packed bus @ ``0x01080000`` (host register path). */
static u16 bus_read(const model2_cgm_merge_ctx *ctx, u32 byte_off)
{
    u32 idx;

    if (!ctx)
        return 0;
    if (ctx->words == 0) {
        if (byte_off + 1u >= 0x10000u)
            return 0;
        return i960_ld_u16(I960_ABS, PALRAM_PACKED_BUS, byte_off);
    }
    if (byte_off + 1u >= ctx->word_cap * 2u)
        return 0;
    idx = byte_off / 2u;
    return ctx->words[idx];
}

static void bus_write(model2_cgm_merge_ctx *ctx, u32 byte_off, u16 value)
{
    u32 idx;

    if (!ctx)
        return;
    if (ctx->words == 0) {
        if (byte_off + 1u >= 0x10000u)
            return;
        i960_st_u16(I960_ABS, PALRAM_PACKED_BUS, byte_off, value);
        return;
    }
    if (byte_off + 1u >= ctx->word_cap * 2u)
        return;
    idx = byte_off / 2u;
    ctx->words[idx] = value;
}

/* Returns merged color15 (0..0x7fff) or 0xffffffff on bounds reject. */
u32 palram_bus_merge(model2_cgm_merge_ctx *ctx, u32 g0, u32 g1, u32 g2)
{
    u32 g5_bound;
    u32 g4_lo;
    u32 g6;
    u32 g1_sum;
    u32 g4_idx;
    u32 g7_base;
    u32 g5_idx;
    u32 g0_n;
    u32 g6_n;
    u32 g4_addr;
    u32 byte_base;
    u32 byte_off;
    i32 shift_amt;
    u32 insert_mask;
    u32 new_bits;
    u32 existing;
    u32 merged;

    if (!ctx)
        return 0xffffffffu;

    g5_bound = ctx->merge_max & 0xffffu;
    g4_lo = ctx->merge_lo & 0xffffu;
    g6 = (g1 - g5_bound) & 0xffffu;
    g1_sum = (g0 + g4_lo) & 0xffffu;

    if (g1_sum < g4_lo || g1_sum > (ctx->merge_max2 & 0xffffu))
        return 0xffffffffu;
    if (g6 < (ctx->merge_mid & 0xffffu) || g6 > g5_bound)
        return 0xffffffffu;

    g4_idx = (g6 >> 3) & 0xffffu;
    g7_base = ctx->g13_mask & 0xffffu;
    g4_idx = ((g4_idx << 6) + g7_base) & 0xffffu;
    g5_idx = (g1_sum >> 3) & 0xffffu;
    g0_n = g6 & 7u;
    g6_n = g1_sum & 7u;
    g4_addr = ((g4_idx + g5_idx) << 5) & 0xffffu;
    byte_base = g4_addr;

    if (g6_n > 3u) {
        u32 g1_n = g1_sum & 3u;
        u32 nibble_off = (g1_n << 2) & 0xffffu;

        byte_off = byte_base + nibble_off + 2u;
        existing = bus_read(ctx, byte_off);
        shift_amt = (i32)nibble_off - 12;
    } else {
        u32 g1_n;
        u32 nibble_off;

        byte_off = byte_base + (g0_n * 4u);
        existing = bus_read(ctx, byte_off);
        g1_n = g1_sum & 3u;
        nibble_off = (g1_n << 2) & 0xffffu;
        shift_amt = (i32)nibble_off - 12;
    }

    if (shift_amt >= 16)
        insert_mask = 0;
    else if (shift_amt <= 0)
        insert_mask = shift_amt < 0
            ? (0xffffu >> (u32)(-shift_amt)) & 0xffffu
            : 0xffffu;
    else
        insert_mask = (0xffffu << (u32)shift_amt) & 0xffffu;

    if (shift_amt < 0)
        new_bits = (g2 & 0xffffu) >> (u32)(-shift_amt);
    else
        new_bits = (g2 & 0xffffu) << (u32)shift_amt;
    new_bits &= 0xffffu;
    merged = (existing & ~insert_mask) | (new_bits & insert_mask);
    merged &= 0xffffu;
    bus_write(ctx, byte_off, (u16)merged);
    return merged & 0x7fffu;
}

/* @0x2A4E0 register ABI: g0/g1 bus coords, g2 = raw u16 (D path, no XOR). */
void palram_bus_merge_host(u32 arg0, u32 arg1, u32 arg2)
{
    model2_cgm_merge_ctx ctx;

    ctx.words = 0;
    ctx.word_cap = 0x8000u;
    ctx.merge_lo = i960_ld_u32(I960_WORKRAM, MERGE_LO, 0);
    ctx.merge_mid = i960_ld_u32(I960_WORKRAM, MERGE_MID, 0);
    ctx.merge_max2 = i960_ld_u32(I960_WORKRAM, MERGE_MAX2, 0);
    ctx.merge_max = i960_ld_u32(I960_WORKRAM, MERGE_MAX, 0);
    ctx.g13_mask = i960_ld_u32(I960_WORKRAM, G13_MASK, 0);
    (void)palram_bus_merge(&ctx, arg0, arg1, arg2);
}
