/* Comm-attract inner mode 6 @ 0x113A0 (second splash / re-seed, splash_bind 2). */
/* source: decomp/disasm/maincpu/maincpu_0113a0_200.asm */
// @rom 0x113a0 +0x200 comm_attract_inner_6

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include <stdio.h>

static u32 comm_attract_inner_6_texture_pick(u32 script_idx)
{
    u32 course_variant;

    course_variant = i960_ld_u32(I960_WORKRAM, COURSE_VARIANT_INDEX, 0);
    /* Jump table @ 0x5B04C8 — same 5ae430/440/450 bases as inner_4. */
    if (script_idx == 0u)
        return model2_workram_mirror_u32(0x005ae430u + (course_variant << 2));
    if (script_idx <= 2u)
        return model2_workram_mirror_u32(0x005ae440u + (course_variant << 2));
    return model2_workram_mirror_u32(0x005ae450u + (course_variant << 2));
}

static void comm_attract_inner_6_animate_tail(void)
{
    u32 script_idx;
    u32 tex_desc;
    u32 link_word;
    u32 inner;

    /* @0x11490–0x1159C */
    script_idx = i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7f4, 0, 0x3e7u);
    i960_st_u32(I960_WORKRAM, 0x20a7f8, 0, 0x3e7u);
    tex_desc = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);

    /* Unsigned subo 1 / cmpobl 6 — same as inner_4. */
    script_idx -= 1u;
    if (script_idx <= 6u)
        tex_desc = comm_attract_inner_6_texture_pick(script_idx);

    i960_st_u32(I960_WORKRAM, 0x20a808, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a800, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a80c, 0, tex_desc);
    link_word = model2_workram_mirror_u32(tex_desc + 0xc);
    i960_st_u32(I960_WORKRAM, 0x20a804, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7fc, 0, link_word);

    tile_texture_descriptor_apply(0x9au, 0, 0);
    game_subsys_init_stub(0, 0, 0);
    game_d6_mode_apply(1, 30, 0);
    game_d6_count_apply(15, 0, 0);
    comm_attract_course_init(0, 0, 0);

    i960_st_u32(I960_WORKRAM, 0x20a794, 0, 0);
    inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7d4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7d8, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, inner + 1u);
    fprintf(stderr, "lift: inner6 reseed → inner %u tex=%#x\n",
            (unsigned)(inner + 1u), tex_desc);
}

static void comm_attract_inner_6_animate_body(void)
{
    u32 board_type;

    /* @0x11454–0x1148C */
    g0 = i960_ld_u32(I960_WORKRAM, 0x20a7a4, 0);
    cgm_1111_flush((u32)g0, 0, 0);
    tile_map_banks_clear(0, 0, 0);
    board_type = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    i960_st_u8(I960_ABS, 0x181c000u, 0, 0xffu);
    if (board_type != 3u)
        i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b1cc0u);
    cgm_scratch_pool_reset(0, 0, 0);
    comm_attract_inner_6_animate_tail();
}

static void comm_attract_inner_6_animate(void)
{
    u32 frame;

    /* @0x11440 */
    frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    if (frame != 0) {
        comm_attract_countdown_tick(0, 0, 0);
        return;
    }

    comm_attract_inner_6_animate_body();
}

void comm_attract_inner_6(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x113A0 */
    gate = comm_attract_slot_scan(0, 0, 0);
    if (gate == 0)
        return;

    /* @0x113AC: cmpibe 0,20a788 → animate when gate cleared. */
    if (i960_ld_u32(I960_WORKRAM, 0x20a788, 0) == 0) {
        comm_attract_inner_6_animate();
        return;
    }

    /* @0x113B8–0x1143C — splash bind with script index 2. */
    comm_attract_catalog_seed(0, 0, 0);
    tile_attract_palram_gate(0xffffu);
    i960_st_u8(I960_ABS, 0x181c000u, 0, 0);
    tile_map_banks_clear(0, 0, 0);

    g0 = 0;
    g1 = 0;
    g2 = 0x2879db0u;
    g3 = 0;
    g4 = 1;
    g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
    i960_st_u32(I960_WORKRAM, 0x20a7a4, 0, (u32)g0);

    /* @0x114xx: cmpibe 0,202018 → skip; else palette 0x3F. */
    if (i960_ld_u8(I960_WORKRAM, 0x202018, 0) != 0u)
        comm_palette_index_call(0x3fu);

    /* @0x1140C: splash_bind(2); 0x20a7dc ← 0 (g14). */
    comm_attract_splash_bind(2);
    i960_st_u32(I960_WORKRAM, 0x20a7dc, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a774, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a808, 0, 0xffffff88u);
    {
        u32 nz = 0, i;
        for (i = 0; i < 0x4000u; i += 2u)
            if (i960_ld_u16(I960_ABS, 0x01004000u, i) != 0)
                nz++;
        fprintf(stderr, "lift: inner6 splash bind (script 2) L1_nz=%u\n", nz);
    }
}
