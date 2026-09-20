/* Fill logo tile stripe @ 0x326C8 (bal) using scaled halfword from 0x20D824.
 * source: disasm/maincpu/maincpu_0326c8_58.asm */
// @rom 0x326c8 +0x58 attract_logo_tile_stripe

#include "i960_lift.h"
#include "i960_mem.h"

void attract_logo_tile_stripe(u32 arg0, u32 arg1, u32 arg2)
{
    u32 tile;
    u32 row;
    u32 row_limit;
    u32 map;
    i32 col_left;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    tile = (u32)i960_ld_u16(I960_WORKRAM, 0x20d824, 0);
    tile = tile + 0xffff8000u; /* lda 0xffff8000(g4) */
    row_limit = 31u + 14u; /* 45 */
    for (row = 30; (i32)row <= (i32)row_limit; row++) {
        map = 0x01000058u + (row << 7);
        col_left = 15;
        do {
            col_left -= 1;
            i960_st_u16(I960_ABS, map, 0, (u16)tile);
            tile = tile + 1u;
            map = map + 2u;
        } while (col_left >= 0);
    }
}
