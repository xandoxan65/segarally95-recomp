/* Geo register flip / CRX upload wait @ 0x3BA0 (geo_renderer_init callee). */
// @rom 0x3ba0 +0x68 geo_reg_flip_wait

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

void geo_reg_flip_wait(u32 base_vaddr, u32 arg1, u32 arg2)
{
    u32 status_idx;
    u32 status;
    const u32 limit = 0x8000u;

    (void)arg1;
    (void)arg2;

    geo_vsync_wait(0, 0, 0);
    status_idx = 0;
    for (;;) {
        g0 = base_vaddr;
        g1 = status_idx;
        g2 = 0x800u;
        geo_texture_addr_write((u32)g0, (u32)g1, (u32)g2);
        i960_mmio_write_u32(0x8000f0, (u32)g14);
        geo_vsync_wait(0, 0, 0);
        geo_vsync_wait(0, 0, 0);
        geo_vsync_wait(0, 0, 0);
        status = i960_ld_u32(I960_ABS, status_idx, 0x2000u);
        if (status >= limit)
            break;
        status_idx = i960_ld_u32(I960_ABS, base_vaddr, 0x2000u);
        (void)status_idx;
        /* Lift: hardware would advance flip status; seed done so one pass suffices. */
        i960_st_u32(I960_ABS, base_vaddr, 0x2000u, limit);
        break;
    }
}
