/* Semantic C from MAME disasm @ 0x3b7c0 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03b7c0_1ec.asm */
// @rom 0x3b7c0 +0x1ec geo_attract_course_catalog_span

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"

/*
 * Walk course record → catalog index, push CATALOG_VADDR[ix] quads to prg_fifo,
 * then walk a span list pushing additional catalog rows.
 *
 * g0 = course table *row EA* (0x5dca00 + course*3*4 from lda, not a load).
 * g1 = span_a, g2 = span_b, g3 = budget, g4 = gate flag
 */

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
}

void geo_attract_course_catalog_span(u32 rec_va, u32 span_a, u32 span_b)
{
    u32 rec_ptr;
    u32 cursor;
    u32 idx_ptr;
    u32 threshold;
    u32 catalog_ix;
    u32 bank_ctr;
    u32 gate;
    u32 flag;
    u32 wr_slot;
    u32 wr_ptr;
    u32 list_base;
    u32 list_len;
    u32 ix;
    u32 start_ix;
    u32 remain;
    u32 take;
    u32 next_ix;
    u32 budget;
    u32 w0, w1, w2, w3;
    u32 cat_base;

    budget = g3;
    flag = g4;
    start_ix = 0;
    list_len = 0;
    ix = 0;

    rec_ptr = i960_ld_u32(I960_WORKRAM, rec_va, 0);
    cursor = rec_ptr + 8u;
    idx_ptr = rec_ptr + 4u;
    for (;;) {
        threshold = i960_ld_u32(I960_WORKRAM, cursor, 0);
        catalog_ix = i960_ld_u32(I960_WORKRAM, idx_ptr, 0);
        idx_ptr += 8u;
        if ((i32)span_a <= (i32)threshold)
            break;
        cursor += 8u;
    }

    geo_bank_poke0();
    {
        u32 base = CATALOG_VADDR + (catalog_ix << 4);

        w0 = i960_ld_u32(I960_ROM, base, 0);
        w1 = i960_ld_u32(I960_ROM, base, 4);
        w2 = i960_ld_u32(I960_ROM, base, 8);
        w3 = i960_ld_u32(I960_ROM, base, 12);
        i960_mmio_write_u32(GEO_PRG_FIFO, w0);
        i960_mmio_write_u32(GEO_PRG_FIFO, w1);
        i960_mmio_write_u32(GEO_PRG_FIFO, w2);
        i960_mmio_write_u32(GEO_PRG_FIFO, w3);
        bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        /* addo uses g3 after ldq — catalog word3, not caller budget. */
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + w3);
    }

    gate = i960_host_race_course_index();
    if ((gate == 0u || gate == 4u) && flag != 0u)
        geo_attract_copro_marker_burst(0, 0, 0);

    wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    wr_ptr = i960_ld_u32(I960_MMIO, 0x802008, 0);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);
    i960_st_u32(I960_WORKRAM, wr_slot, 0, wr_ptr);
    list_base = i960_ld_u32(I960_WORKRAM, rec_va, 4);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_mmio_write_u32(0x801008, wr_ptr + 0x34u);

    /* @0x3B890–0x3B8B0: clamp index from span_a - span_b. */
    {
        i32 diff = (i32)span_a - (i32)span_b;

        list_len = i960_ld_u32(I960_WORKRAM, list_base, 0);
        if (diff < 0)
            ix = list_len - 1u;
        else if ((u32)diff < list_len)
            ix = (u32)diff;
        else
            ix = 0u;
    }
    start_ix = ix;

    bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    i960_mmio_write_u32(0x800160, 0u);
    remain = budget - bank_ctr;
    i960_mmio_write_u32(GEO_PRG_FIFO, i960_ld_u32(I960_WORKRAM, 0x5da1f0, 0));
    if ((i32)remain <= 0)
        goto epilogue;

    for (;;) {
        u32 list_ptr = i960_ld_u32(I960_WORKRAM, rec_va, 8);
        u32 entry = i960_ld_u32(I960_WORKRAM, list_ptr, ix << 2);

        geo_bank_poke0();
        cat_base = CATALOG_VADDR + (entry << 4);
        w0 = i960_ld_u32(I960_ROM, cat_base, 0);
        w1 = i960_ld_u32(I960_ROM, cat_base, 4);
        w2 = i960_ld_u32(I960_ROM, cat_base, 8);
        w3 = i960_ld_u32(I960_ROM, cat_base, 12);

        if (remain == 0u)
            take = w3;
        else if (w3 >= remain)
            take = remain;
        else
            take = w3;

        i960_mmio_write_u32(GEO_PRG_FIFO, w0);
        i960_mmio_write_u32(GEO_PRG_FIFO, w1);
        i960_mmio_write_u32(GEO_PRG_FIFO, w2);
        i960_mmio_write_u32(GEO_PRG_FIFO, take);

        bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + take);

        next_ix = ix + span_b;
        if ((i32)next_ix < 0)
            next_ix = list_len - 1u;
        else if (next_ix >= list_len)
            next_ix = 0u;

        /*
         * @0x3B97C–0x3B980: mov next_ix → r4, then be to epilogue when
         * next_ix == start_ix. ix must become next_ix even on full-cycle exit
         * (otherwise span_table path_le sees [start, last] and over-emits).
         */
        ix = next_ix;
        if (ix == start_ix)
            break;
        remain = budget - i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        if ((i32)remain <= 0)
            break;
    }

epilogue:
    /* @0x3B994: g0=gate, g1=span_b, g2=list_len, g3=start_ix, g4=ix */
    g2 = list_len;
    g3 = start_ix;
    g4 = ix;
    geo_attract_span_table_draw(
        i960_host_race_course_index(), span_b, list_len);
}
