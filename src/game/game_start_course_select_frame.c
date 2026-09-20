/* Mode-3/5 course-select frame @ 0x15380 — input, icon geo emit, view push.
 * Championship/practice course icon quads @ 0x2865680+ plus the indexed
 * catalog icon @ 0x2864b40(mirror ix) and a rotating pen/spinner highlight
 * (four geo_attract_object_pen draws @ 0x20a8d0 with alternating rotation
 * and pen index across the loop).
 * Was staged as null (see game_start_course_select.c); the icons never drew.
 * source: disasm/maincpu/maincpu_015380_c3c.asm */
// @rom 0x15380 +0xc3c game_start_course_select_frame

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
extern void geo_attract_object_pen(void *arg0, u32 arg1, u32 arg2);

static void geo_bank_poke0(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
}

/* Reads a 4-word main_data record and stq's it to the GEO program fifo;
 * returns word3 (record count) so callers can bump 0x20b940. */
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

/* @0x20a290 write-pointer bind: hands the current GEO write cursor to the
 * workram slot the ROM is tracking, then advances the slot and the TGP
 * command stream (repeats through this function @ 0x154D4/0x155B4/0x15750/
 * 0x1588C/0x15928). */
/*
 * ROM pattern (@ 0x154D4 / 0x155B4 / …): advance 0x20a290 first, store the
 * GEO write cursor to the old slot, then TGP 0x05. Skip the null deref when
 * the slot is still 0, but always advance so later binds stay sequenced.
 */
static void geo_wr_slot_bind(void)
{
    u32 slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    u32 wr_ptr = i960_mmio_read_u32(0x802008);

    i960_st_u32(I960_WORKRAM, 0x20a290, 0, slot + 4u);
    if (slot != 0)
        i960_st_u32(I960_WORKRAM, slot, 0, wr_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_mmio_write_u32(0x801008, wr_ptr + 0x34u);
}

/* GEO window/coordinate push: gate @ 0x8000a0 then three floats to the prg fifo. */
static void geo_window_push3(u32 x, u32 y, u32 z)
{
    i960_mmio_write_u32(0x8000a0u, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, x);
    i960_mmio_write_u32(GEO_PRG_FIFO, y);
    i960_mmio_write_u32(GEO_PRG_FIFO, z);
}

/*
 * @0x15418-0x15FA0: GEO window 400x400, TGP push/translate, five main_data
 * icon quads (0x2865730 header + the four championship/practice course icons
 * @ 0x2865680/0x2865660/0x28656f0/0x2865710) plus the choice-indexed course
 * icon @ 0x2864b40(mirror ix), and the four-draw pen/spinner tail through
 * geo_attract_object_pen @ 0x20a8d0. Private host frame — ROM `lda 0x120(sp),sp`
 * establishes a callee frame distinct from the caller's fp.
 */
static void game_start_course_select_geo_emit(void)
{
    uintptr_t fp_save = fp;
    u8 frame[0x140];
    u32 c0, c1, c2;
    u32 d0, d1, d2;
    u32 choice;
    u32 ix;
    u32 count;
    u32 i;
    void *obj_ptr;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    /* @0x15418-0x15458: window 400x400 + translate from 0x20a930/34/38. */
    c0 = rom_const_u32(0x005b4350u);
    c1 = rom_const_u32(0x005b4354u);
    c2 = rom_const_u32(0x005b4358u);
    d0 = i960_ld_u32(I960_WORKRAM, 0x20a930, 0);
    d1 = i960_ld_u32(I960_WORKRAM, 0x20a934, 0);
    d2 = i960_ld_u32(I960_WORKRAM, 0x20a938, 0);

    i960_st_u32(I960_WORKRAM, 0x202270, 0, 0x43c80000u); /* 400.f */
    i960_st_u32(I960_WORKRAM, 0x202274, 0, 0x43c80000u);

    i960_mmio_write_u32(0x800090u, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x43c80000u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x43c80000u);

    geo_window_push3(c0, c1, c2);

    /* @0x154A4-0x15528: TGP push(0x23) / clear-R(0x25) / translate(0x27)(d0,d1,d2) + bind #1. */
    i960_mmio_write_u32(0x884000, 0x11802323u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, d0);
    i960_mmio_write_u32(0x884000, d1);
    i960_mmio_write_u32(0x884000, d2);
    geo_wr_slot_bind();

    /* @0x15544-0x1557C: header record @ 0x2865730. */
    count = prg_stq_main_data(0x02865730u);
    geo_bump_count(count);

    /* @0x15584-0x15604: second translate (0x5b4370/74/78) + bind #2. */
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, rom_const_u32(0x005b4370u));
    i960_mmio_write_u32(0x884000, rom_const_u32(0x005b4374u));
    i960_mmio_write_u32(0x884000, rom_const_u32(0x005b4378u));
    geo_wr_slot_bind();

    /* @0x1560C-0x15678: fp+0x40+i*0xc triples for the four course slots —
     * the current choice gets the "selected" pair (0x5b4350), others the
     * "unselected" pair (0x5b4360), gated by wheel input or the D6 flag. */
    for (i = 0; i <= 3u; i++) {
        uintptr_t slot = fp + 0x40u + i * 0xcu;
        u32 choice_lp = i960_ld_u32(I960_WORKRAM, 0x20a8c4, 0);
        int use_selected = 0;

        if (i == choice_lp) {
            u32 gate_lp = game_start_select_timer_gate();

            if (gate_lp == 2u)
                use_selected = 1;
            else if ((i960_ld_u32(I960_WORKRAM, 0x202008, 0) >> 2) & 1u)
                use_selected = 1;
        }

        if (use_selected) {
            *(u32 *)(slot + 0) = rom_const_u32(0x005b4350u);
            *(u32 *)(slot + 4) = rom_const_u32(0x005b4354u);
            *(u32 *)(slot + 8) = rom_const_u32(0x005b4358u);
        } else {
            *(u32 *)(slot + 0) = rom_const_u32(0x005b4360u);
            *(u32 *)(slot + 4) = rom_const_u32(0x005b4364u);
            *(u32 *)(slot + 8) = rom_const_u32(0x005b4368u);
        }
    }

    /* @0x1567C-0x15708: draw the selected course's icon — mirror table
     * 0x5b4340[choice] gives the catalog index; icon at 0x2864b40 + (ix<<4);
     * window push uses the loop's slot0 triple (fp+0x40/44/48). */
    choice = i960_ld_u32(I960_WORKRAM, 0x20a8c4, 0);
    ix = rom_const_u32(0x005b4340u + choice * 4u);
    count = prg_stq_main_data(CATALOG_VADDR + (ix << 4));
    geo_window_push3(*(u32 *)(fp + 0x40), *(u32 *)(fp + 0x44), *(u32 *)(fp + 0x48));

    /* @0x15708-0x15798: TGP translate (-3.5,-6.6,0); count from the selected
     * icon (above) is folded into the 0x20b940 bump here (@ 0x15774), then bind #3. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, 0xc0600000u); /* -3.5f */
    i960_mmio_write_u32(0x884000, 0xc0d33333u); /* -6.6f */
    i960_mmio_write_u32(0x884000, 0u);
    geo_bump_count(count);
    geo_wr_slot_bind();

    /* @0x157BC-0x15844: course icon @ 0x2865680, TGP 0x10802121, slot1 window push. */
    count = prg_stq_main_data(0x02865680u);
    geo_bump_count(count);
    i960_mmio_write_u32(0x884000, 0x10802121u);
    geo_window_push3(*(u32 *)(fp + 0x4c), *(u32 *)(fp + 0x50), *(u32 *)(fp + 0x54));

    /* @0x1584C-0x158C8: TGP translate (+3.5,+6.6,0) then bind #4. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, 0x40600000u); /* +3.5f */
    i960_mmio_write_u32(0x884000, 0x40d33333u); /* +6.6f */
    i960_mmio_write_u32(0x884000, 0u);
    geo_wr_slot_bind();

    /* @0x158EC-0x159B0: course icon @ 0x2865660, TGP 0x10802121, bind #5, slot2 push. */
    count = prg_stq_main_data(0x02865660u);
    geo_bump_count(count);
    i960_mmio_write_u32(0x884000, 0x10802121u);
    geo_wr_slot_bind();
    geo_window_push3(*(u32 *)(fp + 0x58), *(u32 *)(fp + 0x5c), *(u32 *)(fp + 0x60));

    /* @0x159D4-0x15A48: course icon @ 0x28656f0 (no TGP cmd, no bind) + slot3 push. */
    count = prg_stq_main_data(0x028656f0u);
    geo_bump_count(count);
    geo_window_push3(*(u32 *)(fp + 0x64), *(u32 *)(fp + 0x68), *(u32 *)(fp + 0x6c));

    /* @0x15A70-0x15B38: course icon @ 0x2865710 (no window push); TGP
     * clear-R(0x25)/translate(0x27), x=0, y=0 — the z=15.0 write is
     * scheduled after the pen/spin object clear below but belongs to this
     * same TGP command (ROM interleaves unrelated workram stores between
     * the y and z fifo writes; the fifo itself only sees x,y,z in order). */
    count = prg_stq_main_data(0x02865710u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, 0u); /* x */
    geo_bump_count(count);
    i960_mmio_write_u32(0x884000, 0u); /* y */

    /* @0x15AE8-0x15B28: seed -0.3f quad + zero the pen/spin object triple
     * @ 0x20a8d0 (position/scratch; obj+4 @ 0x20a8d4 is left readable by
     * the pen-emit chain but never written by lifted code, so it stays 0). */
    i960_st_u32(I960_WORKRAM, 0x20a91c, 0, 0xbe99999au);
    i960_st_u32(I960_WORKRAM, 0x20a918, 0, 0xbe99999au);
    i960_st_u32(I960_WORKRAM, 0x20a914, 0, 0xbe99999au);
    i960_st_u32(I960_WORKRAM, 0x20a8d0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a8d4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a8d8, 0, 0);

    /* @0x15B30-0x15BC0: finish the translate z (15.0f), TGP pitch marker
     * (0x29) + 0.2f, then GEO window 0x03 @ 0x800030 with trapezoid eye
     * centers (eye0 ≈ 100,256) for the spinner — not a literal GEO quad.
     * Icon catalogs above were emitted under the bootstrap-centered window. */
    i960_mmio_write_u32(0x884000, 0x41700000u); /* z = 15.0f */
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, 0x3e4ccccdu); /* 0.2f */
    i960_mmio_write_u32(0x800030u, 0u);
    {
        u32 pen0 = i960_ld_u8(I960_WORKRAM, 0x20a920, 0);

        *(u32 *)(fp + 0xe0) = 0x80u;
        *(u32 *)(fp + 0xe4) = 0x01f00200u;
        *(u32 *)(fp + 0xe8) = 0x00640100u;
        *(u32 *)(fp + 0xec) = 0x00a40174u;
        i960_mmio_write_u32(GEO_PRG_FIFO, *(u32 *)(fp + 0xe0));
        i960_mmio_write_u32(GEO_PRG_FIFO, *(u32 *)(fp + 0xe4));
        i960_mmio_write_u32(GEO_PRG_FIFO, *(u32 *)(fp + 0xe8));
        i960_mmio_write_u32(GEO_PRG_FIFO, *(u32 *)(fp + 0xec));
        i960_mmio_write_u32(GEO_PRG_FIFO, 0x01500174u);
        i960_mmio_write_u32(GEO_PRG_FIFO, 0x018c0100u);

        /* @0x15BC8-0x15C34: pen low-nibble reset; jp (0x20201b, seat select)
         * refreshes the low 2 bits of 0x20a921. */
        i960_st_u32(I960_WORKRAM, 0x202278, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x20a910, 0, 0xbe99999au); /* -0.3f */
        pen0 &= ~0xfu;
        i960_st_u8(I960_WORKRAM, 0x20a920, 0, (u8)pen0);

        {
            u32 jp = i960_ld_u8(I960_WORKRAM, 0x20201b, 0);
            u32 pen1 = i960_ld_u8(I960_WORKRAM, 0x20a921, 0);

            if (jp != 0u)
                pen1 = (pen1 & ~3u) | ((jp - 1u) & 3u);
            else
                pen1 &= ~3u;
            i960_st_u8(I960_WORKRAM, 0x20a921, 0, (u8)pen1);
        }
    }

    /*
     * @0x15C3C-0x15FA0: four icon draws through the pen/spin object @
     * 0x20a8d0 (geo_attract_object_pen). Rotation (obj+0x1c @ 0x20a8ec)
     * alternates -phase / phase+pi across the four slots; the neg03 quad
     * (0x20a91c/18/14/10) is obj4 (0x20a8d4, read-only in this call chain
     * so effectively 0) minus 0.3 each time; pen low-nibble is 0 for
     * slots 0-1 and 1 for slots 2-3. phase (0x20a8c8) accumulates +0.01
     * at the end for next frame.
     */
    obj_ptr = i960_vaddr_ptr(0x20a8d0);
    {
        u32 phase, obj4, pen, rot;
        double neg03;

        /* Slot0: window push fp+0x40/44/48; rotation = -phase. */
        phase = i960_ld_u32(I960_WORKRAM, 0x20a8c8, 0);
        rot = phase ^ 0x80000000u;
        geo_window_push3(*(u32 *)(fp + 0x40), *(u32 *)(fp + 0x44), *(u32 *)(fp + 0x48));
        i960_st_u32(I960_WORKRAM, 0x20a8ec, 0, rot);
        geo_attract_object_pen(obj_ptr, 0x3f800000u, 0x1869fu);

        /* Slot1 setup: neg03 = obj4-0.3; pen &= ~0xF; rotation = phase+pi. */
        obj4 = i960_ld_u32(I960_WORKRAM, 0x20a8d4, 0);
        pen = i960_ld_u8(I960_WORKRAM, 0x20a920, 0);
        phase = i960_ld_u32(I960_WORKRAM, 0x20a8c8, 0);
        neg03 = i960_u32_to_f64(obj4) - i960_rifl_read(0x33333333u, 0x3fd33333u);
        i960_st_u32(I960_WORKRAM, 0x202278, 0, 1u);
        pen &= ~0xfu;
        i960_st_u8(I960_WORKRAM, 0x20a920, 0, (u8)pen);
        rot = (u32)i960_f64_to_u32(i960_u32_to_f64(phase)
                                    + i960_rifl_read(0x54442d18u, 0x400921fbu));
        geo_window_push3(*(u32 *)(fp + 0x4c), *(u32 *)(fp + 0x50), *(u32 *)(fp + 0x54));
        {
            u32 nv = (u32)i960_f64_to_u32(neg03);

            i960_st_u32(I960_WORKRAM, 0x20a91c, 0, nv);
            i960_st_u32(I960_WORKRAM, 0x20a918, 0, nv);
            i960_st_u32(I960_WORKRAM, 0x20a914, 0, nv);
            i960_st_u32(I960_WORKRAM, 0x20a910, 0, nv);
        }
        i960_st_u32(I960_WORKRAM, 0x20a8ec, 0, rot);
        geo_attract_object_pen(obj_ptr, 0x3f800000u, 0x1869fu);

        /* Slot2 setup: neg03 = obj4-0.3; pen = (pen&~0xF)|1; rotation = -phase. */
        obj4 = i960_ld_u32(I960_WORKRAM, 0x20a8d4, 0);
        pen = i960_ld_u8(I960_WORKRAM, 0x20a920, 0);
        phase = i960_ld_u32(I960_WORKRAM, 0x20a8c8, 0);
        neg03 = i960_u32_to_f64(obj4) - i960_rifl_read(0x33333333u, 0x3fd33333u);
        i960_st_u32(I960_WORKRAM, 0x202278, 0, 2u);
        rot = phase ^ 0x80000000u;
        pen = (pen & ~0xfu) | 1u;
        i960_st_u8(I960_WORKRAM, 0x20a920, 0, (u8)pen);
        geo_window_push3(*(u32 *)(fp + 0x58), *(u32 *)(fp + 0x5c), *(u32 *)(fp + 0x60));
        i960_st_u32(I960_WORKRAM, 0x20a8ec, 0, rot);
        {
            u32 nv = (u32)i960_f64_to_u32(neg03);

            i960_st_u32(I960_WORKRAM, 0x20a91c, 0, nv);
            i960_st_u32(I960_WORKRAM, 0x20a918, 0, nv);
            i960_st_u32(I960_WORKRAM, 0x20a914, 0, nv);
            i960_st_u32(I960_WORKRAM, 0x20a910, 0, nv);
        }
        geo_attract_object_pen(obj_ptr, 0x3f800000u, 0x1869fu);

        /* Slot3 setup: neg03 = obj4-0.3; pen = (pen&~0xF)|1; rotation = phase+pi. */
        obj4 = i960_ld_u32(I960_WORKRAM, 0x20a8d4, 0);
        pen = i960_ld_u8(I960_WORKRAM, 0x20a920, 0);
        phase = i960_ld_u32(I960_WORKRAM, 0x20a8c8, 0);
        neg03 = i960_u32_to_f64(obj4) - i960_rifl_read(0x33333333u, 0x3fd33333u);
        i960_st_u32(I960_WORKRAM, 0x202278, 0, 3u);
        pen &= ~0xfu;
        rot = (u32)i960_f64_to_u32(i960_u32_to_f64(phase)
                                    + i960_rifl_read(0x54442d18u, 0x400921fbu));
        pen |= 1u;
        i960_st_u8(I960_WORKRAM, 0x20a920, 0, (u8)pen);
        geo_window_push3(*(u32 *)(fp + 0x64), *(u32 *)(fp + 0x68), *(u32 *)(fp + 0x6c));
        {
            u32 nv = (u32)i960_f64_to_u32(neg03);

            i960_st_u32(I960_WORKRAM, 0x20a91c, 0, nv);
            i960_st_u32(I960_WORKRAM, 0x20a918, 0, nv);
            i960_st_u32(I960_WORKRAM, 0x20a914, 0, nv);
            i960_st_u32(I960_WORKRAM, 0x20a910, 0, nv);
        }
        i960_st_u32(I960_WORKRAM, 0x20a8ec, 0, rot);
        geo_attract_object_pen(obj_ptr, 0x3f800000u, 0x1869fu);

        /* @0x15F68-0x15FA0: advance the spin phase accumulator for next frame. */
        phase = i960_ld_u32(I960_WORKRAM, 0x20a8c8, 0);
        {
            double next = i960_u32_to_f64(phase)
                + i960_rifl_read(0x47ae147bu, 0x3f847ae1u); /* +0.01 */

            i960_st_u32(I960_WORKRAM, 0x202278, 0, 0);
            i960_st_u32(I960_WORKRAM, 0x20a8c8, 0, (u32)i960_f64_to_u32(next));
        }
    }

    fp = fp_save;
}

void game_start_course_select_frame(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    gate = game_start_select_timer_gate();

    /* @0x15398-0x15408: gate==2 -> wheel/pedal drives course selection. */
    if (gate == 2u) {
        u32 handle = i960_ld_u8(I960_WORKRAM, 0x202051, 0);
        int arm = 0;

        if ((handle & 0xffu) <= 0xafu)
            i960_st_u32(I960_WORKRAM, 0x20a8c0, 0, 1u);

        if (i960_ld_u32(I960_WORKRAM, 0x20a8c0, 0) != 0u) {
            handle = i960_ld_u8(I960_WORKRAM, 0x202051, 0);
            if ((handle & 0xffu) > 0xb0u)
                arm = 1;
        }
        if (!arm) {
            u32 flags64 = i960_ld_u8(I960_WORKRAM, 0x202064, 0);

            if ((flags64 >> 4) & 1u)
                arm = 1;
        }
        if (arm)
            game_start_select_timer_arm();

        /* @0x153F4-0x15408: refresh 0x20a8c4 via scene_lookup(4, choice). */
        {
            u32 choice2 = i960_ld_u32(I960_WORKRAM, 0x20a8c4, 0);

            g0 = 4;
            g0 = scene_lookup_fn((u32)g0, choice2);
            i960_st_u32(I960_WORKRAM, 0x20a8c4, 0, (u32)g0);
        }
    }

    /* @0x1540C: always redraw the course-select icons at (52, 2). */
    game_start_select_icons(52u, 2u, 0);
    game_start_course_select_geo_emit();
}
