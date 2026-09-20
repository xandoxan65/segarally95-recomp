/* Semantic C from MAME disasm @ 0x23a80 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_023a80_640.asm */
// @rom 0x23a80 +0x640 geo_attract_glyph_emit

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"

/*
 * arg0 = pen descriptor vaddr (0x5c2780[…]).
 * arg1 = host pointer to attract object (fp shadow).
 * arg2 = remaining geo budget (caller g2, typically 0x1869f).
 */

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
}

static void prg_stq_catalog(u32 catalog_ix, u32 count_override, int use_override)
{
    u32 base = CATALOG_VADDR + (catalog_ix << 4);
    u32 w0 = i960_ld_u32(I960_ROM, base, 0);
    u32 w1 = i960_ld_u32(I960_ROM, base, 4);
    u32 w2 = i960_ld_u32(I960_ROM, base, 8);
    u32 w3 = i960_ld_u32(I960_ROM, base, 12);

    if (use_override)
        w3 = count_override;

    geo_bank_poke0();
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
}

static void wr_slot_bind(void)
{
    u32 wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    u32 wr_ptr = i960_ld_u32(I960_MMIO, 0x802008, 0);

    i960_st_u32(I960_WORKRAM, wr_slot, 0, wr_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);
    i960_mmio_write_u32(0x801008, wr_ptr + 0x34u);
}

void geo_attract_glyph_emit(void * arg0, void * arg1, u32 arg2)
{
    u32 pen_va = (u32)(uintptr_t)arg0;
    u8 *obj = (u8 *)arg1;
    u32 *obj_w;
    u32 flag;
    u32 base_ix;
    u32 cat_ix;
    u32 budget;
    u32 remain;
    u32 bank_ctr;
    u32 take;
    u32 x0, y0, z0, x1;
    u32 sum;
    u32 scaled;
    u32 dx, dy;
    u32 pen_hdr;
    u32 iter;
    u32 list_ptr;
    uintptr_t obj_save;
    double f;
    double adj;

    if (!obj || pen_va == 0)
        return;

    obj_w = (u32 *)(void *)obj;
    budget = arg2;
    obj_save = (uintptr_t)arg1;

    /* @0x23A98–0x23AEC: pen base index. */
    flag = model2_workram_mirror_u16(pen_va + 0x52u) & 0x8888u;
    if (flag != 0)
        base_ix = model2_workram_mirror_u32(pen_va + 0x10u);
    else
        base_ix = model2_workram_mirror_u32(pen_va + 0xcu);
    if (model2_workram_mirror_u32(pen_va + 0x14u) != 0)
        base_ix += (u32)(obj[0x51] & 3u);

    pen_hdr = model2_workram_mirror_u32(pen_va + 0);

    /* @0x23B10–0x23B58: sum of four floats at obj+0x40..0x4c, * 0.25. */
    x0 = obj_w[0x40 / 4];
    y0 = obj_w[0x44 / 4];
    z0 = obj_w[0x48 / 4];
    x1 = obj_w[0x4c / 4];
    sum = (u32)i960_f64_to_u32(
        i960_u32_to_f64(x0) + i960_u32_to_f64(y0)
        + i960_u32_to_f64(z0) + i960_u32_to_f64(x1));
    f = i960_u32_to_f64(sum) * i960_rifl_read(0, 0x3fd00000u);
    adj = i960_rifl_read(0x47ae147bu, 0x3f947ae1u);
    scaled = (u32)i960_f64_to_u32(f - adj);

    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, obj_w[0]);
    i960_mmio_write_u32(0x884000, scaled);
    i960_mmio_write_u32(0x884000, obj_w[2]);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, obj_w[0x1c / 4]);

    /* @0x23BD0–0x23C28: (x0+y0)-(z0+x1) * const → 0x14802929 payload. */
    dx = (u32)i960_f64_to_u32(
        (i960_u32_to_f64(x0) + i960_u32_to_f64(y0))
        - (i960_u32_to_f64(z0) + i960_u32_to_f64(x1)));
    dx = (u32)i960_f64_to_u32(
        i960_u32_to_f64(dx) * i960_rifl_read(0xb4395810u, 0x3fce76c8u));
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, dx);

    dy = (u32)i960_f64_to_u32(
        (i960_u32_to_f64(x0) + i960_u32_to_f64(z0))
        - (i960_u32_to_f64(y0) + i960_u32_to_f64(x1)));
    dy = (u32)i960_f64_to_u32(
        i960_u32_to_f64(dy) * i960_rifl_read(0x3b645a1du, 0x3fd54fdfu));
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    i960_mmio_write_u32(0x884000, dy);

    wr_slot_bind();

    /* @0x23CCC–0x23CF8: first catalog push from pen+4.
     * ROM keeps r7 = catalog word3 and does NOT bump 0x20b940 here — that
     * addo lands between yaw and pitch of the body pose @ 0x23D8C. */
    cat_ix = model2_workram_mirror_u32(pen_va + 4u);
    {
        u32 first_w3 = i960_ld_u32(I960_ROM, CATALOG_VADDR + (cat_ix << 4), 12);

        prg_stq_catalog(cat_ix, 0, 0);

        i960_mmio_write_u32(0x884000, 0x10802121u);
        i960_mmio_write_u32(0x884000, 0x10002020u);
        i960_mmio_write_u32(0x884000, 0x13802727u);
        i960_mmio_write_u32(0x884000, obj_w[0]);
        i960_mmio_write_u32(0x884000, obj_w[1]);
        i960_mmio_write_u32(0x884000, obj_w[2]);
        i960_mmio_write_u32(0x884000, 0x15002a2au);
        i960_mmio_write_u32(0x884000, obj_w[0x1c / 4]);
        /* @0x23D70–0x23D98: bank += first catalog w3 (r7), then pitch. */
        bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        i960_mmio_write_u32(0x884000, 0x14802929u);
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + first_w3);
        i960_mmio_write_u32(0x884000, obj_w[0x18 / 4]);
        i960_mmio_write_u32(0x884000, 0x15802b2bu);
        i960_mmio_write_u32(0x884000, obj_w[0x20 / 4]);
        wr_slot_bind();
    }

    /* @0x23DF8–0x23E68: second catalog from base_ix, count clamped to remain. */
    bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    remain = budget - bank_ctr;
    if ((i32)remain < 1)
        remain = 1u;

    {
        u32 w0, w1, w2, w3;
        u32 base = CATALOG_VADDR + (base_ix << 4);

        w0 = i960_ld_u32(I960_ROM, base, 0);
        w1 = i960_ld_u32(I960_ROM, base, 4);
        w2 = i960_ld_u32(I960_ROM, base, 8);
        w3 = i960_ld_u32(I960_ROM, base, 12);
        take = w3;
        if (remain != 0u && w3 >= remain)
            take = remain;
        prg_stq_catalog(base_ix, take, 1);
        bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + take);
        (void)w0;
        (void)w1;
        (void)w2;
    }

    /* @0x23E80–0x23EAC: secondary setup → object opcode prg pushes. */
    if (flag != 0) {
        geo_attract_pen_prg_push_a((u32)(obj[0x50] & 15u), 0, 0);
    } else {
        /* ROM: g0=obj, g1=obj+0x18, g2=0x20220c, g3=pen_mode */
        g3 = (u32)(obj[0x50] & 15u);
        geo_attract_pen_prg_push_b(
            (uintptr_t)arg1,
            (uintptr_t)arg1 + 0x18u,
            0x20220cu);
    }

    /* @0x23EB0–0x23F10: optional third catalog from pen+0x14. */
    if ((obj[0x50] & 0x10u) != 0) {
        u32 extra = model2_workram_mirror_u32(pen_va + 0x14u);

        if (extra != 0) {
            u32 w3 = i960_ld_u32(I960_ROM, CATALOG_VADDR + (extra << 4), 12);

            prg_stq_catalog(extra, 0, 0);
            bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
            i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + w3);
        }
    }

    /* @0x23F18–0x24080: four wheel list entries (g13 0..3; cmpi/bg vs 3). */
    bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    remain = budget - bank_ctr;
    if ((i32)remain >= 1) {
        u32 r5bits = obj_w[0x2c / 4] ^ 0x80000000u;
        u8 *obj_cursor = obj;
        u32 hdr = pen_hdr;

        list_ptr = pen_hdr + 4u;
        for (iter = 0; iter < 4u; iter++) {
            u32 a = i960_ld_u32(I960_WORKRAM, list_ptr, 0);
            u32 b = ((u32 *)(void *)obj_cursor)[0x30 / 4];
            u32 c = i960_ld_u32(I960_WORKRAM, list_ptr, 4);
            u32 d = i960_ld_u32(I960_WORKRAM, hdr, 0);
            u32 entry;
            u32 w3;

            i960_mmio_write_u32(0x884000, 0x10002020u);
            i960_mmio_write_u32(0x884000, 0x13802727u);
            i960_mmio_write_u32(0x884000, d);
            i960_mmio_write_u32(
                0x884000,
                (u32)i960_f64_to_u32(i960_u32_to_f64(a) + i960_u32_to_f64(b)));
            i960_mmio_write_u32(0x884000, c);
            /* @0x23F70–0x23FA8: yaw only for g13<=1, always from obj+0x24
             * (fixed base at 0xb0(fp) — not the advancing scale cursor). */
            if (iter <= 1u) {
                i960_mmio_write_u32(0x884000, 0x15002a2au);
                i960_mmio_write_u32(0x884000, obj_w[0x24 / 4]);
            }
            i960_mmio_write_u32(0x884000, 0x14802929u);
            i960_mmio_write_u32(0x884000, r5bits);

            wr_slot_bind();
            entry = model2_workram_mirror_u32(pen_va + 8u);
            w3 = i960_ld_u32(I960_ROM, CATALOG_VADDR + (entry << 4), 12);
            prg_stq_catalog(entry, 0, 0);
            i960_mmio_write_u32(0x884000, 0x10802121u);
            bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
            i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + w3);

            list_ptr += 12u;
            hdr += 12u;
            obj_cursor += 4;

            bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
            remain = budget - bank_ctr;
            if ((i32)remain < 1)
                break;
        }
    }

    i960_mmio_write_u32(0x884000, 0x10802121u);
    bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    remain = budget - bank_ctr;
    /* cmpibge 5,g4 @ 0x240A0: skip call when 5 >= remain; call when remain > 5. */
    if ((i32)remain > 5) {
        geo_attract_object_extra((void *)(uintptr_t)obj_save, 0, 0);
    }
}
