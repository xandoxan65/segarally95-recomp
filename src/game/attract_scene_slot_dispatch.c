/* Attract scene-slot dispatch @ 0x1C0E0 — callx 0x5BA820[2020ac&15].
 *
 * ROM table @ 0x5BA820 (maincpu mirror 0x1B820), resolve
 * rom = 0x1000 + (staged - 0x5A0000):
 *   [0] 0x5B4FD0 → 0x15FD0 game_start_course_display
 *   [1] 0x5B52E0 → 0x162E0 game_start_course_select
 *   [2] 0x5BA940 → 0x1B940 attract_hud_setup
 *   [3] 0x5BABA0 → 0x1BBA0 (post-hud; not car select)
 *
 * Practice uses a different table @ 0x5BA880 (dispatch @ 0x1C1A0):
 *   [2] 0x5B3D60 → 0x14D60 game_start_car_display
 *   [3] 0x5B4200 → 0x15200 game_start_car_select
 *
 * source: disasm/maincpu/maincpu_01c0e0_58.asm + ROM words @ 0x1B820/0x1B880 */
// @rom 0x1c0e0 +0x58 attract_scene_slot_dispatch

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host_staging.h"
#include "model2_rom.h"

#define SCENE_SLOT_TABLE 0x005ba820u

void attract_scene_slot_dispatch(u32 arg0, u32 arg1, u32 arg2)
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
    handler = i960_ld_u32(I960_WORKRAM, SCENE_SLOT_TABLE, index << 2);
    if (handler == 0)
        handler = model2_workram_mirror_u32(SCENE_SLOT_TABLE + (index << 2));

    changed = (prev != frame) ? 1u : 0u;
    i960_st_u32(I960_WORKRAM, 0x20aabc, 0, changed);
    i960_st_u32(I960_WORKRAM, 0x20aab4, 0, frame);

    if (handler == 0) {
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x202098, 0, 2);
        return;
    }
    if (!i960_host_staging_call_lifted(handler))
        i960_call_indirect(handler);
}
