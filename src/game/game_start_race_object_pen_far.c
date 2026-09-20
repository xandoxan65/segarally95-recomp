/* Far object-pen emit @ 0x24560 — pen_desc mode ≠ 1.
 *
 * prg_budget promotes depth-list modes to 2 when view-depth > 0x213970
 * (~25). Unlifted call_rom no-ops, so world-yaw cars never drew and only
 * desert object_pen(follow) remained (rear-to-cam cancel).
 *
 * Disasm: push → 0x27 raw XYZ → yaw/pitch/roll → wr_slot → half-extent from
 * obj+0x30..0x3c → catalog pen+0xc with Y+=half → three catalogs from pen+8
 * → pop. pen+0 is a pointer to a 3-float local offset.
 *
 * Mid path @ 0x240C0 (scale≤100) shares the same orientation contract; until
 * that leaf is fully lifted, pen_desc routes both sides here so cars still
 * film with object yaw under the cam view.
 *
 * g0 = pen VA, g1 = host object pointer, g2 = budget.
 *
 * source: disasm/maincpu/maincpu_024560_280.asm */
// @rom 0x24560 +0x268 game_start_race_object_pen_far
// @rom 0x240c0 +0x4 game_start_race_object_pen_mid

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
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

static void prg_stq_words(u32 w0, u32 w1, u32 w2, u32 w3)
{
    geo_bank_poke0();
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
}

static u32 catalog_word(u32 ix, u32 off)
{
    return i960_ld_u32(I960_ROM, CATALOG_VADDR + (ix << 4), off);
}

void game_start_race_object_pen_far(u32 arg0, u32 arg1, u32 arg2)
{
    u32 pen_va = arg0 != 0u ? arg0 : (u32)g0;
    /* ROM g1 is the pose guest VA; never a host pointer (u32 truncates). */
    u32 obj_va = arg1 != 0u ? arg1 : (u32)g1;
    u32 budget = arg2 != 0u ? arg2 : (u32)g2;
    u8 *obj;
    u32 *obj_w;
    u32 pen_hdr;
    u32 cat_ix;
    u32 remain;
    u32 take;
    u32 half;
    u32 i;
    u32 w0, w1, w2, w3;
    u32 first_w3;
    static int logged;

    obj = model2_ram_mut(obj_va);
    if (!obj || pen_va == 0u || obj_va == 0u)
        return;

    obj_w = (u32 *)(void *)obj;
    pen_hdr = model2_workram_mirror_u32(pen_va);
    if (pen_hdr == 0u)
        return;

    if (!logged) {
        u32 follow = i960_ld_u32(I960_WORKRAM, 0x213980, 0);
        u32 fy = follow ? i960_ld_u32(I960_ABS, follow, 0x1c) : 0u;

        lift_log(
                "lift: object_pen_far pen=%#x obj=%#x yaw=%.3g "
                "follow_yaw=%.3g d8=%u\n",
                pen_va, obj_va, i960_u32_to_f64(obj_w[0x1c / 4]),
                i960_u32_to_f64(fy),
                (unsigned)i960_ld_u32(I960_WORKRAM, 0x2139d8, 0));
        fflush(stderr);
        logged = 1;
    }

    /* @0x24564–0x245F4: push + translate raw XYZ + yaw/pitch/roll. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, obj_w[0]);
    i960_mmio_write_u32(0x884000, obj_w[1]);
    i960_mmio_write_u32(0x884000, obj_w[2]);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, obj_w[0x1c / 4]);
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, obj_w[0x18 / 4]);
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    i960_mmio_write_u32(0x884000, obj_w[0x20 / 4]);
    wr_slot_bind();

    /* @0x24620–0x24680: mean(obj+0x30..0x3c)*0.25 → second 0x27 Y addend. */
    half = (u32)i960_f64_to_u32(
        (i960_u32_to_f64(obj_w[0x30 / 4]) + i960_u32_to_f64(obj_w[0x34 / 4])
         + i960_u32_to_f64(obj_w[0x38 / 4]) + i960_u32_to_f64(obj_w[0x3c / 4]))
        * i960_rifl_read(0, 0x3fd00000u));

    remain = budget - i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    if ((i32)remain < 1)
        remain = 1u;

    /* @0x2468C–0x24724: catalog pen+0xc, then 0x27(pen_hdr.xyz) with Y+=half. */
    cat_ix = model2_workram_mirror_u32(pen_va + 0xcu);
    w0 = catalog_word(cat_ix, 0);
    w1 = catalog_word(cat_ix, 4);
    w2 = catalog_word(cat_ix, 8);
    w3 = catalog_word(cat_ix, 12);
    take = w3;
    if (remain != 0u && w3 >= remain)
        take = remain;
    first_w3 = take;
    prg_stq_words(w0, w1, w2, take);

    {
        u32 bank = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        u32 px = model2_workram_mirror_u32(pen_hdr);
        u32 py = model2_workram_mirror_u32(pen_hdr + 4u);
        u32 pz = model2_workram_mirror_u32(pen_hdr + 8u);

        i960_mmio_write_u32(0x884000, 0x13802727u);
        i960_mmio_write_u32(0x884000, px);
        i960_mmio_write_u32(
            0x884000,
            (u32)i960_f64_to_u32(i960_u32_to_f64(py) + i960_u32_to_f64(half)));
        i960_mmio_write_u32(0x884000, pz);
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank + take);
        wr_slot_bind();
    }

    /* @0x24754–0x247B4: three catalogs from pen+8; bank += first take each. */
    cat_ix = model2_workram_mirror_u32(pen_va + 8u);
    for (i = 0; i < 3u; i++) {
        u32 bank = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);

        w0 = catalog_word(cat_ix, 0);
        w1 = catalog_word(cat_ix, 4);
        w2 = catalog_word(cat_ix, 8);
        w3 = catalog_word(cat_ix, 12);
        prg_stq_words(w0, w1, w2, w3);
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank + first_w3);
        wr_slot_bind();
    }

    i960_mmio_write_u32(0x884000, 0x10802121u);
}

/* @0x240C0 stub — route to far until the extent-Y prologue is lifted. */
void game_start_race_object_pen_mid(u32 arg0, u32 arg1, u32 arg2)
{
    game_start_race_object_pen_far(arg0, arg1, arg2);
}
