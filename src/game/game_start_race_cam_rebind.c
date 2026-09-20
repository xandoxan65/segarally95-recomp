/* Race cam phase rebind @ 0x202E0 — from chase / alt-chase tails.
 *
 * If 0x2140c0 != 0 install staged 0x5BEF60 (ROM 0x1FF60);
 * else install staged 0x5BEDC0 (chase @ 0x1FDC0).
 * Both go through cam_bind @ 0x20310 (slot store only — not sentinel, so
 * no cam_seed).
 *
 * source: disasm/maincpu/maincpu_0202e0_40.asm */
// @rom 0x202e0 +0x30 game_start_race_cam_rebind

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

void game_start_race_cam_rebind(u32 arg0, u32 arg1, u32 arg2)
{
    u32 phase;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    phase = i960_ld_u32(I960_WORKRAM, 0x2140c0, 0);
    if (phase != 0u)
        g0 = 0x005bef60u;
    else
        g0 = 0x005bedc0u;
    game_start_race_cam_bind((u32)g0, 0, 0);
}
