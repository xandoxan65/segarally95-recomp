/* Blit descriptor publish @ 0x26860 (clone tail @ 0x5C58A4). */
// @rom 0x26860 +0x48 boot_tile_blit_register

#include "i960_lift.h"
#include "i960_mem.h"

void boot_tile_blit_register(u32 arg0, u32 arg1, u32 arg2)
{
    u32 dest;
    u32 len;
    u32 count;
    u32 bind_idx;
    u32 node;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    dest = (u32)g0;
    len = (u32)g2;

    count = i960_ld_u32(I960_WORKRAM, 0x20b900, 0);
    if (count >= 0x3fu)
        return;

    /* @0x26880/0x29F88: bind ordinal from 20B950 table keyed by publish count. */
    bind_idx = i960_ld_u16(I960_WORKRAM, 0x20b950, count << 3);
    node = i960_ld_u32(I960_WORKRAM, 0x20b600, bind_idx << 2);

    i960_st_u32(I960_WORKRAM, node, 0, dest);
    i960_st_u32(I960_WORKRAM, node, 8, len);
    i960_st_u32(I960_WORKRAM, 0x20b900, 0, count + 1u);
}
