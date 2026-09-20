/* Semantic C from MAME disasm @ 0x3ef70 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03ef70_4c.asm */
// @rom 0x3ef70 +0x4c geo_attract_pen_prg_push_a

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_memory.h"

/*
 * Table @ 0x5ddb70[pen_mode*16] → object opcode prep + stq to prg_fifo.
 * g0 = pen mode (obj[0x50] & 15).
 */
void geo_attract_pen_prg_push_a(u32 pen_mode, u32 arg1, u32 arg2)
{
    u32 rec = 0x5ddb70u + (pen_mode << 4);
    u32 half_arg;
    u32 opcode;
    u32 w2, w3;
    u32 bank;

    (void)arg1;
    (void)arg2;

    half_arg = i960_ld_u32(I960_WORKRAM, rec, 4);
    opcode = geo_prg_object_opcode_prep(half_arg);

    bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;
    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);

    w2 = i960_ld_u32(I960_WORKRAM, rec, 8);
    w3 = i960_ld_u32(I960_WORKRAM, rec, 12);
    i960_mmio_write_u32(GEO_PRG_FIFO, opcode);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0u);
}
