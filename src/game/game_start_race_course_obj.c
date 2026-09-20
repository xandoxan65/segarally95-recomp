/* Desert course-list objects (spectators / roadside / animals).
 *
 * Kind 4 init @ 0x2B580 installs per-frame 0x5CA420 (ROM 0x2B420).
 * Kind 21 init @ 0x2BB80 installs 0x5CAB50 (ROM 0x2BB50). Draw @ 0x2BB50
 * returns when 0x202230!=0 (practice), 0x217184 bit2, or attract.
 *
 * source: disasm/maincpu/maincpu_02b580_180.asm,
 *         maincpu_02b420_160.asm, maincpu_02bb50_40.asm */
// @rom 0x2b580 +0x20 game_start_race_course_obj_init_4
// @rom 0x2b420 +0x15c game_start_race_course_obj_draw_4
// @rom 0x2bb80 +0x20 game_start_race_course_obj_init_21
// @rom 0x2bb50 +0x2c game_start_race_course_obj_draw_21

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include <stdio.h>

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
}

static void wr_slot_bind_from(u32 geo_wr, u32 wr_slot)
{
    i960_st_u32(I960_WORKRAM, wr_slot, 0, geo_wr);
    i960_mmio_write_u32(0x801008, geo_wr + 0x34u);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);
}

void game_start_race_course_obj_draw_4(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 side;
    u32 x, y, z;
    u32 eye_x, eye_z;
    u32 wr_slot;
    u32 geo_wr;
    u32 cat_ix;
    u32 w0, w1, w2, w3;
    u32 bump;
    static int logged;

    (void)arg1;
    (void)arg2;

    /* @0x2B420–0x2B448: side xor 0x2020a4 selects TGP payload order. */
    side = (u32)i960_ld_u8(I960_ABS, node, 0x60)
        ^ (u32)i960_ld_u8(I960_WORKRAM, 0x2020a4, 0);
    x = i960_ld_u32(I960_ABS, node, 0x18);
    y = i960_ld_u32(I960_ABS, node, 0x1c);
    z = i960_ld_u32(I960_ABS, node, 0x20);
    eye_x = i960_ld_u32(I960_WORKRAM, 0x20220c, 0);
    eye_z = i960_ld_u32(I960_WORKRAM, 0x202214, 0);

    if (!logged) {
        lift_log(
                "lift: course_obj_draw_4 node=%#x xyz=(%.3g,%.3g,%.3g) slot=%u\n",
                (unsigned)node,
                i960_u32_to_f64(x), i960_u32_to_f64(y), i960_u32_to_f64(z),
                (unsigned)i960_ld_u32(I960_ABS, node, 0x5c));
        fflush(stderr);
        logged = 1;
    }

    wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    geo_wr = i960_ld_u32(I960_ABS, 0x00802008u, 0);
    i960_st_u32(I960_WORKRAM, wr_slot, 0, geo_wr);

    i960_mmio_write_u32(0x884000, 0x2e805d5du);
    if ((side & 1u) != 0u) {
        /* @0x2B450: obj, eye.xz, obj again. */
        i960_mmio_write_u32(0x884000, x);
        i960_mmio_write_u32(0x884000, y);
        i960_mmio_write_u32(0x884000, z);
        i960_mmio_write_u32(0x884000, eye_x);
        i960_mmio_write_u32(0x884000, x);
        i960_mmio_write_u32(0x884000, eye_z);
        i960_mmio_write_u32(0x884000, z);
    } else {
        i960_mmio_write_u32(0x884000, x);
        i960_mmio_write_u32(0x884000, y);
        i960_mmio_write_u32(0x884000, z);
        i960_mmio_write_u32(0x884000, x);
        i960_mmio_write_u32(0x884000, eye_x);
        i960_mmio_write_u32(0x884000, z);
        i960_mmio_write_u32(0x884000, eye_z);
    }
    wr_slot_bind_from(geo_wr, wr_slot);

    cat_ix = model2_workram_mirror_u32(
        0x5ca210u + (i960_ld_u32(I960_ABS, node, 0x5c) << 2));
    geo_bank_poke0();
    w0 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat_ix << 4), 0);
    w1 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat_ix << 4), 4);
    w2 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat_ix << 4), 8);
    w3 = i960_ld_u32(I960_ABS, CATALOG_VADDR + (cat_ix << 4), 12);
    bump = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bump + w3);
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
}

void game_start_race_course_obj_init_4(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;

    (void)arg1;
    (void)arg2;
    /* @0x2B598: per-frame 0x5CA420. */
    i960_st_u32(I960_ABS, node, 4, 0x005ca420u);
}

void game_start_race_course_obj_draw_21(u32 arg0, u32 arg1, u32 arg2)
{
    /* @0x2BB50: ret if practice (0x202230!=0), 0x217184 bit2, or attract. */
    if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) != 0u)
        return;
    if ((i960_ld_u8(I960_WORKRAM, 0x217184, 0) & 4u) != 0u)
        return;
    if (i960_ld_u32(I960_WORKRAM, 0x202098, 0) == 2u)
        return;
    game_start_race_course_obj_draw_4(arg0, arg1, arg2);
}

void game_start_race_course_obj_init_21(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;

    (void)arg1;
    (void)arg2;
    i960_st_u32(I960_ABS, node, 4, 0x005cab50u);
}
