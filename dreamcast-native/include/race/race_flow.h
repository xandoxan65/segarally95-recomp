/* Race flow and object bookkeeping: the per-tick race body after the countdown (clock, checkpoints, laps, finish)
 * and the object lists the race frame rebuilds each tick (lifted from game_start_race_slot3 @ 0x1D160,
 * game_start_race_obj_bind @ 0x23110, game_start_race_list_flag_prep @ 0x1C2A0 and their callees).
 * All state stays in RAM at the original's addresses. */
#ifndef SR_RACE_FLOW_H
#define SR_RACE_FLOW_H
#include <stdint.h>

/* Globals the routines below share. */
#define RF_SUB          0x2020a8u /* race sub-state: 6/7 race body, 8 finished (slot4), 10 finish fanfare (slot5) */
#define RF_FINISH_FLAG  0x2020b0u /* set to 1 at the finish */
#define RF_RACE_TIME    0x2020bcu /* ticks since GO */
#define RF_LAP          0x2020c0u /* current lap */
#define RF_LAPS         0x2020c4u /* laps to run */
#define RF_NEAR_COUNT   0x2020c8u /* obj_scan result */
#define RF_LAP_TIMES    0x2020d0u /* ticks per lap */
#define RF_NET_TIMER    0x20a560u /* < 0 in a solo race; else the linked race's timer */
#define RF_CHECKPOINT   0x20ac98u /* next checkpoint index into the pairs at 0x20aca0 */
#define RF_CP_PAIRS     0x20aca0u /* {span id, bonus ticks} per checkpoint; bonus < 0 marks the finish */
#define RF_CLOCK        0x20b0b0u /* remaining time in ticks */
#define RF_SPEED        0x20b0b4u /* player speed (float) */
#define RF_SPAN_PROGRESS 0x2140c4u
#define RF_SPAN_ID      0x2140c8u
#define RF_OBJ_COUNT    0x213978u /* entries in RF_OBJ_LIST */
#define RF_OBJ_LIST     0x213980u /* object (pose block) addresses */
#define RF_POSE_COUNT   0x2139c8u /* pose blocks at RF_POSES */
#define RF_DEPTH_LIST   0x2139f0u /* 16-byte records {flags, mode, pose, depth} */
#define RF_POSES        0x213b40u /* 0x58-byte pose blocks */
#define RF_POSE_SIZE    0x58u
#define RF_COMM_PEERS   0x1a121dcu /* comm board peer rows, 0x1c0 bytes each */
#define RF_COMM_STRIDE  0x1c0u
#define RF_SRAM_LAP_BEST 0x1d00300u /* backup SRAM: best lap per car, 0x194 apart */
#define RF_SRAM_DIRTY   0x1d002e8u
#define RF_SRAM_RECORDS 0x1d002f0u /* backup SRAM: record banks, 0x194 bytes each */
#define RF_SRAM_BANK    0x194u

void sr_race_slot3(void);                   /* 0x1D160: race body after the countdown */
void sr_pose_ring_step(void);               /* 0x46160: pose ring / ghost buffer append */
void sr_gate_walk(void);                    /* 0x227B0 */
void sr_lap_tick(void);                     /* 0x1C8B8 */
/* Count of objects near object `self_ix` (plus 0x2021f0). */
uint32_t sr_obj_scan(uint32_t self_ix);     /* 0x22940 */
/* 1 when the other object is ahead on the packed span or inside the XZ box (TGP 0x2c805959). */
uint32_t sr_obj_span_test(uint32_t self_z, const uint32_t self_xyz[3], uint32_t other_z,
                          const uint32_t other_xyz[3]);  /* 0x22850 */
void sr_race_finish_clock(uint32_t clock);  /* 0x1C808: 0x20aea8 = clock + 0x3b */
void sr_lap_best(void);                     /* 0x1C940 */
void sr_lap_record(uint32_t time);          /* 0x18270 */

void sr_obj_bind(uint32_t net_timer);       /* 0x23110 */
void sr_obj_list(uint32_t net_timer);       /* 0x22E20 (linked race only) */
void sr_obj_publish(void);                  /* 0x22C70 */
void sr_obj_sort_index(void);               /* 0x231F0 */
void sr_obj_rank_fill(void);                /* 0x23130 */
void sr_obj_depth_pass(uint32_t start);     /* 0x23280 */
/* budget: 0x20aac0 (timer-3 based draw budget); link: the caller's g14, the original's return address
 * (0x1BC54 from the race frame), which it stores as a list mode. */
void sr_list_flag_prep(uint32_t budget, uint32_t link);  /* 0x1C2A0 */

#endif
