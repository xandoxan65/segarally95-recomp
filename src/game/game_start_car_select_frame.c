/* Mode-3/5 car-select frame @ 0x14820 — input, select icons, car geo emit.
 * Catalogs @ 0x2865650 / indexed 0x2864b40 / 0x2865620 / 0x2865740 / 0x28656a0
 * with view floats from 0x5b3800+ keyed by course choice @ 0x214354.
 * source: disasm/maincpu/maincpu_014820_528.asm */
// @rom 0x14820 +0x528 game_start_car_select_frame

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

extern u32 game_start_select_timer_gate(void);
extern void game_start_select_timer_arm(void);
extern void game_start_select_icons(u32 arg0, u32 arg1, u32 arg2);

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
}

static u32 prg_stq_main_data(u32 vaddr)
{
    const u8 *p = model2_rom_at(vaddr);
    u32 w0, w1, w2, w3;

    if (!p)
        return 0u;
    w0 = (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
    w1 = (u32)p[4] | ((u32)p[5] << 8) | ((u32)p[6] << 16) | ((u32)p[7] << 24);
    w2 = (u32)p[8] | ((u32)p[9] << 8) | ((u32)p[10] << 16) | ((u32)p[11] << 24);
    w3 = (u32)p[12] | ((u32)p[13] << 8) | ((u32)p[14] << 16) | ((u32)p[15] << 24);

    geo_bank_poke0();
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
    return w3;
}

static u32 rom_const_u32(u32 vaddr)
{
    u32 w = i960_ld_u32(I960_WORKRAM, vaddr, 0);

    if (w == 0)
        w = model2_workram_mirror_u32(vaddr);
    return w;
}

static void geo_bump_count(u32 count)
{
    u32 bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);

    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + count);
}

/*
 * TGP 0x05 matrix upload. `advance` matches ROM: first bind @ 0x1499C does
 * not addo 0x20a290; the mid-translate @ 0x14A40 and second bind @ 0x14A94 do.
 */
static void geo_wr_slot_bind(int advance)
{
    u32 slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    u32 wr_ptr = i960_mmio_read_u32(0x802008);

    if (slot != 0)
        i960_st_u32(I960_WORKRAM, slot, 0, wr_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_mmio_write_u32(0x801008, wr_ptr + 0x34u);
    if (advance)
        i960_st_u32(I960_WORKRAM, 0x20a290, 0, slot + 4u);
}

static void geo_window_push3(u32 x, u32 y, u32 z)
{
    i960_mmio_write_u32(0x8000a0u, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, x);
    i960_mmio_write_u32(GEO_PRG_FIFO, y);
    i960_mmio_write_u32(GEO_PRG_FIFO, z);
}

/*
 * Private host frame — ROM `lda 0x100(sp),sp` with locals at 0x40(fp)…0x110(fp).
 * Choice-keyed view triples land at 0x40/50/60 before the trailing catalogs.
 */
void game_start_car_select_frame(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    u8 frame[0x120];
    u32 gate;
    u32 pad;
    u32 choice;
    u32 lookup;
    u32 count;
    u32 ix;
    u32 c0, c1, c2;
    u32 d0, d1, d2;
    u32 sx, sy;
    u32 v0x, v0y, v0z;
    u32 v1x, v1y, v1z;
    u32 v2x, v2y, v2z;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    gate = game_start_select_timer_gate();
    if (gate == 2u) {
        /* @0x14848–0x148C4: wheel / pedal arm + scene_lookup(5) → 0x20a8bc /
         * course index @ 0x214354. */
        pad = i960_ld_u8(I960_WORKRAM, 0x202051, 0) & 0xffu;
        if (pad <= 0xafu)
            i960_st_u32(I960_WORKRAM, 0x20a894, 0, 1u);

        if (i960_ld_u32(I960_WORKRAM, 0x20a894, 0) != 0u) {
            pad = i960_ld_u8(I960_WORKRAM, 0x202051, 0) & 0xffu;
            if (pad > 0xb0u)
                game_start_select_timer_arm();
            else if ((i960_ld_u8(I960_WORKRAM, 0x202064, 0) & 0x10u) != 0u)
                game_start_select_timer_arm();
        } else if ((i960_ld_u8(I960_WORKRAM, 0x202064, 0) & 0x10u) != 0u) {
            game_start_select_timer_arm();
        }

        g0 = 5;
        g1 = i960_ld_u32(I960_WORKRAM, 0x20a8bc, 0);
        g0 = scene_lookup_fn((u32)g0, (u32)g1);
        lookup = (u32)g0;
        i960_st_u32(I960_WORKRAM, 0x20a8bc, 0, lookup);
        choice = lookup - 2u;
        if ((i32)choice <= 0)
            choice = 0u;
        i960_st_u32(I960_WORKRAM, 0x214354, 0, choice);
    }

    /* @0x148CC: select icons (52, 2). */
    game_start_select_icons(52u, 2u, 0);

    /* @0x148D8–0x1494C: window 400x400 + translate from 0x5b3800 / 0x5b34d0. */
    c0 = rom_const_u32(0x005b3800u);
    c1 = rom_const_u32(0x005b3804u);
    c2 = rom_const_u32(0x005b3808u);
    d0 = rom_const_u32(0x005b34d0u);
    d1 = rom_const_u32(0x005b34d4u);
    d2 = rom_const_u32(0x005b34d8u);

    i960_st_u32(I960_WORKRAM, 0x202270, 0, 0x43c80000u);
    i960_st_u32(I960_WORKRAM, 0x202274, 0, 0x43c80000u);

    i960_mmio_write_u32(0x800090u, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x43c80000u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x43c80000u);

    geo_window_push3(c0, c1, c2);

    i960_mmio_write_u32(0x884000, 0x11802323u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, d0);
    i960_mmio_write_u32(0x884000, d1);
    i960_mmio_write_u32(0x884000, d2);
    /* @0x1499C: 0x05, no 0x20a290 advance. */
    geo_wr_slot_bind(0);

    /*
     * @0x149E8–0x14AA4: header catalog @ 0x2865650, then 0x25/0x27 with
     * (0, 0x20a8a4, 0x20a8a8). ROM advances 0x20a290 mid-command (@ 0x14A40,
     * addo g5,4 using the g5 still live from the prior bind) before the
     * translate floats and the second 0x05 (@ 0x14A84).
     */
    count = prg_stq_main_data(0x02865650u);
    sx = i960_ld_u32(I960_WORKRAM, 0x20a8a4, 0);
    sy = i960_ld_u32(I960_WORKRAM, 0x20a8a8, 0);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    {
        u32 slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);

        /* @0x14A40: addo g5,4 / st 0x20a290 — even when slot was 0. */
        i960_st_u32(I960_WORKRAM, 0x20a290, 0, slot + 4u);
    }
    geo_bump_count(count);
    i960_mmio_write_u32(0x884000, 0u);
    i960_mmio_write_u32(0x884000, sx);
    i960_mmio_write_u32(0x884000, sy);
    /* @0x14A94: second 0x05 + addo 0x20a290. */
    geo_wr_slot_bind(1);

    /*
     * @0x14AAC–0x14B44: when gate==2 or 0x202008 bit2, pick light triples
     * from 0x5b3800 / 0x5b3810 / 0x5b3818 keyed by 0x214354 (0/1/else).
     * These feed GEO 0x0A (via 0x8000a0) — not TGP translate. Panel X is
     * object-space in catalogs 0x2865620 / 5740 / 56a0 (disasm has no per-
     * panel 0x27). MAME geo_w eye bits are unused here: 0x202278 stays 0
     * (unlike course_select which sets banks 0..3 around trapezoid 0x03).
     *
     * Defaults always match ROM's pre-branch loads: all three slots get
     * 5b3810 / 5b3818 even when the choice gate is skipped (ROM still
     * ldq 0x40/50/60 afterward).
     */
    {
        u32 a0 = rom_const_u32(0x005b3800u);
        u32 a1 = rom_const_u32(0x005b3804u);
        u32 a2 = rom_const_u32(0x005b3808u);
        u32 b0 = rom_const_u32(0x005b3810u);
        u32 b1 = rom_const_u32(0x005b3814u);
        u32 b2 = rom_const_u32(0x005b3818u);

        v0x = b0;
        v0y = b1;
        v0z = b2;
        v1x = b0;
        v1y = b1;
        v1z = b2;
        v2x = b0;
        v2y = b1;
        v2z = b2;

        gate = game_start_select_timer_gate();
        if (gate == 2u || ((i960_ld_u32(I960_WORKRAM, 0x202008, 0) >> 2) & 1u) != 0u) {
            choice = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
            if (choice == 0u) {
                v0x = a0;
                v0y = a1;
                v0z = a2;
            } else if (choice == 1u) {
                v1x = a0;
                v1y = a1;
                v1z = a2;
            } else {
                v2x = a0;
                v2y = a1;
                v2z = a2;
            }

            /* @0x14B70: indexed car icon from 0x5b34e0[choice&3]. */
            ix = rom_const_u32(0x005b34e0u + ((choice & 3u) << 2));
            count = prg_stq_main_data(CATALOG_VADDR + (ix << 4));
            geo_bump_count(count);
        }

        *(u32 *)(fp + 0x40) = v0x;
        *(u32 *)(fp + 0x44) = v0y;
        *(u32 *)(fp + 0x48) = v0z;
        *(u32 *)(fp + 0x50) = v1x;
        *(u32 *)(fp + 0x54) = v1y;
        *(u32 *)(fp + 0x58) = v1z;
        *(u32 *)(fp + 0x60) = v2x;
        *(u32 *)(fp + 0x64) = v2y;
        *(u32 *)(fp + 0x68) = v2z;
    }

    /*
     * @0x14BD4–0x14C64: light 0x40, then catalog 0x2865620, then light 0x50.
     * ROM order: 0x0A(0x40) → stq 5620 → 0x0A(0x50) → bump.
     */
    geo_window_push3(*(u32 *)(fp + 0x40), *(u32 *)(fp + 0x44),
                     *(u32 *)(fp + 0x48));
    count = prg_stq_main_data(0x02865620u);
    geo_window_push3(*(u32 *)(fp + 0x50), *(u32 *)(fp + 0x54),
                     *(u32 *)(fp + 0x58));
    geo_bump_count(count);

    /* @0x14C94–0x14CE0: catalog 0x2865740 then light 0x60. */
    count = prg_stq_main_data(0x02865740u);
    geo_window_push3(*(u32 *)(fp + 0x60), *(u32 *)(fp + 0x64),
                     *(u32 *)(fp + 0x68));
    geo_bump_count(count);

    /* @0x14D08: trailing catalog @ 0x28656a0. */
    count = prg_stq_main_data(0x028656a0u);
    geo_bump_count(count);

    fp = fp_save;
}
