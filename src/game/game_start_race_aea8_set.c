/* Race 0x20aea8 store helper @ 0x1C808 — bal from slot3.
 * mov g14,g1; mov 0,g14; lda 0x3b(g0),g0; st g0 → 0x20aea8; bx (g1).
 * source: disasm/maincpu/maincpu_01c808_20.asm */
// @rom 0x1c808 +0x1c game_start_race_aea8_set

#include "i960_lift.h"
#include "i960_mem.h"

void game_start_race_aea8_set(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;

    g14 = 0;
    /* @0x1C810: lda 0x3b(g0) — byte offset into the g0 object. */
    i960_st_u32(I960_WORKRAM, 0x20aea8, 0, arg0 + 0x3bu);
}
