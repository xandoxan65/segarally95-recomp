/* Race sub-slot dispatch @ 0x1CA30 — callx 0x5BBA10[2020a8>>1].
 * race_frame calls this every frame after logo_path zeros 0x2020a8.
 * Table @ ROM 0x1CA10 / workram 0x5BBA10 (rom = staged − 0x59f000):
 *   [0] 0x5BBA70 → 0x1CA70 game_start_race_slot0
 *   [1] 0x5BBD20 → 0x1CD20 game_start_race_slot1
 *   [2] 0x5BBE30 → 0x1CE30 game_start_race_slot2 (countdown)
 *   [3] 0x5BC160 → 0x1D160 game_start_race_slot3
 *   [4..] 0x5BC680… still unlifted; empty slots advance 0x2020ac.
 *
 * Lifted slots 0–3 are called directly from the ROM table (symbols/
 * staging_tables.yaml) — same targets as callx through 0x5BBA10, without
 * depending on a possibly-stale workram cell for the handler word.
 *
 * source: disasm/maincpu/maincpu_01ca30_40.asm */
// @rom 0x1ca30 +0x38 game_start_race_sub_dispatch

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "i960_host_staging.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

#define RACE_SUB_TABLE 0x005bba10u

void game_start_race_sub_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 index;
    u32 handler;
    u32 frame;
    u32 sub;
    static int logged;
    static u32 last_idx = 0xffffffffu;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    sub = i960_ld_u32(I960_WORKRAM, 0x2020a8, 0);
    index = (u32)((i32)sub >> 1);
    handler = i960_ld_u32(I960_WORKRAM, RACE_SUB_TABLE, index << 2);
    if (handler == 0)
        handler = model2_workram_mirror_u32(RACE_SUB_TABLE + (index << 2));

    if (!logged || index != last_idx) {
        lift_log(
                "lift: race_sub_dispatch idx=%u sub=%u handler=0x%x\n",
                (unsigned)index, (unsigned)sub, (unsigned)handler);
        fflush(stderr);
        logged = 1;
        last_idx = index;
    }

    /* Direct calls for RE-documented lifted slots (ROM table @ 0x1CA10). */
    switch (index) {
    case 0:
        game_start_race_slot0(0, 0, 0);
        return;
    case 1:
        game_start_race_slot1(0, 0, 0);
        return;
    case 2:
        game_start_race_slot2(0, 0, 0);
        return;
    case 3:
        game_start_race_slot3(0, 0, 0);
        return;
    default:
        break;
    }

    if (handler == 0) {
        /* @0x1CA48: empty slot → advance scene table 0x2020ac. */
        frame = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0);
        i960_st_u32(I960_WORKRAM, 0x2020ac, 0, frame + 1u);
        return;
    }
    if (!i960_host_staging_call_lifted(handler))
        i960_call_indirect(handler);
}
