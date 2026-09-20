/* Semantic C from MAME disasm @ 0x3ef10 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03ef10_58.asm */
// @rom 0x3ef10 +0x58 geo_attract_pen_prg_push_b

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_memory.h"

/*
 * Angle-aware pen push: table @ 0x5ddb70[pen_mode*16] + object deltas → prg stq.
 * ROM ABI (glyph @ 0x23E98):
 *   g0 = object*, g1 = object+0x18*, g2 = 0x20220c, g3 = pen_mode.
 * Host path passes full-width object pointers (do not narrow to u32).
 * Remap for angle_push: g0/g1 = table words, g2/g3 = object/object+0x18,
 * g4/g5 = wr_base/pen_mode. angle_push accepts host or guest object*.
 */
void geo_attract_pen_prg_push_b(uintptr_t obj, uintptr_t obj18, u32 wr_base)
{
    u32 pen_mode = (u32)g3;
    u32 rec = 0x5ddb70u + (pen_mode << 4);
    u32 table0 = i960_ld_u32(I960_WORKRAM, rec, 0);
    u32 table1 = i960_ld_u32(I960_WORKRAM, rec, 4);
    u32 opcode;
    u32 w2, w3;
    u32 bank;

    g0 = table0;
    g1 = table1;
    g2 = obj;
    g3 = obj18;
    g4 = wr_base;
    g5 = pen_mode;
    opcode = geo_attract_pen_angle_push(0, 0, 0);

    bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;
    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);

    w2 = i960_ld_u32(I960_WORKRAM, rec, 8);
    w3 = i960_ld_u32(I960_WORKRAM, rec, 12);
    i960_mmio_write_u32(GEO_PRG_FIFO, opcode);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0u);
}
