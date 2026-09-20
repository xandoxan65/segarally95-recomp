/* Semantic C from MAME disasm @ 0x32150 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_032150_244.asm */
// @rom 0x32150 +0x244 geo_attract_copro_object

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

/*
 * Attract copro → prg_fifo object submit (callee of geo_attract_copro_submit).
 *
 * g0 = record word0 @ 0x20cd90[obj*9]
 * g1 = object index
 * Returns 1 if object consumed, 0 if still pending.
 *
 * Path (word0>>2)&15 == 0 pushes catalog quad @ 0x28655c0 to prg_fifo;
 * nonzero path pushes 0x28655d0 + marker 0x48c35000 and updates float fields.
 */

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
}

static void prg_stq_rom(u32 vaddr)
{
    i960_mmio_write_u32(GEO_PRG_FIFO, i960_ld_u32(I960_ROM, vaddr, 0));
    i960_mmio_write_u32(GEO_PRG_FIFO, i960_ld_u32(I960_ROM, vaddr, 4));
    i960_mmio_write_u32(GEO_PRG_FIFO, i960_ld_u32(I960_ROM, vaddr, 8));
    i960_mmio_write_u32(GEO_PRG_FIFO, i960_ld_u32(I960_ROM, vaddr, 12));
}

static u32 obj_scaled9(u32 obj)
{
    return obj + (obj << 3);
}

u32 geo_attract_copro_object(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode;
    u32 obj;
    u32 ix;
    u32 slot_va;
    u32 rec_va;
    u32 count_w;
    u32 f_bits;
    u32 f_sum;
    double f;
    double adj;
    u32 w0, w1, w2, w3;
    u32 bank_ctr;
    u32 packed;
    u32 hi;
    u32 lo;
    u32 r4, r5, r6, r7, r8;
    u32 g2w, g3w, g4w, g5w, g6w, g7w, g9w;

    (void)arg2;
    obj = arg1;
    mode = (arg0 >> 2) & 15u;

    if (mode == 0) {
        /* @0x32168–0x32194: bank clear + catalog stq @ 0x28655c0. */
        geo_bank_poke0();
        w0 = i960_ld_u32(I960_ROM, 0x28655c0u, 0);
        w1 = i960_ld_u32(I960_ROM, 0x28655c0u, 4);
        w2 = i960_ld_u32(I960_ROM, 0x28655c0u, 8);
        w3 = i960_ld_u32(I960_ROM, 0x28655c0u, 12);
        i960_mmio_write_u32(GEO_PRG_FIFO, w0);
        i960_mmio_write_u32(GEO_PRG_FIFO, w1);
        i960_mmio_write_u32(GEO_PRG_FIFO, w2);
        i960_mmio_write_u32(GEO_PRG_FIFO, w3);

        ix = obj_scaled9(obj);
        slot_va = 0x20cda4u + (ix << 2);
        f_bits = i960_ld_u32(I960_WORKRAM, slot_va, 0);
        f = i960_u32_to_f64(f_bits);

        bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + w3);

        /* @0x321C4: cmprl fp1,+0 → pick ±epsilon adjust. */
        if (f > 0.0) {
            adj = i960_rifl_read(0x47ae147bu, 0x3f947ae1u);
            f = f - adj;
        } else {
            adj = i960_rifl_read(0xd2f1a9fcu, 0x3f60624du);
            f = f - adj;
        }
        f_bits = (u32)i960_f64_to_u32(f);

        ix = obj_scaled9(obj);
        rec_va = 0x20cd90u + (ix << 2);
        g4w = i960_ld_u32(I960_WORKRAM, rec_va, 0x1c);
        f_sum = (u32)i960_f64_to_u32(i960_u32_to_f64(f_bits) + i960_u32_to_f64(g4w));
        i960_st_u32(I960_WORKRAM, slot_va, 0, f_bits);
        i960_st_u32(I960_WORKRAM, rec_va, 0x1c, f_sum);

        /* @0x32230: bl if sum < 0 → return 1. */
        if (i960_u32_to_f64(f_sum) < 0.0)
            return 1u;

        /* @0x32234–0x32264: decrement 10-bit counter in rec word0. */
        packed = i960_ld_u32(I960_WORKRAM, rec_va, 0);
        hi = ((packed >> 10) - 1u) & 0xffu;
        lo = packed & 0xfffc03ffu;
        packed = lo | (hi << 10);
        i960_st_u32(I960_WORKRAM, rec_va, 0, packed);
        if (hi == 0)
            return 1u;
        return 0u;
    }

    /* @0x32268: nonzero mode — alternate catalog + float field walk. */
    i960_mmio_write_u32(0x800160, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x40800000u);
    geo_bank_poke0();
    prg_stq_rom(0x28655d0u);
    i960_mmio_write_u32(0x800160, 0u);

    ix = obj_scaled9(obj);
    rec_va = 0x20cd90u + (ix << 2);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x48c35000u);

    r8 = rec_va + 20u;
    g9w = i960_ld_u32(I960_WORKRAM, r8, 0);
    f = i960_u32_to_f64(g9w);
    adj = i960_rifl_read(0x47ae147bu, 0x3f847ae1u);
    f = f - adj;
    r5 = (u32)i960_f64_to_u32(f);

    bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    r6 = rec_va + 4u;
    r7 = rec_va + 0x1cu;
    g2w = i960_ld_u32(I960_WORKRAM, r6, 0);
    g7w = i960_ld_u32(I960_WORKRAM, r7, 0);
    g6w = i960_ld_u32(I960_WORKRAM, rec_va, 0x10);
    r4 = rec_va + 12u;
    g5w = i960_ld_u32(I960_WORKRAM, r4, 0);
    g7w = (u32)i960_f64_to_u32(i960_u32_to_f64(r5) + i960_u32_to_f64(g7w));
    g3w = i960_ld_u32(I960_WORKRAM, rec_va, 0x18);
    g4w = i960_ld_u32(I960_WORKRAM, rec_va, 8);
    g2w = (u32)i960_f64_to_u32(i960_u32_to_f64(g2w) + i960_u32_to_f64(g6w));
    g6w = i960_ld_u32(I960_WORKRAM, rec_va, 0x20);
    g5w = (u32)i960_f64_to_u32(i960_u32_to_f64(g5w) + i960_u32_to_f64(g3w));
    g4w = (u32)i960_f64_to_u32(i960_u32_to_f64(g4w) + i960_u32_to_f64(g6w));

    /* @0x32348: addo r15 — r15 was 4th word of 0x28655d0 via movt/movq path (=1). */
    count_w = i960_ld_u32(I960_ROM, 0x28655d0u, 12);
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + count_w);

    i960_st_u32(I960_WORKRAM, r6, 0, g2w);
    i960_st_u32(I960_WORKRAM, r4, 0, g5w);
    i960_st_u32(I960_WORKRAM, rec_va, 8, g4w);
    i960_st_u32(I960_WORKRAM, r8, 0, r5);
    i960_st_u32(I960_WORKRAM, r7, 0, g7w);

    /* @0x32370: bge if g7 >= 0 → return 0; else return 1. */
    if (i960_u32_to_f64(g7w) >= 0.0)
        return 0u;
    return 1u;
}
