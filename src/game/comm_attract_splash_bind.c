/* Splash texture-bank bind @ 0xF670 (table @ 0x5DCCF0 → 0x214350).
 * source: disasm/maincpu/maincpu_00f670_80.asm — bank select only. */
// @rom 0xf670 +0x50 comm_attract_splash_bind

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"

void comm_attract_splash_bind(u32 script_index)
{
    u32 bank;

    i960_st_u32(I960_WORKRAM, 0x214354, 0, script_index);
    bank = model2_workram_mirror_u32(0x005dccf0u + (script_index << 2));
    i960_st_u32(I960_WORKRAM, 0x21384c, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x213840, 0, 4u);
    g0 = bank;
    g1 = 1;
    i960_st_u32(I960_WORKRAM, 0x214350, 0, bank);
    texture_bank_select((u32)g0, (u32)g1, 0);
    i960_st_u32(I960_WORKRAM, 0x213840, 0, 6u);
}
