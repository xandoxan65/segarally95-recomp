/* Game-start display setup @ 0x14160 — mode-3 table slot 2 (staged 0x5B3160).
 * Layer enable, tile wipe, CGM catalogs, scene draw, palette, then 20209c += 1.
 * source: disasm/maincpu/maincpu_014160_400.asm */
// @rom 0x14160 +0x2e0 game_start_display_setup

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_geo.h"

#include "lift_syms.h"
#include "game_start_icon_batch.h"

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
    u32 a = 0x01008000u; /* clrbit 11 of 0x01008800 */
    u32 b = 0x01008800u;

    for (i = 0; i <= 0x17fu; i++) {
        i960_st_u16(I960_ABS, a, 0, 0);
        i960_st_u16(I960_ABS, b, 0, 0);
        a += 2u;
        b += 2u;
    }
}

void game_start_display_setup(u32 arg0, u32 arg1, u32 arg2)
{
    u32 slot;
    u32 draw_g3;
    u32 sub;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* Drop attract PRG mesh so mode-select black-punch cannot reveal it. */
    model2_geo_clear();

    /* @0x14160: stl g4/g5 @ 0x20a880 — 0 and bit30. */
    i960_st_u32(I960_WORKRAM, 0x20a880, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a884, 0, 1u << 30);
    i960_st_u32(I960_WORKRAM, 0x20a888, 0, 0x41e00000u);

    layer_setbit15_enable();

    /* @0x14204 setbit 15,0,g0 → palram color 0x8000. */
    g0 = 0x8000u;
    tile_attract_palram_gate((u32)g0);
    /* @0x14230: call 0x269d0 — banks 0/2 only (0x269f0 has zero ROM callers). */
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
    g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

    if (i960_ld_u8(I960_WORKRAM, 0x202019, 0) == 0) {
        g0 = 0;
        g1 = 40; /* addo 31,9 */
        g2 = 0x020b65f4u;
        g3 = 0;
        g4 = 0;
        g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

        g0 = 4;
        g1 = 30;
        g2 = 0x0210cf40u;
        g3 = 0;
        g4 = 0;
        g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
        slot = (u32)g0;
        g0 = 33; /* addo 31,2 */
        g1 = 30;
        g3 = 2;
        draw_g3 = 2;
    } else {
        g0 = 0;
        g1 = 40;
        g2 = 0x020b65f4u;
        g3 = 1;
        g4 = 0;
        g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

        g0 = 4;
        g1 = 30;
        g2 = 0x0210cf40u;
        g3 = 1;
        g4 = 0;
        g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
        slot = (u32)g0;
        g0 = 33;
        g1 = 30;
        g3 = 3;
        draw_g3 = 3;
    }

    /* Host latch — ROM 0x20a8c0 is the course-select pedal flag, not a batch. */
    game_start_icon_batch_set(slot);

    g2 = slot;
    g3 = draw_g3;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    /* ROM stos g14 — guest link is 0 after leaf ``mov 0,g14``. */
    i960_st_u32(I960_WORKRAM, 0x20a870, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a874, 0, 0);

    g0 = scene_lookup_fn(2, 0);
    i960_st_u32(I960_WORKRAM, 0x202230, 0, (u32)g0);

    geo_view_params(0, 0, 0);

    layer_clearbit15_disable();

    g0 = 0x9du;
    comm_palette_index_call((u32)g0);
    i960_st_u32(I960_WORKRAM, 0x20b190, 0, 0x9du);

    sub = i960_ld_u32(I960_WORKRAM, 0x20209c, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, sub);
}
