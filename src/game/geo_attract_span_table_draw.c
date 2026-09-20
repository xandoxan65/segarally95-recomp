/* Semantic C from MAME disasm @ 0x47920 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_047920_180.asm */
// @rom 0x47920 +0x180 geo_attract_span_table_draw

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_memory.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Walk 12-byte span records [a, b, slot] from table @ 0x5e66d0[g0].
 * Caller (catalog_span): g0=course, g1=span_b, g2=list_len, g3=start_ix, g4=ix.
 *
 * Emit is index-gated (path_le / path_gt), not “all desert slots every frame”.
 * Desert @0x5e6670: slot3 when walk window covers index 43 (forward
 * span_a≈42–43 → path_le; reverse wrap often path_gt — look for both).
 */

static void span_emit_log(const char *path, u32 table_ix, u32 slot, i32 r6,
                          i32 r5, i32 r7, i32 a, i32 b)
{
    const char *span_log = getenv("I960_GEO_SPAN_LOG");
    static unsigned s_by_slot[18];
    static unsigned s_total;

    if (!span_log || !span_log[0] || span_log[0] == '0')
        return;
    /* Always keep a few lines per slot (slot3 arrives late in desert). */
    if (slot < 18u && s_by_slot[slot] >= 6u && s_total >= 64u)
        return;
    if (slot < 18u)
        s_by_slot[slot]++;
    s_total++;
    lift_log(
            "lift: span_table %s course=%u slot=%u r6=%d r5=%d r7=%d a=%d b=%d\n",
            path, (unsigned)table_ix, (unsigned)slot, r6, r5, r7, a, b);
}

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
}

static void prg_stq_slot(u32 slot)
{
    u32 off = slot << 2;
    u32 w0 = i960_ld_u32(I960_WORKRAM, 0x2172a0u + off, 0);
    u32 w1 = i960_ld_u32(I960_WORKRAM, 0x217320u + off, 0);
    u32 w2 = i960_ld_u32(I960_WORKRAM, 0x2173a0u + off, 0);

    geo_bank_poke0();
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0u);
}

void geo_attract_span_table_draw(u32 table_ix, u32 span_b, u32 list_len)
{
    i32 r7 = (i32)list_len;
    i32 r6 = (i32)g3;
    i32 r5 = (i32)g4;
    u32 rec;
    u32 b_ptr;
    u32 slot_ptr;
    u32 a;
    u32 b;
    u32 slot;

    /* @0x47924–0x4793C */
    if ((i32)span_b < 0) {
        r6 = (i32)g4;
        r5 = (i32)g3;
    }

    rec = i960_ld_u32(I960_WORKRAM, 0x5e66d0u + (table_ix << 2), 0);
    tile_cursor_seed(1u, 20u);

    b_ptr = rec + 4u;
    a = i960_ld_u32(I960_WORKRAM, rec, 0);
    b = i960_ld_u32(I960_WORKRAM, b_ptr, 0);

    if (r6 <= r5)
        goto path_le;

    /* @0x47968: r6 > r5 */
    if ((i32)a < 0)
        return;
    slot_ptr = rec + 8u;
    slot = i960_ld_u32(I960_WORKRAM, slot_ptr, 0);

loop_gt:
    /* @0x47980–0x4799C branch ladder → emit @ 0x479A0 or skip @ 0x479F0 */
    if (r6 > (i32)a)
        goto gt_a;
    if (r7 > (i32)a)
        goto emit_gt;
gt_a:
    if (r6 > (i32)b)
        goto gt_b;
    if (r7 > (i32)b)
        goto emit_gt;
gt_b:
    if (0 > (i32)a)
        goto gt_aneg;
    if (r5 >= (i32)a)
        goto emit_gt;
gt_aneg:
    if (0 > (i32)b)
        goto skip_gt;
    if (r5 < (i32)b)
        goto skip_gt;

emit_gt:
    span_emit_log("emit_gt", table_ix, slot, r6, r5, r7, (i32)a, (i32)b);
    prg_stq_slot(slot);

skip_gt:
    rec += 12u;
    a = i960_ld_u32(I960_WORKRAM, rec, 0);
    b_ptr += 12u;
    slot_ptr += 12u;
    b = i960_ld_u32(I960_WORKRAM, b_ptr, 0);
    if ((i32)a < 0)
        return;
    slot = i960_ld_u32(I960_WORKRAM, slot_ptr, 0);
    goto loop_gt;

path_le:
    /* @0x47A0C */
    if ((i32)a < 0)
        return;
    b_ptr = rec + 4u;
    slot_ptr = rec + 8u;
    slot = i960_ld_u32(I960_WORKRAM, slot_ptr, 0);

loop_le:
    /* @0x47A24–0x47A30 */
    if (r6 > (i32)a)
        goto le_a;
    if (r5 >= (i32)a)
        goto emit_le;
le_a:
    if (r6 > (i32)b)
        goto skip_le;
    if (r5 < (i32)b)
        goto skip_le;

emit_le:
    span_emit_log("emit_le", table_ix, slot, r6, r5, r7, (i32)a, (i32)b);
    prg_stq_slot(slot);

skip_le:
    rec += 12u;
    a = i960_ld_u32(I960_WORKRAM, rec, 0);
    b_ptr += 12u;
    slot_ptr += 12u;
    b = i960_ld_u32(I960_WORKRAM, b_ptr, 0);
    if ((i32)a < 0)
        return;
    slot = i960_ld_u32(I960_WORKRAM, slot_ptr, 0);
    goto loop_le;
}
