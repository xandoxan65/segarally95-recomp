/* Semantic C from MAME disasm @ 0x32410 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_032410_274.asm */
// @rom 0x32410 +0x274 geo_attract_copro_marker_burst

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"

/*
 * Fixed copro marker bursts + two prg_fifo quads from workram transform slots
 * @ 0x2172a0 / 0x217320 / 0x2173a0. Called from catalog_span when gate allows.
 */

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
}

static void prg_stq_transform_slot(void)
{
    u32 w0 = i960_ld_u32(I960_WORKRAM, 0x2172a0, 0);
    u32 w1 = i960_ld_u32(I960_WORKRAM, 0x217320, 0);
    u32 w2 = i960_ld_u32(I960_WORKRAM, 0x2173a0, 0);

    geo_bank_poke0();
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0u);
}

static u32 wr_slot_bind(void)
{
    u32 wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    u32 wr_ptr = i960_ld_u32(I960_MMIO, 0x802008, 0);

    i960_st_u32(I960_WORKRAM, wr_slot, 0, wr_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_mmio_write_u32(0x801008, wr_ptr + 0x34u);
    return wr_slot;
}

void geo_attract_copro_marker_burst(u32 arg0, u32 arg1, u32 arg2)
{
    u32 one = (u32)i960_f64_to_u32(1.0);
    u32 wr_slot;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x32414–0x324BC: first marker stream. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, 0x43a34937u);
    i960_mmio_write_u32(0x884000, 0xc23a3021u);
    i960_mmio_write_u32(0x884000, 0x43bd05e3u);
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    i960_mmio_write_u32(0x884000, 0x3dee4baeu);
    i960_mmio_write_u32(0x884000, 0x14002828u);
    i960_mmio_write_u32(0x884000, 0x42200000u);
    i960_mmio_write_u32(0x884000, one);
    i960_mmio_write_u32(0x884000, 0x42200000u);

    wr_slot = wr_slot_bind();
    prg_stq_transform_slot();

    /* @0x32540–0x32588: second markers; slot+4 lands mid-stream @ 0x32590. */
    i960_mmio_write_u32(0x884000, 0x10802121u);
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, 0x42931062u);
    i960_mmio_write_u32(0x884000, 0xc1458937u);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);
    i960_mmio_write_u32(0x884000, 0xc1c41687u);
    i960_mmio_write_u32(0x884000, 0x14002828u);
    i960_mmio_write_u32(0x884000, 0x42200000u);
    i960_mmio_write_u32(0x884000, one);
    i960_mmio_write_u32(0x884000, 0x42200000u);

    wr_slot = wr_slot_bind();
    prg_stq_transform_slot();
    i960_mmio_write_u32(0x884000, 0x10802121u);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);
}
