/* Attract countdown threshold tile clear @ 0xF620. */
/* source: decomp/disasm/maincpu/maincpu_00f620_60.asm */
// @rom 0xf620 +0x48 comm_attract_threshold_tile_clear

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_rom.h"

/*
 * Caller (inner_3/5/7): g0 = 12 when frame hits countdown threshold.
 * Clears two tile-map rectangles via boot_tile_map_fill, then clears
 * 0x20a78c (scene hook). Not on the cam/matrix path.
 */
void comm_attract_threshold_tile_clear(u32 arg0, u32 arg1, u32 arg2)
{
    u32 saved = arg0;

    (void)arg1;
    (void)arg2;

    /* @0xF620–0xF63C: stq g4..g7 @ 0x213840 = (4, 0, 0x100, saved_g0). */
    i960_st_u32(I960_WORKRAM, 0x213840, 0, 4u);
    i960_st_u32(I960_WORKRAM, 0x213844, 0, 0u);
    i960_st_u32(I960_WORKRAM, 0x213848, 0, 0x100u);
    i960_st_u32(I960_WORKRAM, 0x21384c, 0, saved);

    /* @0xF630–0xF648: fill (7,5) size 55×21. */
    boot_tile_map_fill(7u, 5u, 55u, 21u);

    /* @0xF64C–0xF65C: fill (15,30) size 35×6. */
    boot_tile_map_fill(15u, 30u, 35u, 6u);

    /* @0xF660: st g14 → 0x20a78c (link ≈ 0 on this path). */
    i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0u);
}
