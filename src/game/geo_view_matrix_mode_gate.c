/* Matrix mode gate @ 0x3AE50 — hysteresis on 0x214330 / 0x21432C.
 * source: disasm/maincpu/maincpu_03ae50_f4.asm */
// @rom 0x3ae50 +0xf4 geo_view_matrix_mode_gate

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

void geo_view_matrix_mode_gate(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode;
    u32 count;
    float x;

    (void)arg1;
    (void)arg2;

    mode = i960_ld_u32(I960_WORKRAM, 0x214330, 0);
    x = (float)i960_u32_to_f64(arg0);

    if (mode == 1u)
        goto mode1;
    if ((i32)mode > 1) {
        if (mode == 2u) {
            /* bal 0x5cce8; g0 &= 7; palette via table 0x5d9e30 */
            g0 = geo_rng_u8((u32)g0, (u32)g1, (u32)g2);
            g0 = (u32)g0 & 7u;
            g0 = i960_ld_u32(I960_WORKRAM, 0x5d9e30, (u32)g0 << 2);
            comm_palette_index_call((u32)g0);
            i960_st_u32(I960_WORKRAM, 0x214330, 0, 0);
        }
        return;
    }
    if (mode != 0u)
        return;

    /* mode 0: climb toward 1 when x > 0.15 */
    if (x > 0.15f) {
        count = i960_ld_u32(I960_WORKRAM, 0x21432c, 0);
        i960_st_u32(I960_WORKRAM, 0x21432c, 0, count + 1u);
        if ((i32)count > 60) /* addo 31,29 */
            i960_st_u32(I960_WORKRAM, 0x214330, 0, 1u);
        return;
    }
    count = i960_ld_u32(I960_WORKRAM, 0x21432c, 0);
    if ((i32)count <= 0)
        count = 0;
    else
        count -= 1u;
    i960_st_u32(I960_WORKRAM, 0x21432c, 0, count);
    return;

mode1:
    if (x < 0.01f) {
        tile_texture_descriptor_apply(0x6cu, 0x40u, 0);
        i960_st_u32(I960_WORKRAM, 0x21432c, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x214330, 0, 2u);
    }
}
