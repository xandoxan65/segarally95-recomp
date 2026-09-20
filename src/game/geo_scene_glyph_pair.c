/* Glyph pair @ 0x5CC0 — two opcode dispatches then script gate.
 * source: disasm/maincpu/maincpu_005cc0_38.asm */
// @rom 0x5cc0 +0x38 geo_scene_glyph_pair

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

u32 geo_scene_glyph_pair(u32 arg0, u32 arg1, u32 arg2)
{
    u32 saved = arg0;
    u8 opcode;

    (void)arg2;

    if (arg1 != 0)
        opcode = (u8)i960_ld_u8(I960_WORKRAM, 0x5a4750, 0);
    else
        opcode = (u8)i960_ld_u8(I960_WORKRAM, 0x5a4751, 0);

    boot_tile_opcode_dispatch((u32)opcode);
    boot_tile_opcode_dispatch(31u + 1u);
    g0 = saved;
    boot_tile_script_gate((u32)g0, (u32)g1, (u32)g2);
    return saved;
}
