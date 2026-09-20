/* Scroll tile-map rows via workram 0x20B200 buffer @ 0x268B0. */
// @rom 0x268b0 +0x58 boot_tile_scroll_copy

#include "i960_lift.h"
#include "i960_mem.h"

void boot_tile_scroll_copy(u32 arg0, u32 arg1, u32 arg2)
{
    u32 scroll;
    u32 rows_left;
    u32 buf_idx;
    u32 dest;
    u32 src;
    u32 word;
    u32 lane;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    scroll = i960_ld_u32(I960_WORKRAM, 0x20b1f0, 0);
    if (scroll == 0)
        return;

    rows_left = 16u;
    while (scroll != 0 && rows_left != 0) {
        buf_idx = (i960_ld_u32(I960_WORKRAM, 0x20b1f4, 0) - scroll) & 0x7fu;
        dest = i960_ld_u32(I960_WORKRAM, 0x20b200, buf_idx * 8u);
        src = i960_ld_u32(I960_WORKRAM, 0x20b200, buf_idx * 8u + 4u);
        /* @0x268DC/@0x268E4 are ldq/stq: copy both 16-byte halves. */
        for (lane = 0; lane < 16u; lane += 4u) {
            word = i960_ld_u32(I960_ABS, src, lane);
            i960_st_u32(I960_ABS, dest, lane, word);
            word = i960_ld_u32(I960_ABS, src, 16u + lane);
            i960_st_u32(I960_ABS, dest, 16u + lane, word);
        }
        scroll--;
        rows_left--;
    }
    i960_st_u32(I960_WORKRAM, 0x20b1f0, 0, scroll);
}
