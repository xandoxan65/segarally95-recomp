/* Race sub-slot reset @ 0x1C9C0 — zeros 0x2020a8 then light init.
 * Called from attract_hud_setup logo_path before advancing to race_frame.
 * Without this, 0x1CA30 indexes garbage and race setup never runs.
 * source: disasm/maincpu/maincpu_01c9c0_60.asm */
// @rom 0x1c9c0 +0x44 game_start_race_sub_reset

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "i960_fp.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_sub_reset(u32 arg0, u32 arg1, u32 arg2)
{
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x1C9C0: stos g14 → 0x2020a8 (race sub-slot index). */
    i960_st_u32(I960_WORKRAM, 0x2020a8, 0, 0);

    /* @0x1C9C8: race colorbase CGMs (0x20fe200 / 0x289bc78 / 0x289c0e8). */
    game_start_race_cgm_colorbase_seed(0, 0, 0);
    /* @0x1C9CC: bal 0x2ab48 — fetch list head @ 0x20c97c → 0x20ab64. */
    g0 = game_start_race_list_head_fetch(0, 0, 0);

    i960_st_u32(I960_WORKRAM, 0x20ab64, 0, (u32)g0);

    if (i960_ld_u8(I960_WORKRAM, 0x20201a, 0) == 1u) {
        /* @0x1C9E4: movr +1.0 → 0x20ac74. */
        i960_st_u32(I960_WORKRAM, 0x20ac74, 0, 0x3f800000u);
    } else {
        i960_st_u32(I960_WORKRAM, 0x20ac74, 0, 0);
    }

    if (!logged) {
        lift_log( "lift: race_sub_reset (0x2020a8=0)\n");
        logged = 1;
    }
}
