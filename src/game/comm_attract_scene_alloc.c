/* Comm-attract scene buffer alloc @ 0x12B70 (callx target @ workram 0x20A78C). */
// @rom 0x12b70 +0xfc comm_attract_scene_alloc

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

static u32 scene_catalog(u32 x, u32 y, u32 catalog, u32 mode, u32 flags)
{
    g0 = x;
    g1 = y;
    g2 = catalog;
    g3 = mode;
    g4 = flags;
    return catalog_draw_setup(x, y, catalog);
}

void comm_attract_scene_alloc(u32 arg0, u32 arg1, u32 arg2)
{
    u32 slot;
    u32 board_type;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    scene_list_seed(0, 0, 0);
    comm_scene_catalog_init(1, 0, 0);

    slot = scene_catalog(48, 43, 0x2123b54u, 0, 4);
    i960_st_u32(I960_WORKRAM, 0x20a7a8, 0, slot);
    slot = scene_catalog(29, 45, 0x289a500u, 2, 4);
    i960_st_u32(I960_WORKRAM, 0x20a7ac, 0, slot);
    slot = scene_catalog(0, 0, 0x28bfeacu, 0, 4);
    i960_st_u32(I960_WORKRAM, 0x20a798, 0, slot);
    slot = scene_catalog(20, 3, 0x286a980u, 0, 4);
    i960_st_u32(I960_WORKRAM, 0x20a7b4, 0, slot);
    slot = scene_catalog(0, 0, 0x289c558u, 0, 4);
    i960_st_u32(I960_WORKRAM, 0x20a7a0, 0, slot);

    board_type = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    if (board_type == 1u || board_type == 2u) {
        slot = scene_catalog(0, 0, 0x7484cu, 0, 4);
        i960_st_u32(I960_WORKRAM, 0x20a7b8, 0, slot);
    }

    i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0);
}
