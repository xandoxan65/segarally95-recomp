/* Tile script gate @ 0x29100 — bit5 of 0x202008 selects run vs format.
 * source: disasm/maincpu/maincpu_029100_18.asm */
// @rom 0x29100 +0x18 boot_tile_script_gate

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

void boot_tile_script_gate(u32 arg0, u32 arg1, u32 arg2)
{
    u32 flags;

    g0 = arg0;
    g1 = arg1;
    g2 = arg2;

    flags = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
    if ((flags >> 5) & 1u)
        boot_tile_script_run((u32)g0);
    else
        boot_tile_script_format((u32)g0, (u32)g1, (u32)g2);
}
