/* Host shim: symbol was wrongly bound to mid-HUD @ 0x12d00.
 * ROM 0x12d00 is inside comm_attract_scene_hud (@ 0x12cc0), not an entry.
 * Callers that need HUD stamps must use 0x12cc0. */

#include "i960_lift.h"

void scene_setup_init(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;
}
