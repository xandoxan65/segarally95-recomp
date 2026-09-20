/* Submode jump table @ 0x4DD0: (0x20209C & 7) → handler in staged workram table @ 0x5A3DB0. */
// @rom 0x4dd0 +0x5c geo_submode_dispatch

#include "i960_lift.h"
#include "i960_mem.h"

void geo_submode_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 submode;
    u32 handler;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    submode = i960_ld_u32(I960_WORKRAM, 0x20209c, 0) & 7u;
    handler = i960_ld_u32(I960_WORKRAM, 0x5a3db0, submode << 2);
    if (handler)
        i960_call_indirect(handler);
    else {
        submode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x202098, 0, submode + 1);
    }

    if (i960_ld_u32(I960_WORKRAM, 0x202074, 0) & 8u) {
        submode = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, submode + 1);
    }
}
