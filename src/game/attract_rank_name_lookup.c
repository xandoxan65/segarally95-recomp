/* Rank-name table lookup @ 0x12b20 (call from scene_hud_alt digit loop).
 * Walks pointer list @ 0x5b1ad0; 3-byte compare via libc_scanf_setup.
 * Returns match index, or -1 when none.
 * source: disasm/maincpu/maincpu_012b20_80.asm */
// @rom 0x12b20 +0x50 attract_rank_name_lookup

#include "i960_lift.h"
#include "i960_mem.h"

extern void *libc_scanf_setup(void *arg0, void *arg1, u32 arg2);

u32 attract_rank_name_lookup(u32 needle_va)
{
    u32 cur;
    u32 idx;
    u32 entry;
    void *cmp;

    entry = i960_ld_u32(I960_WORKRAM, 0x5b1ad0, 0);
    idx = 0;
    cur = 0x005b1ad0u;
    if (entry == 0u)
        return (u32)(0u - 1u);

    for (;;) {
        entry = i960_ld_u32(I960_ABS, cur, 0);
        cmp = libc_scanf_setup((void *)(uintptr_t)entry,
                               (void *)(uintptr_t)needle_va, 3u);
        if ((u32)(uintptr_t)cmp == 0u)
            return idx;
        cur += 4u;
        entry = i960_ld_u32(I960_ABS, cur, 0);
        idx++;
        if (entry == 0u)
            break;
    }
    return (u32)(0u - 1u);
}
