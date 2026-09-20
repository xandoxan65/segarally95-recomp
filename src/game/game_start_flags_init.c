/* Game-start flag seed @ 0x1ABB0 — called from mode-3 phase 0.
 * source: disasm/maincpu/maincpu_01abb0_80.asm */
// @rom 0x1abb0 +0x28 game_start_flags_init

#include "i960_lift.h"
#include "i960_mem.h"

void game_start_flags_init(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x1ABB0: call 0x235A0 — larger workram table seed; leave unlifted. */
    i960_call_rom(0x235a0);
    i960_st_u32(I960_WORKRAM, 0x2139d4, 0, 1);
    i960_st_u32(I960_WORKRAM, 0x20a5b4, 0, 1);
    i960_st_u32(I960_WORKRAM, 0x2140c0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a5b0, 0, 0);
}
