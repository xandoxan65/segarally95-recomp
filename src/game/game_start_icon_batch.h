/* Host-only icon batch latch for mode-select redraw.
 * ROM 0x20a8c0 is the pedal confirm flag on the course-select frame
 * (@ 0x153B8) — do not store catalog batch indices there. */

#ifndef GAME_START_ICON_BATCH_H
#define GAME_START_ICON_BATCH_H

#include "i960_lift.h"

void game_start_icon_batch_set(u32 batch);
u32 game_start_icon_batch_get(void);

#endif
