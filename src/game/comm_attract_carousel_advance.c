/* Attract carousel script advance @ 0xFDC8 (inner_3 per-frame step). */
// @rom 0xfdc8 +0x74 comm_attract_carousel_advance

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

void comm_attract_carousel_advance(u32 arg0, u32 arg1, u32 arg2)
{
    u32 frame_ctr;
    u32 link_cur;
    u32 table_idx;
    u32 desc_base;
    u32 ix;
    u32 limit;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0xFDD0–0xFE00 */
    frame_ctr = i960_ld_u32(I960_WORKRAM, 0x20a804, 0);
    link_cur = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
    table_idx = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    desc_base = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);

    frame_ctr++;
    /* lda 0x1(g5), g5 — add 1, not a memory load. */
    link_cur += 1u;
    i960_st_u32(I960_WORKRAM, 0x20a804, 0, frame_ctr);
    i960_st_u32(I960_WORKRAM, 0x20a7fc, 0, link_cur);

    /* @0xFE08–0xFE14: lda (g6)[g6*8] → idx*9; ld 0x10(g7)[g4*4]. */
    ix = table_idx + (table_idx << 3);
    if (desc_base >= WORKRAM_BASE && desc_base < WORKRAM_BASE + WORKRAM_SIZE)
        limit = i960_ld_u32(I960_WORKRAM, desc_base + 0x10u + (ix << 2), 0);
    else
        limit = model2_workram_mirror_u32(desc_base + 0x10u + (ix << 2));
    if ((signed int)link_cur <= (signed int)limit)
        return;

    /* @0xFE18–0xFE30 */
    table_idx++;
    i960_st_u32(I960_WORKRAM, 0x20a800, 0, table_idx);
    ix = table_idx + (table_idx << 3);
    if (desc_base >= WORKRAM_BASE && desc_base < WORKRAM_BASE + WORKRAM_SIZE)
        link_cur = i960_ld_u32(I960_WORKRAM, desc_base + 0xcu + (ix << 2), 0);
    else
        link_cur = model2_workram_mirror_u32(desc_base + 0xcu + (ix << 2));
    i960_st_u32(I960_WORKRAM, 0x20a7fc, 0, link_cur);
}
