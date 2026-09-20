/* Semantic C from MAME disasm @ 0x3ee98 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03ee98_68.asm */
// @rom 0x3ee98 +0x68 geo_prg_object_opcode_prep

#include "i960_lift.h"
#include "i960_mem.h"

/*
 * bal target from geo_attract_pen_prg_push_a.
 * Clears halfword slots @ 0x214380 and returns object opcode 0x800100+count
 * (cmd bits → object_data in the host DL runner).
 */
u32 geo_prg_object_opcode_prep(u32 half_arg)
{
    u32 n = (half_arg >> 1) - 1u;
    u32 cursor = i960_ld_u32(I960_WORKRAM, 0x214374, 0);
    u32 opcode = 0x800100u + cursor;

    /* Loop while n != -1: clear two halfword slots per iter, cursor += 2. */
    while (n != 0xffffffffu) {
        u32 a = cursor;
        u32 b = cursor + 1u;

        n--;
        i960_st_u16(I960_WORKRAM, 0x214380u + (a << 1), 0, 0);
        i960_st_u16(I960_WORKRAM, 0x214380u + (b << 1), 0, 0);
        cursor = b + 1u;
    }

    i960_st_u32(I960_WORKRAM, 0x214374, 0, cursor);
    return opcode;
}
