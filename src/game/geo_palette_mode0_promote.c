/* Palette irq mode 0 @ 0x33864 — promote queued state from 0x213840. */
// @rom 0x33864 +0x2c geo_palette_mode0_promote

#include "i960_lift.h"
#include "i960_mem.h"

void geo_palette_mode0_promote(u32 arg0, u32 arg1, u32 arg2)
{
    u32 w0;
    u32 w1;
    u32 w2;
    u32 w3;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x33864: st g14 → clear cursor / 0x213838 */
    i960_st_u32(I960_WORKRAM, 0x213834, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x213838, 0, 0);

    /* @0x3386C: ldq 0x213840 → @0x33884 stq 0x213850; clear 0x213840 */
    w0 = i960_ld_u32(I960_WORKRAM, 0x213840, 0);
    w1 = i960_ld_u32(I960_WORKRAM, 0x213844, 0);
    w2 = i960_ld_u32(I960_WORKRAM, 0x213848, 0);
    w3 = i960_ld_u32(I960_WORKRAM, 0x21384c, 0);
    i960_st_u32(I960_WORKRAM, 0x213840, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x213850, 0, w0);
    i960_st_u32(I960_WORKRAM, 0x213854, 0, w1);
    i960_st_u32(I960_WORKRAM, 0x213858, 0, w2);
    i960_st_u32(I960_WORKRAM, 0x21385c, 0, w3);
}
