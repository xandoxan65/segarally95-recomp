/* Course-variant catalog seed @ 0x3DE18 (bal from inner_2 init @ 0x10728). */
// @rom 0x3de18 +0x20 comm_attract_catalog_seed

#include "i960_lift.h"
#include "i960_mem.h"

void comm_attract_catalog_seed(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x3DE20: subo 1,0,g4 → -1 into 2140C4 / 2140C8. */
    i960_st_u32(I960_WORKRAM, 0x2140c4, 0, (u32)-1);
    i960_st_u32(I960_WORKRAM, 0x2140c8, 0, (u32)-1);
}
