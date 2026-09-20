/* Desert race list-node per-frame @ 0x427A0 — staged as 0x5E17A0 after init.
 *
 * Private host frame (lda 0xa0(sp),sp). Node VA in r6. Advances track index,
 * pose_step, TGP rotate, GEO 0x05 binds + catalogs @ 0x28657c0/7a0/790/7b0,
 * pen push, optional RNG jitter (skipped → inactive 41ca0 path), obj near-flag,
 * slot_flag.
 *
 * source: disasm/maincpu/maincpu_0427a0_c00.asm */
// @rom 0x427a0 +0xc10 game_start_race_desert_node_frame

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_hw.h"
#include "model2_rom.h"
#include "model2_memory.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

static u32 frame_u32(u8 *frame, u32 off)
{
    u32 v;

    memcpy(&v, frame + off, 4);
    return v;
}

static void frame_st32(u8 *frame, u32 off, u32 v)
{
    memcpy(frame + off, &v, 4);
}

static void frame_st64(u8 *frame, u32 off, u32 lo, u32 hi)
{
    frame_st32(frame, off, lo);
    frame_st32(frame, off + 4u, hi);
}

static i32 ld_s16(u32 ea)
{
    return (i32)(signed short)(u16)i960_ld_u16(I960_ABS, ea, 0);
}

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
    memcpy(&w0, p + 0, 4);
    memcpy(&w1, p + 4, 4);
    memcpy(&w2, p + 8, 4);
    memcpy(&w3, p + 12, 4);
    geo_bank_poke0();
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
    return w3;
}

static void geo_wr_slot_bind_05(void)
{
    u32 slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    u32 wr = i960_mmio_read_u32(0x802008);

    i960_st_u32(I960_WORKRAM, 0x20a290, 0, slot + 4u);
    if (slot != 0)
        i960_st_u32(I960_WORKRAM, slot, 0, wr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_mmio_write_u32(0x801008, wr + 0x34u);
    geo_bank_poke0();
}

void game_start_race_desert_node_frame(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0xc0];
    u32 node;
    u32 table;
    u32 view;
    u32 idx_ptr;
    i32 cur_s;
    i32 view_s;
    i32 lim_s;
    u32 t_bits;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_desert_node_frame\n");
        fflush(stderr);
        logged = 1;
    }

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0xa0;
    g14 = 0;
    node = arg0;

    /*
     * Catalogs @ 0x42E4C+ do 0x20 onto the current matrix then 0x05. Prior
     * list nodes / copro may have dirtied TGP; re-assert the latched cam
     * view so GEO 0x0B is view×object (not a stray car-local parent).
     */
    model2_hw_restore_latched_view_matrix();

    /*
     * @0x427B4–0x427F0: pick view base into 0x216a54.
     *   cmpibe 0,mode(0x202230) → 0x213b40 (championship)
     *   else cmpibl 0,timer → *0x213b00 when timer < 0 (practice)
     *   else fallthrough → 0x213b40
     * Practice is mode==1; *0x213b00 is the rank-sorted pose from
     * sort_index/rank_fill (not a hard-coded car retarget).
     */
    {
        u32 mode = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
        u32 timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);

        if (mode == 0u || (i32)timer >= 0) {
            i960_st_u32(I960_WORKRAM, 0x216a54, 0, 0x213b40u);
        } else {
            i960_st_u32(I960_WORKRAM, 0x216a54, 0,
                        i960_ld_u32(I960_WORKRAM, 0x213b00, 0));
        }
        {
            static int view_logged;

            if (!view_logged) {
                lift_log(
                        "lift: desert_view mode=%u timer=%d base=%#x "
                        "(213b00=%#x)\n",
                        (unsigned)mode, (int)(i32)timer,
                        (unsigned)i960_ld_u32(I960_WORKRAM, 0x216a54, 0),
                        (unsigned)i960_ld_u32(I960_WORKRAM, 0x213b00, 0));
                fflush(stderr);
                view_logged = 1;
            }
        }
    }

    /* @0x427F4–0x42854: TGP 0x58 with view/node angles → 0xb0(fp). */
    view = i960_ld_u32(I960_WORKRAM, 0x216a54, 0);
    {
        u32 v0 = i960_ld_u32(I960_ABS, view, 0);
        u32 v1 = i960_ld_u32(I960_ABS, view, 4);
        u32 n_yaw = i960_ld_u32(I960_ABS, node, 0x18);
        u32 n_roll = i960_ld_u32(I960_ABS, node, 0x20);

        i960_mmio_write_u32(0x884000, 0x2c005858u);
        i960_mmio_write_u32(0x884000, v0);
        i960_mmio_write_u32(0x884000, n_yaw);
        i960_mmio_write_u32(0x884000, (u32)g14);
        i960_mmio_write_u32(0x884000, (u32)g14);
        i960_mmio_write_u32(0x884000, v1);
        i960_mmio_write_u32(0x884000, n_roll);
        frame_st32(frame, 0xb0, i960_mmio_read_u32(0x884000));
    }

    table = game_start_race_desert_table_pick(0, 0, 0);
    /* lda 0x54(r6),g6 — address of halfword index, not a loaded pointer. */
    idx_ptr = node + 0x54u;
    cur_s = ld_s16(idx_ptr);
    view = i960_ld_u32(I960_WORKRAM, 0x216a54, 0);
    {
        i32 scaled = cur_s + (cur_s << 2); /* g4*5 */

        view_s = ld_s16(view + 0x56u);
        lim_s = ld_s16(table + (u32)(scaled << 2));
    }

    /* @0x4287C–0x428A8: advance index / copy pose when view_s < lim. */
    if (view_s < lim_s) {
        /* ld 0x10(g0): g0 is table from table_pick, not the node. */
        u32 src = i960_ld_u32(I960_ABS, table, 0x10);

        i960_st_u32(I960_ABS, node, 0x14, (u32)ld_s16(view + 0x54u));
        i960_st_u32(I960_ABS, node, 0x18, i960_ld_u32(I960_ABS, src, 0));
        i960_st_u32(I960_ABS, node, 0x1c, i960_ld_u32(I960_ABS, src, 4));
        i960_st_u32(I960_ABS, node, 0x5c, (u32)g14);
        i960_st_u32(I960_ABS, node, 0x20, i960_ld_u32(I960_ABS, src, 8));
        {
            u16 h = (u16)i960_ld_u16(I960_ABS, idx_ptr, 0);

            i960_st_u16(I960_ABS, idx_ptr, 0, (u16)(h + 1u));
        }
    }

    t_bits = i960_ld_u32(I960_ABS, node, 0x5c);
    {
        i32 t_i = (i32)(signed short)(u16)i960_ld_u16(I960_ABS, table, 2);
        double t_lim = (double)t_i;

        if (i960_u32_to_f64(t_bits) >= t_lim)
            goto inactive_bit;
    }

    /* Active: set node+2 bit1, near_pick, pose_step. */
    {
        u32 b = i960_ld_u8(I960_ABS, node + 2u, 0);

        i960_st_u8(I960_ABS, node + 2u, 0, (u8)(b | 2u));
    }
    {
        u32 idx = i960_ld_u32(I960_ABS, node, 0x14);
        u32 best = game_start_race_desert_near_pick(node + 0x18u, idx, 0);

        i960_st_u32(I960_ABS, node, 0x14, best);
    }
    game_start_race_desert_pose_step(node, 0, 0);

    /* @0x428EC–0x42A14: TGP rotate from node angles + 0x42/0x48 → 0x30/34/38. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x28));
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x68));
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x6c));
    i960_mmio_write_u32(0x884000, 0x21004242u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x74));
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, 0x24004848u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    /* Disasm @ 0x429B8/0x429C4: shro 1 of 0x21004242 → two 0x21 pops. */
    i960_mmio_write_u32(0x884000, 0x10802121u);
    i960_mmio_write_u32(0x884000, 0x10802121u);
    i960_mmio_write_u32(0x884000, 0x21804343u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_st_u32(I960_ABS, node, 0x30, i960_mmio_read_u32(0x884000));
    i960_st_u32(I960_ABS, node, 0x34, i960_mmio_read_u32(0x884000));
    i960_st_u32(I960_ABS, node, 0x38, i960_mmio_read_u32(0x884000));

    /* @0x42A14–0x42B34: view_table_index_a/b + radius/falloff floats. */
    {
        u32 course = i960_host_race_course_index();

        g0 = course;
        g1 = i960_ld_u32(I960_ABS, node, 0x14);
        g2 = (uintptr_t)(node + 0x18u);
        g3 = 0;
        geo_view_table_index_a((void *)(uintptr_t)g0, (u32)g1, (void *)(uintptr_t)g2);

        {
            u32 a = i960_ld_u32(I960_ABS, node, 0x18);
            u32 b = i960_ld_u32(I960_ABS, node, 0x30);

            i960_st_u32(I960_ABS, node, 0x18,
                        (u32)i960_f64_to_u32(i960_u32_to_f64(a) + i960_u32_to_f64(b)));
        }
        {
            u32 a = i960_ld_u32(I960_ABS, node, 0x20);
            u32 b = i960_ld_u32(I960_ABS, node, 0x38);

            i960_st_u32(I960_ABS, node, 0x20,
                        (u32)i960_f64_to_u32(i960_u32_to_f64(a) + i960_u32_to_f64(b)));
        }
        i960_st_u32(I960_ABS, node, 0x24, i960_ld_u32(I960_ABS, node, 0x6c));
        i960_st_u32(I960_ABS, node, 0x2c, i960_ld_u32(I960_ABS, node, 0x68));

        {
            u32 r = i960_ld_u32(I960_ABS, node, 0x60);
            double rd = i960_u32_to_f64(r) + i960_rifl_read(0, 0x3fe80000u);

            i960_st_u32(I960_ABS, node, 0x60, (u32)i960_f64_to_u32(rd));
        }
        {
            /* @0x42A94–0x42B30: 0x64 += const; beam = 0xb0(fp)*k;
             * 0x5c += tab1 − tab1*beam  (disasm mul/sub/add, no shortcuts). */
            u32 r = i960_ld_u32(I960_ABS, node, 0x64);
            double rd = i960_u32_to_f64(r)
                + i960_rifl_read(0x88888888u, 0x40028888u);
            u32 tab1 = i960_ld_u32(I960_ABS, table, 4);
            double k = i960_rifl_read(0xe434a9b1u, 0x3f4767dcu);
            double beam = i960_u32_to_f64(frame_u32(frame, 0xb0)) * k;
            double t1 = i960_u32_to_f64(tab1);
            double tcur = i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x5c));
            double delta = t1 - (t1 * beam);

            i960_st_u32(I960_ABS, node, 0x64, (u32)i960_f64_to_u32(rd));
            i960_st_u32(I960_ABS, node, 0x5c,
                        (u32)i960_f64_to_u32(tcur + delta));
        }

        g0 = node + 0x18u;
        g1 = (uintptr_t)(fp + 0x40);
        g2 = 0;
        geo_view_table_index_b((void *)(uintptr_t)g0, (void *)(uintptr_t)g1, (u32)g2);
    }

    /*
     * table_index_b plane path: disasm bare 0x2f; host wraps push→0x2f→pop
     * so look-along R is not left live. Still re-assert latched cam view
     * before catalogs @ 0x42E4C (0x20/translate/yaw/0x05) in case prior
     * list/copro dirtied TGP — compose must stay view×Ry(node+0x28).
     */
    model2_hw_restore_latched_view_matrix();

    /* @0x42B38–0x42E4C: optional close-up scale/catalog when 0x5c(fp) small. */
    {
        u32 gate = frame_u32(frame, 0x5c);

        if ((i32)gate <= 14) {
            double y = i960_u32_to_f64(frame_u32(frame, 0x58));
            double lim = i960_rifl_read(0, 0xc02e0000u); /* -15.0 */

            if (y > lim) {
                u32 sx = frame_u32(frame, 0x50);
                u32 sy = frame_u32(frame, 0x4c);
                u32 sz = frame_u32(frame, 0x54);
                u32 d0, d1;
                u32 height = frame_u32(frame, 0x58);

                i960_mmio_write_u32(0x884000, 0x10002020u);
                i960_mmio_write_u32(0x884000, 0x1f003e3eu);
                i960_mmio_write_u32(0x884000, sx);
                i960_mmio_write_u32(0x884000, sy);
                d0 = i960_mmio_read_u32(0x884000);
                i960_mmio_write_u32(0x884000, 0x1f003e3eu);
                i960_mmio_write_u32(0x884000, sx);
                i960_mmio_write_u32(0x884000, sz);
                d1 = i960_mmio_read_u32(0x884000);

                {
                    /*
                     * Disasm @ 0x42B5C/0x42BCC/0x42BEC: fp1 keeps height from
                     * 0x58 through the ble; yoff = (ny + height) + 0x3f9eb851.
                     * Translate payload order @ 0x42C18/30/40: *(node+0x18),
                     * yoff, *(node+0x20).
                     */
                    u32 ny = i960_ld_u32(I960_ABS, node, 0x1c);
                    u32 tx = i960_ld_u32(I960_ABS, node, 0x18);
                    u32 tz = i960_ld_u32(I960_ABS, node, 0x20);
                    double yoff =
                        (i960_u32_to_f64(ny) + i960_u32_to_f64(height))
                        + i960_rifl_read(0xeb851eb8u, 0x3f9eb851u);

                    i960_mmio_write_u32(0x884000, 0x13802727u);
                    i960_mmio_write_u32(0x884000, tx);
                    i960_mmio_write_u32(0x884000, (u32)i960_f64_to_u32(yoff));
                    i960_mmio_write_u32(0x884000, tz);
                }

                {
                    double k = i960_rifl_read(0x47ae147bu, 0x3f847ae1u);
                    double halfish = i960_rifl_read(0x9999999au, 0x3fe99999u);
                    double sc = i960_u32_to_f64(frame_u32(frame, 0x58)) * k;
                    u32 sc_bits;

                    sc = halfish - sc;
                    sc_bits = (u32)i960_f64_to_u32(sc);

                    i960_mmio_write_u32(0x884000, 0x15002a2au);
                    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x28));
                    i960_mmio_write_u32(0x884000, 0x1a003434u);
                    i960_mmio_write_u32(0x884000, (d0 << 16) >> 16);
                    i960_mmio_write_u32(0x884000, 0x19003232u);
                    i960_mmio_write_u32(0x884000, (d1 << 16) >> 16);
                    i960_mmio_write_u32(0x884000, 0x14002828u);
                    i960_mmio_write_u32(0x884000, sc_bits);
                    i960_mmio_write_u32(0x884000, 0x3f800000u);
                    i960_mmio_write_u32(0x884000, sc_bits);
                    geo_wr_slot_bind_05();
                    {
                        u32 count = prg_stq_main_data(0x028657c0u);

                        i960_st_u32(I960_WORKRAM, 0x20b940, 0,
                                    i960_ld_u32(I960_WORKRAM, 0x20b940, 0) + count);
                    }
                    i960_mmio_write_u32(0x884000, 0x15002a2au);
                    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x64));
                    geo_wr_slot_bind_05();
                    {
                        u32 count = prg_stq_main_data(0x028657a0u);

                        i960_st_u32(I960_WORKRAM, 0x20b940, 0,
                                    i960_ld_u32(I960_WORKRAM, 0x20b940, 0) + count);
                    }
                    i960_mmio_write_u32(0x884000, 0x10802121u);
                }
            }
        }
    }

    /* @0x42E4C–0x431E0: main translate/rotate + catalogs 790/7a0/7b0 + pen. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x18));
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x1c));
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x20));
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x28));
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x2c));
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x24));
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, 0xc0200000u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    geo_wr_slot_bind_05();
    {
        u32 count = prg_stq_main_data(0x02865790u);

        i960_st_u32(I960_WORKRAM, 0x20b940, 0,
                    i960_ld_u32(I960_WORKRAM, 0x20b940, 0) + count);
    }
    g3 = 10;
    geo_attract_pen_prg_push_b((uintptr_t)(node + 0x18u),
                               (uintptr_t)(node + 0x24u),
                               0x20220cu);
    i960_mmio_write_u32(0x884000, 0x10802121u);
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x60));
    geo_wr_slot_bind_05();
    {
        u32 count = prg_stq_main_data(0x028657a0u);

        i960_st_u32(I960_WORKRAM, 0x20b940, 0,
                    i960_ld_u32(I960_WORKRAM, 0x20b940, 0) + count);
    }
    i960_mmio_write_u32(0x884000, 0x10802121u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, 0xbf933333u);
    i960_mmio_write_u32(0x884000, 0xc0c00000u);
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, node, 0x64));
    geo_wr_slot_bind_05();
    {
        u32 count = prg_stq_main_data(0x028657b0u);

        i960_st_u32(I960_WORKRAM, 0x20b940, 0,
                    i960_ld_u32(I960_WORKRAM, 0x20b940, 0) + count);
    }
    i960_mmio_write_u32(0x884000, 0x10802121u);

    /*
     * @0x431E0 gate on 0x5c(fp): cmpib* lit,reg as lit ? reg → rng for
     * {2,3,6,7}. Else fall to 0x43320.
     */
    {
        i32 gate = (i32)frame_u32(frame, 0x5c);

        if (gate == 2 || gate == 3 || gate == 6 || gate == 7) {
            u32 r0 = geo_rng_u8(0, 0, 0);
            double span = i960_rifl_read(0, 0x40dfffc0u);
            double k = i960_rifl_read(0x39ffd60fu, 0x3f1797ccu);
            double dx = ((double)(r0 & 0xffffu) - span) * k;
            u32 x = (u32)i960_f64_to_u32(
                i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x18)) + dx);
            u32 y = (u32)i960_f64_to_u32(
                i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x1c))
                + i960_u32_to_f64(frame_u32(frame, 0x58))
                + i960_rifl_read(0x47ae147bu, 0x3f847ae1u));
            u32 r1 = geo_rng_u8(0, 0, 0);
            double dz = ((double)(r1 & 0xffffu) - span) * k;
            u32 z = (u32)i960_f64_to_u32(
                i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x20)) + dz);
            u32 scale = frame_u32(frame, 0x58);

            /* Negate scale for g1 (cpysre → −|0x58(fp)|). */
            scale ^= 0x80000000u;
            frame_st32(frame, 0x70, x);
            frame_st32(frame, 0x74, y);
            frame_st32(frame, 0x78, z);
            /* Guest scratch — avoid truncating host frame pointers on arm64. */
            i960_st_u32(I960_WORKRAM, 0x216a70, 0, x);
            i960_st_u32(I960_WORKRAM, 0x216a74, 0, y);
            i960_st_u32(I960_WORKRAM, 0x216a78, 0, z);
            game_start_race_desert_rng_jitter(0x216a70u, scale, 0);
        }
    }
    game_start_race_desert_tgp55_walk(0, 0, 0);
    game_start_race_desert_obj_near_flag(node, 0, 0);
    goto slot_tail;

inactive_bit:
    {
        u32 b = i960_ld_u8(I960_ABS, node + 2u, 0);

        i960_st_u8(I960_ABS, node + 2u, 0, (u8)(b & ~2u));
    }

slot_tail:
    /* @0x43340–0x433B0: slot_flag with rank 3 or −1. */
    {
        u32 flags = i960_ld_u8(I960_ABS, node, 2);

        if (flags & 2u) {
            u32 tcur = i960_ld_u32(I960_ABS, node, 0x5c);
            u32 lo = i960_ld_u32(I960_ABS, table, 8);
            u32 hi = i960_ld_u32(I960_ABS, table, 0xc);

            if (i960_u32_to_f64(tcur) >= i960_u32_to_f64(lo)
                && i960_u32_to_f64(tcur) < i960_u32_to_f64(hi)) {
                g3 = 3;
                game_start_race_desert_slot_flag(node + 0x18u, node + 0x24u, 0);
            } else {
                g3 = (u32)(0u - 1u);
                game_start_race_desert_slot_flag(node + 0x18u, node + 0x24u, 0);
            }
        } else {
            g3 = (u32)(0u - 1u);
            game_start_race_desert_slot_flag(node + 0x18u, node + 0x24u, 0);
        }
    }

    (void)frame_st64;
    fp = fp_save;
    sp = sp_save;
}
