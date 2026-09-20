/* View matrix constant seed @ 0x39690 (geo_view_params callee). */
// @rom 0x39690 +0x88 geo_view_matrix_seed

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

void geo_view_matrix_seed(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;

    i960_st_u32(I960_WORKRAM, 0x214284, 0, 0x41c80000u);
    i960_st_u32(I960_WORKRAM, 0x214288, 0, 0x43820000u);
    i960_st_u32(I960_WORKRAM, 0x21427c, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x214280, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x21428c, 0, 0x42f00000u);

    g0 = geo_view_fov_scale(arg0, 0, 0);

    /* stq r4,0x2142a0 and stq r8,0x2142b0 with r4..r11 cleared */
    i960_st_u32(I960_WORKRAM, 0x2142a0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2142a4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2142a8, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2142ac, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2142b0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2142b4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2142b8, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2142bc, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x214294, 0, (u32)g0);
    i960_st_u32(I960_WORKRAM, 0x214290, 0, (u32)g0);
}
