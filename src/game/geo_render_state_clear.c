/* Geo render workram slot clear @ 0x478D0 (geo_renderer_init callee). */
// @rom 0x478d0 +0x50 geo_render_state_clear

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

void geo_render_state_clear(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_st_u8(I960_WORKRAM, 0x217420, 0, (u8)g14);
    i960_st_u32(I960_WORKRAM, 0x217424, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x217428, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x218430, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x218434, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x218438, 0, 0);
    geo_reg_texture_sync((u32)g0, (u32)g1, (u32)g2);
}
