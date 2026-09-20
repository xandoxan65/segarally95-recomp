/* Scratch chain enqueue @ 0x26918 (flush @ 0x268B0 → palram[slot<<5]). */
// @rom 0x26918 +0x60 palram_colorbase_upload

#include "i960_lift.h"
#include "i960_mem.h"

#define SCRATCH_COLORBASE_BASE  0x01800000u

void palram_colorbase_upload(u32 slot, u32 merge_index, u32 arg2)
{
    u32 count;
    u32 write_idx;
    u32 chain_entry;
    u32 scratch_ptr;

    (void)arg2;

    /* @0x26920–0x2692C: count in 0x20b1f0; full when count > 0x7f. */
    count = i960_ld_u32(I960_WORKRAM, 0x20b1f0, 0);
    if (count > 0x7fu)
        return;

    /* @0x26930: write index is 0x20b1f4, not the count. */
    write_idx = i960_ld_u32(I960_WORKRAM, 0x20b1f4, 0);
    /* @0x26938–0x2693C: lda 0x1800000(slot<<5). */
    scratch_ptr = SCRATCH_COLORBASE_BASE + ((slot & 0x3ffu) << 5);
    /* @0x26948: lda 0x20b200[write*8] — address of ring slot. */
    chain_entry = 0x20b200u + ((write_idx & 0x7fu) << 3);
    i960_st_u32(I960_WORKRAM, chain_entry, 0, scratch_ptr);
    i960_st_u32(I960_WORKRAM, chain_entry, 4, merge_index);
    /* @0x26944 / 0x26950–0x26968 */
    count = count + 1u;
    write_idx = (write_idx + 1u) & 0x7fu;
    i960_st_u32(I960_WORKRAM, 0x20b1f4, 0, write_idx);
    i960_st_u32(I960_WORKRAM, 0x20b1f0, 0, count);
}
