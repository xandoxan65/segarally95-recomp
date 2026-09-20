/* Practice scene-slot dispatch @ 0x1C1A0 — table slot 7.
 * Same shape as attract_scene_slot_dispatch but jump table @ 0x5BA880.
 * source: disasm/maincpu/maincpu_01c1a0_80.asm */
// @rom 0x1c1a0 +0x58 game_start_practice_slot_dispatch

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host_staging.h"
#include "model2_rom.h"

#define PRACTICE_SLOT_TABLE 0x005ba880u

void game_start_practice_slot_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 frame;
    u32 prev;
    u32 index;
    u32 handler;
    u32 changed;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    frame = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0);
    prev = i960_ld_u32(I960_WORKRAM, 0x20aab4, 0);
    index = frame & 15u;
    handler = i960_ld_u32(I960_WORKRAM, PRACTICE_SLOT_TABLE, index << 2);
    if (handler == 0)
        handler = model2_workram_mirror_u32(PRACTICE_SLOT_TABLE + (index << 2));

    changed = (prev != frame) ? 1u : 0u;
    i960_st_u32(I960_WORKRAM, 0x20aabc, 0, changed);
    i960_st_u32(I960_WORKRAM, 0x20aab4, 0, frame);

    if (handler == 0) {
        /* ROM: fall back to attract (main mode 2). */
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x202098, 0, 2);
        return;
    }
    if (!i960_host_staging_call_lifted(handler))
        i960_call_indirect(handler);
}
