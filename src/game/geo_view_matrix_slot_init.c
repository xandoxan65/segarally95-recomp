/* Matrix slot init @ 0x3B008 — pick/clamp slot index into g0 from frame +0x78.
 * source: disasm/maincpu/maincpu_03b008_114.asm */
// @rom 0x3b008 +0x114 geo_view_matrix_slot_init

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

u32 geo_view_matrix_slot_init(u32 arg0, u32 arg1, u32 arg2)
{
    u32 counter;
    u32 slot;
    float v7c;
    float thresh;
    float t;

    (void)arg2;

    if (i960_ld_u32(I960_WORKRAM, 0x214120, 0) == 0u) {
        g0 = 1;
        return (u32)g0;
    }

    counter = i960_ld_u32(I960_WORKRAM, 0x2142e0, 0);
    v7c = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, arg0 + 0x7cu, 0));
    slot = i960_ld_u32(I960_WORKRAM, arg0 + 0x78u, 0);
    i960_st_u32(I960_WORKRAM, 0x2142e0, 0, counter + 1u);

    if ((i32)counter <= 29) {
        g0 = slot;
        return (u32)g0;
    }

    /* fp3 ≈ 7903 from movrl(1, 0x40bef000) */
    thresh = (float)i960_rifl_read(1u, 0x40bef000u);
    if (v7c > thresh) {
        t = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2142f0, 0));
        if (t < 0.1f) {
            slot += 1u;
            goto clamp;
        }
    }

    /* arg1 is throttle fraction float bits (matrix_prep r5), not an int. */
    t = (float)i960_u32_to_f64(arg1) * (float)i960_rifl_read(0u, 0x40a57c00u);
    t = t + (float)i960_rifl_read(1u, 0x40a9c800u);
    if (v7c < t)
        slot -= 1u;

clamp:
    if ((i32)slot <= 0)
        slot = 1;
    else if ((i32)slot > 4)
        slot = 4;

    if (i960_ld_u32(I960_WORKRAM, arg0 + 0x78u, 0) != slot)
        i960_st_u32(I960_WORKRAM, 0x2142e0, 0, 0);
    g0 = slot;
    return (u32)g0;
}
