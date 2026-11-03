/* The race's sub-states around the race body (include/race/race_phases.h).
 *
 * 0x2020a8 counts the race through table 0x1CA10 (index 0x2020a8 >> 1; an even value is a slot's first tick):
 *   0     slot0  setup: checkpoint pairs, time table, lap banks, pose ring, start gate (one tick)
 *   2/3   slot1  bonus time blend (61 ticks)
 *   4/5   slot2  countdown: HUD PRG, wait 166 ticks, the start lights PRG, digits 1-2-3, GO (0x214120 = 1)
 *   6/7   slot3  the race (race_flow.c)
 *   8/9   slot4  course complete, after the finish line
 *   10/11 slot5  time up
 * The fanfare PRG handlers slot4/slot5 install count 0x20ab60 and leave the race by advancing the scene table
 * 0x2020ac after 480 ticks.
 *
 * Checked by the scripted practice drives (diff_check.sh): slot0, slot1, slot2 (solo), slot5 and the practice
 * time-up fanfare in every drive; slot4 and the practice course-complete fanfare with the autopilot's lap
 * (docs/dreamcast-port.md). Not reached in practice, so unchecked: the linked race's peer barriers (0x20a581 > 0)
 * and camera rebind (0x20a560 > 0), the championship fanfares and car-class branches (0x202230 == 0), the
 * keyframe single-sample mode 3. */
#include "race/race_phases.h"
#include "race/course.h"
#include "race/course_hooks.h"
#include "race/camera.h"
#include "race/fp.h"
#include "race/hud.h"
#include "race/phase_hooks.h"
#include "race/race_flow.h"
#include "race/sound_cmd.h"
#include "sr_mem.h"
#include "model2_tgp.h"
#include "sr_tgp.h"

#define RP_MODE         0x202230u /* 0 championship, 1 practice */
#define RP_CAR          0x214354u /* car class (the course in a championship) */
#define RP_GO_STATE     0x214120u /* 0 before GO, 1 racing, 2 over */
#define RP_HUD_MODE     0x20a564u
#define RP_ALT_VIEW     0x2020a4u
#define RP_PITCH        0x20ab5cu
#define RP_PEER_ROWS    0x1a121dcu
#define RP_SLOT1_TICKS  0x2021f4u
#define RP_SLOT1_SNAP   0x20aad8u
#define RP_SLOT2_PHASE  0x20aadcu
#define RP_SLOT2_WAIT   0x20aae0u /* float: ticks before the start lights */
#define RP_SLOT4_PHASE  0x20aae4u
#define RP_SLOT5_PHASE  0x20aae8u
#define RP_FINISH_CLOCK 0x20aea8u
#define RP_PRG_PAIR     0x202270u /* two GEO PRG words the slot pushes after the handler */
#define RP_PRG_COUNT    0x20b0b8u
#define RP_BONUS_LIST   0x20ab70u /* bonus word pointers, -1 terminated (race_flow.c: RF_HANDLERS) */

/* ---- small callees ---- */

/* 0x47330 copy_pair_list: (a, b) pairs from two ROM tables until the next a is negative. */
static void copy_pairs(uint32_t *dst_a, uint32_t *dst_b, uint32_t src_a, uint32_t src_b) {
    uint32_t a = *dst_a, b = *dst_b, peek;
    do {
        sr_st32(a, sr_rom_mirror32(src_a));
        sr_st32(b, sr_rom_mirror32(src_b));
        src_a += 4;
        peek = sr_rom_mirror32(src_a);
        src_b += 4;
        a += 8;
        b += 8;
    } while((int32_t)peek >= 0);
    *dst_a = a;
    *dst_b = b;
}

void sr_rank_list_seed(uint32_t dest) {
    uint32_t course, view, rows, tab_a, tab_b, off, a, b, i;

    if(dest == 0) return;
    course = sr_race_course_index();
    view = (sr_ld8(0x20201du) << 5) & 0x60u;
    if(sr_ld32(RP_MODE) == 0) {
        rows = sr_rom_mirror32(0x5e5570u + view + course * 4);
        tab_b = sr_rom_mirror32(0x5e5cf0u + course * 4);
    } else {
        rows = sr_rom_mirror32(0x5e55f0u + view + course * 4);
        tab_b = sr_rom_mirror32(0x5e6310u + course * 4);
    }
    sr_st32(RF_LAPS, rows);
    a = dest;
    b = dest + 4;
    tab_a = 0x5e5670u + ((course & 3u) << 5);
    off = (sr_ld8(0x20201cu) << 2) & 12u;
    copy_pairs(&a, &b, tab_a, tab_b + (off + rows * 16u) * 4u);
    for(i = 1; (int32_t)i < (int32_t)rows; i++) copy_pairs(&a, &b, tab_a, tab_b + ((sr_ld8(0x20201cu) << 4) & 0x30u));
    sr_st32(a, 0);
    sr_st32(a + 4, (uint32_t)-1);
}

void sr_lap_bank_seed(void) { sr_st32(0x20a9fcu, 16); }

void sr_lap_bank_clear(uint32_t clock) {
    uint32_t i;
    for(i = 0; i < 0x40u; i++) {
        sr_st32(RF_LAP_TIMES + i * 4, 0);
        sr_st32(0x20afb0u + i * 4, 0);
    }
    if(sr_ld32(RP_MODE) == 1 || (int32_t)sr_race_course_index() < 2) sr_lap_bank_seed();
    sr_st32(RF_CLOCK, clock);
    sr_st32(RF_RACE_TIME, 0);
    sr_st32(0x20aea4u, 0);
    sr_st32(0x20aeacu, 0);
}

void sr_pose_ring_init(void) {
    const uint32_t course = sr_race_course_index(), cursor = 0x217260u + (course << 2);
    sr_st32(0x217250u, 0);
    sr_st32(0x217288u, 0);
    sr_st32(0x21728cu, 0);
    sr_st32(0x217254u, course);
    sr_st32(0x217284u, cursor);
    sr_st32(cursor, 0);
    sr_st32(0x217280u, 0x556420u + course * 0x9ab0u);
}

void sr_gate_bind(void) {
    sr_st32(0x20afa4u, 0);
    sr_st32(0x20afa0u, sr_ld32(RP_MODE) == 0 ? 0x5c0d60u : 0x5c0fa0u);
}

void sr_hud_row_seed(void) {
    const int wide = sr_ld32(0x2139d4u) != 0;
    sr_st32(0x20aeb8u, (uint32_t)-1);
    sr_st32(0x20aebcu, wide ? 50 : 27);
    sr_st32(0x20aec0u, wide ? 40 : 9);
}

void sr_race_sound_mute(void) {
    sr_sound_cmd_fill(0xad, 0);
    sr_sound_cmd_fill(0xaf, 0);
    sr_sound_cmd_fill(0xb5, 0);
    sr_sound_cmd_fill(0xb8, 0);
}

void sr_race_idle_wheel(void) {
    if(!sr_practice_skip || sr_ld32(0x202098u) != 3 || (sr_ld32(0x20209cu) & 15u) != 7 ||
       (sr_ld32(RP_SCENE) & 15u) < 4 || (int32_t)sr_ld32(RP_GO_STATE) > 0)
        return;
    sr_hook_analog_idle(1);
    sr_st8(0x202050u, 0x80);
    sr_st8(0x202051u, 0);
    sr_st8(0x202052u, 0);
}

/* Linked race: 1 when every peer row with our timer is in state 7 or 8 (slot4 / slot5 at 0x1D9B8). */
static int peers_finished(void) {
    const int32_t peers = (int8_t)sr_ld8(0x20a581u);
    int32_t matched = 0, i;
    const uint32_t timer = sr_ld32(RF_NET_TIMER);
    for(i = 0; i < peers; i++) {
        const uint32_t row = RP_PEER_ROWS + (uint32_t)i * RF_COMM_STRIDE;
        if(sr_ld32(row) == timer) {
            const uint32_t kind = sr_ld32(sr_ld32(row - 4));
            if(kind != 8 && kind != 7) break;
            matched++;
        }
    }
    if(matched == peers) return 1;
    return !((int32_t)sr_ld32(RF_NET_TIMER) >= 0 && sr_ld32(0x20a758u) == 0);
}

/* ---- slot 0 (0x1CA70) ---- */

/* The scene text columns of one rank row: x offset from 0x20aad0, record. */
static const struct { int8_t dx; uint8_t glyph; } k_rank_cols[8] = {
    {-2, 0}, {0, 20}, {2, 21}, {3, 20}, {5, 20}, {7, 22}, {8, 20}, {10, 20},
};

void sr_race_slot0(void) {
    uint32_t rows, row, ptr, bit;

    sr_st32(0x20aad0u, 3);
    sr_st32(0x20aad4u, sr_ld32(0x2139d4u) == 0 ? 10 : 8);
    sr_st32(RP_GO_STATE, 0);
    sr_st32(RF_FINISH_FLAG, 0);
    sr_st32(0x20a760u, 0);
    sr_hook_analog_idle(0); /* host keyboard recock (reference only) */

    sr_rank_list_seed(RF_CP_PAIRS);
    if((int32_t)sr_ld32(RF_NET_TIMER) < 0) {
        /* Bonus pointers: the second word of each checkpoint pair until the next pair's is negative. */
        uint32_t n = 0, src = RF_CP_PAIRS + 4, dst = RP_BONUS_LIST;
        if((int32_t)sr_ld32(RF_CP_PAIRS + 4) > 0) {
            for(;;) {
                const uint32_t w0 = sr_ld32(src);
                uint32_t w1;
                src += 8;
                w1 = sr_ld32(src);
                sr_st32(dst, w0);
                dst += 4;
                n++;
                if((int32_t)w1 < 0 || n >= 0x100u) break; /* the reference's runaway cap */
            }
        }
        sr_st32(RP_BONUS_LIST + (n << 2), (uint32_t)-1);
    }

    /* The lap rows of the time table (capped at 64 rows as the reference does). */
    rows = sr_ld32(RF_LAPS);
    if((int32_t)rows > 0x40) rows = 0x40;
    for(row = 0; (int32_t)row < (int32_t)rows; row++) {
        const uint32_t y = sr_ld32(0x20aad4u) + row * 3;
        unsigned c;
        for(c = 0; c < 8; c++)
            sr_draw_scene(sr_ld32(0x20aad0u) + (uint32_t)(int32_t)k_rank_cols[c].dx, y, sr_ld32(0x20ac88u),
                               c == 0 ? row + 11 : k_rank_cols[c].glyph, 0);
    }

    {
        const uint32_t clock = sr_ld32(RF_CP_PAIRS + 4);
        sr_st32(RF_LAP, 0);
        sr_st32(RF_CHECKPOINT, 1);
        sr_lap_bank_clear(clock);
    }
    sr_pose_ring_init();
    /* ROM 0x5ADB0: unlifted, a no-op in the reference. */
    sr_gate_bind();
    sr_sound_cmd(0x9a);
    sr_st32(RF_NEAR_COUNT, sr_obj_scan(0));
    {
        const uint32_t course = sr_race_course_index();
        sr_sound_cmd(sr_rom_mirror32((sr_ld32(RP_MODE) == 0 ? 0x5bb720u : 0x5bb760u) + (course << 3)));
    }
    ptr = sr_ld32(0x2140ccu);
    bit = ptr != 0 ? sr_ld32(ptr + 0x3c) ^ 0x80000000u : 0;
    sr_st32(RP_PITCH, bit);
    sr_st32(RF_SUB, sr_ld32(RF_SUB) + 2);
}

/* ---- slot 1 (0x1CD20) ---- */

void sr_race_slot1(void) {
    uint32_t count;

    if((sr_ld32(RF_SUB) & 1) == 0) {
        sr_st32(RP_SLOT1_TICKS, 0);
        sr_st32(RP_SLOT1_SNAP, sr_ld32(RF_CLOCK));
        sr_st32(0x20aeb4u, sr_ld32(RP_FINISH_CLOCK) != 0); /* 0x1E478 */
        sr_st32(RF_SUB, sr_ld32(RF_SUB) + 1);
        return;
    }
    count = sr_ld32(RP_SLOT1_TICKS);
    if((int32_t)count > 60) {
        sr_st32(RP_FINISH_CLOCK, 0); /* 0x1C7E8 */
        sr_st32(0x20aeb4u, 0);
        sr_hud_row_seed();
        sr_st32(RF_SUB, sr_ld32(RF_SUB) + 1);
    } else if((int32_t)count <= 50) {
        /* Blend the clock toward snapshot + the previous finish clock over 50 ticks; a tick sound per second. */
        const uint32_t step = (sr_ld32(RP_FINISH_CLOCK) * count) / 50u; /* unsigned, as the lift divides */
        const int32_t before = (int32_t)sr_ld32(RF_CLOCK) / 60;
        const uint32_t blended = step + sr_ld32(RP_SLOT1_SNAP);
        sr_st32(RF_CLOCK, blended);
        if((int32_t)blended / 60 != before) sr_sound_cmd(0x94);
    }
    sr_st32(RP_SLOT1_TICKS, sr_ld32(RP_SLOT1_TICKS) + 1);
}

/* ---- slot 2 (0x1CE30) ---- */

static void slot2_next_phase(void) { sr_st32(RP_SLOT2_PHASE, sr_ld32(RP_SLOT2_PHASE) + 1); }

static void slot2_phase1(void) {
    /* Linked race: wait until every peer with our timer is ready (state 3). Same 64-byte stride as the lift. */
    const int32_t need = (int8_t)sr_ld8(0x20a581u);
    int32_t matched = 0;
    if(need > 0) {
        const uint32_t timer = sr_ld32(RF_NET_TIMER);
        uint32_t off = 0;
        while(matched < need) {
            if(sr_ld32(RP_PEER_ROWS + off) == timer && sr_ld32(sr_ld32(0x1a12348u + off)) != 3) break;
            matched++;
            off += 64;
        }
    }
    if(matched != (int8_t)sr_ld8(0x20a581u) && (int32_t)sr_ld32(RF_NET_TIMER) >= 0 && sr_ld32(0x20a758u) == 0)
        return;
    sr_st32(RP_SLOT2_WAIT, sr_ld32(RP_MODE) == 1 || sr_ld32(RP_CAR) == 0 ? 0x43260000u /* 166 */ : 0);
    sr_st32(RP_COUNTDOWN_T, 0);
    slot2_next_phase();
}

void sr_race_slot2(void) {
    const uint32_t sub = sr_ld32(RF_SUB);
    float t;

    if((sub & 1) == 0) {
        sr_st32(RP_SLOT2_PHASE, 0);
        sr_st32(RF_SUB, sub + 1);
    }
    t = sr_f(sr_ld32(RP_COUNTDOWN_T));
    switch(sr_ld32(RP_SLOT2_PHASE)) {
    case 0:
        sr_st32(0x20a760u, 3);
        sr_st32(RP_HUD_MODE, 6);
        sr_race_prg_slot(RP_PRG_HUD);
        sr_st32(RF_NEAR_COUNT, sr_obj_scan(0));
        sr_st32(RP_PITCH, sr_ld32(sr_ld32(RF_OBJ_LIST) + 0x1c) ^ 0x80000000u);
        slot2_next_phase();
        break;
    case 1:
        slot2_phase1();
        break;
    case 2: {
        /* +1 a tick until past the wait (compared before rounding, as the lift does), then the start lights. */
        const double next = D(sr_ld32(RP_COUNTDOWN_T)) + 1.0;
        sr_st32(RP_COUNTDOWN_T, F(next));
        if(next <= D(sr_ld32(RP_SLOT2_WAIT))) break;
        sr_st32(RP_COUNTDOWN_T, 0);
        sr_race_prg_slot(RP_PRG_COUNTDOWN);
        sr_sound_cmd(0);
        slot2_next_phase();
        break;
    }
    case 3: /* digits as the clock passes 30, 60, 90 */
        if(t > 30.0f) {
            sr_sound_cmd(1);
            slot2_next_phase();
        }
        break;
    case 4:
        if(t > 60.0f) {
            sr_sound_cmd(2);
            slot2_next_phase();
        }
        break;
    case 5:
        if(t > 90.0f) {
            const uint32_t car = sr_ld32(RP_CAR);
            sr_sound_cmd(3);
            sr_st32(RP_GO_STATE, 1);
            /* The course music (raw 0x214354 index, as the ROM). */
            if(sr_ld32(RP_MODE) == 0 || (int32_t)sr_ld32(RF_NET_TIMER) < 0)
                sr_sound_cmd(sr_rom_mirror32(0x5bb6e0u + (car << 2)));
            else
                sr_sound_cmd(sr_rom_mirror32(0x5bb700u + (car << 2)));
            sr_sound_cmd(0x46);
            slot2_next_phase();
            sr_st32(RF_SUB, sr_ld32(RF_SUB) + 1);
        }
        break;
    default:
        break;
    }
}

/* ---- slots 4 and 5 (0x1D680, 0x1D8D0) ---- */

/* The result banner (0x288cf60, layer 19): 0/1 in time (by 0x20ac70), `solo` in a solo race. */
static void result_banner(uint32_t solo) {
    uint32_t glyph;
    if((int32_t)sr_ld32(RF_NET_TIMER) < 0) glyph = solo;
    else glyph = sr_ld32(0x20ac70u) != 0 ? 0 : 1;
    sr_hook_catalog_draw(0, 19, 0x0288cf60u, glyph, 0);
}

void sr_race_slot4(void) {
    uint32_t phase;

    if((sr_ld8(RF_SUB) & 1) == 0) sr_st32(RP_SLOT4_PHASE, 0);
    phase = sr_ld32(RP_SLOT4_PHASE);
    if(phase == 0) {
        sr_tile_banks_clear();
        if((int32_t)sr_ld32(RF_NET_TIMER) > 0) sr_cam_bind(SR_CAM_SENTINEL, 0); /* linked race only: unchecked */
        sr_st32(RP_ALT_VIEW, 0);
        /* ROM 0x1EBD0: unlifted, a no-op in the reference. */
        sr_race_sound_mute();
        sr_st32(RP_GO_STATE, 2);
        sr_st32(RP_FANFARE_T, 0);
        sr_st32(RP_HUD_MODE, 7);
        result_banner(2);
        sr_race_prg_slot(RP_PRG_FINISH);
        sr_st32(RP_SLOT4_PHASE, sr_ld32(RP_SLOT4_PHASE) + 1);
        sr_st32(RF_SUB, sr_ld32(RF_SUB) + 1);
    } else if(phase == 1) {
        int go = 1;
        if(sr_ld32(RP_MODE) == 0) {
            /* Championship: the car class's fanfare (unchecked). */
            const uint32_t car = sr_ld32(RP_CAR);
            if(car == 2) {
                sr_sound_cmd(0x9e);
                sr_race_prg_slot(0x5bfa50u);
            } else if(car == 3) {
                sr_sound_cmd(0xa3);
                sr_sound_cmd(0x42);
                sr_race_prg_slot(0x5bfbf0u);
            } else {
                sr_sound_cmd(0x9e);
                sr_race_prg_slot(0x5bf880u);
            }
        } else if(peers_finished()) {
            sr_sound_cmd(0x9e);
            sr_sound_cmd(0x3f);
            sr_race_prg_slot(0x5bfcf0u);
        } else {
            go = 0;
        }
        if(go) sr_st32(RP_SLOT4_PHASE, sr_ld32(RP_SLOT4_PHASE) + 1);
    }
    if((int32_t)sr_ld32(RF_NET_TIMER) < 0) sr_gate_walk();
}

void sr_race_slot5(void) {
    uint32_t phase;

    if((sr_ld8(RF_SUB) & 1) == 0) {
        sr_st32(RP_SLOT5_PHASE, 0);
        sr_st32(RF_SUB, sr_ld32(RF_SUB) + 1);
    }
    phase = sr_ld32(RP_SLOT5_PHASE);
    if(phase == 0) {
        sr_race_prg_slot(RP_PRG_FINISH);
        sr_tile_banks_clear();
        sr_race_sound_mute();
        sr_st32(RP_ALT_VIEW, 0);
        sr_sound_cmd(0xa3);
        sr_st32(RP_GO_STATE, 2);
        sr_st32(0x20a770u, 0);
        sr_st32(RP_HUD_MODE, 8);
        result_banner(3);
        sr_st32(RP_SLOT5_PHASE, 1);
        return;
    }
    if(phase != 1 || !peers_finished()) return;
    sr_st32(RP_FANFARE_T, 0);
    sr_race_prg_slot(sr_ld32(RP_MODE) == 0 ? 0x5bfff0u : 0x5bfe70u);
    sr_st32(RP_SLOT5_PHASE, 2);
}

/* ---- sub-state reset (0x1C9C0) ---- */

void sr_race_sub_reset(uint32_t link) {
    sr_st32(RF_SUB, 0);
    /* 0x40790: the race CGM colorbases, then the course table for the unlifted companion tick 0x40820. */
    sr_st32(0x215c20u, link);
    sr_st32(0x215c24u, sr_hook_catalog_draw(0, 0, 0x020fe200u, 0, 4));
    sr_hook_catalog_draw(0, 0, 0x0289bc78u, 0, 4);
    sr_hook_catalog_draw(0, 0, 0x0289c0e8u, 0, 4);
    sr_st32(0x215c28u, (uint32_t)-1);
    sr_st32(0x215c30u, sr_rom_mirror32(0x5df770u + (sr_race_course_index() << 2)));
    sr_st32(0x20ab64u, sr_ld32(0x20c97cu)); /* 0x2AB48 */
    sr_st32(0x20ac74u, sr_ld8(0x20201au) == 1 ? 0x3f800000u : 0);
}

/* ---- the race GEO PRG slot (0x21370) and its handlers ---- */

static void fanfare_tick(uint32_t limit, uint32_t advance) {
    const uint32_t t = sr_ld32(RP_FANFARE_T);
    if(t > limit) sr_st32(RP_SCENE, sr_ld32(RP_SCENE) + advance);
    sr_st32(RP_FANFARE_T, t + 1);
}

void sr_fanfare_practice_win(void) { fanfare_tick(480, 1); }
void sr_fanfare_practice_dnf(void) { fanfare_tick(480, 7); }
void sr_fanfare_champ_dnf(void) { fanfare_tick(480, 1); }
void sr_fanfare_champ_win_0_cont(void) { fanfare_tick(480, 1); }
void sr_fanfare_champ_win_2_cont(void) { fanfare_tick(480, 1); }
void sr_fanfare_champ_win_3(void) { fanfare_tick(0x1a4, 1); }

/* At 120 ticks the win fanfares chain to their continuation. */
static void fanfare_chain(uint32_t slot, uint32_t next) {
    const uint32_t t = sr_ld32(RP_FANFARE_T);
    if(t == 120) sr_st32(slot ? slot : RP_PRG_SLOT, next);
    sr_st32(RP_FANFARE_T, t + 1);
}

void sr_fanfare_champ_win_0(uint32_t slot) { fanfare_chain(slot, 0x5bf7c0u); }
void sr_fanfare_champ_win_2(uint32_t slot) { fanfare_chain(slot, 0x5bf990u); }

/* 0x3F4D0: one 7-word keyframe from a table of `limit` frames, at t clamped to [0, limit - 1]. */
static void keyframe_copy(uint32_t t_bits, uint32_t limit, uint32_t src, uint32_t dst) {
    double t = D(t_bits);
    const double lim = D(limit);
    int32_t ix;
    uint32_t i;
    if(t < 0.0) t = 0.0;
    else if(!(t < lim)) t = lim - 1.0;
    ix = (int32_t)t;
    if(ix < 0) ix = 0;
    src += (uint32_t)ix * 28u;
    for(i = 0; i < 7; i++) sr_st32(dst + i * 4, sr_ld32(src + i * 4));
}

#define KF_SCRATCH 0x215b80u /* four samples, 0x20 apart */
#define KF_POSE    0x215c00u

/* 0x3F550: the pose at t, a uniform cubic B-spline through the keyframes around it (mode 3: the keyframe). Word 6
 * (draw gate) comes from the keyframe at floor(t). */
static void keyframe_eval(uint32_t dest, uint32_t src, uint32_t t_bits, uint32_t limit, uint32_t mode) {
    const double t = D(t_bits);
    double fl, u, u2, u3, inv6, w0, w1, w2, w3;
    uint32_t i;

    if(mode == 3) {
        keyframe_copy(t_bits, limit, src, KF_SCRATCH + 0x20);
        for(i = 0; i < 7; i++) sr_st32(dest + i * 4, sr_ld32(KF_SCRATCH + 0x20 + i * 4));
        return;
    }
    fl = (double)(int32_t)t;
    u = t - fl;
    for(i = 0; i < 4; i++) keyframe_copy(F(fl + ((double)i - 1.0)), limit, src, KF_SCRATCH + i * 0x20);
    u2 = u * u;
    u3 = u2 * u;
    inv6 = sr_d(0xc62af21cu, 0x3fc55554u);
    w0 = (1.0 - u) * (1.0 - u) * (1.0 - u) * inv6;
    w1 = (3.0 * u3 - 6.0 * u2 + 4.0) * inv6;
    w2 = (-3.0 * u3 + 3.0 * u2 + 3.0 * u + 1.0) * inv6;
    w3 = u3 * inv6;
    for(i = 0; i < 6; i++) {
        const double a = D(sr_ld32(KF_SCRATCH + i * 4)), b = D(sr_ld32(KF_SCRATCH + 0x20 + i * 4));
        const double c = D(sr_ld32(KF_SCRATCH + 0x40 + i * 4)), d = D(sr_ld32(KF_SCRATCH + 0x60 + i * 4));
        sr_st32(dest + i * 4, F(w0 * a + w1 * b + w2 * c + w3 * d));
    }
    sr_st32(dest + 0x18, sr_ld32(KF_SCRATCH + 0x38));
}

/* The pose at 0x215c00 on the TGP: translate, then the three rotations. */
static void tgp_pose(void) {
    uint32_t w[3] = {sr_ld32(KF_POSE), sr_ld32(KF_POSE + 4), sr_ld32(KF_POSE + 8)};
    sr_tgp_cmd(0x13802727u, w, 3, 0, 0);
    w[0] = sr_ld32(KF_POSE + 0x14);
    sr_tgp_cmd(0x15802b2bu, w, 1, 0, 0);
    w[0] = sr_ld32(KF_POSE + 0x10);
    sr_tgp_cmd(0x15002a2au, w, 1, 0, 0);
    w[0] = sr_ld32(KF_POSE + 0xc);
    sr_tgp_cmd(0x14802929u, w, 1, 0, 0);
}

/* What the countdown drew in its last tick, for the renderer (not RAM: the arcade's GEO side). */
static sr_countdown_draw_t s_cd_draws[8];
static uint32_t s_cd_n, s_cd_tick;

uint32_t sr_countdown_draws(const sr_countdown_draw_t **out) {
    *out = s_cd_draws;
    /* only the draws of the last tick (0x202008 counts the ticks; the countdown runs before the count) */
    return sr_ld32(0x202008u) - s_cd_tick <= 1u ? s_cd_n : 0;
}

/* Bind the current matrix and draw catalog `ix` of the table at 0x2864b40 (as the course's catalogs). */
static void emit_catalog(uint32_t ix) {
    const uint32_t cursor = sr_ld32(0x20a290u), wr = sr_hook_course_write_ptr();
    const uint32_t ea = 0x2864b40u + (ix << 4);
    uint32_t c0, c1, c2, c3;
    if(s_cd_tick != sr_ld32(0x202008u)) s_cd_n = 0, s_cd_tick = sr_ld32(0x202008u);
    if(s_cd_n < 8) {
        s_cd_draws[s_cd_n].oba = sr_ld32(ea + 8);
        model2_tgp_matrix(s_cd_draws[s_cd_n++].m);
    }
    if(cursor != 0) sr_st32(cursor, wr);
    sr_tgp_cmd(0x02800505u, 0, 0, 0, 0);
    sr_hook_course_reg(0x801008u, wr + 0x34u);
    sr_hook_course_bank(sr_ld32(0x202278u) & 3u);
    c0 = sr_ld32(ea);
    c1 = sr_ld32(ea + 4);
    c2 = sr_ld32(ea + 8);
    c3 = sr_ld32(ea + 12);
    if(cursor != 0) sr_st32(0x20a290u, cursor + 4);
    sr_st32(0x20b940u, sr_ld32(0x20b940u) + c3);
    sr_hook_course_catalog(c0, c1, c2, c3);
}

uint32_t sr_countdown_object_step(uint32_t table, uint32_t t, uint32_t step, uint32_t limit, uint32_t mesh_base,
                                  uint32_t mode, uint32_t count) {
    int32_t remain = (int32_t)count - 1;
    uint32_t rec = table;

    /* 0x14-byte records: push depth, kind (0 drawn, 1/2 pose only), ?, catalog, keyframe offset. */
    while(remain != -1) {
        const uint32_t depth = sr_ld32(rec), kind = sr_ld32(rec + 4);
        int32_t pops;
        keyframe_eval(KF_POSE, mesh_base + sr_ld32(rec + 0x10), t, limit, mode);
        if(depth != 0) sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
        if(kind == 0) {
            tgp_pose();
            if(sr_ld32(KF_POSE + 0x18) != 0) emit_catalog(sr_ld32(rec + 0xc));
        } else if(kind == 1 || kind == 2) {
            tgp_pose();
        }
        rec += 0x14;
        pops = remain == 0 ? (int32_t)depth + 1 : (int32_t)depth - (int32_t)sr_ld32(rec) + 1;
        remain--;
        while(pops-- > 0) sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
    }
    {
        const double next = D(t) + D(step), lim = D(limit);
        if(next < lim) return F(next);
        if(mode == 1) return F(lim - 1.0);
        if(mode == 2) return 0;
        return F(next);
    }
}

void sr_countdown_prg(void) {
    uint32_t t;

    sr_hud_frame();
    /* The start lights' GEO window and view. */
    sr_hook_course_reg(0x800030u, 0);
    sr_hook_course_prg(0x80u);
    sr_hook_course_prg(0x01f00200u);
    sr_hook_course_prg(0x00f80140u);
    sr_hook_course_prg(0x00f80140u);
    sr_hook_course_prg(0x00f80140u);
    sr_hook_course_prg(0x00f80140u);
    sr_st32(RP_PRG_PAIR, 0x44000000u);
    sr_st32(RP_PRG_PAIR + 4, 0x44000000u);
    t = sr_ld32(RP_COUNTDOWN_T);
    sr_hook_course_reg(0x800090u, 0);
    sr_hook_course_prg(0x44000000u);
    sr_hook_course_prg(0x44000000u);
    sr_hook_course_reg(0x8000a0u, 0);
    sr_hook_course_prg(0);
    sr_hook_course_prg(0xbf34fdf4u);
    sr_hook_course_prg(0x3f34fdf4u);
    sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
    sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
    /* Five light records at 0x5c0180, keyframes at 0x205ba20, +0.5 a tick up to 120 (held at 119). */
    t = sr_countdown_object_step(0x5c0180u, t, 0x3f000000u, 0x42f00000u, 0x0205ba20u, 1, 5);
    sr_st32(RP_COUNTDOWN_T, t);
    if(sr_f(t) > 95.0f) sr_race_prg_slot(RP_PRG_HUD_GO);
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
}

/* The installed handler, by its staged address. Others (0x5bf790 among them) are not lifted: no-ops in the
 * reference. 0x21340's extra ROM call 0x21430 is unlifted too. */
static void prg_run(uint32_t handler, uint32_t slot) {
    switch(handler) {
    case RP_PRG_HUD:
    case RP_PRG_HUD_GO: sr_hud_frame(); break;
    case RP_PRG_COUNTDOWN: sr_countdown_prg(); break;
    case 0x5bfcf0u: sr_fanfare_practice_win(); break;
    case 0x5bfe70u: sr_fanfare_practice_dnf(); break;
    case 0x5bfff0u: sr_fanfare_champ_dnf(); break;
    case 0x5bf880u: sr_fanfare_champ_win_0(slot); break;
    case 0x5bf7c0u: sr_fanfare_champ_win_0_cont(); break;
    case 0x5bfa50u: sr_fanfare_champ_win_2(slot); break;
    case 0x5bf990u: sr_fanfare_champ_win_2_cont(); break;
    case 0x5bfbf0u: sr_fanfare_champ_win_3(); break;
    default: break;
    }
}

void sr_race_prg_slot(uint32_t handler) {
    uint32_t lo, hi, count, flags;

    if(handler != 0) {
        sr_st32(RP_PRG_SLOT, handler);
        return;
    }
    lo = sr_ld32(RP_PRG_PAIR);
    hi = sr_ld32(RP_PRG_PAIR + 4);
    handler = sr_ld32(RP_PRG_SLOT);
    if(handler != 0) prg_run(handler, RP_PRG_SLOT);
    sr_st32(RP_PRG_PAIR, lo);
    sr_st32(RP_PRG_PAIR + 4, hi);
    count = sr_ld32(RP_PRG_COUNT);
    sr_hook_course_reg(0x800090u, 0);
    sr_hook_course_prg(lo);
    sr_hook_course_prg(hi);
    sr_st32(RP_PRG_COUNT, count + 1);
    flags = sr_ld8(0x20204cu) & ~0x20u;
    if(count & 0x20u) flags |= 0x20u;
    sr_st8(0x20204cu, flags);
}
