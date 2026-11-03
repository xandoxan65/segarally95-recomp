/* Callees of the player car's per-tick scene update (geo_view_scene_frame @ 0x34F40, node 0x500000), ported in
 * code/race/player_scene.c. scene_frame itself is not ported yet; these are the routines it calls, with the
 * implicit i960 inputs (registers, the caller's frame) made explicit. Offsets are the original's.
 *
 * `frame` below is scene_frame's own 0x220-byte i960 frame (its fp): the original callees keep the caller's fp
 * and use its slots as temporaries, and scene_frame reads several of them afterwards (noted per routine). */
#ifndef SR_RACE_PLAYER_SCENE_H
#define SR_RACE_PLAYER_SCENE_H
#include <stdint.h>
#include "race/sound_cmd.h"

/* Work tables the four wheel corners share (CRX RAM). */
#define PS_CORNER_ROAD   0x2138d0u /* 4 x 0x24: road_query_b output per corner (+0x0c normal, +0x18 height, +0x1c flags) */
#define PS_CORNER_DIST   0x213890u /* 4 words: |corner offset| (0 when the corner is in the air) */
#define PS_CORNER_SIN    0x213880u /* 4 words: signed sine between the corner offset and the heading pair */
#define PS_CORNER_HEIGHT 0x2138a0u /* 4 words: probe y + road height, copied to pose+0x40 by the epilogue */
#define PS_CORNER_FORCE  0x2138b0u /* 4 words: spring force magnitude */
#define PS_HIT_MASK      0x213960u /* bit i: corner i touched a wall cell (flag bit 7) in scene_prep */

/* 0x34380 geo_view_scene_prep: probe the four corners of the car on the road. Rotates the four corner offsets
 * (TGP 0x42/0x48/0x43) by `ang` (xyz, scene_frame fp+0x140), adds `pos` (fp+0x130), queries the road under each
 * corner into PS_CORNER_ROAD, builds the wall push per corner and, when any corner hits a wall, corrects
 * node+0x48 / +0x2c / +0x34, ang[1], pos[0] and pos[2]. Returns the largest wall weight (float bits; 0 when no
 * wall; scene_frame keeps it at fp+0x1a0 for the wall latch and wall scrub). `pos` and `ang` are host words
 * (normally frame+0x130 / frame+0x140).
 * Frame slots written (scene_frame reads them afterwards): 0x40..0x6b corner positions (world), 0x70..0x9f
 * rotated corner offsets (0x74+12i zeroed when a wall was hit), 0xa0..0xcf wall push per corner, 0xd0..0xd8
 * last plane normal / reduction vector, 0xe0..0xe8 first corner's first cross product. */
uint32_t sr_player_scene_prep(uint32_t node, uint8_t *pos, uint8_t *ang, uint8_t *frame);

/* 0x37200 geo_view_copro_vec_push: spring force of wheel corner `slot` (0..3, the original's g6).
 * pos (g1): the corner's world position minus nothing (its offset vector, 3 words); vec (g2): rotated basis whose
 * y is added to the road height; dir (g3): heading pair (x at +0, z at +8); scale (g4): float bits (0.25);
 * out (g5): force vector written (3 words). Updates node+0x12 (nibble per corner: road flag low bits),
 * node+0x64+4*slot (compression), PS_CORNER_* and 0x214208 (OR of flag nibbles << 1).
 * Returns 1 when the corner is on the ground (the original's g0), 0 when in the air or on a wall cell. */
uint32_t sr_player_wheel_force(uint32_t node, const uint8_t *pos, const uint8_t *vec, const uint8_t *dir,
                               uint32_t scale, uint8_t *out, uint32_t slot);

/* 0x34CE0 geo_view_mode_latch: wall-contact latch. x (float bits) is scene_frame fp+0x1a0, the largest wall
 * weight scene_prep returned (0 when no corner touches a wall). Counts ticks without contact in 0x213870, keeps
 * the flag 0x21396c and the force-feedback mode 0x2142c8 (+0x2142cc/d0 from scene_prep's 0x213964/68), and queues
 * the impact sounds (0x47+0x0e when x > 0.03, else 0x49). */
void sr_player_mode_latch(uint32_t x);

/* 0x395D8 geo_view_fov_scale: steering input 0x202050 as a signed amount, clamped to +-80 and scaled by
 * 0.84/80 (float bits). */
uint32_t sr_steer_amount(void);

/* 0x39A90 geo_view_pitch_smooth: 0x2142c0 += (steer - 0x2142c0) * 0.9; returns the new value negated
 * (scene_frame stores it to node+0x74). */
uint32_t sr_player_steer_smooth(void);

/* 0x33E90 geo_view_object_bind: gravity scale 0x214124 = 9.81 / (3500 + 10 * gap) from the nearest other car
 * (span difference and TGP distance, signed, minimum over cars and 0); 9.81/2600 when that sum is below 2600.
 * `pose` is the player's pose block (node+0x8c). Frame slots written: 0x40..0x48 own pose xyz; with other cars
 * 0x50..0x58 the last car's xyz. */
void sr_player_gap_scale(uint32_t pose, uint8_t *frame);

/* 0x34078 geo_view_scene_epilogue: copy the node into its pose block (node+0x8c), corner heights from
 * PS_CORNER_HEIGHT to pose+0x40..0x4c. */
void sr_player_pose_publish(uint32_t node);

/* 0x34F40 geo_view_scene_frame: the player car's whole per-tick update (list handler of node 0x500000):
 * corner probes, spring forces, force/torque sums, the frame update, integration, sounds, pose copy. Its only
 * input is the node; it keeps its own frame. Code: code/race/player_scene_frame.c. */
void sr_player_scene_frame(uint32_t node);

/* 0x37870 geo_view_frame_update (ported in the player_update module): drive, steering and engine for the node.
 * a1 = scene_frame fp+0x120 (3 words: TGP input, overwritten), a2 = fp+0x150 (3 words, accumulated into),
 * corr = F(0.005 * D(fmul(node+0x50, fsub([0x2138a4], [0x2138a0])))). */
void sr_player_frame_update(uint32_t node, uint32_t a1[3], uint32_t a2[3], uint32_t corr);

/* 0x5CCE8 geo_rng_u8: the game's 31-bit random generator (seed 0x2197b0). Returns the new seed. */
uint32_t sr_rng_next(void);


#endif
