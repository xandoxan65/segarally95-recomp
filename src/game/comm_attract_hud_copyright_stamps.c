/* Copyright / SEGA stamp refresh @ 0x12C70 (epilogue of scene_hud_alt @ 0x1320C). */
// @rom 0x12c70 +0x40 comm_attract_hud_copyright_stamps

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void comm_attract_hud_copyright_stamps(u32 arg0, u32 arg1, u32 arg2)
{
    u32 slot;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x12C70–0x12C74: CREDIT glyph pass (mode 2). */
    comm_scene_catalog_init(2, 0, 0);

    /* @0x12C78–0x12C90 */
    slot = i960_ld_u32(I960_WORKRAM, 0x20a7a8, 0);
    g0 = 48; /* addo 31,17 */
    g1 = 43; /* addo 31,12 */
    g2 = slot;
    g3 = 0;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    /* @0x12C94–0x12CAC */
    slot = i960_ld_u32(I960_WORKRAM, 0x20a7ac, 0);
    g0 = 29;
    g1 = 45; /* addo 31,14 */
    g2 = slot;
    g3 = 0;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
}
