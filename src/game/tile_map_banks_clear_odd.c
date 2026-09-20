/* Clear tile map banks 1/3 @ 0x269F0 (pair of 0x269D0).
 * Present in ROM but never called (static scan: zero call / fptr sites).
 * Lift kept for completeness; do not invent call sites. */
// @rom 0x269f0 +0x18 tile_map_banks_clear_odd

#include "i960_lift.h"
#include "lift_syms.h"

void tile_map_banks_clear_odd(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    g0 = 0x01002000u;
    tile_map_bank_clear((u32)g0);
    g0 = 0x01006000u;
    tile_map_bank_clear((u32)g0);
}
