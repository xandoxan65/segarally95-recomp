#ifndef SR_FRAME_CLOCK_H
#define SR_FRAME_CLOCK_H
#include <stdint.h>

/* One simulation tick per displayed frame (59.94 Hz; the arcade's vblank is 57.5 Hz), counted from the PVR's
 * vblanks rather than a wall clock (the approach Daytona's port measured). KOS counts vbl_count on every vblank
 * and frame_count on the vblanks that flip a finished render; their difference grows only when a frame is late,
 * so an on-time frame reads 1 tick and a frame k vblanks late reads 1+k. The first call primes and runs no tick. */
typedef struct { uint32_t missed; uint8_t primed; } sr_frame_clock_t;
#define SR_FRAME_TICKS_LIMIT 4u /* catch-up bound after a stall */
uint32_t sr_frame_ticks(sr_frame_clock_t *clock);
#endif
