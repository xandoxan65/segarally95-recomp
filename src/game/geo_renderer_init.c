/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
// @rom 0x3720 +0x70 geo_renderer_init

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

void geo_renderer_init(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    geo_palette_lut_upload((u32)g0, (u32)g1, (u32)g2);
    geo_lumaram_init((u32)g0, (u32)g1, (u32)g2);
    g5 = 4;
    i960_st_u16(I960_ABS, 0x10000000, 0, 4);
    geo_lumaram_pattern((u32)g0, (u32)g1, (u32)g2);
    geo_reg_bootstrap((void *)(uintptr_t)g0, (u32)g1);
    geo_mode_setup((u32)g0, (u32)g1);
    g0 = 0;
    i960_st_u32(I960_WORKRAM, 0x202004, 0, (u32)g14);
    g1 = 0;
    texture_bank_select((u32)g0, (u32)g1, (u32)g2);

    do {
        g0 = (u32)-1;
        g1 = 0;
        texture_bank_select((u32)g0, (u32)g1, (u32)g2);
        g4 = i960_ld_u32(I960_WORKRAM, 0x20227c, 0);
    } while (g4 != 0);

    geo_render_state_clear((u32)g0, (u32)g1, (u32)g2);
    g0 = 0x00202280u;
    geo_crx_lut_upload((u32)g0, (u32)g1, (u32)g2);
    g0 = 0x00202280u;
    geo_reg_flip_wait((u32)g0, (u32)g1, (u32)g2);
    i960_st_u32(I960_WORKRAM, 0x202278, 0, (u32)g14);
}
