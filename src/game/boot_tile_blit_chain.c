/* Walk workram 0x20B600 descriptor list and memcpy tiles @ 0x26800. */
// @rom 0x26800 +0x58 boot_tile_blit_chain

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

#define TILE_BLIT_LIST_HEAD  0x20B600u

void boot_tile_blit_chain(u32 arg0, u32 arg1, u32 arg2)
{
    u32 count;
    u32 node;
    u32 iter;
    u32 src_table;
    u32 dst;
    u32 src;
    u32 len;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    count = i960_ld_u32(I960_WORKRAM, 0x20b900, 0);
    /* @0x26808: lda 0x20b600,r4 — list head is a struct at fixed workram. */
    node = TILE_BLIT_LIST_HEAD;
    iter = 0;
    while (node != 0 && iter < count) {
        src_table = i960_ld_u32(I960_WORKRAM, node, 4);
        dst = i960_ld_u32(I960_WORKRAM, node, 0);
        src = i960_ld_u32(I960_WORKRAM, src_table, 0);
        len = i960_ld_u32(I960_WORKRAM, node, 8);
        g0 = dst;
        g1 = src;
        g2 = len;
        i960_call_rom(0x5daa0);
        node = i960_ld_u32(I960_WORKRAM, node, 12);
        iter++;
    }
}
