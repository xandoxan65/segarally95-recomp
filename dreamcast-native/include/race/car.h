/* The player car's per-tick update (lifted from game_start_race_obj_car_frame @ 0x2BCF0 and its callees).
 * A car is a list node in work RAM (the player's at 0x500100); `node` is its i960 address. Offsets are the
 * original's (docs/dreamcast-port.md: names follow once the whole tick is checked). */
#ifndef SR_RACE_CAR_H
#define SR_RACE_CAR_H
#include <stdint.h>

/* Node fields the routines below share. */
#define CAR_FLAGS10   0x10u /* word copied to pose+0x50 */
#define CAR_FLAGS11   0x11u /* byte: low 6 bits own, high 2 from pose+0x51 */
#define CAR_POS       0x14u /* xyz */
#define CAR_VEL       0x2cu /* xyz */
#define CAR_ANG       0x38u /* xyz */
#define CAR_SPAN_ACC  0x88u /* u16 span index wrapped to the course; +0x8a unwrapped */
#define CAR_POSE      0x8cu /* pose block address (0x213b40 + slot) */
#define CAR_ROAD_XZ   0x90u /* road point world XZ at the lateral offset (+0x98 for z) */
#define CAR_ROAD_Y    0x94u /* road height under the car */
#define CAR_LATERAL   0xa8u /* lateral offset across the span */
#define CAR_SPAN      0xe4u /* current span row */

void sr_car_frame(uint32_t node);      /* 0x2BCF0: the whole per-tick update */
void sr_pose_publish(uint32_t node);   /* 0x2BC08 */
void sr_car_peer(uint32_t node);       /* 0x2CC78 */
/* `frame` is the caller's (car_frame's) i960 frame: both write temporaries there. */
void sr_road_span(uint32_t node, uint32_t span_base, uint8_t *frame);                               /* 0x311E0 */
void sr_car_yaw_step(uint32_t node);   /* 0x2F3C0 */
void sr_car_motion(uint32_t node, uint32_t spans);   /* 0x304F0 */
void sr_car_velocity_step(uint32_t node);                                          /* 0x30E00 */
/* target / angles: the predicted position and angles (host words, integrate's frame). Returns the deepest wall
 * penetration as float bits. */
uint32_t sr_car_contact_solve(uint32_t node, uint8_t *target, const uint8_t *angles); /* 0x2F8A0 */
void sr_car_index_refresh(uint32_t node, uint32_t pred[3]);                        /* 0x30320 */
void sr_car_integrate(uint32_t node, uint32_t spans);                               /* 0x2CCE0 */
void sr_car_e8_mode1(uint32_t node);                                               /* 0x2E650 (not ported) */
void sr_car_e8_mode2(uint32_t node, uint32_t spans);                               /* 0x2EEF0 (not ported) */
void sr_road_angles(uint8_t *sample, uint32_t vel_va, uint32_t ang_va);            /* 0x2B6E0 (not ported) */
void sr_road_span_basis(uint32_t row, uint32_t t, uint32_t ox, uint32_t out, uint8_t *frame);       /* 0x31320 */

#endif
