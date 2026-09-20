/* Palram gate helper @ 0x272B0 (call site from phase comm). */
// @rom 0x272b0 +0x18 tile_attract_palram_gate

#include "i960_lift.h"
#include "i960_mem.h"

void tile_attract_palram_gate(u32 color_word)
{
    if (i960_ld_u32(I960_WORKRAM, 0x213850, 0) != 0)
        return;
    i960_st_u16(I960_ABS, 0x01800000u, 0, (u16)color_word);
    /* 0x33018 — runtime comm helper; static lift no-op. */
}
