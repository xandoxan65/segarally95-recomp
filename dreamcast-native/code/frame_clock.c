#include "frame_clock.h"
#include <dc/pvr.h>

uint32_t sr_frame_ticks(sr_frame_clock_t *clock) {
    pvr_stats_t stats;
    if(pvr_get_stats(&stats)) return clock->primed ? 1u : 0u;
    const uint32_t missed = (uint32_t)(stats.vbl_count - stats.frame_count);
    if(!clock->primed) { clock->primed = 1; clock->missed = missed; return 0; }
    const uint32_t late = missed - clock->missed;
    clock->missed = missed;
    return late < SR_FRAME_TICKS_LIMIT ? 1u + late : SR_FRAME_TICKS_LIMIT;
}
