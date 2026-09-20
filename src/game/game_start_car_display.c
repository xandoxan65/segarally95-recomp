/* Mode-3/5 table slot [2] @ 0x14D60 — car-select display setup.
 * Clears stale course-select mesh, draws car-select tile catalogs, advances
 * 0x2020ac to the per-frame car-select loop @ 0x15200.
 * source: disasm/maincpu/maincpu_014d60_2c8.asm */
// @rom 0x14d60 +0x2c8 game_start_car_display

#include "i960_lift.h"
#include "lift_log.h"
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

void game_start_car_display(u32 arg0, u32 arg1, u32 arg2)
{
    u32 frame;
    u32 slot;
    u32 lookup;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* Drop course-select PRG mesh so black-punch cannot reveal it. */
    model2_geo_clear();

    /* @0x14D60–0x14D88: car geo scale scratch @ 0x20a8a0/a4/a8. */
    i960_st_u32(I960_WORKRAM, 0x20a8a8, 0, 0x41f00000u);
    i960_st_u32(I960_WORKRAM, 0x20a8a0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a8a4, 0, 0x3fb33333u);

    g0 = 5;
    g1 = 0;
    g0 = scene_lookup_fn((u32)g0, (u32)g1);
    lookup = (u32)g0;
    i960_st_u32(I960_WORKRAM, 0x20a8bc, 0, lookup);

    layer_setbit15_enable();

    /* @0x14E1C: setbit 15,0 → g0 = 0x8000 for palram gate. */
    g0 = 0x8000u;
    tile_attract_palram_gate((u32)g0);
    tile_map_banks_clear(0, 0, 0);
    scene_list_seed(0, 0, 0);
    tile_halfword_wipe();

    /*
     * Car-select BG CGM (g4=1 → map @ 0x01004000) is MAME tile_layer[2]
     * after draw_common's layer>>=1 — scroll from 0x20b918 / 0x20b920
     * (tile_ram[0x5002]/[0x5006]), not pair1's 0x20b916/91e.
     *
     * 0x20b920 is also ctrl for pairs 2–3 (MAME ctrl =
     * tile_ram[0x5004+((layer>>1)&2)]). Ranking only pokes 0x20b91c
     * (pairs 0–1); leftover low bits / bit14 on 0x20b920 still scroll or
     * special-window the BG. mode_select zeros 0x20b914/918 on exit but
     * never clears 0x20b920. Keep bit15 set (layer disabled during CGM
     * upload); clearbit15 at function end yields scroll 0.
     *
     * Also drop ranking bit14 on 0x20b91c so pairs 0–1 (name strips) use
     * the standard win path — same as viewer strip on game-start.
     */
    {
        u16 ctrl;

        i960_st_u16(I960_WORKRAM, 0x20b914, 0, 0);
        i960_st_u16(I960_WORKRAM, 0x20b916, 0, 0);
        i960_st_u16(I960_WORKRAM, 0x20b918, 0, 0);
        i960_st_u16(I960_WORKRAM, 0x20b91a, 0, 0);
        /* pair2 vscr/ctrl: disabled + zero scroll (not 0x20b91e). */
        i960_st_u16(I960_WORKRAM, 0x20b920, 0, 0x8000u);
        i960_st_u16(I960_WORKRAM, 0x20b922, 0, 0x8000u);
        ctrl = (u16)i960_ld_u16(I960_WORKRAM, 0x20b91c, 0);
        ctrl = (u16)((ctrl & (u16)~0x4000u) | 0x8000u);
        i960_st_u16(I960_WORKRAM, 0x20b91c, 0, ctrl);
        i960_st_u16(I960_WORKRAM, 0x20b91e, 0, 0x8000u);
        i960_st_u16(I960_ABS, 0x0100a000u, 0, 0);
        i960_st_u16(I960_ABS, 0x0100a002u, 0, 0);
        i960_st_u16(I960_ABS, 0x0100a004u, 0, 0);
        i960_st_u16(I960_ABS, 0x0100a006u, 0, 0);
        i960_st_u16(I960_ABS, 0x0100a008u, 0, 0x8000u);
        i960_st_u16(I960_ABS, 0x0100a00au, 0, 0x8000u);
        i960_st_u16(I960_ABS, 0x0100a00cu, 0, 0x8000u);
        i960_st_u16(I960_ABS, 0x0100a00eu, 0, 0x8000u);
    }

    g0 = 0x4b0u;
    game_start_catalog_seed((u32)g0, 0, 0);

    g0 = 0;
    g1 = 0;
    g2 = 0x02150eecu;
    g3 = 0;
    g4 = 1;
    catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);

    /* @0x14EA4: car-name strip catalog @ 0x2090c70. */
    g0 = 7;
    g1 = 11;
    g2 = 0x02090c70u;
    g3 = 0;
    g4 = 0;
    g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
    slot = (u32)g0;

    g0 = 24;
    g1 = 11;
    g2 = slot;
    g3 = 1;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    g0 = 41; /* addo 31,10 */
    g1 = 11;
    g2 = slot;
    g3 = 2;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

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

    /* @0x14F34 bal 0x13B58 — unlifted matrix leaf; g0 = 0x5b3d50. */
    g0 = 0x005b3d50u;
    i960_call_rom(0x13b58);

    i960_st_u32(I960_WORKRAM, 0x2020b4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a894, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a890, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a8b0, 0, 0);

    frame = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x2020ac, 0, frame);

    layer_clearbit15_disable();

    lift_log( "lift: car_display → scene_frame %u lookup=%u\n",
            (unsigned)frame, (unsigned)lookup);
}
