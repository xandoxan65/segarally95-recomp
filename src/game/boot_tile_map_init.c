/* Tile-map bank clear + layer table seed @ 0x26A10. */
// @rom 0x26a10 +0x148 boot_tile_map_init

#include "i960_lift.h"
#include "i960_mem.h"

static void tile_map_stq_zero(u32 base, u32 count)
{
    u32 cursor;
    u32 left;

    cursor = base;
    left = count;
    while (left != (u32)-1) {
        i960_st_u32(I960_ABS, cursor, 0, 0);
        i960_st_u32(I960_ABS, cursor, 4, 0);
        i960_st_u32(I960_ABS, cursor, 8, 0);
        i960_st_u32(I960_ABS, cursor, 12, 0);
        left--;
        cursor += 0x10u;
    }
}

void boot_tile_map_init(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_st_u16(I960_WORKRAM, 0x20b914, 0, (u16)g14);
    i960_st_u16(I960_WORKRAM, 0x20b916, 0, (u16)g14);
    i960_st_u16(I960_WORKRAM, 0x20b918, 0, (u16)g14);
    i960_st_u16(I960_WORKRAM, 0x20b91a, 0, (u16)g14);
    i960_st_u16(I960_WORKRAM, 0x20b91c, 0, (u16)g14);
    i960_st_u16(I960_WORKRAM, 0x20b91e, 0, (u16)g14);
    i960_st_u16(I960_WORKRAM, 0x20b920, 0, (u16)g14);
    i960_st_u16(I960_WORKRAM, 0x20b922, 0, (u16)g14);

    g0 = 0x01000000u;
    i960_call_rom(0x26b60);
    g0 = 0x01002000u;
    i960_call_rom(0x26b60);
    g0 = 0x01004000u;
    i960_call_rom(0x26b60);
    g0 = 0x01006000u;
    i960_call_rom(0x26b60);

    tile_map_stq_zero(0x0100c000u, 0xbfu);
    tile_map_stq_zero(0x0100d000u, 0xbfu);
    tile_map_stq_zero(0x01008000u, 0x2fu);
    tile_map_stq_zero(0x01008400u, 0x2fu);
    tile_map_stq_zero(0x01008800u, 0x2fu);
    tile_map_stq_zero(0x01008c00u, 0x2fu);
}
