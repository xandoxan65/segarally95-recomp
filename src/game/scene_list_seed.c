/* Scene draw list reset @ 0x29A68. */
// @rom 0x29a68 +0x28 scene_list_seed

#include "i960_lift.h"
#include "i960_mem.h"

void scene_list_seed(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_st_u32(I960_WORKRAM, 0x20c954, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20c958, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20c950, 0, 9);
}
