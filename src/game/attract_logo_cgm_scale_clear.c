/* Clear scaled logo scratch rows @ 0x32720 (after sega_mini CGM submit).
 * source: disasm/maincpu/maincpu_032720_70.asm */
// @rom 0x32720 +0x70 attract_logo_cgm_scale_clear

#include "i960_lift.h"
#include "i960_mem.h"

void attract_logo_cgm_scale_clear(u32 arg0, u32 arg1, u32 arg2)
{
    u32 ptr;
    u32 step;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    ptr = i960_ld_u32(I960_WORKRAM, 0x20d824, 0);
    ptr = (ptr << 5) + 0x01080000u;
    for (step = 0; step <= 0xffu; step++) {
        ptr = ptr + 12u;
        i960_st_u32(I960_ABS, ptr, 0xfffffff4u, 0);
        i960_st_u32(I960_ABS, ptr, 0xfffffff8u, 0);
        i960_st_u32(I960_ABS, ptr, 0xfffffffcu, 0);
        i960_st_u32(I960_ABS, ptr, 0, 0);
        ptr = ptr + 16u;
        i960_st_u32(I960_ABS, ptr, 0xfffffff4u, 0);
        i960_st_u32(I960_ABS, ptr, 0xfffffff8u, 0);
        i960_st_u32(I960_ABS, ptr, 0xfffffffcu, 0);
        i960_st_u32(I960_ABS, ptr, 0, 0);
        ptr = ptr + 4u;
    }
}
