/* Mode-3 submode-3 frame @ 0x13CE0 — input, select icons, view push.
 * Tile mid-labels (a_champ_e / a_pura_e) + TGP catalog quads @ 0x28656c0+.
 * source: disasm/maincpu/maincpu_013ce0_380.asm (+ tail through ret @ 0x14150) */
// @rom 0x13ce0 +0x370 game_start_mode_select_frame

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include "lift_syms.h"
#include "game_start_icon_batch.h"

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

static void prg_stq_main_data(u32 vaddr)
{
    const u8 *p = model2_rom_at(vaddr);
    u32 w0, w1, w2, w3;

    if (!p)
        return;
    w0 = (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
    w1 = (u32)p[4] | ((u32)p[5] << 8) | ((u32)p[6] << 16) | ((u32)p[7] << 24);
    w2 = (u32)p[8] | ((u32)p[9] << 8) | ((u32)p[10] << 16) | ((u32)p[11] << 24);
    w3 = (u32)p[12] | ((u32)p[13] << 8) | ((u32)p[14] << 16) | ((u32)p[15] << 24);

    geo_bank_poke0();
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
}

static u32 rom_const_u32(u32 vaddr)
{
    u32 w = i960_ld_u32(I960_WORKRAM, vaddr, 0);

    if (w == 0)
        w = model2_workram_mirror_u32(vaddr);
    return w;
}

/*
 * @0x13D70–0x14150: GEO window + TGP push/translate + championship/practice
 * catalog quads. Prior lift only dumped three stq catalogs and skipped the
 * 0x14002828 scale, 0x20b940 bumps, selection branch, and trailing 0x2865770.
 */
static void game_start_select_geo_emit(void)
{
    uintptr_t fp_save = fp;
    u8 frame[0x90];
    u32 gate;
    u32 choice;
    u32 wr_slot;
    u32 wr_ptr;
    u32 x, y, z;
    u32 c0, c1, c2;
    u32 d0, d1;
    u32 one;
    u32 count;
    u32 bank_ctr;
    u32 flags;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    /* @0x13D70: staged floats @ 0x5b2cc0 / 0x5b2cd0 (ROM 0x13cc0). */
    c0 = rom_const_u32(0x005b2cc0u);
    c1 = rom_const_u32(0x005b2cc4u);
    c2 = rom_const_u32(0x005b2cc8u);
    d0 = rom_const_u32(0x005b2cd0u);
    d1 = rom_const_u32(0x005b2cd4u);
    one = rom_const_u32(0x005b2cd8u); /* 0.34f — Z offset after mid catalogs */

    i960_st_u32(I960_WORKRAM, 0x202270, 0, 0x43c80000u); /* 400.f */
    i960_st_u32(I960_WORKRAM, 0x202274, 0, 0x43c80000u);

    i960_mmio_write_u32(0x800090, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x43c80000u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x43c80000u);

    x = i960_ld_u32(I960_WORKRAM, 0x20a880, 0);
    y = i960_ld_u32(I960_WORKRAM, 0x20a884, 0);
    z = i960_ld_u32(I960_WORKRAM, 0x20a888, 0);

    i960_mmio_write_u32(0x8000a0, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, c0);
    i960_mmio_write_u32(GEO_PRG_FIFO, c1);
    i960_mmio_write_u32(GEO_PRG_FIFO, c2);

    /* TGP push / clear-R / translate (@ 0x13DE4+). */
    i960_mmio_write_u32(0x884000, 0x11802323u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, x);
    i960_mmio_write_u32(0x884000, y);
    i960_mmio_write_u32(0x884000, z);

    /*
     * @0x13E34: TGP 0x05, no 0x20a290 advance (g5 stays live for @0x13ECC).
     * Skip the *(slot) store only when slot is still 0 (host null safety);
     * never skip the 0x05 itself.
     */
    wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    wr_ptr = i960_mmio_read_u32(0x802008);
    if (wr_slot != 0)
        i960_st_u32(I960_WORKRAM, wr_slot, 0, wr_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_mmio_write_u32(0x801008, wr_ptr + 0x34u);

    /* @0x13E80: base catalog, then TGP scale (1.2, 1, 1). */
    prg_stq_main_data(0x028656c0u);
    {
        const u8 *p = model2_rom_at(0x028656c0u);

        count = p ? ((u32)p[12] | ((u32)p[13] << 8) | ((u32)p[14] << 16)
                     | ((u32)p[15] << 24))
                  : 0u;
    }
    i960_mmio_write_u32(0x884000, 0x14002828u);
    i960_mmio_write_u32(0x884000, 0x3f99999au); /* 1.2f */
    i960_mmio_write_u32(0x884000, (u32)i960_f64_to_u32(1.0));
    i960_mmio_write_u32(0x884000, (u32)i960_f64_to_u32(1.0));

    bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + count);

    /* @0x13ECC: addo g5,4 / st 0x20a290 — even when the prior slot was 0. */
    wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);

    /*
     * Second 0x05 bind (@ 0x13EE0+): advance first, store wr_ptr to the old
     * slot (skip null), then emit 0x05.
     */
    wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    wr_ptr = i960_mmio_read_u32(0x802008);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);
    if (wr_slot != 0)
        i960_st_u32(I960_WORKRAM, wr_slot, 0, wr_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_mmio_write_u32(0x801008, wr_ptr + 0x34u);

    gate = game_start_select_timer_gate();
    flags = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
    choice = i960_ld_u32(I960_WORKRAM, 0x202230, 0);

    /*
     * @0x13F3C: gate==2 → always emit selection catalog.
     * Else require 0x202008 bit2 (ROM bbc 2).
     *
     * fp float layout after the branch (fed to GEO around the trailing
     * catalogs) matches the two ROM stores at 0x13F58 / 0x13FC8:
     *   choice 0: 0x40=(0,0), 0x48=1.0, 0x50=(0,-0.94), 0x58=0.34
     *   choice 1: 0x40=(0,-0.94), 0x48=0.34, 0x50=(0,0), 0x58=1.0
     */
    if (gate == 2u || ((flags >> 2) & 1u) != 0u) {
        /*
         * ROM: one bank poke then stq. prg_stq_main_data already pokes —
         * a second poke here doubled 0x00800000 and made object_data eat
         * the real tpa as tha (live: objects=4 verts=18, base span only).
         */
        if (choice == 0u) {
            *(u32 *)(fp + 0x40) = c0;
            *(u32 *)(fp + 0x44) = c1;
            *(u32 *)(fp + 0x48) = c2; /* 1.0 */
            *(u32 *)(fp + 0x50) = d0;
            *(u32 *)(fp + 0x54) = d1;
            *(u32 *)(fp + 0x58) = one; /* 0.34 */
            prg_stq_main_data(0x028656e0u);
            {
                const u8 *p = model2_rom_at(0x028656e0u);

                count = p ? ((u32)p[12] | ((u32)p[13] << 8)
                             | ((u32)p[14] << 16) | ((u32)p[15] << 24))
                          : 0u;
            }
        } else {
            *(u32 *)(fp + 0x50) = c0;
            *(u32 *)(fp + 0x54) = c1;
            *(u32 *)(fp + 0x40) = d0;
            *(u32 *)(fp + 0x44) = d1;
            *(u32 *)(fp + 0x48) = one; /* 0.34 */
            *(u32 *)(fp + 0x58) = c2; /* 1.0 */
            prg_stq_main_data(0x02865780u);
            {
                const u8 *p = model2_rom_at(0x02865780u);

                count = p ? ((u32)p[12] | ((u32)p[13] << 8)
                             | ((u32)p[14] << 16) | ((u32)p[15] << 24))
                          : 0u;
            }
        }
        bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + count);
    } else {
        /* @0x14044: both slots get the 0x5b2cd0 pair; 0x48/0x58 = 0.34. */
        *(u32 *)(fp + 0x40) = d0;
        *(u32 *)(fp + 0x44) = d1;
        *(u32 *)(fp + 0x50) = d0;
        *(u32 *)(fp + 0x54) = d1;
        *(u32 *)(fp + 0x48) = one;
        *(u32 *)(fp + 0x58) = one;
    }

    /* @0x14058: GEO push 0x40/0x44/0x48, then catalog 0x28656d0. */
    i960_mmio_write_u32(0x8000a0, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, *(u32 *)(fp + 0x40));
    i960_mmio_write_u32(GEO_PRG_FIFO, *(u32 *)(fp + 0x44));
    i960_mmio_write_u32(GEO_PRG_FIFO, *(u32 *)(fp + 0x48));

    prg_stq_main_data(0x028656d0u);
    {
        const u8 *p = model2_rom_at(0x028656d0u);

        count = p ? ((u32)p[12] | ((u32)p[13] << 8) | ((u32)p[14] << 16)
                     | ((u32)p[15] << 24))
                  : 0u;
    }

    /* @0x140CC: GEO push 0x50/0x54/0x58, then bump + trailing 0x2865770. */
    i960_mmio_write_u32(0x8000a0, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, *(u32 *)(fp + 0x50));
    i960_mmio_write_u32(GEO_PRG_FIFO, *(u32 *)(fp + 0x54));
    i960_mmio_write_u32(GEO_PRG_FIFO, *(u32 *)(fp + 0x58));

    bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + count);

    prg_stq_main_data(0x02865770u);
    {
        const u8 *p = model2_rom_at(0x02865770u);

        count = p ? ((u32)p[12] | ((u32)p[13] << 8) | ((u32)p[14] << 16)
                     | ((u32)p[15] << 24))
                  : 0u;
    }
    bank_ctr = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bank_ctr + count);

    fp = fp_save;
}

static void game_start_redraw_mode_icons(void)
{
    u32 batch = game_start_icon_batch_get();
    u32 g3_a;
    u32 g3_b;
    u32 limit;
    int jpn = (i960_ld_u8(I960_WORKRAM, 0x202019, 0) == 0);

    g3_a = jpn ? 0u : 1u; /* a_champ / a_champ_e */
    g3_b = jpn ? 2u : 3u; /* a_pura / a_pura_e */

    limit = i960_ld_u32(I960_WORKRAM, 0x20c954, 0);
    if (batch != 0u && batch != (u32)-1 && batch + g3_b >= limit) {
        static int s_logged;

        if (!s_logged) {
            lift_log(
                    "lift: mode_select icon batch=%u g3_b=%u limit=%u — reseed\n",
                    (unsigned)batch, (unsigned)g3_b, (unsigned)limit);
            s_logged = 1;
        }
        batch = 0;
    }

    if (batch == 0u || batch == (u32)-1) {
        g0 = 4;
        g1 = 30;
        g2 = 0x0210cf40u;
        g3 = g3_a;
        g4 = 0;
        batch = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
        g0 = 33;
        g1 = 30;
        g2 = batch;
        g3 = g3_b;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        game_start_icon_batch_set(batch);
        return;
    }

    g0 = 4;
    g1 = 30;
    g2 = batch;
    g3 = g3_a;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    g0 = 33;
    g1 = 30;
    g2 = batch;
    g3 = g3_b;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
}

void game_start_mode_select_frame(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;
    u32 handle;
    u32 flags;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    gate = game_start_select_timer_gate();

    /* @0x13CF0: gate==2 → read wheel / START for selection. */
    if (gate == 2u) {
        handle = (u8)i960_ld_u8(I960_WORKRAM, 0x202051, 0);
        if (handle <= 0xafu) {
            i960_st_u32(I960_WORKRAM, 0x20a870, 0, 1u);
        }

        if (i960_ld_u32(I960_WORKRAM, 0x20a870, 0) != 0u) {
            handle = (u8)i960_ld_u8(I960_WORKRAM, 0x202051, 0);
            if (handle > 0xb0u)
                game_start_select_timer_arm();
        }

        flags = (u8)i960_ld_u8(I960_WORKRAM, 0x202064, 0);
        if ((flags >> 4) & 1u)
            game_start_select_timer_arm();

        /* @0x13D4C–0x13D5C: refresh 0x202230 via scene_lookup(2, …). */
        {
            u32 prev = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
            u32 steer = (u32)i960_ld_u8(I960_WORKRAM, 0x202050, 0) & 0xffu;

            g1 = prev;
            g0 = scene_lookup_fn(2u, (u32)g1);
            i960_st_u32(I960_WORKRAM, 0x202230, 0, (u32)g0);
            if ((u32)g0 != prev) {
                lift_log(
                        "lift: mode_select choice %u→%u steer=0x%02x\n",
                        (unsigned)prev, (unsigned)g0, (unsigned)steer);
            }
        }
    }

    /* @0x13D64: always redraw countdown / select glyphs at (52, 2). */
    game_start_select_icons(52u, 2u, 0);
    game_start_redraw_mode_icons();
    game_start_select_geo_emit();
}
