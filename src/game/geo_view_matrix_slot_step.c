/* Matrix slot step @ 0x3AF58 — advances 0x2142E0; may rewrite g0 from 0x202044.
 * source: disasm/maincpu/maincpu_03af58_9c.asm */
// @rom 0x3af58 +0x9c geo_view_matrix_slot_step

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

u32 geo_view_matrix_slot_step(u32 arg0, u32 arg1, u32 arg2)
{
    u32 slot;
    u32 counter;
    u32 next;
    u32 ctrl;
    i32 delta;

    (void)arg1;
    (void)arg2;

    slot = arg0;
    ctrl = i960_ld_u32(I960_WORKRAM, 0x202044, 0);
    counter = i960_ld_u32(I960_WORKRAM, 0x2142e0, 0);

    if ((u8)ctrl == (u8)arg0) {
        next = 0;
        g0 = slot;
        i960_st_u32(I960_WORKRAM, 0x2142e0, 0, next);
        return (u32)g0;
    }

    next = counter + 1u;
    if ((i32)counter <= 6) {
        i960_st_u32(I960_WORKRAM, 0x2142e0, 0, next);
        g0 = slot;
        return (u32)g0;
    }

    ctrl = i960_ld_u32(I960_WORKRAM, 0x202044, 0);
    if ((u8)ctrl != 0) {
        if ((signed char)(u8)ctrl < (signed char)(u8)arg0) {
            i960_st_u32(I960_WORKRAM, 0x2142fc, 0, 6u);
        } else {
            delta = -8;
            if (i960_ld_u32(I960_WORKRAM, 0x2139cc, 0) == 1u)
                delta = -6;
            i960_st_u32(I960_WORKRAM, 0x214300, 0, (u32)delta);
        }
    }
    g0 = i960_ld_u32(I960_WORKRAM, 0x202044, 0);
    i960_st_u32(I960_WORKRAM, 0x2142e0, 0, next);
    return (u32)g0;
}
