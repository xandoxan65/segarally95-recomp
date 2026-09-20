/* Comm-board idle gate @ 0x2220 (game_mode_index_step callee). */
// @rom 0x2220 +0x18 comm_board_idle_gate

#include "i960_lift.h"
#include "i960_mem.h"

void comm_board_idle_gate(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (i960_ld_u32(I960_WORKRAM, 0x202090, 0) == 0)
        i960_call_rom(0x1cb8);
    else
        i960_call_rom(0x21c0);
}
