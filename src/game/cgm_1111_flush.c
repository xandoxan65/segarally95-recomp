/* CGM 1111 batch flush / compact @ 0x2A120.
 * Removes span at batch g0 from the 0x20B950 node table and shrinks 0x20C954.
 * source: disasm/maincpu/maincpu_02a120_e0.asm */
// @rom 0x2a120 +0xe0 cgm_1111_flush

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

#define BATCH_NODE_BASE 0x20b950u

static u32 batch_node(u32 idx)
{
    return BATCH_NODE_BASE + (idx * 8u);
}

void cgm_1111_flush(u32 arg0, u32 arg1, u32 arg2)
{
    u32 span;
    u32 limit;
    u32 node;
    u32 node_hi;
    u32 colorbase;
    u32 src_idx;
    u32 dst_idx;
    u32 adjust;
    u32 pal_base;
    u32 pal_lim;
    u32 pal_idx;
    u32 pal_ptr;
    u32 color_slots;

    (void)arg1;
    (void)arg2;

    /* @0x2A120: ldob 0x20b953[g0*8],g1 */
    span = i960_ld_u8(I960_WORKRAM, batch_node(arg0), 3);
    if (span == 0)
        return;

    limit = i960_ld_u32(I960_WORKRAM, 0x20c954, 0);
    node = batch_node(arg0);
    node_hi = i960_ld_u8(I960_WORKRAM, node, 2);
    src_idx = arg0 + span;
    colorbase = i960_ld_u16(I960_WORKRAM, node, 0);

    /* @0x2A14C–0x2A188: slide later nodes down over the removed span. */
    if ((i32)src_idx < (i32)limit) {
        adjust = node_hi << 7;
        dst_idx = arg0;
        while (src_idx < limit) {
            u32 src = batch_node(src_idx);
            u32 dst = batch_node(dst_idx);
            u32 w0 = i960_ld_u32(I960_WORKRAM, src, 0);
            u32 w1 = i960_ld_u32(I960_WORKRAM, src, 4);
            u16 lo;

            i960_st_u32(I960_WORKRAM, dst, 0, w0);
            i960_st_u32(I960_WORKRAM, dst, 4, w1);
            lo = i960_ld_u16(I960_WORKRAM, dst, 0);
            lo = (u16)(lo - (u16)adjust);
            i960_st_u16(I960_WORKRAM, dst, 0, lo);
            src_idx++;
            dst_idx++;
        }
    }

    /* @0x2A18C–0x2A1A0: shrink batch limit. */
    pal_lim = i960_ld_u32(I960_WORKRAM, 0x20c950, 0);
    limit = limit - span;
    pal_idx = (u32)((i32)colorbase >> 7); /* shrdi 7,g3,r5 */
    pal_base = node_hi + pal_idx;
    i960_st_u32(I960_WORKRAM, 0x20c954, 0, limit);

    /* @0x2A1A8–0x2A1D8: release colorbase slots via palram_colorbase_upload. */
    if ((i32)pal_base < (i32)pal_lim) {
        pal_ptr = 0x01800000u + (pal_base << 5);
        color_slots = pal_idx;
        while ((i32)pal_base < (i32)pal_lim) {
            g0 = color_slots;
            g1 = pal_ptr;
            palram_colorbase_upload((u32)g0, (u32)g1, (u32)g2);
            pal_lim = i960_ld_u32(I960_WORKRAM, 0x20c950, 0);
            color_slots++;
            pal_ptr += 0x20u;
            pal_base++;
        }
    }

    /* @0x2A1DC–0x2A1E8 */
    pal_lim = i960_ld_u32(I960_WORKRAM, 0x20c950, 0);
    i960_st_u32(I960_WORKRAM, 0x20c950, 0, pal_lim - node_hi);
}
