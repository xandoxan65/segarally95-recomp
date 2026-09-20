/* Seed tile cursor @ 0x20B1A4–0x20B1AC (bal target from tile text paths). */
// @rom 0x26e18 +0x24 tile_cursor_seed

#include "i960_lift.h"
#include "i960_mem.h"

void tile_cursor_seed(u32 x, u32 y)
{
    i960_st_u32(I960_WORKRAM, 0x20b1a4, 0, x);
    i960_st_u32(I960_WORKRAM, 0x20b1a8, 0, x);
    i960_st_u32(I960_WORKRAM, 0x20b1ac, 0, y);
}
