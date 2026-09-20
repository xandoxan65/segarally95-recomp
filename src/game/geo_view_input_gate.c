/* Input gate @ 0x388D0 — stall counter at 0x214250; returns 0 or 5.0f bits.
 * Callers: geo_view_frame_update @ 0x379F4 / 0x37A34 (also 0x37840).
 * source: disasm/maincpu/maincpu_0388d0_7c.asm */
// @rom 0x388d0 +0x7c geo_view_input_gate

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

u32 geo_view_input_gate(u32 arg0, u32 arg1, u32 arg2)
{
    i32 r4;
    u32 counter;
    u32 limit;

    (void)arg1;
    (void)arg2;

    r4 = (i32)arg0;
    if (r4 != 0)
        goto after_zero_hook;

    /* g0 == 0: palette index 0x96, then fall into increment path */
    comm_palette_index_call(0x96u);
    g0 = 0;
    goto check_r4_sign;

after_zero_hook:
    g0 = 0;

check_r4_sign:
    if (r4 >= 0)
        goto nonnegative;

    /* g0 < 0: clear stall counter, return 0 */
    i960_st_u32(I960_WORKRAM, 0x214250, 0, 0);
    g0 = 0;
    return (u32)g0;

nonnegative:
    if (r4 == 0)
        goto increment;

    /* g0 > 0: if counter <= 10, clear and return 0; else fault then 5.0f */
    counter = i960_ld_u32(I960_WORKRAM, 0x214250, 0);
    if ((i32)counter <= 10) {
        i960_st_u32(I960_WORKRAM, 0x214250, 0, 0);
        g0 = 0;
        return (u32)g0;
    }
    tile_texture_descriptor_apply(0x47u, 0x7fu, 0);
    g0 = 0x40a00000u; /* 5.0f */
    i960_st_u32(I960_WORKRAM, 0x214250, 0, 0);
    return (u32)g0;

increment:
    counter = i960_ld_u32(I960_WORKRAM, 0x214250, 0) + 1u;
    limit = 31u + 28u; /* 59 */
    i960_st_u32(I960_WORKRAM, 0x214250, 0, counter);
    if ((i32)counter > (i32)limit)
        g0 = 0;
    else
        g0 = 0x40a00000u; /* 5.0f */
    return (u32)g0;
}
