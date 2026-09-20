/* CGM record tile blit @ 0x2A200 (batched u16 tile map upload, XOR g13). */
// @rom 0x2a200 +0x80 cgm_record_tile_blit

#include "i960_lift.h"
#include "i960_mem.h"

#define CGM_G13_MASK 0x20C958u

static u32 cgm_tile_map_base(void)
{
    if (((u32)g4 & 1u) != 0)
        return 0x01004000u;
    return 0x01000000u;
}

void cgm_record_tile_blit(u32 row, u32 col, u32 width, u32 height)
{
    u32 tile_base;
    u32 g13_mask;
    u32 y;
    u32 x;
    u32 map_row;
    u32 map_col;

    tile_base = cgm_tile_map_base();
    if ((g4 & 2u) != 0)
        tile_base += 0x2000u;

    g13_mask = i960_ld_u32(I960_WORKRAM, CGM_G13_MASK, 0);
    /* @0x2A234 setbit 6; @0x2A240 setbit 15 — XOR mask for compiled u16 atoms. */
    g13_mask |= 0x8040u;
    map_row = row & 0xffffu;
    map_col = col & 0xffffu;

    for (y = 0; y < (height & 0xffffu); y++) {
        u32 map_x = map_col;
        u32 map_y = map_row + y;

        for (x = 0; x < (width & 0xffffu); x++) {
            u32 map_index;
            u16 raw;
            u16 entry;

            if (map_x > 63u || map_y > 63u)
                break;
            map_index = (map_x << 6) + map_y;
            raw = i960_ld_u16(I960_ABS, tile_base, map_index * 2u);
            entry = (u16)((raw ^ (u16)g13_mask) & 0xffffu);
            i960_st_u16(I960_ABS, tile_base, map_index * 2u, entry);
            map_x++;
        }
    }
}
