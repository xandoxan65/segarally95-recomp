/* Comm-attract inner mode 2 @ 0x10710 (splash init + animate gate). */
// @rom 0x10710 +0xc0 comm_attract_inner_2

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "cgm_format.h"
#include "i960_host.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include <stdio.h>

static u32 comm_attract_inner_2_texture_pick(u32 script_idx)
{
    u32 course_variant;

    course_variant = i960_ld_u32(I960_WORKRAM, COURSE_VARIANT_INDEX, 0);
    if (script_idx == 0u)
        return model2_workram_mirror_u32(0x005ae430u + (course_variant << 2));
    if (script_idx <= 2u)
        return model2_workram_mirror_u32(0x005ae440u + (course_variant << 2));
    return model2_workram_mirror_u32(0x005ae450u + (course_variant << 2));
}

static void comm_attract_inner_2_animate_tail(void)
{
    u32 script_idx;
    u32 tex_desc;
    u32 link_word;
    u32 inner;

    lift_log( "lift: inner2 animate_tail enter\n");

    /* @0x10824–0x10858: script rotation via 0x5AF85C jump table. */
    script_idx = i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7f4, 0, 999u);
    i960_st_u32(I960_WORKRAM, 0x20a7f8, 0, 999u);
    tex_desc = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);

    /* @0x108xx: subo 1 / cmpobl 6 — unsigned; 0 wraps and keeps prior tex. */
    script_idx -= 1u;
    if (script_idx <= 6u)
        tex_desc = comm_attract_inner_2_texture_pick(script_idx);

    lift_log( "lift: inner2 tex_desc=%#x script=%u\n", tex_desc, script_idx);

    /* @0x108B0–0x108DC */
    i960_st_u32(I960_WORKRAM, 0x20a808, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a800, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a80c, 0, tex_desc);
    link_word = model2_workram_mirror_u32(tex_desc + 0xc);
    i960_st_u32(I960_WORKRAM, 0x20a804, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7fc, 0, link_word);

    lift_log( "lift: inner2 before texture_descriptor link=%#x\n", link_word);
    tile_texture_descriptor_apply(0x9au, 0, 0);
    lift_log( "lift: inner2 before subsys_init\n");
    game_subsys_init_stub(0, 0, 0);
    game_d6_mode_apply(1, 50, 0);
    game_d6_count_apply(30, 0, 0);
    lift_log( "lift: inner2 before course_init\n");
    comm_attract_course_init(0, 0, 0);
    lift_log( "lift: inner2 after course_init\n");

    /* @0x10904–0x10930: advance inner mode 2 → 3. */
    i960_st_u32(I960_WORKRAM, 0x20a794, 0, 0);
    inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7d4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7d8, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, inner + 1u);
    lift_log( "lift: inner2 advanced to %u\n", inner + 1u);
}

static void comm_attract_inner_2_animate_body(void)
{
    u32 board_type;

    lift_log( "lift: inner2 animate_body enter\n");

    /* @0x107E8–0x107F4 */
    g0 = i960_ld_u32(I960_WORKRAM, 0x20a7a4, 0);
    lift_log( "lift: inner2 before cgm_1111_flush g0=%#x\n", (u32)g0);
    cgm_1111_flush((u32)g0, 0, 0);
    lift_log( "lift: inner2 after cgm_1111_flush\n");
    tile_map_banks_clear(0, 0, 0);

    /* @0x107F8–0x10820 */
    board_type = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    i960_st_u8(I960_ABS, 0x181c000u, 0, 0xffu);
    if (board_type != 3u)
        i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b1cc0u);
    cgm_scratch_pool_reset(0, 0, 0);
    lift_log( "lift: inner2 before animate_tail\n");
    comm_attract_inner_2_animate_tail();
}

static void comm_attract_inner_2_animate(void)
{
    u32 frame;

    /* @0x107D4 */
    frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    if (frame != 0) {
        comm_attract_countdown_tick(0, 0, 0);
        return;
    }

    comm_attract_inner_2_animate_body();
}

void comm_attract_inner_2(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;
    u32 catalog_addr;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x10710 */
    gate = comm_attract_slot_scan(0, 0, 0);
    if (gate == 0)
        return;

    /* @0x10724: cmpibe 0,20a788 → animate when gate cleared. */
    if (i960_ld_u32(I960_WORKRAM, 0x20a788, 0) == 0) {
        comm_attract_inner_2_animate();
        return;
    }

    /* @0x10728–0x10740 */
    comm_attract_catalog_seed(0, 0, 0);
    g0 = 0xffffu;
    tile_attract_palram_gate((u32)g0);
    i960_st_u8(I960_ABS, 0x181c000u, 0, 0);
    tile_map_banks_clear(0, 0, 0);

    /* @0x10744–0x1077C */
    if (i960_ld_u8(I960_WORKRAM, 0x202019, 0) == 1u)
        catalog_addr = 0x205fbc0u;
    else
        catalog_addr = 0x2879db0u;

    g0 = 0;
    g1 = 0;
    g2 = catalog_addr;
    g3 = 0;
    g4 = 1;
    g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
    i960_st_u32(I960_WORKRAM, 0x20a7a4, 0, (u32)g0);

    /*
     * banks_clear wiped L2 (CREDIT). Catalog seed @ scene_alloc uses g4=4
     * (deferred — no immediate draw). Paint CREDIT / copyright via the same
     * path as per-frame HUD @ 0x12CC0 before the splash-hold hook clear.
     */
    {
        u32 tick = i960_ld_u32(I960_WORKRAM, 0x202008, 0);

        /* Stamp draw needs (tick&31)==0 and bit5; align if vsync left bit5 clear. */
        if ((tick & 31u) != 0u || (tick & (1u << 5)) == 0u)
            i960_st_u32(I960_WORKRAM, 0x202008, 0, 32u);
        comm_attract_scene_hud(0, 0, 0);
    }

    /* @0x10788–0x10798: cmpibe 0,202018 → skip; else palette 0xa7. */
    if (i960_ld_u8(I960_WORKRAM, 0x202018, 0) != 0u)
        comm_palette_index_call(0xa7u);

    /* @0x1079C–0x107C8 */
    i960_st_u32(I960_WORKRAM, 0x20a808, 0, 0xfffffe5cu);
    comm_attract_splash_bind(0);
    i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a774, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7dc, 0, 1u);

    i960_host_milestone_boot_notify_splash(catalog_addr, (u32)g0);
}
