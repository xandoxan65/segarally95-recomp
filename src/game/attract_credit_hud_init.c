/* Attract CREDIT HUD init @ 0xF4D0 (inner jump table slot 11).
 * source: disasm/maincpu/maincpu_00f4d0_60.asm + maincpu_00f4e0_160.asm */
// @rom 0xf4d0 +0x110 attract_credit_hud_init

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void attract_credit_hud_init(u32 arg0, u32 arg1, u32 arg2)
{
    u32 phase;
    u16 w;
    u16 mask;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_st_u32(I960_WORKRAM, 0x213840, 0, 6);
    i960_st_u32(I960_WORKRAM, 0x213850, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20227c, 0, (u32)g14);
    g0 = 1u << 15;
    tile_attract_palram_gate((u32)g0);
    boot_tile_map_init(0, 0, 0);
    scene_list_seed(0, 0, 0);

    comm_scene_catalog_init(1, 0, 0);
    g0 = 1;
    i960_call_rom(0x1ac40);

    phase = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, phase + 1u);

    attract_credit_hud_frame(0, 0, 0);

    /* Layer word sanitize @ 0xF52C–0xF5D8 */
    mask = 0x7fff;
    i960_st_u8(I960_ABS, 0x0181c000u, 0, 0xff);

    w = i960_ld_u16(I960_ABS, 0x0100a008u, 0);
    i960_st_u16(I960_ABS, 0x0100a008u, 0, (u16)(w & mask));
    w = i960_ld_u16(I960_WORKRAM, 0x20b91c, 0);
    i960_st_u16(I960_WORKRAM, 0x20b91c, 0, (u16)(w & mask));

    w = i960_ld_u16(I960_ABS, 0x0100a00au, 0);
    i960_st_u16(I960_ABS, 0x0100a00au, 0, (u16)(w & mask));
    w = i960_ld_u16(I960_WORKRAM, 0x20b91e, 0);
    i960_st_u16(I960_WORKRAM, 0x20b91e, 0, (u16)(w & mask));

    w = i960_ld_u16(I960_ABS, 0x0100a00cu, 0);
    i960_st_u16(I960_ABS, 0x0100a00cu, 0, (u16)(w & mask));
    w = i960_ld_u16(I960_WORKRAM, 0x20b920, 0);
    i960_st_u16(I960_WORKRAM, 0x20b920, 0, (u16)(w & mask));

    w = i960_ld_u16(I960_ABS, 0x0100a00eu, 0);
    i960_st_u16(I960_ABS, 0x0100a00eu, 0, (u16)(w & mask));
    w = i960_ld_u16(I960_WORKRAM, 0x20b922, 0);
    i960_st_u16(I960_WORKRAM, 0x20b922, 0, (u16)(w & mask));
}
