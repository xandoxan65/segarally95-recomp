/* Count tile blanker @ 0x27160 — write g0 zeros from cursor (X cap 61).
 * source: disasm/maincpu/maincpu_027160_70.asm */
// @rom 0x27160 +0x70 boot_tile_script_blank_n

#include "i960_lift.h"
#include "i960_mem.h"

void boot_tile_script_blank_n(u32 arg0, u32 arg1, u32 arg2)
{
    u32 count;
    u32 row;
    u32 col;
    u32 map;
    u32 remain;
    u32 limit;

    (void)arg1;
    (void)arg2;

    count = arg0;
    row = i960_ld_u32(I960_WORKRAM, 0x20b1ac, 0);
    col = i960_ld_u32(I960_WORKRAM, 0x20b1a8, 0);
    remain = count - 1u;
    map = 0x01000000u + (((row << 6) + col) << 1);
    if ((i32)count <= 0)
        return;

    limit = 31u + 30u; /* 61 */
    for (;;) {
        i960_st_u16(I960_ABS, map, 0, 0);
        if (col > limit)
            return;
        if ((i32)remain <= 0)
            return;
        col = col + 1u;
        map = map + 2u;
        remain = remain - 1u;
        if ((i32)remain < 0)
            return;
    }
}
