/* Lap / HUD bank clear @ 0x1C830 — slot0 after draw rows.
 *
 * Clears 0x40 words at 0x2020d0 and the parallel bank at 0x20afb0, optionally
 * bals 0x18248 when choice/course gate hits, then stores the incoming g0
 * (slot0's 0x20aca4 load) into 0x20b0b0 and zeros 0x2020bc / 0x20aea4 / 0x20aeac.
 *
 * 0x20b0b0 is the span slot1 snapshots for its 60-frame blend — silent
 * call_rom here left it stale so the countdown chain starved.
 *
 * source: disasm/maincpu/maincpu_01c830_80.asm */
// @rom 0x1c830 +0x78 game_start_race_lap_bank_clear

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "i960_host.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_lap_bank_clear(u32 arg0, u32 arg1, u32 arg2)
{
    u32 saved = arg0 != 0u ? arg0 : (u32)g0;
    u32 i;
    u32 lap = 0x002020d0u;
    u32 hud = 0x0020afb0u;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_lap_bank_clear g0=%#x\n", (unsigned)saved);
        fflush(stderr);
        logged = 1;
    }

    /* @0x1C848–0x1C864: 0x40 iterations, st 0 → *lap++ / *hud; hud walks +4 via lda. */
    for (i = 0; i < 0x40u; i++) {
        i960_st_u32(I960_WORKRAM, lap, 0, 0);
        i960_st_u32(I960_WORKRAM, hud, 0, 0);
        lap += 4u;
        hud += 4u;
    }

    /*
     * @0x1C868–0x1C880: if 0x202230 == 1 OR 0x214354 < 2 → bal 0x18248.
     * Practice / desert (course 0) takes this gate — seeds 0x20a9fc = 16.
     */
    if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) == 1u
        || (i32)i960_host_race_course_index() < 2) {
        game_start_race_lap_bank_seed(0, 0, 0);
    }

    i960_st_u32(I960_WORKRAM, 0x20b0b0, 0, saved);
    i960_st_u32(I960_WORKRAM, 0x2020bc, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20aea4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20aeac, 0, 0);
}
