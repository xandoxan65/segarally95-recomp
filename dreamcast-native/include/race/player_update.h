/* Helpers of the player car's frame update (geo_view_frame_update @ 0x37870, called from geo_view_scene_frame for
 * the scene node at 0x500000). Each takes what the original reads from g0..g2 as arguments and returns its g0
 * result; float words are raw bits as in RAM. Everything else is read and written at the original's addresses.
 *
 * Globals they share (CRX RAM; names follow once frame_update is ported):
 *   0x202049  byte sent to the drive board (IO 0x01c00008): the steering wheel's force command
 *   0x214230..0x21424c  skid sound latch (row's sound ids, current level, playing flag)
 *   0x214250  airborne tick counter
 *   0x2142c4..0x2142d4  wheel force mode, countdown and previous input
 *   0x2142e4  blended pitch
 *   0x20b100.. sound command ring (0x20b180 pending bytes, 0x20b184 write cursor, 0x20b18c staging packet) */
#ifndef SR_RACE_PLAYER_UPDATE_H
#define SR_RACE_PLAYER_UPDATE_H
#include <stdint.h>

/* 0x37870 geo_view_frame_update: the player car's per-tick update (code/race/player_frame.c).
 * node = the scene node (0x500000); a1 = 3 words at the caller's frame +0x120, sent to the TGP and replaced by
 * their rotation into the car's frame; a2 = 3 words at +0x150, the body forces are added to them; corr = the
 * caller's g3 (node+0x50 * (0x2138a4 - 0x2138a0) * 0.005). Writes the dynamics row copy 0x2141c0..0x2141f0,
 * 0x214254..0x214264, the slip 0x21420c and everything its callees write (matrix_prep, steer/force feedback,
 * skid sound, pitch blend). */
void sr_player_frame_update(uint32_t node, uint32_t a1[3], uint32_t a2[3], uint32_t corr);

/* 0x388D0: airborne gate. state 0: in the air this tick (plays sound 0x96, counts 0x214250; returns 5.0f for the
 * first 59 ticks, then 0); state > 0: on the ground (after more than 10 airborne ticks plays landing sound 0x47 and
 * returns 5.0f, else 0; clears the counter); state < 0: reset (clears the counter, returns 0).
 * frame_update passes 0 or 1, geo_view_table_seed -1. Returns float bits. */
uint32_t sr_player_airborne_gate(int32_t state);

/* 0x38680: skid sound. level = 0x214254 as float bits (< 0 resets the latch), row = the selected 0x5d67a0 dynamics
 * row (selects the sound pair in table 0x5dcd10). Runs every 8th tick (0x202008 & 7 == 0). */
void sr_player_skid_sound(uint32_t level, uint32_t row);

/* 0x39C40: steering wheel force. in = node+0x74, target = r5 + correction (float bits), slip = 0x21420c.
 * Writes the drive board byte 0x202049 according to the mode 0x2142c8 (0 normal, 1 countdown, 2/3 latch). */
void sr_player_force_feedback(uint32_t in, uint32_t target, uint32_t slip);

/* 0x39B80: angle -> FOV-weighted lerp of the tables 0x5d8330 / 0x5d8430 (weight 0x214270). Float bits. */
uint32_t sr_player_trig_scale(uint32_t angle);

/* 0x3AA38: blended pitch: writes 0x2142e4 from sum (r8 + r6 in frame_update) and base (0x214258). */
void sr_player_pitch_blend(uint32_t sum, uint32_t base);

/* 0x38A18: x + sin(x * pi) * (x > 0 ? [0x5d6688] : [0x5d668c]), sin on the TGP (0x0a801515). Float bits. */
uint32_t sr_player_sin_mix(uint32_t x);

/* 0x38958 / 0x389B8: s * sqrt(x) + (1 - s) * x * x with s = [scale_va]. Float bits. */
#define SR_PLAYER_SQRT_MIX_A 0x5d6680u /* geo_view_sqrt_mix_a, 0x38958 */
#define SR_PLAYER_SQRT_MIX_B 0x5d6684u /* geo_view_sqrt_mix_b, 0x389B8 */
uint32_t sr_player_sqrt_mix(uint32_t x, uint32_t scale_va);

/* Sound commands (shared helpers, kept here until a sound module exists):
 * 0x26140: enqueue the 3-byte packet `index` of table 0x5c3ce0.
 * 0x261B0: the same through the staging packet 0x20b18c, a 0xff third byte replaced by `fill`. */

#endif
