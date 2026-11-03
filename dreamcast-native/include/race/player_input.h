/* The player's controls as the car update reads them (lifted from geo_view_matrix_prep @ 0x3A320,
 * geo_view_scene_apply @ 0x39750 and their callees). geo_view_frame_update calls both for the player's car, the
 * scene node at 0x500000 (`cam` below).
 *
 * Inputs: 0x202050 steer (0x80 centre), 0x202051 accelerator, 0x202052 brake (bytes, rest 0x30), 0x202044 gear.
 * Float values are raw IEEE single bits, as in RAM. None of these routines reads or leaves i960 registers: the
 * register effects the lifted versions have are listed with each one. */
#ifndef SR_RACE_PLAYER_INPUT_H
#define SR_RACE_PLAYER_INPUT_H
#include <stdint.h>

/* Node fields of the player's car used here. */
#define PLAYER_FLAGS10   0x10u /* byte: bit 4 set while the brake is pressed past 0.6 */
#define PLAYER_GEAR      0x78u /* current gear 1..4 */
#define PLAYER_RPM       0x7cu /* engine revs 0..8800 */
#define PLAYER_ROT_STEP  0x80u /* wheel turn per tick */
#define PLAYER_ROT       0x84u /* wheel turn accumulator */

/* 0x3A320: engine revs, gear change and drive force from the accelerator, brake and gear lever. `cam` is the node
 * (0x500000), `scratch` the clutch-slip word the caller passes (0x214254). Updates node+0x78..0x84, the engine
 * state at 0x2142e0..0x214330 and the engine sound commands; returns the net drive force (float bits) that
 * frame_update stores at 0x214258.
 * Expects i960 g14 = 0 (frame_update clears it before the call): the lifted code stores g14 as "zero" and pushes
 * it to the TGP. Leaves the TGP matrix stack as it found it. The lifted version also leaves g1 = the angle_frac
 * scale and other g registers clobbered; frame_update does not read them. */
uint32_t sr_player_matrix_prep(uint32_t cam, uint32_t scratch);

/* 0x3AAE8: engine torque at `rpm` (float bits) from the curve at *0x2142f8, times `throttle` and the scales at
 * 0x5d930c and 0x214124. Returns the torque; the lifted version also leaves that scale in g1 (*scale_out if
 * non-null). */
uint32_t sr_player_torque(uint32_t rpm, uint32_t throttle, uint32_t *scale_out);

/* 0x3B128: brake force from 0x202052 (0.5 once 0x214120 > 1), scaled by 0x5d9310 and 0x214124, never negative.
 * Sets or clears bit 4 of cam+0x10 (brake past 0.6). */
uint32_t sr_player_brake(uint32_t cam);

/* 0x3AE50: the over-rev state 0x214330 / 0x21432c from the torque `torque` (float bits): counts ticks above 0.15
 * to state 1, which plays a sound and goes to 2 once the torque falls below 0.01; state 2 plays one of eight
 * random sounds (0x5d9e30, RNG 0x2197b0) and returns to 0. */
void sr_player_overrev(uint32_t torque);

/* 0x3B008: automatic gearbox (0x2139d0 == 0). Practice drives use the manual box, so the game never calls it there;
 * it was checked by a temporary experiment calling both versions every tick with the shift counter forced to 30
 * (16446 calls per drive, shifts up and down, 0 mismatches). Returns the gear for cam+0x78 (1..4; 1 before the race starts),
 * shifting up above 7903 rpm with no slip (0x2142f0 < 0.1) and down below a throttle-dependent speed, at most
 * once every 30 ticks (0x2142e0). `throttle` is matrix_prep's accelerator fraction. */
uint32_t sr_player_gear_auto(uint32_t cam, uint32_t throttle);

/* 0x3AF58: manual gearbox. `gear` is the current gear (cam+0x78); returns the new one. A change of the lever
 * 0x202044 takes effect after 8 ticks and starts the shift timers 0x2142fc (down) or 0x214300 (up). */
uint32_t sr_player_gear_manual(uint32_t gear);

/* 0x3AC50: engine sound. `rpm` and `throttle` (float bits) pick the pitch 0x214304 and volume 0x214308 and send
 * them to the sound board when they change; 0x214208 bits 8-9 mute it. */
void sr_player_engine_sound(uint32_t rpm, uint32_t throttle);

/* 0x39750: steering feedback. Mixes the wheel angle (0x202050), the recent road forces 0x2142a0..0x2142bc and
 * the last angle into the force-feedback byte 0x202049. `prev` is the caller's g0, stored at 0x214290 as the
 * next tick's "previous" value. The lifted version leaves g14 = 0 (fov_scale's bal clears it). */
void sr_player_steer_feedback(uint32_t prev);

/* 0x395D8: the wheel angle from 0x202050: (byte - 0x80) clamped to +-80, times 0.84/80; float bits. The lifted
 * version leaves g0 = the result and clears g14. */

#endif
