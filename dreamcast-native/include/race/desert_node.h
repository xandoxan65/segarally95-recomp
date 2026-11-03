/* The desert course's animated object (code/race/desert_node.c): the helicopter that flies along a scripted
 * path over the desert stage. In desert practice it is the third race list node (work RAM 0x500200), installed
 * by node_init (0x43790) and then run each tick by node_frame (0x427A0, staged as 0x5e17a0).
 *
 * Node layout (offsets from the node, float bits unless noted):
 *   +0x02 byte   bit 1: on its path this tick (time below the table's end)
 *   +0x14        nearest point of the course centre line (index into 0x215c40, 3 words per point)
 *   +0x18..0x20  position xyz            +0x24..0x2c  angles (+0x24 pitch, +0x28 yaw, +0x2c roll)
 *   +0x30..0x38  path drift (rotated offset added to x/z)
 *   +0x54 half   next path key          +0x58 byte bit 0: close to the camera object (sound playing)
 *   +0x5c        path time              +0x60 / +0x64  main / tail rotor angle
 *   +0x68 / +0x6c tilt from the path's turn rate (copied to +0x2c / +0x24)   +0x70 their divisor (10.0)
 *   +0x74        drift amount (set once by node_init)
 *
 * The path table (table_pick): +0x02 half row count (also the path's end time), +0x04 time rate, +0x08/+0x0c
 * time window for the 0x20a730 flag, +0x10 row base (rows of 24 bytes: xyz, -, angle at +0x10); key k's span
 * limit is the halfword at +20*k.
 * Dust puffs: up to 64 entries of 0x1c bytes from 0x216a70 { xyz, life at +0x18 }, life <= -1.5 is free. */
#ifndef SR_RACE_DESERT_NODE_H
#define SR_RACE_DESERT_NODE_H
#include <stdint.h>

#define SR_DESERT_VIEW_BASE  0x216a54u /* pose the path keys follow (cam follow block or the leading car) */
#define SR_DESERT_DUST       0x216a88u /* first dust puff's life word (xyz is at -0x18) */
#define SR_DESERT_DUST_END   0x21716cu
#define SR_DESERT_FLAG       0x20a730u /* near flag byte; 0x20a738..0x20a74c position and angles */
#define SR_DESERT_CATALOGS   0x20b940u /* running count of GEO catalog words, shared with every drawer */

/* 0x43790 game_start_race_desert_node_init: seed the node and install node_frame (+4 <- 0x5e17a0). */
void sr_desert_node_init(uint32_t node);
/* 0x427A0 game_start_race_desert_node_frame: one tick of the helicopter. */
void sr_desert_node_frame(uint32_t node);

/* 0x41B78 game_start_race_desert_table_pick: the path table for this course and mode (0x5e0b10 / 30 / 50). */
uint32_t sr_desert_path_table(void);
/* 0x41F80 game_start_race_desert_near_pick: of centre line points index-1, index, index+1, the one nearest to
 * the xz of `query` (VA of xyz). */
uint32_t sr_desert_near_pick(uint32_t query, uint32_t index);
/* 0x422B0 game_start_race_desert_pose_step: position y, yaw and pitch/roll from the path rows around node+0x5c;
 * the TGP gives the turn (node+0x68/+0x6c). */
void sr_desert_pose_step(uint32_t node);
/* 0x41BE0 game_start_race_desert_rng_jitter: start a dust puff at xyz (VA of 3 words) with life
 * (15 - scale) * 0.095, in the first free slot; nothing when scale >= 15 or all 64 slots are live. */
void sr_desert_dust_spawn(uint32_t xyz, uint32_t scale);
/* 0x41CA0 game_start_race_desert_tgp55_walk: draw the live dust puffs (TGP 0x55 billboard + catalog) and age
 * them by 0.06. */
void sr_desert_dust_draw(void);
/* 0x42190 game_start_race_desert_obj_near_flag: within 35 of the camera object (0x213980) start the rotor
 * sound (0x97), leaving it stop it; node+0x58 bit 0 remembers which. */
void sr_desert_sound_near(uint32_t node);
/* 0x46470 game_start_race_desert_slot_flag: publish pos/angles (VAs of 3 words) to 0x20a738.. with `lane` and
 * `rank` (capped at 3) in the flag byte 0x20a730; rank < 0 clears the flag when the lane matches. Returns the
 * original's g0. */
uint32_t sr_desert_slot_flag(uint32_t pos, uint32_t angles, uint32_t lane, int32_t rank);

#endif
