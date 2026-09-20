/* Batch stream pointer @ 0x29AE8 (alias 0x20B954 → 0x20B950 row + 4). */
// @rom 0x29ae8 scene_stream_lookup

#include "i960_lift.h"
#include "i960_mem.h"

u32 scene_stream_lookup(u32 index, u32 base)
{
    u32 sum;
    u32 limit;

    sum = index + base;
    limit = i960_ld_u32(I960_WORKRAM, 0x20c954, 0);
    /* @0x29AF4: index 0 with limit 0 still fails cmpibl — use row when catalog ctrl set. */
    if (sum >= limit) {
        u32 entry = 0x20b950u + (sum << 3);
        u32 ctrl = i960_ld_u32(I960_WORKRAM, entry, 4);

        if (ctrl != 0)
            return ctrl;
        return 0;
    }
    return i960_ld_u32(I960_WORKRAM, 0x20b954, sum << 3);
}
