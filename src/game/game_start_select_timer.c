/* Mode-select timer gate helpers @ 0x146A8 / 0x14678.
 * 0x20a8b8 is the championship/practice countdown (seeded 0x4b0+0x3c).
 * source: disasm/maincpu/maincpu_0146a8_200.asm */
// @rom 0x146a8 +0x38 game_start_select_timer_gate
// @rom 0x14678 +0x18 game_start_select_timer_arm

#include "i960_lift.h"
#include "i960_mem.h"

/*
 * Returns:
 *   2 — timer > 60 (input armed)
 *   1 — 0 < timer ≤ 60
 *   0 — timer exhausted
 */
u32 game_start_select_timer_gate(void)
{
    u32 t = i960_ld_u32(I960_WORKRAM, 0x20a8b8, 0);

    if (t > 60u) /* addo 31,29 */
        return 2u;
    if (t == 0u)
        return 0u;
    return 1u;
}

/* @0x14678: arm timer to 60 after a confirm edge. */
void game_start_select_timer_arm(void)
{
    i960_st_u32(I960_WORKRAM, 0x20a8b8, 0, 60u);
}
