/* Time/score field split @ 0x27c68 (bal from scene_hud_alt digit loop).
 * Fills out[0..3] halfword fields for HUD digit blits.
 * source: disasm/maincpu/maincpu_027c68_80.asm */
// @rom 0x27c68 +0x5c tile_attract_time_split

#include "i960_lift.h"

void tile_attract_time_split(u32 value, u16 out[4])
{
    i32 v = (i32)value;
    i32 g4;
    i32 g5;
    i32 g6;
    i32 g0v;

    /* remi/divi with g7 = addo(31,29) = 60. */
    g4 = v % 60;
    g5 = v / 0xe10;
    g6 = v / 60;
    g4 = g4 * 100;
    g0v = v / 0x34bc0;
    g5 = g5 % 60;
    g6 = g6 % 60;
    g4 = g4 + 0x1e;
    g4 = g4 / 60;

    out[0] = (u16)g0v;
    out[1] = (u16)g5;
    out[2] = (u16)g6;
    out[3] = (u16)g4;
}
