/* The race body after the countdown (0x1D160, race sub-slot 3) and its callees: pose ring, start gate, clock,
 * checkpoints and laps, the finish, lap bests and the record table in backup SRAM.
 *
 * The comm board rows (0x1a121dc, 0x1c0 bytes per peer) are read as in a linked race; in a solo race 0x20a560 is
 * negative and they are skipped. The HUD is race/hud.h; sound goes through sound_cmd.h.
 *
 * Checked by the scripted practice drives (diff_check.sh, 6000 and 20000 ticks): the clock, the time-up window,
 * the start gate, the pose ring and ghost buffer, the near scan with its TGP box test. Not reached there, so
 * unchecked: checkpoints, laps and the finish (no drive passes the first checkpoint before time is up), so
 * lap_best, lap_record and finish_clock; the comm board peer block (0x20a758 is set in practice). */
#include "race/race_flow.h"
#include "race/course.h"
#include "race/fp.h"
#include "race/hud.h"
#include "race/sound_cmd.h"
#include "sr_tgp.h"
#include <math.h>

#define TICKS_PER_SECOND  60u    /* the original's `addo 31,29` */
#define SECTION_TICKS     0x12cu /* lap / finish window in units of 0x2140c4 */

/* ---- pose ring (0x46160) ---- */

#define RING_INDEX   0x217250u /* appends so far */
#define RING_BASE    0x51c400u
#define RING_CAP     0xa8bu
#define GHOST_DEST   0x217280u /* buffer base */
#define GHOST_CURSOR 0x217284u /* address of the record counter */
#define GHOST_ARMED  0x217288u
#define GHOST_GEN    0x21728cu
#define GHOST_PEER   0x217290u
#define GHOST_SPAN_TABLE 0x5e5010u /* span limits per generation, 16 bytes per course (program ROM mirror) */

static void copy_pose(uint32_t dst, uint32_t src) {
    uint32_t i;
    for(i = 0; i < RF_POSE_SIZE; i += 4) sr_st32(dst + i, sr_ld32(src + i));
}

void sr_pose_ring_step(void) {
    const uint32_t ring_ix = sr_ld32(RING_INDEX);
    const int32_t ring_words = (int32_t)ring_ix >> 2;
    uint32_t armed;

    if(!(ring_words > (int32_t)RING_CAP)) {
        copy_pose(RING_BASE + (uint32_t)ring_words * RF_POSE_SIZE, RF_POSES);
        sr_st32(RING_INDEX, ring_ix + 1);
    }

    armed = sr_ld32(GHOST_ARMED);
    if(armed == 0) {
        /* Not armed: arm once the camera span is below this generation's limit and a nearest peer exists. */
        const uint32_t gen = sr_ld32(GHOST_GEN);
        const uint32_t course = sr_race_course_index();
        const int32_t cam_span = (int16_t)sr_ld16(RF_POSES + 0x56);
        const int32_t limit = (int32_t)sr_ld32(GHOST_SPAN_TABLE + (gen << 2) + (course << 4));
        if(cam_span < limit) {
            const int32_t peers = (int32_t)sr_ld32(RF_POSE_COUNT);
            const float cx = sr_f(sr_ld32(RF_POSES)), cz = sr_f(sr_ld32(RF_POSES + 8));
            float best_d = sr_f(0x7f7fffffu);
            uint32_t best_ix = 0, i;
            /* Pose blocks 1.. against block 0 in float, as the lift computes it. */
            for(i = 1; (int32_t)i < peers; i++) {
                const uint32_t pose = RF_POSES + i * RF_POSE_SIZE;
                const float dx = cx - sr_f(sr_ld32(pose)), dz = cz - sr_f(sr_ld32(pose + 8));
                const float dist = sqrtf(dx * dx + dz * dz);
                if(dist < best_d) {
                    best_d = dist;
                    best_ix = i;
                }
            }
            sr_st32(GHOST_PEER, best_ix);
            if(best_ix != 0) armed++;
        }
        sr_st32(GHOST_ARMED, armed);
    }
    if(armed == 0) return;

    {
        const uint32_t cursor = sr_ld32(GHOST_CURSOR);
        const uint32_t gen = sr_ld32(GHOST_GEN);
        const uint32_t best = sr_ld32(GHOST_PEER);
        const uint32_t written = sr_ld32(cursor);
        const uint32_t limit = (gen + 1) * 0xfau;
        const uint32_t words = (uint32_t)((int32_t)written >> 2);
        uint32_t dest;

        if((int32_t)written >= (int32_t)limit) {
            /* This generation is full: disarm and move to the next. */
            sr_st32(GHOST_ARMED, 0);
            sr_st32(GHOST_GEN, gen + 1);
            return;
        }
        dest = sr_ld32(GHOST_DEST);
        copy_pose(dest + words * RF_POSE_SIZE, RF_POSES);
        copy_pose(dest + (words + 0xe1u) * RF_POSE_SIZE, RF_POSES + best * RF_POSE_SIZE);
        sr_st32(cursor, written + 1);
    }
}

/* ---- start gate (0x227B0) ---- */

#define GATE_CAM_LIST 0x2140d0u /* zero-terminated, parallel to RF_OBJ_LIST */
#define GATE_CAM_REF  0x2140ccu
#define GATE_HANDLER  0x20afa0u
#define GATE_COOLDOWN 0x20afa4u

/* The handler gate_bind (0x22768) installs at 0x20afa0: 0x5c0d60 (ROM 0x21D60) in practice, 0x5c0fa0 (ROM 0x21FA0)
 * in a championship. Neither is lifted, so the reference's callx is a no-op and g0 keeps the cam slot word; the
 * port does the same for both (and for any other address). Port them here when the reference lifts them. */
static uint32_t gate_handler(uint32_t handler, uint32_t cam, uint32_t obj) {
    (void)obj;
    switch(handler) {
    case 0x5c0d60u: /* ROM 0x21D60: not lifted */
    case 0x5c0fa0u: /* ROM 0x21FA0: not lifted */
    default:
        return cam;
    }
}

void sr_gate_walk(void) {
    uint32_t cam_ea = GATE_CAM_LIST, obj_ea = RF_OBJ_LIST, hit = 0, cooldown;

    while(sr_ld32(cam_ea) != 0) {
        const uint32_t handler = sr_ld32(GATE_HANDLER);
        uint32_t r = sr_ld32(cam_ea);
        if(handler != 0) r = gate_handler(handler, sr_ld32(cam_ea), sr_ld32(obj_ea));
        if(r != 0) {
            if(sr_ld32(cam_ea) == sr_ld32(GATE_CAM_REF)) hit = 1;
            else if(r == sr_ld32(RF_OBJ_LIST)) hit = 1;
        }
        cam_ea += 4;
        obj_ea += 4;
    }
    cooldown = sr_ld32(GATE_COOLDOWN) + 1;
    sr_st32(GATE_COOLDOWN, cooldown);
    if(hit == 0 || (int32_t)cooldown < 30) return;
    sr_sound_cmd_fill(0x48, 0x7f);
    sr_st32(GATE_COOLDOWN, 0);
}

/* ---- clock and lap counters (0x1C8B8) ---- */

#define RF_TOTAL_TICKS   0x2020b8u
#define RF_SECTION_TICKS 0x20afb0u /* ticks per checkpoint section */

void sr_lap_tick(void) {
    uint32_t clock = sr_ld32(RF_CLOCK), ea;
    const uint32_t race_time = sr_ld32(RF_RACE_TIME), total = sr_ld32(RF_TOTAL_TICKS);
    const uint32_t cp = sr_ld32(RF_CHECKPOINT), lap = sr_ld32(RF_LAP);

    if((int32_t)clock > 0) clock--;
    sr_st32(RF_CLOCK, clock);
    sr_st32(RF_RACE_TIME, race_time + 1);
    sr_st32(RF_TOTAL_TICKS, total + 1);
    ea = RF_SECTION_TICKS + (cp << 2);
    sr_st32(ea, sr_ld32(ea) + 1);
    ea = RF_LAP_TIMES + (lap << 2);
    sr_st32(ea, sr_ld32(ea) + 1);
}

/* ---- near-object scan (0x22940, 0x22850) ---- */

#define RF_NEAR_BASE  0x2021f0u
#define RF_NEAR_FREE  0x20ac70u /* 1 when a near object has none of flag bits 2..5 set */
#define RF_NEAR_BOX   0x214360u /* three thresholds for TGP 0x2c805959 */

uint32_t sr_obj_span_test(uint32_t self_z, const uint32_t self_xyz[3], uint32_t other_z,
                          const uint32_t other_xyz[3]) {
    uint32_t in[6], hit;
    if((self_z >> 16) != (other_z >> 16)) return ((int32_t)other_z >> 16) > ((int32_t)self_z >> 16);
    /* Same span: XZ delta against the box (the y word pushed is 0). */
    in[0] = fsub(other_xyz[0], self_xyz[0]);
    in[1] = sr_ld32(RF_NEAR_BOX);
    in[2] = 0;
    in[3] = sr_ld32(RF_NEAR_BOX + 4);
    in[4] = fsub(other_xyz[2], self_xyz[2]);
    in[5] = sr_ld32(RF_NEAR_BOX + 8);
    sr_tgp_cmd(0x2c805959u, in, 6, &hit, 1);
    return sr_f(hit) > 0.0f;
}

uint32_t sr_obj_scan(uint32_t self_ix) {
    const uint32_t self = sr_ld32(RF_OBJ_LIST + (self_ix << 2));
    const uint32_t self_xyz[3] = {sr_ld32(self), sr_ld32(self + 4), sr_ld32(self + 8)};
    const uint32_t self_z = sr_ld32(self + 0x54);
    const int32_t count = (int32_t)sr_ld32(RF_OBJ_COUNT);
    uint32_t matches = 0, i;

    sr_st32(RF_NEAR_FREE, 0);
    for(i = 0; (int32_t)i < count; i++) {
        uint32_t other, other_xyz[3], hit;
        if(i == self_ix) continue;
        other = sr_ld32(RF_OBJ_LIST + (i << 2));
        other_xyz[0] = sr_ld32(other);
        other_xyz[1] = sr_ld32(other + 4);
        other_xyz[2] = sr_ld32(other + 8);
        hit = sr_obj_span_test(self_z, self_xyz, sr_ld32(other + 0x54), other_xyz);
        matches += hit;
        if(hit && (sr_ld8(other + 0x51) & 0x3c) == 0) sr_st32(RF_NEAR_FREE, 1);
    }
    return sr_ld32(RF_NEAR_BASE) + matches;
}

/* ---- finish, lap best, records ---- */

#define RF_FINISH_CLOCK 0x20aea8u

void sr_race_finish_clock(uint32_t clock) { sr_st32(RF_FINISH_CLOCK, clock + 0x3b); }

/* Best lap per car in backup SRAM: take this lap when there is none or it is strictly faster. */
void sr_lap_best(void) {
    const uint32_t lap = sr_ld32(RF_LAP);
    const uint32_t best = sr_ld32(RF_SRAM_LAP_BEST + sr_ld32(0x214354) * RF_SRAM_BANK);

    if(best == 0 || sr_ld32(RF_LAP_TIMES + (lap << 2)) < best) {
        sr_st32(RF_SRAM_LAP_BEST + sr_ld32(0x214354) * RF_SRAM_BANK, sr_ld32(RF_LAP_TIMES + (lap << 2)));
        sr_st32(RF_SRAM_DIRTY, 1);
        sr_sound_cmd(14);
    }
    sr_st32(RF_LAP, sr_ld32(RF_LAP) + 1);
    sr_hud_lap_time((uint32_t)-1);
}

/* libc strcpy (0x5CDC8) on RAM addresses: nothing when dst is 0 or src is empty, else the string with its NUL.
 * The lifted strcpy dereferences these i960 addresses as host pointers (and never advances src), so the reference
 * crashes where lap_record shifts rows; this is the ROM routine's intent. Unchecked. */
static void ram_strcpy(uint32_t dst, uint32_t src) {
    uint32_t c;
    if(dst == 0 || sr_ld8(src) == 0) return;
    do {
        c = sr_ld8(src++);
        sr_st8(dst++, c);
    } while(c != 0);
}

#define REC_COUNT 0x08u /* runs counted */
#define REC_BEST  0x0cu /* best time */
#define REC_NAME  0x14u /* 48 words from 0x20aed0 (name_copy, 0x1EB88) */
#define REC_ROWS  0xd4u /* 12-byte rows: time, name pointer?, kind u16, u16 */

/* Record table insert at the finish of a solo race (0x18270). The row walk and the row pointers follow the lift
 * word for word. Unchecked: no scripted drive finishes. */
void sr_lap_record(uint32_t time) {
    uint32_t bank = sr_ld32(0x214354) + 1, base, r5 = 0, g5, slot, kind;

    if(sr_ld32(0x2020b4) != 0) bank = 5;
    sr_st32(0x20aa14, bank);
    base = RF_SRAM_RECORDS + bank * RF_SRAM_BANK;

    if((int32_t)sr_ld32(RF_NET_TIMER) < 0) {
        g5 = base;
        while(sr_ld32(g5 + REC_ROWS) < time) {
            r5++;
            if((int32_t)r5 > 15) break;
            g5 = sr_ld32(g5 + REC_BEST);
        }
        if((int32_t)r5 < 15) {
            /* Shift rows down from row 14 to the insert row. */
            uint32_t r6 = 0xa8u, r4 = base + r6;
            const uint32_t need = r5 * 12;
            do {
                const uint32_t t = sr_ld32(r4 + REC_ROWS);
                const uint32_t a = sr_ld16(r4 + REC_ROWS + 8), b = sr_ld16(r4 + REC_ROWS + 10);
                sr_st32(r4 + REC_ROWS + 12, t);
                sr_st16(r4 + REC_ROWS + 20, a);
                sr_st16(r4 + REC_ROWS + 22, b);
                ram_strcpy(r4 + REC_ROWS + 16, r4 + REC_ROWS + 4);
                r6 -= 12;
                r4 -= 12;
            } while(!((int32_t)r6 < (int32_t)need));
        }
        sr_st32(0x20a9fc, r5);
        if(!((int32_t)r5 > 15)) {
            slot = sr_ld32(base + r5 * 12);
            sr_st32(slot + REC_ROWS, time);
            sr_st16(slot + REC_ROWS + 10, 0);
            if((sr_ld8(0x213b90) & 15) == 0 && sr_ld32(0x2139d0) == 0) {
                sr_st16(slot + REC_ROWS + 8, 0);
            } else {
                uint32_t row;
                if((sr_ld8(0x213b90) & 15) == 0) kind = 1;
                else kind = sr_ld32(0x2139d0) == 0 ? 2 : 3;
                row = sr_ld32(0x20a9fc) * 3;
                sr_st16(base + REC_ROWS + 8 + (row << 2), kind);
            }
            {
                const uint32_t best = sr_ld32(base + REC_BEST);
                if(best > time || best == 0) {
                    uint32_t i;
                    sr_st32(base + REC_BEST, time);
                    for(i = 0; i < 48; i++) sr_st32(base + REC_NAME + i * 4, sr_ld32(0x20aed0u + i * 4));
                }
            }
        }
    }
    sr_st32(base + REC_COUNT, sr_ld32(base + REC_COUNT) + 1);
    sr_st32(0x20aa18, 0);
    sr_st32(RF_SRAM_DIRTY, 1);
}

/* ---- race body (0x1D160) ---- */

#define RF_LINK_FLAG    0x20a758u /* nonzero: linked race clock rules */
#define RF_PEER_COUNT   0x20a581u /* s8 */
#define RF_PEER_SELF    0x20a584u /* own peer row */
#define RF_CP_STATE     0x20a760u /* 1 checkpoint passed (banner), 2 finished */
#define RF_CP_PASSED    0x20a770u
#define RF_CP_PASSED2   0x20a774u
#define RF_CP_BANNER    0x20aea0u /* banner ticks left */
#define RF_PEER_DONE    0x20aea4u
#define RF_GAMEOVER_TICKS 0x20aeacu
#define RF_HANDLERS     0x20ab70u
#define RF_GO_STATE     0x214120u /* 1 racing, 2 time up / finished */

/* Peer block for a linked race (0x1D1AC..0x1D2FC): comm board rows matching our timer can add time or end the race. */
static void peer_clock(uint32_t timer) {
    const int32_t peers = (int8_t)sr_ld8(RF_PEER_COUNT);
    uint32_t hit = 0, i;

    if(peers > 0) {
        const uint32_t self = sr_ld32(RF_PEER_SELF);
        for(i = 0; (int32_t)i < peers; i++) {
            const uint32_t row = RF_COMM_PEERS + i * RF_COMM_STRIDE;
            if(sr_ld32(row) != timer) continue;
            if(sr_ld32(sr_ld32(row + 0x16c)) == 1) {
                /* A time handler: take its word (time to add when negative) and clear it. */
                const uint32_t h = sr_ld32(RF_HANDLERS + (sr_ld32(sr_ld32(row + 0x170)) << 2));
                hit = sr_ld32(h);
                sr_st32(h, 0);
                break;
            }
            if(sr_ld32(row + 0x16c) == 2 && i == self) {
                sr_st32(RF_PEER_DONE, 1);
                break;
            }
        }
    }
    if((int32_t)hit < 0) {
        sr_st32(RF_CLOCK, sr_ld32(RF_CLOCK) + hit);
        sr_sound_cmd_fill(0x7f, 0x7f);
        sr_sound_cmd(4);
    }
    if(hit == 0) {
        /* Catch up: half the lead of the best peer over our own row. */
        const uint32_t self = sr_ld32(RF_PEER_SELF);
        const int32_t n = (int8_t)sr_ld8(RF_PEER_COUNT);
        const uint32_t cap = sr_ld32(RF_COMM_PEERS + 0x180 + self * RF_COMM_STRIDE);
        uint32_t best = 0;
        for(i = 0; (int32_t)i < n; i++) {
            const uint32_t row = RF_COMM_PEERS + i * RF_COMM_STRIDE;
            if(sr_ld32(row) == timer && i != self) {
                const uint32_t v = sr_ld32(sr_ld32(row + 0x180));
                if((int32_t)v > (int32_t)best) best = v;
            }
        }
        if((int32_t)best > (int32_t)cap) sr_st32(RF_CLOCK, sr_ld32(RF_CLOCK) + ((best - cap) >> 1));
    }
}

/* Passing a checkpoint: banner on, next checkpoint, extended play when solo or linked. Returns 1 when its bonus
 * applies. */
static uint32_t checkpoint_pass(void) {
    const uint32_t prev = sr_ld32(RF_CHECKPOINT);
    uint32_t timer;
    sr_st32(RF_CP_STATE, 1);
    sr_st32(RF_CP_BANNER, TICKS_PER_SECOND);
    timer = sr_ld32(RF_NET_TIMER);
    sr_st32(RF_CHECKPOINT, prev + 1);
    sr_st32(RF_CP_PASSED, prev);
    sr_st32(RF_CP_PASSED2, prev);
    return (int32_t)timer < 0 || sr_ld32(RF_LINK_FLAG) != 0;
}

/* abs(speed) below this (the double 0x3fb7b425ed097b42, ~0.0926) ends the finish window early. */
#define FINISH_STOP_SPEED 0x3dbda130u /* smallest float >= the double */

void sr_race_slot3(void) {
    uint32_t timer, cp, pair_id, pair_bonus, clock, count;

    if((sr_ld8(RF_SUB) & 1) == 0) sr_st32(RF_SUB, sr_ld32(RF_SUB) + 1);

    sr_pose_ring_step();
    /* ROM 0x1EBD0, and ROM 0x40820 when 0x2140c8 > 0: unlifted, no-ops in the reference. */
    sr_gate_walk();
    sr_lap_tick();

    timer = sr_ld32(RF_NET_TIMER);
    if((int32_t)timer < 0 && sr_ld32(RF_LINK_FLAG) == 0) peer_clock(timer);

    /* Checkpoints: the next pair's span id against the current span (0x1D2FC). */
    cp = sr_ld32(RF_CHECKPOINT);
    pair_id = sr_ld32(RF_CP_PAIRS + (cp << 3));
    pair_bonus = sr_ld32(RF_CP_PAIRS + (cp << 3) + 4);
    if(pair_id == sr_ld32(RF_SPAN_ID)) {
        if((int32_t)pair_bonus >= 0) {
            uint32_t bonus = 0;
            if(pair_id != 0) {
                bonus = checkpoint_pass();
            } else if((int32_t)sr_ld32(RF_SPAN_PROGRESS) > (int32_t)(sr_ld32(RF_LAP) * SECTION_TICKS + 0x118u)) {
                /* Id 0 is the start line: a lap is done once the progress is past this lap's window. */
                sr_hud_lap_time(sr_ld32(RF_LAP));
                sr_lap_best();
                bonus = checkpoint_pass();
            }
            if(bonus && (int32_t)pair_bonus > 0) {
                sr_st32(RF_CLOCK, sr_ld32(RF_CLOCK) + pair_bonus);
                sr_sound_cmd_fill(0x7f, 0x7f);
                sr_sound_cmd(4);
            }
        } else if((int32_t)sr_ld32(RF_SPAN_PROGRESS) > (int32_t)(sr_ld32(RF_LAPS) * SECTION_TICKS - 10u)) {
            /* The finish (negative bonus) after the last lap (0x1D31C). */
            const uint32_t clock_left = sr_ld32(RF_CLOCK);
            uint32_t race_time;
            sr_st32(RF_SUB, 8);
            sr_race_finish_clock(clock_left);
            race_time = sr_ld32(RF_RACE_TIME);
            sr_st32(RF_FINISH_FLAG, 1);
            sr_st32(0x2021d0u + (sr_race_course_index() << 2), race_time);
            sr_st32(RF_SECTION_TICKS, race_time);
            sr_sound_cmd(5);
            if(sr_ld32(0x202230) == 1) sr_lap_record(sr_ld32(RF_RACE_TIME));
            sr_st32(RF_CP_PASSED, 0);
            sr_st32(RF_CP_STATE, 2);
        }
    } else if(sr_ld32(RF_CP_STATE) == 1) {
        /* Off the checkpoint: count the banner down. */
        const uint32_t left = sr_ld32(RF_CP_BANNER) - 1;
        sr_st32(RF_CP_BANNER, left);
        if(left == (uint32_t)-1) {
            sr_st32(RF_CP_STATE, 0);
            sr_st32(RF_CP_PASSED, 0);
        }
    }

    /* Time up or a peer finished: the game-over window (0x1D52C). */
    clock = sr_ld32(RF_CLOCK);
    if(clock == 0 || sr_ld32(RF_PEER_DONE) != 0) {
        uint32_t left;
        if(sr_ld32(RF_GAMEOVER_TICKS) == 0) sr_st32(RF_GAMEOVER_TICKS, SECTION_TICKS);
        left = sr_ld32(RF_GAMEOVER_TICKS);
        sr_st32(RF_GO_STATE, 2);
        left--;
        sr_st32(RF_GAMEOVER_TICKS, left);
        if(left == 0) {
            sr_st32(RF_SUB, 10);
        } else {
            const uint32_t speed = sr_ld32(RF_SPEED) & 0x7fffffffu;
            if(speed > 0x7f800000u || speed < FINISH_STOP_SPEED) sr_st32(RF_SUB, 10);
        }
    } else {
        sr_st32(RF_GO_STATE, 1);
        /* Last five seconds: a tick sound each second. */
        if((int32_t)clock > (int32_t)SECTION_TICKS && clock % TICKS_PER_SECOND == 0) sr_sound_cmd_fill(0x80, 0x40);
    }

    count = sr_obj_scan(0);
    if((int32_t)sr_ld32(RF_NET_TIMER) < 0) {
        if(count == 0 && (sr_ld32(0x202008) & (1u << 5)) != 0) sr_st8(0x20204c, sr_ld8(0x20204c) | 0x80);
        else sr_st8(0x20204c, sr_ld8(0x20204c) & 0x7f);
    }
    sr_st32(RF_NEAR_COUNT, count);
    /* ROM 0x1EA40 (g0 = 0x213b94) when 0x20201d > 1: unlifted, a no-op in the reference. */
}

/* 0x2AAF0, with the free-list pushes 0x2A928 / 0x2A968. */
void sr_race_list_unlink(uint32_t node) {
    uint32_t prev, next, head;
    if(!node) return;
    prev = sr_ld32(node + 8);
    next = sr_ld32(node + 0xc);
    sr_st32(prev + 0xc, next);
    sr_st32(next + 8, prev);
    switch(sr_ld8(node) & 3u) {
    case 0: head = 0x20cab0u; break;
    case 1: head = 0x20cab4u; break;
    default: return;
    }
    sr_st32(node + 0xc, sr_ld32(head));
    sr_st32(head, node);
}
