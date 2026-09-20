/* Fill/clear tile-map rectangle @ 0x27260 (row stride 0x80). */
// @rom 0x27260 +0x50 boot_tile_map_fill

#include "i960_lift.h"
#include "i960_mem.h"

void boot_tile_map_fill(u32 x, u32 y, u32 width, u32 height)
{
    u32 row_base;
    u32 row;
    u32 col;
    u32 map_ptr;

    row_base = (y << 7) + (x << 1);
    for (row = 0; row < height; row++) {
        if (width == 0)
            break;
        map_ptr = 0x01000000u + row_base;
        for (col = 0; col < width; col++) {
            i960_st_u16(I960_ABS, map_ptr, 0, 0);
            map_ptr += 2u;
        }
        row_base += 0x80u;
    }
}
