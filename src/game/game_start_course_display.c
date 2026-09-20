/* Championship/practice → course-select display @ 0x15FD0 — scene table [0].
 * Same shape as game_start_display_setup but uses 0x20a930 scratch, draws BG +
 * title only, then advances 2020ac (not 20209c).
 * source: disasm/maincpu/maincpu_016000_400.asm + bytes @ 0x15fd0 */
// @rom 0x15fd0 +0x310 game_start_course_display

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_geo.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

static void layer_setbit15_enable(void)
{
    u16 v;

    i960_st_u8(I960_ABS, 0x0181c000u, 0, (u8)g14);

    v = (u16)i960_ld_u16(I960_ABS, 0x0100a008u, 0);
    i960_st_u16(I960_ABS, 0x0100a008u, 0, (u16)(v | 0x8000u));

    v = (u16)i960_ld_u16(I960_WORKRAM, 0x20b91c, 0);
    i960_st_u16(I960_WORKRAM, 0x20b91c, 0, (u16)(v | 0x8000u));
    v = (u16)i960_ld_u16(I960_ABS, 0x0100a00au, 0);
    i960_st_u16(I960_ABS, 0x0100a00au, 0, (u16)(v | 0x8000u));

    v = (u16)i960_ld_u16(I960_WORKRAM, 0x20b91e, 0);
    i960_st_u16(I960_WORKRAM, 0x20b91e, 0, (u16)(v | 0x8000u));
    v = (u16)i960_ld_u16(I960_ABS, 0x0100a00cu, 0);
    i960_st_u16(I960_ABS, 0x0100a00cu, 0, (u16)(v | 0x8000u));

    v = (u16)i960_ld_u16(I960_WORKRAM, 0x20b920, 0);
    i960_st_u16(I960_WORKRAM, 0x20b920, 0, (u16)(v | 0x8000u));
    v = (u16)i960_ld_u16(I960_WORKRAM, 0x20b922, 0);
    i960_st_u16(I960_WORKRAM, 0x20b922, 0, (u16)(v | 0x8000u));
    v = (u16)i960_ld_u16(I960_ABS, 0x0100a00eu, 0);
    i960_st_u16(I960_ABS, 0x0100a00eu, 0, (u16)(v | 0x8000u));
}

static void layer_clearbit15_disable(void)
{
    u16 v;
    const u16 mask = 0x7fffu;

    i960_st_u8(I960_ABS, 0x0181c000u, 0, 0xffu);

    v = (u16)i960_ld_u16(I960_ABS, 0x0100a008u, 0);
    i960_st_u16(I960_ABS, 0x0100a008u, 0, (u16)(v & mask));

    v = (u16)i960_ld_u16(I960_WORKRAM, 0x20b91c, 0);
    i960_st_u16(I960_WORKRAM, 0x20b91c, 0, (u16)(v & mask));
    v = (u16)i960_ld_u16(I960_ABS, 0x0100a00au, 0);
    i960_st_u16(I960_ABS, 0x0100a00au, 0, (u16)(v & mask));

    v = (u16)i960_ld_u16(I960_WORKRAM, 0x20b91e, 0);
    i960_st_u16(I960_WORKRAM, 0x20b91e, 0, (u16)(v & mask));
    v = (u16)i960_ld_u16(I960_ABS, 0x0100a00cu, 0);
    i960_st_u16(I960_ABS, 0x0100a00cu, 0, (u16)(v & mask));

    v = (u16)i960_ld_u16(I960_WORKRAM, 0x20b920, 0);
    i960_st_u16(I960_WORKRAM, 0x20b920, 0, (u16)(v & mask));
    v = (u16)i960_ld_u16(I960_WORKRAM, 0x20b922, 0);
    i960_st_u16(I960_WORKRAM, 0x20b922, 0, (u16)(v & mask));
    v = (u16)i960_ld_u16(I960_ABS, 0x0100a00eu, 0);
    i960_st_u16(I960_ABS, 0x0100a00eu, 0, (u16)(v & mask));
}

static void tile_halfword_wipe(void)
{
    u32 i;
    u32 a = 0x01008000u;
    u32 b = 0x01008800u;

    for (i = 0; i <= 0x17fu; i++) {
        i960_st_u16(I960_ABS, a, 0, 0);
        i960_st_u16(I960_ABS, b, 0, 0);
        a += 2u;
        b += 2u;
    }
}

static void copy_matrix_block(u32 dst, u32 src, u32 words)
{
    u32 i;

    for (i = 0; i < words; i++) {
        u32 w = i960_ld_u32(I960_WORKRAM, src, i << 2);

        if (w == 0)
            w = model2_workram_mirror_u32(src + (i << 2));
        i960_st_u32(I960_WORKRAM, dst, i << 2, w);
    }
}

void game_start_course_display(u32 arg0, u32 arg1, u32 arg2)
{
    u32 frame;
    u32 lookup;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    model2_geo_clear();

    /* @0x15FDC–0x15FF8: view scratch @ 0x20a930 (display_setup uses 0x20a880). */
    i960_st_u32(I960_WORKRAM, 0x20a930, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a934, 0, 1u << 30);
    i960_st_u32(I960_WORKRAM, 0x20a938, 0, 0x41e00000u);

    layer_setbit15_enable();

    g0 = 0x8000u;
    tile_attract_palram_gate((u32)g0);
    /* @0x160AC: call 0x269d0 only (banks 0/2). 0x269f0 has zero ROM callers. */
    tile_map_banks_clear(0, 0, 0);
    scene_list_seed(0, 0, 0);
    tile_halfword_wipe();

    g0 = 0x4b0u;
    game_start_catalog_seed((u32)g0, 0, 0);

    g0 = 0;
    g1 = 0;
    g2 = 0x02150eecu;
    g3 = 0;
    g4 = 1;
    catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

    if (i960_ld_u8(I960_WORKRAM, 0x202019, 0) == 0) {
        g0 = 0;
        g1 = 40;
        g2 = 0x020b65f4u;
        g3 = 0;
        g4 = 0;
        catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
    } else {
        g0 = 0;
        g1 = 40;
        g2 = 0x020b65f4u;
        g3 = 1;
        g4 = 0;
        catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
    }

    /* Pedal latch + spare — ROM stos g14. */
    i960_st_u32(I960_WORKRAM, 0x20a8c0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a8c8, 0, 0);

    g0 = scene_lookup_fn(4, 0);
    lookup = (u32)g0;
    i960_st_u32(I960_WORKRAM, 0x20a8c4, 0, lookup);

    /* Matrix block @ 0x5B42E0 → 0x20A8D0 / 0x20A900. */
    copy_matrix_block(0x20a900u, 0x5b4310u, 4);
    copy_matrix_block(0x20a910u, 0x5b4320u, 4);
    copy_matrix_block(0x20a920u, 0x5b4330u, 2);
    copy_matrix_block(0x20a8d0u, 0x5b42e0u, 4);
    copy_matrix_block(0x20a8e0u, 0x5b42f0u, 4);
    copy_matrix_block(0x20a8f0u, 0x5b4300u, 4);

    frame = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x2020ac, 0, frame);

    geo_view_params(0, 0, 0);
    /* @0x161EC bal 0x13B58 — unlifted leaf; skip (matrix already copied). */

    layer_clearbit15_disable();

    if (i960_ld_u32(I960_WORKRAM, 0x20b190, 0) != 0x9du) {
        g0 = 0x9du;
        comm_palette_index_call((u32)g0);
        i960_st_u32(I960_WORKRAM, 0x20b190, 0, 0x9du);
    }

    i960_st_u32(I960_WORKRAM, 0x20a940, 0, 0);
    fprintf(stderr, "lift: course_display → scene_frame %u\n", (unsigned)frame);
}
