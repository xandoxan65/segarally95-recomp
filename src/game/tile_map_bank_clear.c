/* Zero one tile-map bank (512× 16-byte entries) @ 0x26B60. */
// @rom 0x26b60 +0x2c tile_map_bank_clear

#include "i960_lift.h"
#include "i960_mem.h"

void tile_map_bank_clear(u32 map_base)
{
    u32 cursor;
    u32 left;

    cursor = map_base;
    left = 0x1ffu;
    while (left != (u32)-1) {
        i960_st_u32(I960_ABS, cursor, 0, 0);
        i960_st_u32(I960_ABS, cursor, 4, 0);
        i960_st_u32(I960_ABS, cursor, 8, 0);
        i960_st_u32(I960_ABS, cursor, 12, 0);
        left--;
        cursor += 0x10u;
    }
}
