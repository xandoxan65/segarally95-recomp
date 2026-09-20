/* Comm-attract inner mode 4 @ 0x10E30 (post-carousel splash / re-seed). */
/* source: decomp/disasm/maincpu/maincpu_010e30_210.asm */
// @rom 0x10e30 +0x200 comm_attract_inner_4

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include <stdio.h>

static u32 comm_attract_inner_4_texture_pick(u32 script_idx)
{
    u32 course_variant;

    course_variant = i960_ld_u32(I960_WORKRAM, COURSE_VARIANT_INDEX, 0);
    /* Jump table @ 0x5AFF5C — script_idx-1 selects texture base. */
    if (script_idx == 0u)
        return model2_workram_mirror_u32(0x005ae430u + (course_variant << 2));
    if (script_idx <= 2u)
        return model2_workram_mirror_u32(0x005ae440u + (course_variant << 2));
    return model2_workram_mirror_u32(0x005ae450u + (course_variant << 2));
}

static void comm_attract_inner_4_animate_tail(void)
{
    u32 script_idx;
    u32 tex_desc;
    u32 link_word;
    u32 inner;

    /* @0x10F24–0x11028 — mirrors inner_2 animate_tail, then inner++. */
    script_idx = i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7f4, 0, 0x3e7u);
    i960_st_u32(I960_WORKRAM, 0x20a7f8, 0, 0x3e7u);
    tex_desc = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);

    /*
     * @0x10F48–0x10F4C: subo 1,g4 / cmpobl 6,g4 — unsigned; script_idx==0
     * wraps to 0xffffffff and keeps the prior tex_desc (skip jump table).
     */
    script_idx -= 1u;
    if (script_idx <= 6u)
        tex_desc = comm_attract_inner_4_texture_pick(script_idx);

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
    fprintf(stderr, "lift: inner4 reseed → inner %u tex=%#x\n",
            (unsigned)(inner + 1u), tex_desc);
}

static void comm_attract_inner_4_animate_body(void)
{
    u32 board_type;

    /* @0x10EE8–0x10F20 */
    g0 = i960_ld_u32(I960_WORKRAM, 0x20a7a4, 0);
    cgm_1111_flush((u32)g0, 0, 0);
    tile_map_banks_clear(0, 0, 0);
    board_type = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    i960_st_u8(I960_ABS, 0x181c000u, 0, 0xffu);
    if (board_type != 3u)
        i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b1cc0u);
    cgm_scratch_pool_reset(0, 0, 0);
    comm_attract_inner_4_animate_tail();
}

static void comm_attract_inner_4_animate(void)
{
    u32 frame;

    /* @0x10ED4 */
    frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    if (frame != 0) {
        comm_attract_countdown_tick(0, 0, 0);
        return;
    }

    comm_attract_inner_4_animate_body();
}

void comm_attract_inner_4(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x10E30 */
    gate = comm_attract_slot_scan(0, 0, 0);
    if (gate == 0)
        return;

    /* @0x10E3C: cmpibe 0,20a788 → animate when gate cleared. */
    if (i960_ld_u32(I960_WORKRAM, 0x20a788, 0) == 0) {
        comm_attract_inner_4_animate();
        return;
    }

    /* @0x10E48–0x10ED0 — one-shot splash bind after carousel ends. */
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

    /* @0x10E90: cmpibe 0,202018 → skip; else palette 0x3F. */
    if (i960_ld_u8(I960_WORKRAM, 0x202018, 0) != 0u)
        comm_palette_index_call(0x3fu); /* addo 31,28 → 0x3F */

    comm_attract_splash_bind(1);
    i960_st_u32(I960_WORKRAM, 0x20a7dc, 0, 2u);
    i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a774, 0, 0);
    /* lda 0xffffff88 → frame = -120 */
    i960_st_u32(I960_WORKRAM, 0x20a808, 0, 0xffffff88u);
    {
        u32 nz = 0, i;
        for (i = 0; i < 0x4000u; i += 2u)
            if (i960_ld_u16(I960_ABS, 0x01004000u, i) != 0)
                nz++;
        fprintf(stderr, "lift: inner4 splash bind (post-carousel) L1_nz=%u\n", nz);
    }
}
