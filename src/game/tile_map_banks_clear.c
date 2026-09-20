/* Clear tile map banks 0/2 @ 0x269D0 (inner dispatch + geo cluster).
 * ROM only clears 0x01000000 and 0x01004000. Companion 0x269F0 (banks 1/3)
 * has zero call sites — ranking visibility is Sys24 win/ctrl, not this. */
// @rom 0x269d0 +0x18 tile_map_banks_clear

#include "i960_lift.h"
#include "lift_syms.h"

void tile_map_banks_clear(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    g0 = 0x01000000u;
    tile_map_bank_clear((u32)g0);
    g0 = 0x01004000u;
    tile_map_bank_clear((u32)g0);
}
