/* Cold-boot geo view setup wrapper @ 0x2920. */
// @rom 0x2920 +0x10 geo_view_boot_apply

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

void geo_view_boot_apply(u32 arg0, u32 arg1, u32 arg2)
{
    geo_view_params(arg0, arg1, arg2);
    /* stob g14,0x202049 — overwrites the 16 written by geo_view_params */
    i960_st_u8(I960_WORKRAM, 0x202049, 0, (u8)g14);
}
