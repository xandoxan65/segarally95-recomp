/* Race sub-slot 2 @ 0x1CE30 — post-blend countdown phase machine.
 *
 * After slot1 advances 0x2020a8 to 4, race_frame dispatches table[2].
 * Even first hit: clear phase 0x20aadc and bump 2020a8 odd. Odd path:
 * bx 0x5BBE70[phase] for phases 0..5 (countdown setup → digit palettes),
 * then bump 2020a8 so table[3] runs.
 *
 * Phase clock 0x20ab54 is advanced by countdown PRG @ 0x211F0 (via 0x3F9A0)
 * after phase 2 installs it; thresholds are IEEE doubles 30 / 60 / 85.625.
 *
 * source: disasm/maincpu/maincpu_01ce30_330.asm */
// @rom 0x1ce30 +0x320 game_start_race_slot2

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

static void race_slot2_phase0(void)
{
    u32 obj;
    u32 pitch;
    u32 phase;

    /* @0x1CE88: seed lap/HUD params, install per-frame HUD PRG, scan objs. */
    i960_st_u32(I960_WORKRAM, 0x20a760, 0, 3u);
    i960_st_u32(I960_WORKRAM, 0x20a564, 0, 6u);
    g0 = 0x005c0330u;
    game_start_race_geo_prg_slot((u32)g0, 0, 0);
    g0 = 0;
    game_start_race_obj_scan(0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2020c8, 0, (u32)g0);

    obj = i960_ld_u32(I960_WORKRAM, 0x213980, 0);
    pitch = i960_ld_u32(I960_ABS, obj, 0x1cu);
    pitch ^= (1u << 31);
    i960_st_u32(I960_WORKRAM, 0x20ab5c, 0, pitch);

    phase = i960_ld_u32(I960_WORKRAM, 0x20aadc, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20aadc, 0, phase);
}

static void race_slot2_phase1(void)
{
    i32 need;
    i32 matched;
    u32 timer;
    u32 base;
    u32 stride;
    u32 phase;

    /* @0x1CEEC: peer-ready gate (same shape as select_confirm). */
    need = (i32)(signed char)i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
    matched = 0;
    if (need > 0) {
        timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
        base = 0x01a121dcu;
        stride = 0u;
        while (matched < need) {
            u32 slot_timer = i960_ld_u32(I960_ABS, base, 0);

            /* @0x1CF18–0x1CF28: same timer + kind!=3 → abort scan. */
            if (slot_timer == timer) {
                u32 kind_ptr = i960_ld_u32(I960_ABS, 0x01a12348u + stride, 0);
                u32 kind = i960_ld_u32(I960_ABS, kind_ptr, 0);

                if (kind != 3u)
                    break;
            }
            matched++;
            stride += 64u;
            base += 64u;
        }
    }

    need = (i32)(signed char)i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
    if (matched != need) {
        timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
        /*
         * @0x1CF54: cmpibg 0,timer → 0x1cf64 when timer < 0 (solo −1 advances).
         * Else @0x1CF60: cmpibe 0,flag → ret when flag==0 (hold linked start).
         * Hold only when timer >= 0 and 0x20a758 == 0.
         */
        if ((i32)timer >= 0
            && i960_ld_u32(I960_WORKRAM, 0x20a758, 0) == 0u)
            return;
    }

    {
        static int adv_logged;

        if (!adv_logged) {
            lift_log(
                    "lift: race_slot2 phase1→2 timer=%d need=%d matched=%d flag=%u\n",
                    (int)(i32)i960_ld_u32(I960_WORKRAM, 0x20a560, 0),
                    (int)need, (int)matched,
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x20a758, 0));
            fflush(stderr);
            adv_logged = 1;
        }
    }

    /* @0x1CF64–0x1CFAC: seed countdown span; practice → 166.0f. */
    if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) == 1u
        || i960_host_race_course_index() == 0u)
        i960_st_u32(I960_WORKRAM, 0x20aae0, 0, 0x43260000u); /* 166.0f */
    else
        i960_st_u32(I960_WORKRAM, 0x20aae0, 0, 0u);

    i960_st_u32(I960_WORKRAM, 0x20ab54, 0, 0u);
    phase = i960_ld_u32(I960_WORKRAM, 0x20aadc, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20aadc, 0, phase);
}

static void race_slot2_phase2(void)
{
    double cur;
    double lim;
    u32 phase;

    /* @0x1CFB8: +1.0f per frame until past 0x20aae0, then install countdown PRG. */
    cur = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20ab54, 0)) + 1.0;
    lim = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20aae0, 0));
    i960_st_u32(I960_WORKRAM, 0x20ab54, 0, (u32)i960_f64_to_u32(cur));
    if (cur <= lim)
        return;

    i960_st_u32(I960_WORKRAM, 0x20ab54, 0, 0u);
    g0 = 0x005c01f0u;
    game_start_race_geo_prg_slot((u32)g0, 0, 0);
    g0 = 0;
    comm_palette_index_call(0);
    phase = i960_ld_u32(I960_WORKRAM, 0x20aadc, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20aadc, 0, phase);
    {
        static int cd_logged;

        if (!cd_logged) {
            lift_log( "lift: race_slot2 phase2→3 install countdown PRG\n");
            fflush(stderr);
            cd_logged = 1;
        }
    }
}

static int race_slot2_ab54_gt_double_hi(u32 hi_word)
{
    double cur = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20ab54, 0));
    double lim = i960_rifl_read(0, hi_word);

    return cur > lim;
}

static void race_slot2_phase3(void)
{
    u32 phase;

    /* @0x1D010: ab54 > 30.0 → palette digit 1. */
    if (!race_slot2_ab54_gt_double_hi(0x403e0000u))
        return;
    g0 = 1;
    comm_palette_index_call(1);
    phase = i960_ld_u32(I960_WORKRAM, 0x20aadc, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20aadc, 0, phase);
}

static void race_slot2_phase4(void)
{
    u32 phase;

    /* @0x1D058: ab54 > 60.0 → palette digit 2. */
    if (!race_slot2_ab54_gt_double_hi(0x404e0000u))
        return;
    g0 = 2;
    comm_palette_index_call(2);
    phase = i960_ld_u32(I960_WORKRAM, 0x20aadc, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20aadc, 0, phase);
}

static void race_slot2_phase5(void)
{
    u32 phase;
    u32 sub;
    u32 course;

    /* @0x1D0A0: ab54 > 85.625 → palette 3 + course cue; advance sub-slot. */
    if (!race_slot2_ab54_gt_double_hi(0x40568000u))
        return;

    g0 = 3;
    comm_palette_index_call(3);
    i960_st_u32(I960_WORKRAM, 0x214120, 0, 1u);

    if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) == 0u
        && (i32)i960_ld_u32(I960_WORKRAM, 0x20a560, 0) < 0) {
        course = i960_host_race_course_index();
        g0 = model2_workram_mirror_u32(0x5bb6e0u + (course << 2));
        comm_palette_index_call((u32)g0);
    } else {
        course = i960_host_race_course_index();
        g0 = model2_workram_mirror_u32(0x5bb700u + (course << 2));
        comm_palette_index_call((u32)g0);
    }
    g0 = 0x46u;
    comm_palette_index_call(0x46u);

    phase = i960_ld_u32(I960_WORKRAM, 0x20aadc, 0) + 1u;
    sub = i960_ld_u32(I960_WORKRAM, 0x2020a8, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20aadc, 0, phase);
    i960_st_u32(I960_WORKRAM, 0x2020a8, 0, sub);
}

void game_start_race_slot2(u32 arg0, u32 arg1, u32 arg2)
{
    u32 sub;
    u32 phase;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    sub = i960_ld_u32(I960_WORKRAM, 0x2020a8, 0);
    if (!logged) {
        lift_log( "lift: race_slot2 sub=%u\n", (unsigned)sub);
        fflush(stderr);
        logged = 1;
    }

    /* @0x1CE30: even → clear phase, bump to odd. */
    if ((sub & 1u) == 0u) {
        i960_st_u32(I960_WORKRAM, 0x20aadc, 0, 0u);
        i960_st_u32(I960_WORKRAM, 0x2020a8, 0, sub + 1u);
    }

    phase = i960_ld_u32(I960_WORKRAM, 0x20aadc, 0);
    /* @0x1CE60: phase > 5 → done. */
    if (phase > 5u)
        return;

    switch (phase) {
    case 0:
        race_slot2_phase0();
        break;
    case 1:
        race_slot2_phase1();
        break;
    case 2:
        race_slot2_phase2();
        break;
    case 3:
        race_slot2_phase3();
        break;
    case 4:
        race_slot2_phase4();
        break;
    case 5:
        race_slot2_phase5();
        if (logged == 1) {
            lift_log( "lift: race_slot2 done sub→%u\n",
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x2020a8, 0));
            fflush(stderr);
            logged = 2;
        }
        break;
    default:
        break;
    }
}
