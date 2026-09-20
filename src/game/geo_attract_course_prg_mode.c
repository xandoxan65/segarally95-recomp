/* Semantic C from MAME disasm @ 0x3b9b0 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03b9b0_250.asm */
// @rom 0x3b9b0 +0x250 geo_attract_course_prg_mode

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

/*
 * Course-index switch that pushes main_data catalog quads to prg_fifo.
 * Caller (carousel): g0 = course_ix, g1 = span_a.
 */

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
}

static void prg_stq4(u32 w0, u32 w1, u32 w2, u32 w3)
{
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
}

static void bump_bank_ctr(u32 add)
{
    u32 ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);

    i960_st_u32(I960_WORKRAM, 0x20b940, 0, ctr + add);
}

void geo_attract_course_prg_mode(u32 course_ix, u32 span_a, u32 arg2)
{
    u32 w0, w1, w2, w3;
    u32 g5, g6, g7;

    (void)arg2;

    /* @0x3B9BC–0x3B9D0: switch on course_ix. */
    if (course_ix == 2u)
        goto mode_2;
    if (course_ix < 2u) {
        if (course_ix == 0u)
            goto mode_0;
        return;
    }
    if (course_ix == 3u)
        goto mode_3;
    /* @0x3B9D8: cmpibne 4,g0,ret — course 4 falls into mode_0 body. */
    if (course_ix != 4u)
        return;

mode_0:
    /* @0x3B9DC–0x3B9E4: skip first catalog when 25 < span <= 55. */
    if (!(span_a > 25u && span_a <= 55u)) {
        geo_bank_poke0();
        w0 = i960_ld_u32(I960_ROM, 0x28689a0u, 0);
        w1 = i960_ld_u32(I960_ROM, 0x28689a0u, 4);
        w2 = i960_ld_u32(I960_ROM, 0x28689a0u, 8);
        w3 = i960_ld_u32(I960_ROM, 0x28689a0u, 12);
        bump_bank_ctr(w3);
        prg_stq4(w0, w1, w2, w3);
    }
    /*
     * @0x3BA30–0x3BA38: desert/env wrap when span > 36 or span <= 7.
     * Catalog packing (tpa, tha, oba, obc): UV @ 0xc2858, THA = texture_sync
     * slot8 @ 0x217340 (RAM headers, cutout sheet-1), OBA 0x921862, OBC 0x6c.
     * Forest (course 1) never reaches this; mountain uses mode_2.
     */
    if (span_a > 36u || span_a <= 7u) {
        geo_bank_poke0();
        g5 = i960_ld_u32(I960_WORKRAM, 0x217340, 0);
        prg_stq4(0x000c2858u, g5, 0x00921862u, 0x0000006cu);
    }
    return;

mode_2:
    /* @0x3BA9C: gated by 0x2020a4 == 0. */
    if (i960_ld_u32(I960_WORKRAM, 0x2020a4, 0) != 0)
        goto mode_2_tail;
    if (span_a == 22u) {
        geo_bank_poke0();
        w0 = i960_ld_u32(I960_ROM, 0x2869b30u, 0);
        w1 = i960_ld_u32(I960_ROM, 0x2869b30u, 4);
        w2 = i960_ld_u32(I960_ROM, 0x2869b30u, 8);
        w3 = i960_ld_u32(I960_ROM, 0x2869b30u, 12);
        bump_bank_ctr(w3);
        prg_stq4(w0, w1, w2, w3);
    }
    if (span_a == 23u) {
        geo_bank_poke0();
        w0 = i960_ld_u32(I960_ROM, 0x2869b40u, 0);
        w1 = i960_ld_u32(I960_ROM, 0x2869b40u, 4);
        w2 = i960_ld_u32(I960_ROM, 0x2869b40u, 8);
        w3 = i960_ld_u32(I960_ROM, 0x2869b40u, 12);
        bump_bank_ctr(w3);
        prg_stq4(w0, w1, w2, w3);
    }

mode_2_tail:
    if (span_a == 23u) {
        g6 = i960_ld_u32(I960_WORKRAM, 0x2172c8, 0);
        g7 = i960_ld_u32(I960_WORKRAM, 0x217348, 0);
        g5 = i960_ld_u32(I960_WORKRAM, 0x2173c8, 0);
        geo_bank_poke0();
        /* stl g6 → words0/1; st g5; st g14(=0). g7 loaded but only in pair via stl. */
        prg_stq4(g6, g7, g5, 0u);
    }
    return;

mode_3:
    /* @0x3BB9C: emit if 36 < span <= 55. */
    if (span_a > 36u && span_a <= 55u) {
        geo_bank_poke0();
        w0 = i960_ld_u32(I960_ROM, 0x286a540u, 0);
        w1 = i960_ld_u32(I960_ROM, 0x286a540u, 4);
        w2 = i960_ld_u32(I960_ROM, 0x286a540u, 8);
        w3 = i960_ld_u32(I960_ROM, 0x286a540u, 12);
        /* ldq into g0–g3; addo uses g3 (= catalog word3). */
        bump_bank_ctr(w3);
        prg_stq4(w0, w1, w2, w3);
    }
}
